# RUDE HYPE State

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
