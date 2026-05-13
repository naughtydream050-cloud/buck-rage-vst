# RUDE HYPE Current Task

## Phase

REAL_BUILD_HOST_VALIDATION_DSP_IMPLEMENTATION

## Goal

Complete RUDE HYPE as an AI-collaborative image-first Buck saturation plugin: image-accurate UI, circular image knobs, Buck-style melody processing, and Windows/Mac delivery.

## Current Status

- Project separated from BUCK_RAGE into `projects/RUDE_HYPE`.
- Faceplate asset generated from the supplied flat-knob image.
- SHOUT and BURN knob images are cropped with circular alpha.
- Plugin editor display scale is `0.42`, while source assets remain full resolution.
- `ImageKnob.h` rotates the actual knob image via `AffineTransform::rotation`.
- Runtime JPEG fallback now applies a circular ARGB mask before handing knob images to the UI.
- Default parameter values are `0.5` so the reference orientation is preserved at startup.
- Draft PR created: `https://github.com/naughtydream050-cloud/buck-rage-vst/pull/1`.
- GitHub Actions build passed after scale/alpha fixes and produced `RUDE HYPE.vst3`.
- Latest scale/alpha artifact downloaded to `reports/latest/rude-hype/rude-hype-vst3-scale-alpha.zip`.
- Implemented SHOUT as an upper-mid forwardness macro.
- Implemented BURN as a low-safe dirty saturation macro.
- Added `docs/dsp-preview-notes.md`.
- Added `tools/validate_dsp_macro.mjs`.
- GitHub Actions run `25805504329` passed after BUCK TRACK MELODY ENGINE changes.
- Latest melody-engine artifact downloaded to `reports/latest/rude-hype/rude-hype-vst3-melody-engine.zip`.
- Development mode moved to `MCP + GPT consultation + GitHub Actions`.
- Codex owns implementation, file edits, harness, Actions, artifacts, and validation.
- GPT consultation owns DSP direction, architecture review, plugin identity, Mac distribution strategy, and doctrine review.
- GPT consultation approved staged Mac rollout: macOS VST3 first, AU/universal/codesign/notarization later behind credential gates.

## Next

1. Run GitHub Actions Windows + macOS VST3 build.
2. Download Windows and macOS VST3 artifacts.
3. Install or load the latest `RUDE HYPE.vst3` in a VST3 host.
4. Smoke-test SHOUT/BURN in a host with melody material.
5. Confirm output is forward and dense without clipping explosion.
6. Capture a real rendered plugin screenshot from the host.
7. Re-run screenshot diff, hit-area, knob alpha, knob rotation, and DSP macro reports.

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
- Use `ui/spec/ui-spec.json` and compact report JSON.
- Avoid broad Markdown scans and JUCE vendor doc scans.
