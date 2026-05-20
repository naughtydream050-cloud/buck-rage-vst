# VINTAGE RAWNESS SPEC

## Identity

`VINTAGE RAWNESS` is a three-knob image-first vintage destruction plugin for Krump and Buck track melody treatment.

The aesthetic target is ruined metal, chain weight, oxidized silver, and aggressive vintage decay.

## UI Authority

- `ui/spec/ui-spec.json` is the machine-readable source of truth.
- `ui/reference/reference.png` is the visual reference.
- `Resources/faceplate_vintage_rawness.png` is the runtime faceplate.
- `Resources/knob_dirt.png`, `Resources/knob_crush.png`, and `Resources/knob_wobble.png` are rotating RGBA assets.
- `PluginEditor.cpp` consumes layout constants and assets. It is not the UI authority.

## DSP

Processing order:

1. DIRT: saturation and wave shaping
2. CRUSH: bit reduction and sample hold reduction
3. WOBBLE: irregular random-walk modulation

Constraints:

- no heap allocation in `processBlock`
- APVTS managed parameters
- smoothed parameter changes
- denormal protection
- DC blocker
- output gain sanity

## 9/10 Gate

The project cannot be treated as ready unless the harness score is at least 9/10 and these checks pass:

- ui-spec validation
- knob alpha validation
- knob rotation validation
- hit-area validation
- DSP policy check
- Windows VST3 build
- macOS universal VST3/AU build
