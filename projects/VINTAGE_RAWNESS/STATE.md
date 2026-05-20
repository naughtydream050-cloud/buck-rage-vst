# VINTAGE RAWNESS State

## 2026-05-20

### Phase

VINTAGE_RAWNESS_IMAGE_FIRST_UI_ROTATING_KNOBS_BUILD_PIPELINE

### Done

- Created the VINTAGE_RAWNESS project separately from RUDE_HYPE.
- Imported the supplied UI image as `ui/reference/reference.png`.
- Created `Resources/faceplate_vintage_rawness.png`.
- Created circular RGBA knob assets:
  - `Resources/knob_dirt.png`
  - `Resources/knob_crush.png`
  - `Resources/knob_wobble.png`
- Implemented image-first JUCE editor with three rotating knobs.
- Implemented four preset button hit areas.
- Implemented safe APVTS DSP skeleton.
- Created `ui/spec/ui-spec.json`.
- Added `tools/validate_vintage_rawness.mjs`.
- Generated reports:
  - `reports/latest/vintage-rawness-context-pack.json`
  - `reports/latest/vintage-rawness-ui-report.json`
  - `reports/latest/vintage-rawness-knob-report.json`
  - `reports/latest/vintage-rawness-dsp-report.json`
  - `reports/latest/vintage-rawness-screenshot-diff.json`
- Added GitHub Actions workflow for Windows VST3 first.
- Deferred macOS VST3/AU until Windows host validation and live screenshot evidence are stable.

### Known Issues

- Local CMake/MSBuild are not visible in the current Windows shell.
- GitHub Actions Windows build has not been run yet for VINTAGE RAWNESS.
- macOS VST3/AU is intentionally deferred.
- Live host screenshot validation is pending.
- Tester packages are pending until build artifacts exist.
