# RUDE HYPE SPEC

RUDE HYPE is a JUCE VST3 dirty saturation plugin using a measured image-first UI pipeline.

## Identity

- Product name: `RUDE HYPE`
- Format target: VST3
- Host target: FL Studio and VST3-compatible DAWs

## UI Contract

- The UI source of truth is `ui/spec/ui-spec.json`.
- `PluginEditor.cpp` is implementation only.
- CI/text-only builds may load `C:\Users\razor\Downloads\S__45752322.jpg` at runtime and crop knobs from the same image.
- SHOUT and BURN are visible image knobs.
- Knob images must rotate as captured, not be redrawn from JUCE primitives.
- Coordinates remain ratio-first in the spec and float-first in generated code.

## DSP Contract

- Audio parameters: `shout`, `burn`.
- `processBlock` must not allocate heap memory.
- UI thread locks and blocking IO are forbidden in realtime audio code.
