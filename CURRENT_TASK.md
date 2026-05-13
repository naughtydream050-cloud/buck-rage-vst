# RUDE HYPE Current Task

## Phase

IMAGE_KNOB_AUTHORING

## Goal

Make the UI match `S__45752322.jpg` and make SHOUT/BURN knobs rotate using the knob images cut from that same reference.

## Current Status

- Project separated from BUCK_RAGE into `projects/RUDE_HYPE`.
- Faceplate asset generated from the supplied flat-knob image.
- SHOUT and BURN knob images are cropped with circular alpha.
- `ImageKnob.h` rotates the actual knob image via `AffineTransform::rotation`.
- Default parameter values are `0.5` so the reference orientation is preserved at startup.
- Draft PR created: `https://github.com/naughtydream050-cloud/buck-rage-vst/pull/1`.
- GitHub Actions build passed and produced `RUDE HYPE.vst3`.
- Artifact downloaded to `reports/latest/rude-hype/rude-hype-vst3-windows.zip`.

## Next

1. Install or load `reports/latest/rude-hype/artifact/RudeHype_artefacts/Release/VST3/RUDE HYPE.vst3` in a VST3 host.
2. Confirm the runtime reference image path exists: `C:\Users\razor\Downloads\S__45752322.jpg`.
3. Capture a real rendered plugin screenshot from the host.
4. Replace `ui/rendered/rendered.png` with the real screenshot.
5. Re-run screenshot diff, hit-area, and knob rotation reports.

## Token Policy

- Do not send reference PNG/JPG repeatedly to LLMs.
- Use `ui/spec/ui-spec.json` and compact report JSON.
- Avoid broad Markdown scans and JUCE vendor doc scans.
