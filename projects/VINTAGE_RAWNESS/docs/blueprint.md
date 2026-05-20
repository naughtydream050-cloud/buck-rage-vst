# VINTAGE RAWNESS Blueprint

## Doctrine

VINTAGE RAWNESS follows the same image-first plugin doctrine without sharing RUDE_HYPE assets or DSP.

- The reference image defines the UI.
- The faceplate is drawn as an image.
- Knob visuals are cropped RGBA PNGs and rotated directly.
- Preset buttons are hit areas over the image.
- UI correctness is measured through JSON reports, not subjective review.

## Token Policy

- Keep large image files local.
- Pass `reports/latest/vintage-rawness-context-pack.json` for handoff.
- Send ui-spec snippets, compact report JSON, and log tails only.
- Do not send full build logs or broad Markdown scans.

## Build Strategy

- First gate: Windows VST3.
- Second gate: Windows FL Studio host validation and live screenshot diff.
- Later gate: macOS universal VST3 and AU.
- Apple signing/notarization: later optional distribution gate, skipped until credentials exist.

## Packaging Strategy

Tester packages will be created only after build artifacts exist:

- `deliverables/VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST/`
- `deliverables/VINTAGE_RAWNESS_MAC_FL_STUDIO_TEST/`

Current local shell does not expose `cmake` or `msbuild`, so build artifacts must come from GitHub Actions or a shell with the JUCE/CMake/MSBuild toolchain on PATH.
