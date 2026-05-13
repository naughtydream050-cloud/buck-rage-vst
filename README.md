# RUDE HYPE

JUCE VST3 dirty saturation plugin with image-first RUDE HYPE UI.

- UI source: `C:\Users\razor\Downloads\S__45752322.jpg`
- Canvas: `1592x988`
- Runtime fallback: the editor loads the local JPG and crops SHOUT/BURN knobs when embedded BinaryData is unavailable.
- UI SSOT: `ui/spec/ui-spec.json`

## Design

The full panel remains image-first. SHOUT and BURN use cropped image knobs that rotate in JUCE, so the knob texture and marker rotate as captured from the flat reference image.

## Build

```powershell
git submodule update --init
cmake -B build
cmake --build build --config Release
```
