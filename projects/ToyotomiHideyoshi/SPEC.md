# Toyotomi Hideyoshi Phase 1 Specification

## Purpose

A 64-bar Buck/Krump scratch sequencer. Each bar contains 16 independent count slots. Phase 1 implements the UI and durable data model only; audio remains pass-through.

## Visual truth

The supplied 1280 x 853 JPEG is the reference. Preserve its black distressed surface, gold/red/ivory accents, samurai artwork, brush lettering, information hierarchy, and dense panel layout.

## Phase 1 behavior

- Four tabs select bars 1–16, 17–32, 33–48, or 49–64.
- Playing bar and selected bar are independent states.
- The selected bar exposes 16 count slots.
- Each count owns preset, length, speed, pitch, depth, custom-motion flag, and normalized XY motion points.
- Global host-facing values use APVTS; the 1024 detailed count slots use a dedicated ValueTree serialization layer.
- State must round-trip through `getStateInformation` and `setStateInformation`.
- `processBlock` passes input to output, publishes post-pass-through L/R peaks atomically, and reads only minimal playhead data.

## Presets

OFF, FORWARD CUT, BACKSPIN, CHIRP, BABY, TRANSFORM, DRAG, ZIGZAG, TAPE BRAKE, CUSTOM.

## Explicit exclusions

No scratch DSP, reverse DSP, backspin DSP, pitch shift, time stretch, tape stop, filter, gate, stutter, oversampling, post FX, randomize, MIDI learn, undo/redo, or advanced browser.

