# RUDE HYPE Blueprint

## Product Boundary

RUDE HYPE is a plugin project, separate from BUCK_RAGE and unrelated to SPEC AI web service documents.

## UI Doctrine

- The supplied flat reference image is the visual authority.
- `ui/spec/ui-spec.json` is the machine-readable source of truth.
- `PluginEditor.cpp` consumes generated layout constants and assets; it is not the source of truth.
- The faceplate is image-first.
- SHOUT and BURN use cropped knob images and rotate those images directly.
- All hit targets are measured from the reference image.
- The editor displays the full-resolution reference at `0.42` scale to match a natural VST host footprint.
- Knob crops are internal square images, but their visible result must be circular RGBA.

## Harness Doctrine

- Keep large reference images local.
- Generate compact report JSON files for validation.
- Pass only report summaries, spec snippets, and log tails to Codex or external LLM review.
- Score UI with screenshot diff and hit-area validation instead of subjective visual judgment.
- Treat live host capture as the final UI gate.
- Validate knob alpha separately so rectangular crop regressions fail before subjective review.

## Current Build Route

Local toolchain discovery did not find CMake/MSBuild, so the active build route is GitHub Actions:

1. Push text-only RUDE HYPE build changes to Draft PR.
2. Let GitHub Actions build the VST3.
3. Download the artifact.
4. Load `RUDE HYPE.vst3` in a local host.
5. Capture the rendered UI.
6. Re-run screenshot diff and gate reports.

## Distribution Note

The current CI route avoids binary image payloads by loading `C:\Users\razor\Downloads\S__45752322.jpg` at runtime when embedded BinaryData is unavailable. This is acceptable for the lightweight validation loop, but a distribution-ready build should embed `faceplate_rude_hype.png`, `knob_shout.png`, and `knob_burn.png`.
