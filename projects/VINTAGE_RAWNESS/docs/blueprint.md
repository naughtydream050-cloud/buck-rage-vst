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
- Mac tester gate: Universal macOS VST3 package for FL Studio, ad-hoc signed and not notarized.
- Later public-release gate: Developer ID signing, notarization, and optional AU distribution.
- Apple signing/notarization: later optional distribution gate, skipped until credentials exist.

## Packaging Strategy

Tester packages will be created only after build artifacts exist:

- `deliverables/VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST/`
- `deliverables/VINTAGE_RAWNESS_MAC_FL_STUDIO_TEST/`

Current local shell does not expose `cmake` or `msbuild`, so build artifacts must come from GitHub Actions or a shell with the JUCE/CMake/MSBuild toolchain on PATH.

GitHub Actions run `29490538497` produced the Universal `x86_64` + `arm64` Mac FL Studio tester artifact `VINTAGE-RAWNESS-MAC-FL-STUDIO-TEST` id `8372271891`.

The Mac tester package is created on `macos-14` with `ditto`, carries an ad-hoc signature, installs into the user VST3 folder without administrator rights, and explicitly does not claim Developer ID signing, notarization, Gatekeeper clearance, or FL Studio host validation.

Host review found the preset button press state too subtle. The overlay now adds a pressed offset, darker inset, and lower highlight while keeping the faceplate image-first.
