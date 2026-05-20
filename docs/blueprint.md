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

- Keep large reference images local unless they are explicitly committed as distribution assets.
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

GitHub Actions is the active build route:

1. Windows VST3 build.
2. macOS universal VST3/AU candidate build.
3. Mac distribution validation report.
4. Optional Developer ID signing when Apple secrets are present.
5. Optional notarization and stapling when Apple secrets are present.
6. Artifact upload for VST3, AU, and reports.
7. Host validation on real DAWs.

## Mac Distribution Strategy

The Mac pipeline is explicit and gated:

- build universal binaries with `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.
- build AU only on Apple platforms.
- validate universal architecture with `file` output.
- validate source asset presence.
- validate generated BinaryData and compiled asset objects.
- emit `mac-distribution-report.json`.
- skip signing/notarization safely when Apple secrets are absent.
- sign, notarize, staple, and assess when Apple secrets are present.

## Distribution Resource Rule

A Mac-ready RUDE HYPE build must embed these assets:

- `Resources/faceplate_rude_hype.png`
- `Resources/knob_shout.png`
- `Resources/knob_burn.png`

These assets are the Git-managed copies of the Windows-validated RUDE HYPE UI images. The previous runtime fallback to `C:\Users\razor\Downloads\S__45752322.jpg` is allowed only for local lightweight validation. It is not a distribution strategy and must not be the final Mac path.

GitHub Actions run `26140123700` passed resource embedding validation with `embeddedAssets=present`, `assetCount=3`, and universal VST3/AU outputs. The artifacts are test-distribution ready for Mac users who can install unsigned plugins. Public distribution still requires Developer ID signing, notarization, stapling, and host validation.

## Release Readiness

A PR can be treated as Mac distribution ready only when all are true:

- Windows VST3 artifact exists.
- macOS universal VST3 artifact exists.
- macOS universal AU artifact exists.
- `mac-distribution-report.json` has no missing asset or architecture failures.
- `mac-signing-report.json` is passed, not skipped, for public distribution.
- AU validation or Logic scan passes.
- host screenshots confirm faceplate and circular knob alpha.
