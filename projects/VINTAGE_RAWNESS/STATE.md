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
- GitHub Actions run `26168487170` passed the Windows VST3 build.
- Downloaded `VINTAGE-RAWNESS-VST3-Windows` artifact.
- Verified extracted VST3 binary is non-empty: `6948352` bytes.
- Copied the VST3 bundle into `deliverables/VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST/`.
- Detected FL Studio 2025 at `C:\Program Files\Image-Line\FL Studio 2025\FL64.exe`.
- Fixed preset button feedback so pressed buttons visibly sink with a darker inset and bottom highlight.
- GitHub Actions run `26452410502` passed the updated Windows VST3 build.
- Replaced the tester VST3 and zip with the updated artifact.

### Known Issues

- Local CMake/MSBuild are not visible in the current Windows shell.
- macOS VST3/AU is intentionally deferred.
- Live host screenshot validation is pending.
- Common VST3 install is blocked by administrator permission in the current shell.
- FL Studio scan is pending until the plugin is copied to a scanned folder or the tester folder is added as a custom path.
- Host confirmation of the new preset press motion is pending.
