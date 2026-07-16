# VINTAGE RAWNESS State

## 2026-05-20

### Phase

MAC_FL_STUDIO_TESTER_DISTRIBUTION_PACK

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
- Host review showed the translucent preset overlay looked like a wrong transparent frame.
- Replaced translucent preset press feedback with image-first button feedback.
- Preset overlays now crop the actual button art from the faceplate and draw that image for selected/pressed states.
- Local harness passed after the image-button feedback change.
- GitHub Actions run `26640134240` passed the image-button feedback Windows VST3 build.
- Downloaded artifact `VINTAGE-RAWNESS-VST3-Windows` id `7291927857`.
- Refreshed `deliverables/VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST/` and `VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST.zip`.
- User confirmed the preset button UI is correct in FL Studio.
- UI freeze is active for visual assets, knob placement, and preset button behavior.
- Added `KrumpWarpEffect` DSP core.
- Integrated envelope-reactive delay-line warp, asymmetric saturation, crush degradation, post filter, dry/wet mix, safety clip, and DC blocker.
- Updated validation for the new DSP core; `vintage-rawness-dsp-report.json` passed with score `10`.
- GitHub Actions run `26758901821` passed the Windows VST3 build.
- Downloaded artifact `VINTAGE-RAWNESS-VST3-Windows` id `7333238344`.
- Refreshed tester VST3 and `VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST.zip` with the Krump Warp DSP build.
- Intensified DSP with random-walk Wow/Flutter, 2-8ms jitter/slop, low-mid pre-EQ, expanded asymmetric saturation, and vintage HPF/LPF bandpass.
- GitHub Actions run `26891457799` passed the Windows VST3 build.
- Downloaded artifact `VINTAGE-RAWNESS-VST3-Windows` id `7387566402`.
- Refreshed tester VST3 and zip with the tape-drift DSP build.
- FL Studio audio review found CRUSH was reading as clipped/broken rather than dirty, and WOBBLE was not bending enough; the desired WOBBLE target is flanger-like "gunya" movement.
- Retuned `KrumpWarpEffect` DSP only:
  - CRUSH now pre-trims/conditions before quantization, uses gentler bit depth, shorter sample hold, slew smoothing, and tiny grit noise to reduce hard clipping.
  - WOBBLE now uses short modulated delay, LFO phase movement, stereo phase offset, light feedback, envelope-reactive depth, and input/delay comb mixing for flanger-style movement.
  - Internal dry/wet returned to the intended fixed `0.72`.
- Local validation `node projects/VINTAGE_RAWNESS/tools/validate_vintage_rawness.mjs` passed after the retune.
- Local VST3 rebuild is still blocked because `cmake` and `msbuild` are not visible in the current shell; GitHub Actions or a proper toolchain shell must produce the next tester artifact.
- Promoted the VINTAGE_RAWNESS Mac test build from deferred to active GitHub Actions coverage.
- Added a macOS universal CI job for `VINTAGE RAWNESS.vst3` and `VINTAGE RAWNESS.component`.
- Mac test distribution now addresses four install blockers: Intel/Apple Silicon mismatch, VST3/AU host mismatch, downloaded quarantine, and unsigned test-bundle rejection via ad-hoc codesign.
- Added `docs/mac-install-test.md` with per-Mac install, quarantine removal, host cache reset, and local verification commands.
- Added `tools/package_test_deliverables.ps1` to move downloaded/expanded artifacts into `deliverables/VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST` and `deliverables/VINTAGE_RAWNESS_MAC_TEST`.
- Added a VST3-only macOS tester packaging workflow while leaving the Windows workflow unchanged.
- GitHub Actions run `29490538497` passed Universal `x86_64` + `arm64` build validation.
- Applied ad-hoc codesign and passed strict deep verification.
- Passed isolated install/uninstall tests, `ditto` ZIP creation, ZIP extraction, executable-bit, UTF-8 document, quarantine, bundle, and Windows-binary exclusion checks.
- Downloaded artifact `VINTAGE-RAWNESS-MAC-FL-STUDIO-TEST` id `8372271891`.
- Saved `deliverables/VINTAGE_RAWNESS_MAC_FL_STUDIO_TEST.zip` and its expanded tester folder.

### Known Issues

- Local CMake/MSBuild are not visible in the current Windows shell.
- FL Studio Mac host validation is pending.
- macOS public release still needs Developer ID signing, notarization, stapling, and a separate release gate.
- Live host screenshot validation is pending.
- Common VST3 install is blocked by administrator permission in the current shell.
- FL Studio scan is pending until the plugin is copied to a scanned folder or the tester folder is added as a custom path.
- Further work must be Stage 2 function/DSP only unless UI phase is explicitly reopened.
- FL Studio audio validation for DIRT/CRUSH/WOBBLE behavior is pending.
- Current DSP tuning target is WOBBLE max collapse depth, click safety, and DIRT/CRUSH harshness balance.
