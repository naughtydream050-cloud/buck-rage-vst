# 老子-BUCK RAW SHIT- State

## Phase

PHASE_1_UI_FIX_HOST_SCREENSHOT_REVIEW

## Scope Locks

- The supplied UI reference is visual truth.
- UI is image-first; no faceplate, character, cherry blossom, logo, or texture redraw.
- DSP implementation is deferred. Only output gain, real bypass, and atomic stereo metering are in scope.
- Windows VST3 only. macOS is deferred.

## Completed

- Reference faceplate and five circular RGBA knob assets were extracted without redrawing the supplied visual.
- APVTS controls, five presets, functional bypass, oversample state selection, output-gain-only temporary audio path, and atomic stereo metering were implemented.
- Windows VST3 CI passed on run `29497008873`; VST3 binary size is `5897216` bytes.
- Tester ZIP and expanded folder are available under `deliverables/`.
- A host-review revision replaces visible JUCE ComboBox/TextButton controls with transparent hit targets and custom, restrained state text. The faceplate meter slots are darkened so real L/R bars are not visually confused with baked red bars.

## Pending

- Rebuild and FL Studio Windows re-validation of the host-review UI revision.
- Full mastering DSP, macOS build, signing, and notarization are intentionally out of scope.
