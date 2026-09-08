#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build-mac-universal}"
report_path="${2:-reports/mac-signing-report.json}"
mkdir -p "$(dirname "$report_path")"

write_report() {
  local status="$1"
  local reason="$2"
  cat > "$report_path" <<JSON
{
  "schemaVersion": 1,
  "status": "$status",
  "reason": "$reason",
  "notarized": false,
  "stapled": false
}
JSON
  cat "$report_path"
}

required=(APPLE_ID APPLE_TEAM_ID APPLE_APP_PASSWORD MAC_CERTIFICATE MAC_CERTIFICATE_PASSWORD)
missing=()
for key in "${required[@]}"; do
  if [[ -z "${!key:-}" ]]; then missing+=("$key"); fi
done

if [[ ${#missing[@]} -gt 0 ]]; then
  write_report "skipped" "missing_secrets:${missing[*]}"
  exit 0
fi

vst3_bundle="$(find "$build_dir" -name "*.vst3" -type d | head -n 1 || true)"
au_bundle="$(find "$build_dir" -name "*.component" -type d | head -n 1 || true)"
if [[ -z "$vst3_bundle" || -z "$au_bundle" ]]; then
  write_report "blocked" "missing_plugin_bundles"
  exit 1
fi

keychain="$RUNNER_TEMP/rude-hype-signing.keychain-db"
cert_path="$RUNNER_TEMP/rude-hype-cert.p12"

echo "$MAC_CERTIFICATE" | base64 --decode > "$cert_path"
security create-keychain -p "$MAC_CERTIFICATE_PASSWORD" "$keychain"
security set-keychain-settings -lut 21600 "$keychain"
security unlock-keychain -p "$MAC_CERTIFICATE_PASSWORD" "$keychain"
security import "$cert_path" -P "$MAC_CERTIFICATE_PASSWORD" -A -t cert -f pkcs12 -k "$keychain"
security list-keychain -d user -s "$keychain" login.keychain-db
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$MAC_CERTIFICATE_PASSWORD" "$keychain"

identity="Developer ID Application: $APPLE_TEAM_ID"
for bundle in "$vst3_bundle" "$au_bundle"; do
  codesign --force --timestamp --options runtime --sign "$identity" "$bundle"
  codesign --verify --deep --strict --verbose=2 "$bundle"
done

package_dir="$RUNNER_TEMP/rude-hype-notary"
rm -rf "$package_dir"
mkdir -p "$package_dir"
cp -R "$vst3_bundle" "$package_dir/"
cp -R "$au_bundle" "$package_dir/"
notary_zip="$RUNNER_TEMP/RUDE-HYPE-mac-universal.zip"
ditto -c -k --keepParent "$package_dir" "$notary_zip"

xcrun notarytool submit "$notary_zip" \
  --apple-id "$APPLE_ID" \
  --team-id "$APPLE_TEAM_ID" \
  --password "$APPLE_APP_PASSWORD" \
  --wait

for bundle in "$vst3_bundle" "$au_bundle"; do
  xcrun stapler staple "$bundle"
  spctl --assess --type execute --verbose=4 "$bundle" || true
done

cat > "$report_path" <<JSON
{
  "schemaVersion": 1,
  "status": "passed",
  "reason": "signed_notarized_and_stapled",
  "notarized": true,
  "stapled": true,
  "package": "$notary_zip"
}
JSON
cat "$report_path"
