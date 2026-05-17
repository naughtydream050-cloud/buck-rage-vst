#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build-mac-universal}"
report_path="${2:-reports/mac-distribution-report.json}"
require_assets="${REQUIRE_EMBEDDED_ASSETS:-0}"
require_auval="${REQUIRE_AUVAL:-0}"

mkdir -p "$(dirname "$report_path")"

json_escape() {
  python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))'
}

failures=()
notes=()

vst3_bundle="$(find "$build_dir" -name "*.vst3" -type d | head -n 1 || true)"
au_bundle="$(find "$build_dir" -name "*.component" -type d | head -n 1 || true)"

if [[ -z "$vst3_bundle" ]]; then failures+=("missing_vst3_bundle"); fi
if [[ -z "$au_bundle" ]]; then failures+=("missing_au_component"); fi

check_binary_arches() {
  local bundle="$1"
  local label="$2"
  if [[ -z "$bundle" ]]; then return; fi
  local binary="$bundle/Contents/MacOS/RUDE HYPE"
  if [[ ! -f "$binary" ]]; then
    failures+=("missing_${label}_binary")
    return
  fi
  local file_out
  file_out="$(file "$binary")"
  if [[ "$file_out" != *"x86_64"* ]]; then failures+=("${label}_missing_x86_64"); fi
  if [[ "$file_out" != *"arm64"* ]]; then failures+=("${label}_missing_arm64"); fi
  notes+=("${label}_file=${file_out}")
}

check_binary_arches "$vst3_bundle" "vst3"
check_binary_arches "$au_bundle" "au"

asset_count=0
for asset in Resources/faceplate_rude_hype.png Resources/knob_shout.png Resources/knob_burn.png; do
  if [[ -s "$asset" ]]; then
    asset_count=$((asset_count + 1))
  else
    notes+=("missing_source_asset=${asset}")
  fi
done

embedded_assets="unknown"
if [[ -n "$vst3_bundle" && -f "$vst3_bundle/Contents/MacOS/RUDE HYPE" ]]; then
  if strings "$vst3_bundle/Contents/MacOS/RUDE HYPE" | grep -q "faceplate_rude_hype_png"; then
    embedded_assets="present"
  else
    embedded_assets="missing"
  fi
fi

if [[ "$asset_count" -lt 3 ]]; then
  failures+=("source_assets_missing")
fi
if [[ "$embedded_assets" != "present" ]]; then
  failures+=("embedded_assets_missing")
fi

auval_status="skipped"
if [[ -n "$au_bundle" && "$require_auval" == "1" ]]; then
  mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
  rm -rf "$HOME/Library/Audio/Plug-Ins/Components/$(basename "$au_bundle")"
  cp -R "$au_bundle" "$HOME/Library/Audio/Plug-Ins/Components/"
  if auval -v aufx Rhyp Ndre >/tmp/rude-hype-auval.log 2>&1; then
    auval_status="passed"
  else
    auval_status="failed"
    failures+=("auval_failed")
    notes+=("auval_tail=$(tail -n 20 /tmp/rude-hype-auval.log | tr '\n' ' ')")
  fi
fi

distribution_ready="true"
if [[ ${#failures[@]} -gt 0 ]]; then distribution_ready="false"; fi

{
  echo "{"
  echo "  \"schemaVersion\": 1,"
  echo "  \"status\": \"$([[ ${#failures[@]} -eq 0 ]] && echo passed || echo blocked)\","
  echo "  \"distributionReady\": $distribution_ready,"
  echo "  \"vst3Bundle\": $(printf '%s' "$vst3_bundle" | json_escape),"
  echo "  \"auBundle\": $(printf '%s' "$au_bundle" | json_escape),"
  echo "  \"assetCount\": $asset_count,"
  echo "  \"embeddedAssets\": \"$embedded_assets\","
  echo "  \"auval\": \"$auval_status\","
  echo "  \"failures\": ["
  for i in "${!failures[@]}"; do
    comma=","; [[ "$i" == "$((${#failures[@]} - 1))" ]] && comma=""
    echo "    \"${failures[$i]}\"$comma"
  done
  echo "  ],"
  echo "  \"notes\": ["
  for i in "${!notes[@]}"; do
    comma=","; [[ "$i" == "$((${#notes[@]} - 1))" ]] && comma=""
    echo "    $(printf '%s' "${notes[$i]}" | json_escape)$comma"
  done
  echo "  ]"
  echo "}"
} > "$report_path"

cat "$report_path"

if [[ "$require_assets" == "1" && "$distribution_ready" != "true" ]]; then
  exit 1
fi
