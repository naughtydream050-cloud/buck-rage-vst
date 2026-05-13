# RUDE HYPE State

## 2026-05-13

### Phase

IMAGE_KNOB_AUTHORING -> LIVE_ARTIFACT_CAPTURE

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

### Scores

- knob rotation: passed, score 10
- knob alpha: passed, score 10
- hit-area alignment: passed, score 10
- harness: ready, score 10
- live build: passed via GitHub Actions

### Next Priority

Capture the real rendered plugin UI from a host and replace `ui/rendered/rendered.png`, then re-run screenshot diff and hit-area validation against the actual hosted VST3.

### Known Issues

- Local CMake/MSBuild were not visible in PATH, so the local build route remains blocked.
- The current PR uses runtime image fallback for CI/text-only build. The fallback now applies circular alpha masks, but distribution builds should still embed the faceplate and knob images.
- Workflow artifact name is still `BUCK-RAGE-VST3-Windows`; the contained VST3 bundle is `RUDE HYPE.vst3`.
