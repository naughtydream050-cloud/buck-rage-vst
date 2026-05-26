# VINTAGE RAWNESS Current Task

## Phase

VINTAGE_RAWNESS_IMAGE_FIRST_UI_ROTATING_KNOBS_BUILD_PIPELINE

## Goal

Create a new image-first three-knob plugin with rotating DIRT/CRUSH/WOBBLE knobs, four preset hit areas, lightweight safe DSP, harness reports, and Windows/macOS build pipeline wiring.

## Current Status

- Project scaffold created under `01_PLUGINS/projects/VINTAGE_RAWNESS`.
- Reference image imported from `C:\Users\razor\Downloads\S__46284845.jpg`.
- Faceplate PNG and three circular RGBA knob assets generated.
- `ui/spec/ui-spec.json` created with canvas, scale, knob bounds, centers, radii, and preset button bounds.
- JUCE image-first editor implemented.
- APVTS parameters and safe DSP skeleton implemented.
- Preset overlay hit areas implemented.
- Harness script added: `tools/validate_vintage_rawness.mjs`.
- Reports generated under `reports/latest/`.
- UI, knob, screenshot bootstrap, and DSP policy reports passed.
- GitHub Actions workflow added for Windows VST3 first.
- macOS VST3/AU is intentionally deferred until the Windows VST3, host UI capture, and local quality gates are stable.
- Local build is blocked because `cmake` and `msbuild` are not visible in the current shell.
- GitHub Actions Windows build passed on run `26168487170`.
- Artifact `VINTAGE-RAWNESS-VST3-Windows` was generated and downloaded.
- Local tester copy exists at `deliverables/VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST/VINTAGE RAWNESS.vst3`.
- FL Studio 2025 was detected, but copying to `C:\Program Files\Common Files\VST3` is blocked by administrator permission.
- Preset button press feedback was strengthened after host review.
- GitHub Actions Windows build passed again on run `26452410502`.
- Updated tester zip now contains the press-feedback VST3 build.

## Next

1. Install the updated VST3 into a FL Studio-scanned folder with administrator permission or add the tester folder as a custom plugin path.
2. Confirm preset buttons visibly depress on mouse down.
3. Run FL Studio plugin scan.
4. Capture a live host screenshot and replace bootstrap screenshot diff with real rendered evidence.
5. Revisit macOS VST3/AU after Windows host validation.
