# RUDE HYPE Mac Build Notes

## GPT Consultation Result

Status: `APPROVE_WITH_GATES`

The prior GPT consultation approved the staged Mac rollout and specifically warned not to mix final AU/universal/codesign/notarization work into the lightweight build until bundle strategy and Apple credentials exist. This phase follows that guidance by making Mac distribution executable but gated.

## Implemented Pipeline

GitHub Actions now includes `build-macos-distribution`:

- `macos-14` runner.
- universal CMake configure with `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.
- VST3 build.
- AU build on Apple platforms only.
- universal architecture validation with `file` output.
- source resource presence validation.
- embedded BinaryData resource validation.
- optional Developer ID signing when Apple secrets are present.
- optional notarytool submission and stapling when Apple secrets are present.
- validation report artifact upload.

## Required Apple Secrets

The signing/notarization step is intentionally skipped until all of these are configured:

- `APPLE_ID`
- `APPLE_TEAM_ID`
- `APPLE_APP_PASSWORD`
- `MAC_CERTIFICATE`
- `MAC_CERTIFICATE_PASSWORD`

`MAC_CERTIFICATE` must be a base64-encoded Developer ID Application `.p12` certificate.

## Resource Embedding Gate

Distribution builds must embed:

- `Resources/faceplate_rude_hype.png`
- `Resources/knob_shout.png`
- `Resources/knob_burn.png`

CMake only enables `RudeHypeAssets` when all three files are present. The Mac validation script records `source_assets_missing` and `embedded_assets_missing` if the repo is still using the old text-only CI route.

## Current Limitation

The current PR branch now contains the Mac distribution pipeline, but final distribution readiness still depends on committing the actual PNG assets and adding Apple Developer credentials. Without those, the CI can build unsigned candidates and produce reports, but it must not be treated as a fully Gatekeeper-safe public Mac release.

## AU / Logic Gate

The AU target is enabled on Apple builds. Full Logic validation requires one of these routes:

- CI `auval` with `REQUIRE_AUVAL=1` after installing the component into the runner audio plugin folder.
- Manual Logic scan on a real Mac host.

Until that passes, AU is a build artifact, not a fully certified Logic release.
