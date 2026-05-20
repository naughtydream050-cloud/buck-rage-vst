# RUDE HYPE SPEC

RUDE HYPE is a JUCE VST3 dirty saturation plugin using a measured image-first UI pipeline.

## Identity

- Product name: `RUDE HYPE`
- Project path: `01_PLUGINS/projects/RUDE_HYPE`
- Format target: VST3
- Host target: FL Studio and VST3-compatible DAWs

## UI Contract

- The UI source of truth is `ui/spec/ui-spec.json`.
- `PluginEditor.cpp` is implementation only.
- The full panel is drawn from `Resources/faceplate_rude_hype.png`.
- SHOUT and BURN are image knobs cropped from the flat reference image.
- Knob images must rotate as captured, not be redrawn from JUCE primitives.
- The faceplate asset keeps its full reference resolution; the plugin editor displays it at `0.42` scale for a natural VST host size.
- Knob assets must be RGBA PNG with all pixels outside the circular mask fully transparent.
- Coordinates remain ratio-first in the spec and float-first in generated code.
- CI/text-only builds may load `C:\Users\razor\Downloads\S__45752322.jpg` at runtime and crop knobs from the same image. This is allowed only for live host capture bootstrap; distribution builds should embed assets.

## DSP Contract

- Audio parameters:
  - `shout`
  - `burn`
- `processBlock` must not allocate heap memory.
- UI thread locks and blocking IO are forbidden in realtime audio code.

## Quality Gate

- Local JSON reports are the machine execution layer.
- Large images stay local.
- LLM handoff should prefer compact context packs and report summaries.
- Final acceptance requires:
  - UI spec validation passed.
  - knob rotation validation passed.
  - knob alpha validation passed.
  - hit-area validation passed.
  - screenshot diff scored.
  - live host capture when a fresh VST3 is available.
