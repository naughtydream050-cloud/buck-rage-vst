# RUDE HYPE Current Task

## Phase

IMAGE_KNOB_AUTHORING

## Goal

Make the UI match `S__45752322.jpg` and make SHOUT/BURN knobs rotate using the knob images cut from that same reference.

## Current Status

- Project separated from BUCK_RAGE into `projects/RUDE_HYPE`.
- Faceplate asset generated from the supplied flat-knob image.
- SHOUT and BURN knob images are cropped with circular alpha.
- Plugin editor display scale is `0.42`, while source assets remain full resolution.
- `ImageKnob.h` rotates the actual knob image via `AffineTransform::rotation`.
- Runtime JPEG fallback now applies a circular ARGB mask before handing knob images to the UI.
- Default parameter values are `0.5` so the reference orientation is preserved at startup.
- Draft PR created: `https://github.com/naughtydream050-cloud/buck-rage-vst/pull/1`.
- GitHub Actions build passed and produced `RUDE HYPE.vst3`.
- Latest artifact downloaded to `reports/latest/rude-hype/rude-hype-vst3-windows-latest.zip`.

## Next

1. Rebuild the PR artifact after scale/alpha fixes.
2. Install or load the latest `RUDE HYPE.vst3` in a VST3 host.
3. Confirm the runtime reference image path exists: `C:\Users\razor\Downloads\S__45752322.jpg`.
4. Capture a real rendered plugin screenshot from the host.
5. Replace `ui/rendered/rendered.png` with the real screenshot.
6. Re-run screenshot diff, hit-area, knob alpha, and knob rotation reports.

## Token Policy

- Do not send reference PNG/JPG repeatedly to LLMs.
- Use `ui/spec/ui-spec.json` and compact report JSON.
- Avoid broad Markdown scans and JUCE vendor doc scans.
