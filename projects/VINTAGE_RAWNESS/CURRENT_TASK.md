# VINTAGE RAWNESS Current Task

## Phase

MAC_FL_STUDIO_TESTER_DISTRIBUTION_PACK

## Goal

Deliver a macOS Universal VST3 tester package for FL Studio without changing the frozen UI, DSP, presets, or Windows tester build.

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
- macOS tester distribution was explicitly promoted while the public release signing/notarization gate remains deferred.
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
- Added the dedicated `.github/workflows/vintage-rawness-macos.yml` workflow without modifying the Windows workflow.
- GitHub Actions run `29490538497` passed the macOS Release VST3 build and package validation.
- Verified Universal architectures `x86_64` and `arm64`.
- Applied and verified ad-hoc codesign. Developer ID signing and notarization remain intentionally absent.
- CI executed `INSTALL.command` and `UNINSTALL.command` against an isolated temporary HOME and preserved a sentinel plugin.
- CI created `VINTAGE_RAWNESS_MAC_FL_STUDIO_TEST.zip` with macOS `ditto` and revalidated the extracted bundle, command permissions, signature, architecture, UTF-8 documentation, and absence of Windows binaries.
- Artifact `VINTAGE-RAWNESS-MAC-FL-STUDIO-TEST` id `8372271891` was downloaded.
- Local tester ZIP and expanded folder now exist under `deliverables/`.
- FL Studio Mac host validation is still pending.

## Next

1. Send `deliverables/VINTAGE_RAWNESS_MAC_FL_STUDIO_TEST.zip` to a Mac FL Studio tester.
2. Install by right-clicking `INSTALL.command`, selecting Open, and rescanning FL Studio plugins.
3. Record Apple Silicon and Intel Mac host results separately.
4. Keep Developer ID signing/notarization deferred until public distribution is explicitly requested.
5. Return to the pending Windows DSP rebuild and FL Studio audio review without changing the frozen UI.
