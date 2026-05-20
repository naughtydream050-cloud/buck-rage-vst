# RUDE HYPE Current Task

## Phase

RESOURCE_ASSET_COMMIT_MAC_EMBEDDING_VALIDATION

## Goal

Move RUDE HYPE from first-stage Windows/macOS VST3 build success toward a normal Mac plugin delivery path: universal VST3/AU, embedded image resources, optional Developer ID signing, optional notarization, and host validation.

## Current Status

- Project separated from BUCK_RAGE into `projects/RUDE_HYPE`.
- Faceplate asset generated from the supplied flat-knob image.
- SHOUT and BURN knob images are cropped with circular alpha.
- Plugin editor display scale is `0.42`, while source assets remain full resolution.
- `ImageKnob.h` rotates the actual knob image via `AffineTransform::rotation`.
- Runtime JPEG fallback applies a circular ARGB mask before handing knob images to the UI.
- Default parameter values are `0.5` so the reference orientation is preserved at startup.
- Draft PR: `https://github.com/naughtydream050-cloud/buck-rage-vst/pull/1`.
- Windows VST3 build has passed in GitHub Actions.
- macOS arm64 VST3 build has passed in GitHub Actions.
- SHOUT is implemented as an upper-mid forwardness macro.
- BURN is implemented as a low-safe dirty saturation macro.
- Development mode is `MCP + GPT consultation + GitHub Actions`.
- GPT consultation approved staged Mac rollout and requires gates for AU/universal/codesign/notarization.
- CMake now builds VST3 on all platforms and AU on Apple platforms.
- CMake now embeds `RudeHypeAssets` only when all distribution PNGs exist.
- GitHub Actions now includes a Mac universal VST3/AU distribution candidate job.
- Mac validation now emits `mac-distribution-report.json`.
- Signing/notarization now emits `mac-signing-report.json` and skips safely until Apple secrets exist.
- Windows-validated PNG assets are being committed to `Resources/` so Mac VST3/AU bundles can embed the same faceplate and rotating knob images.
- Windows-validated PNG assets are now committed and pushed to the PR branch.
- GitHub Actions run `26140123700` passed with Windows VST3, macOS universal VST3, macOS universal AU, and macOS report artifacts.
- `mac-distribution-report.json` passed with `distributionReady=true`, `embeddedAssets=present`, and `assetCount=3`.
- macOS VST3 and AU binaries validate as universal `x86_64 + arm64`.

## Active Gates

1. Host-test the macOS universal VST3 in FL Studio Mac, Ableton Live, or Reaper.
2. Host-test the macOS universal AU in Logic.
3. Run AU validation and real Logic scan when a Mac host is available.
4. Add Apple Developer secrets when ready for public distribution.
5. Re-run signing/notarization and confirm codesign, notarytool, stapler, and spctl reports.
6. Capture host screenshots to confirm faceplate rendering and circular knob alpha.

## Collaboration Contract

- Before DSP chain changes, UI doctrine changes, PluginEditor responsibility changes, image-first exceptions, SHOUT/BURN meaning changes, Mac build pipeline, AU validation, codesign/notarization, or major architecture rewrites, consult GPT using `reports/latest/rude-hype-context-pack.json` as the handoff payload.
- Do not send large images, full build logs, or broad Markdown scans to GPT.
- Use compact report JSON, context-pack summaries, and log tails only.
- Keep `PluginEditor.cpp` as implementation consumer.
- Keep `ui/spec/ui-spec.json` as UI SSOT.
- Keep faceplate PNG as the visual truth.
- Keep image knobs rotating via `AffineTransform` with circular alpha.

## Token Policy

- Do not send reference PNG/JPG repeatedly to LLMs.
- Use `ui/spec/ui-spec.json`, compact report JSON, and log tails.
- Avoid broad Markdown scans and JUCE vendor doc scans.
