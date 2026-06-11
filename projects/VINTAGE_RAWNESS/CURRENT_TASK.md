# VINTAGE RAWNESS Current Task

## Phase

KRUMP_WARP_EFFECT_CORE

## Goal

Evolve VINTAGE_RAWNESS into an input-processing dark industrial warp effect. Keep the frozen image-first UI, DIRT/CRUSH/WOBBLE controls, four presets, and Windows VST3 first.

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
- Host review showed the translucent press overlay looked like a wrong transparent frame.
- Preset button feedback was changed to image-first rendering: the button area is cropped from the faceplate and the real button image is drawn on selection/press instead of drawing a translucent JUCE rectangle.
- Local harness passed after the image-button feedback change.
- Local CMake/MSBuild validation is still unavailable because `cmake` and `msbuild` are not visible in the current shell.
- GitHub Actions Windows build passed on run `26640134240`.
- Artifact `VINTAGE-RAWNESS-VST3-Windows` was downloaded from artifact id `7291927857`.
- Tester VST3 and `VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST.zip` were refreshed with the image-button feedback build.
- User confirmed the preset button UI is correct in FL Studio.
- UI is now frozen for visual assets, knob placement, and preset button behavior.
- Next work must not change `Resources/`, visual `ui/spec/ui-spec.json` values, ImageKnob appearance, or preset button visuals unless UI phase is explicitly reopened.
- Added `Source/KrumpWarpEffect.h` and `Source/KrumpWarpEffect.cpp`.
- Integrated `KrumpWarpEffect` into `PluginProcessor`.
- Implemented envelope follower, delay-line pitch/frequency warp, asymmetric saturation, bit/sample degradation, post filter, dry/wet mix, safety clip, and DC blocker.
- Updated DSP validation to check the new DSP core and no heap allocation in `processBlock`.
- DSP report passed with score `10`.
- GitHub Actions Windows build passed on run `26758901821`.
- Artifact `VINTAGE-RAWNESS-VST3-Windows` was downloaded from artifact id `7333238344`.
- Tester VST3 and `VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST.zip` were refreshed with the Krump Warp DSP build.
- Intensified the Krump Warp DSP toward tape-drift lo-fi collapse:
  - random-walk Wow 0.5-2Hz
  - random-walk Flutter 8-25Hz
  - 2-8ms jitter/slop tied to WOBBLE and envelope
  - 180/340Hz pre-EQ low-mid boost
  - extreme asymmetric tape saturation with expanded drive range
  - vintage BPF with 150-250Hz HPF and 3-5kHz four-stage LPF
- GitHub Actions Windows build passed on run `26891457799`.
- Artifact `VINTAGE-RAWNESS-VST3-Windows` was downloaded from artifact id `7387566402`.
- Tester VST3 and zip were refreshed with the tape-drift DSP build.
- Latest FL Studio audio feedback: CRUSH dirt sounds like hard clipping, and WOBBLE is not yet "gunya"; target is flanger-like movement.
- Retuned `KrumpWarpEffect` DSP only:
  - softened CRUSH with input conditioning, gentler bit depth, shorter sample hold, slew smoothing, and tiny grit noise
  - rebuilt WOBBLE around short modulated delay, stereo LFO offset, light feedback, envelope-reactive depth, and comb-style input/delay mix
  - restored fixed dry/wet to `0.72`
- Local validation passed with `node projects/VINTAGE_RAWNESS/tools/validate_vintage_rawness.mjs`.
- A new Windows VST3 artifact still needs to be built via GitHub Actions or a shell with CMake/MSBuild.
- Added macOS universal GitHub Actions build coverage for both VST3 and AU.
- Added Mac test install notes for the four common Mac blockers: universal arch, VST3/AU format coverage, quarantine removal, and ad-hoc codesign verification.
- Added a packaging script to move expanded artifacts into Windows and Mac test deliverable folders.

## Next

1. Push the latest DSP and macOS workflow changes to GitHub.
2. Let GitHub Actions build Windows VST3, macOS universal VST3, macOS universal AU, and reports.
3. Download/expand artifacts and run `projects/VINTAGE_RAWNESS/tools/package_test_deliverables.ps1`.
4. Install the updated Windows tester VST3 in FL Studio.
5. Send the Mac test zip to the four Macs and follow `docs/mac-install-test.md`.
6. Check whether CRUSH now reads as dirty lo-fi instead of clipped, and whether WOBBLE gives flanger-like gunya movement.
