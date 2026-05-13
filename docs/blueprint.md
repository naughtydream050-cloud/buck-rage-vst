# RUDE HYPE Blueprint

## Product Boundary

RUDE HYPE is a plugin project, separate from BUCK_RAGE and unrelated to SPEC AI web service documents.

## Collaboration Mode

RUDE HYPE uses `MCP + GPT consultation + GitHub Actions` as the default collaboration loop.

Codex owns:

- JUCE/C++ implementation
- file edits
- ImageKnob fixes
- CMake
- GitHub Actions
- VST3/AU build plumbing
- harness reports
- artifact download and validation
- state synchronization

GPT consultation owns:

- DSP direction
- Buck-style sound analysis
- SHOUT/BURN doctrine
- UI doctrine review
- image-first design review
- Mac plugin distribution strategy
- architecture review
- plugin identity review

Before any DSP chain change, UI doctrine change, PluginEditor responsibility change, image-first exception, SHOUT/BURN meaning change, Mac build pipeline, AU validation, codesign/notarization, or major architecture rewrite, consult GPT using `reports/latest/rude-hype-context-pack.json` as the payload.

Do not send GPT large images, broad Markdown scans, full generated headers, or full build logs. Send context-pack JSON, report summaries, focused diffs, and log tails only.

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

## DSP Doctrine

- SHOUT pushes forward.
- BURN melts reality.
- Density matters more than raw loudness.
- Upper-mid energy should sit around melody presence, not harsh fizz.
- Low content must remain stable through the BURN split.
- All macro movement must remain realtime safe and smoothed.

## Current Build Route

Local toolchain discovery did not find CMake/MSBuild, so the active build route is GitHub Actions:

1. Push text-only RUDE HYPE build changes to Draft PR.
2. Let GitHub Actions build the VST3.
3. Download the artifact.
4. Load `RUDE HYPE.vst3` in a local host.
5. Capture the rendered UI.
6. Re-run screenshot diff and gate reports.

## Mac Build Strategy

Future Mac plugin delivery assumes:

- GitHub Actions macOS runner.
- first-stage macOS VST3 artifact on `macos-14`.
- later universal binary.
- later VST3 and AU outputs.
- later codesign.
- later notarization.
- later AU validation.

Mac pipeline work must be reviewed through GPT consultation before implementation. The first consultation approved a staged rollout: VST3 on macOS first, then AU/universal/codesign/notarization only after stable bundle IDs, packaging, and Apple Developer credentials exist.

## Distribution Note

The current CI route avoids binary image payloads by loading `C:\Users\razor\Downloads\S__45752322.jpg` at runtime when embedded BinaryData is unavailable. This is acceptable for the lightweight validation loop, but a distribution-ready build should embed `faceplate_rude_hype.png`, `knob_shout.png`, and `knob_burn.png`.
