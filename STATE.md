# RUDE HYPE State

## 2026-05-17 - Resource Asset Commit

### Phase

RESOURCE_ASSET_COMMIT_MAC_EMBEDDING_VALIDATION

### Done

- Confirmed the Windows-validated RUDE HYPE image assets exist locally:
  - `projects/RUDE_HYPE/Resources/faceplate_rude_hype.png`
  - `projects/RUDE_HYPE/Resources/knob_shout.png`
  - `projects/RUDE_HYPE/Resources/knob_burn.png`
- Copied those exact PNG files into the GitHub PR repo `Resources/` directory.
- Preserved the image-first rule:
  - faceplate PNG remains visual truth.
  - knob PNG files remain the rotating assets.
  - JUCE still consumes BinaryData and does not repaint the texture.
- Confirmed CMake already registers the assets through `RudeHypeAssets` when all three PNGs are present.

### Expected Gate Change

- `mac-distribution-report.json` should move from `blocked` to `passed` for:
  - `source_assets_missing`
  - `embedded_assets_missing`
- Signing/notarization remains gated until Apple Developer secrets are configured.

### Next Priority

Re-run GitHub Actions and confirm regenerated artifacts:

1. `RUDE-HYPE-VST3-Windows`
2. `RUDE-HYPE-VST3-macOS-universal`
3. `RUDE-HYPE-AU-macOS-universal`
4. `RUDE-HYPE-macOS-distribution-reports`

## 2026-05-20 - Resource Embedding Passed

### Phase

RESOURCE_ASSET_COMMIT_MAC_EMBEDDING_VALIDATION

### Done

- Pushed the Windows-validated RUDE HYPE PNG assets to the GitHub PR branch.
- Fixed macOS resource embedding validation to check generated JUCE BinaryData and compiled asset objects instead of brittle final-binary string retention.
- GitHub Actions run `26140123700` passed:
  - Windows VST3 artifact uploaded.
  - macOS universal VST3 artifact uploaded.
  - macOS universal AU artifact uploaded.
  - macOS distribution reports artifact uploaded.
- macOS distribution validation passed:
  - `distributionReady=true`
  - `embeddedAssets=present`
  - `assetCount=3`
  - VST3 binary is universal `x86_64 + arm64`
  - AU binary is universal `x86_64 + arm64`

### Current Artifact Links

- Windows VST3: `https://github.com/naughtydream050-cloud/buck-rage-vst/actions/runs/26140123700/artifacts/7100955034`
- macOS universal VST3: `https://github.com/naughtydream050-cloud/buck-rage-vst/actions/runs/26140123700/artifacts/7100937555`
- macOS universal AU: `https://github.com/naughtydream050-cloud/buck-rage-vst/actions/runs/26140123700/artifacts/7100937977`
- macOS reports: `https://github.com/naughtydream050-cloud/buck-rage-vst/actions/runs/26140123700/artifacts/7100938163`

### Known Issues

- Apple Developer signing/notarization is still skipped because Apple secrets are not configured.
- AU build exists, but Logic/auval host validation is not yet proven on a real Mac.
- These artifacts are suitable for test distribution; final public Mac distribution still needs Developer ID signing, notarization, stapling, and host validation.

## 2026-05-17

### Phase

MAC_DISTRIBUTION_AU_COMPATIBILITY_PIPELINE

### Done

- Started the Mac distribution and AU compatibility pipeline.
- Used the existing GPT consultation result as the strategy guard: AU/universal/codesign/notarization must be gated behind bundle strategy and Apple credentials.
- Confirmed the PR branch previously had Windows VST3 and macOS arm64 VST3 success.
- Confirmed the GitHub PR did not contain distribution PNG assets under `Resources/`.
- Confirmed local RUDE_HYPE PNG assets exist, but they were not pushed because giant binary transfer through LLM/MCP would violate the token policy.
- Updated CMake:
  - VST3 on all platforms.
  - AU only on Apple platforms.
  - macOS deployment target `11.0`.
  - bundle id `com.naughtydream.rudehype`.
  - conditional `RudeHypeAssets` BinaryData target when all three PNG resources exist.
- Added `tools/ci/validate_mac_distribution.sh`:
  - checks VST3 bundle presence.
  - checks AU component presence.
  - checks `arm64` and `x86_64` architecture output.
  - checks source assets.
  - checks embedded BinaryData resource symbols.
  - emits `mac-distribution-report.json`.
- Added `tools/ci/sign_and_notarize_mac.sh`:
  - imports Developer ID certificate when secrets exist.
  - signs VST3 and AU.
  - submits notarization with notarytool.
  - staples accepted bundles.
  - emits `mac-signing-report.json`.
  - skips safely when Apple secrets are missing.
- Updated GitHub Actions:
  - Windows VST3 artifact remains.
  - Mac universal VST3/AU candidate build added.
  - Mac validation report artifact added.
  - optional sign/notarize step added.

### Current Gate Status

- Windows VST3: previously passed.
- macOS arm64 VST3: previously passed.
- macOS universal VST3/AU: pipeline added, CI result pending.
- Resource embedding: pipeline added, but distribution PNG assets still need to be committed to repo.
- Codesign: pipeline added, requires Apple Developer secrets.
- Notarization: pipeline added, requires Apple Developer secrets.
- Logic/AU validation: AU build path added, real validation still pending.

### Known Issues

- Full Mac distribution cannot be marked ready until `Resources/faceplate_rude_hype.png`, `Resources/knob_shout.png`, and `Resources/knob_burn.png` are committed as binary assets.
- Apple Developer credentials are not present in the repo secrets yet.
- Real DAW host validation is still pending.

### Next Priority

1. Commit binary PNG resources through a non-LLM-heavy route.
2. Re-run GitHub Actions and confirm universal VST3/AU artifacts.
3. Add Apple secrets and rerun sign/notarization.
4. Validate AU in Logic or via `auval`.
5. Capture Mac host screenshots for image-first UI evidence.

## 2026-05-14

### Phase

REAL_BUILD_HOST_VALIDATION_DSP_IMPLEMENTATION

### Done

- Promoted RUDE_HYPE development to `MCP + GPT consultation + GitHub Actions` collaboration mode.
- Locked Codex role to implementation, MCP/GitHub Actions, harness, artifacts, and validation.
- Locked GPT consultation role to design direction, DSP review, architecture review, Mac plugin strategy, and doctrine review.
- Standardized `reports/latest/rude-hype-context-pack.json` as the preferred handoff payload.
- Preserved image-first doctrine:
  - faceplate PNG is visual truth.
  - `ui/spec/ui-spec.json` is SSOT.
  - `PluginEditor.cpp` is implementation consumer.
  - image knobs rotate via `AffineTransform`.
  - circular alpha is required.
- Marked future Mac build strategy assumptions:
  - GitHub Actions macOS runner.
  - universal binary.
  - codesign.
  - notarization.
  - AU validation.
- GPT consultation approved staged Mac rollout with macOS VST3 first.
- Added macOS VST3 GitHub Actions workflow draft.
- Added `docs/mac-build-notes.md`.

### Next Priority

Run Windows + macOS VST3 CI, download artifacts, then host-validate the current melody-engine builds.

### Known Issues

- Live DAW screenshot and live audio review are still pending.
- Mac build pipeline is implemented as a first-stage VST3 CI gate; AU/universal/codesign/notarization remain deferred.
- Local CMake/MSBuild remain unavailable in the current shell, so GitHub Actions is the active build route.

## 2026-05-13

### Phase

IMAGE_KNOB_AUTHORING -> BUCK_TRACK_MELODY_ENGINE

### Done

- Split RUDE HYPE into its own plugin project under `01_PLUGINS/projects/RUDE_HYPE`.
- Generated image-first local assets from `C:\Users\razor\Downloads\S__45752322.jpg`.
- Created `ImageKnob.h` so SHOUT and BURN rotate their cropped knob images directly.
- Preserved `ui/spec/ui-spec.json` as the UI source of truth.
- Kept large image payloads local and used compact JSON reports for LLM handoff.
- Created Draft PR: `https://github.com/naughtydream050-cloud/buck-rage-vst/pull/1`.
- GitHub Actions run `25779423030` passed after PR state sync.
- Downloaded latest artifact to `reports/latest/rude-hype/rude-hype-vst3-windows-latest.zip`.
- Verified artifact contains `RUDE HYPE.vst3`.
- Added `displayScale = 0.42` so the UI opens around `669x415` instead of the full `1592x988` reference size.
- Reworked the runtime JPEG crop fallback to create ARGB circular knob images.
- Re-applied circular alpha masks to local knob PNG assets.
- Added `validate_knob_alpha.mjs`.
- GitHub Actions run `25799777671` passed after scale/alpha fixes.
- Downloaded scale/alpha artifact to `reports/latest/rude-hype/rude-hype-vst3-scale-alpha.zip`.
- Replaced the simple saturation/LPF DSP with a layered two-knob melody engine.
- SHOUT now drives upper-mid excitation, micro motion, soft clipping, and width.
- BURN now drives low-safe transistor saturation, wavefolding, downsample flavor, tape compression, and fizz.
- Added `docs/dsp-preview-notes.md`.
- Added `tools/validate_dsp_macro.mjs`.
- DSP macro validation passed, score 10.
- GitHub Actions run `25805504329` passed after BUCK TRACK MELODY ENGINE changes.
- Downloaded melody-engine artifact to `reports/latest/rude-hype/rude-hype-vst3-melody-engine.zip`.

### Scores

- knob rotation: passed, score 10
- knob alpha: passed, score 10
- hit-area alignment: passed, score 10
- harness: ready, score 10
- DSP macro: passed, score 10
- live build: passed via GitHub Actions after BUCK TRACK MELODY ENGINE changes
- live build: passed via GitHub Actions after scale/alpha fixes

### Next Priority

Build the BUCK TRACK MELODY ENGINE revision, then host-test SHOUT/BURN with melody material and capture live UI evidence.

### Known Issues

- Local CMake/MSBuild were not visible in PATH, so the local build route remains blocked.
- The current PR uses runtime image fallback for CI/text-only build. The fallback now applies circular alpha masks, but distribution builds should still embed the faceplate and knob images.
- Workflow artifact name is still `BUCK-RAGE-VST3-Windows`; the contained VST3 bundle is `RUDE HYPE.vst3`.
