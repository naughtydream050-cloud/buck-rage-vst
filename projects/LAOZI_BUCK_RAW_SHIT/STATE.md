# 老子-BUCK RAW SHIT- State

## Phase

PHASE_2_DSP_IMPLEMENTATION_AND_PRESET_TUNING

## Scope Locks

- The supplied UI reference is visual truth.
- UI is image-first; no faceplate, character, cherry blossom, logo, or texture redraw.
- Image-first UI remains frozen. DSP is now limited to PRESSURE saturation, KICK low-band impact, AURA high-band/M-S width, GLUE linked compression, DC blocking, output gain, bypass, and atomic stereo metering.
- Windows VST3 only. macOS is deferred.

## Completed

- Reference faceplate and five circular RGBA knob assets were extracted without redrawing the supplied visual.
- APVTS controls, five presets, functional bypass, oversample state selection, output-gain-only temporary audio path, and atomic stereo metering were implemented.
- Windows VST3 CI passed on run `29497008873`; VST3 binary size is `5897216` bytes.
- Tester ZIP and expanded folder are available under `deliverables/`.
- A host-review revision replaces visible JUCE ComboBox/TextButton controls with transparent hit targets and custom, restrained state text. The faceplate meter slots are darkened so real L/R bars are not visually confused with baked red bars.
- The DSP preset bank uses 12 sound-character names, never artist names, with reference material supplied for RAZOR FACE, BRAVODOMO, MOZARF, and RUGA.

## Pending

- Windows CI and FL Studio re-validation of the Phase 2 DSP build.
- True oversampling processing/latency compensation, macOS build, signing, and notarization remain out of scope.
