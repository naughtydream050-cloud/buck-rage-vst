# Bounded Sol continuation: live XY pitch and aligned 1024-sample topology

Plan SHA-256: `afecb981150273788a022eb2c1abd5bc5a65507dbff507f5787f750cefca6407`
Acceptance Criteria SHA-256: `04eff4c6460ba7e7bf4540659390fa3028cd3763cb9bc237bf314ec8604b2834`
Base commit: `0e5852058cc9e8b3717ecddcc141bb9600b10b38`
Task class: `CRITICAL` / realtime DSP.

This task clarifies the implementation topology already required by the approved
Plan; it does not change its audio behavior or acceptance criteria.

## Allowed paths

- `Source/XYPadEngine.cpp`
- `Source/XYPadEngine.h`
- `Source/PluginProcessor.cpp`
- `Source/PluginProcessor.h`
- `tests/XYPadEngineTests.cpp`
- `tests/RuntimeSmokeTests.cpp`

## Required topology

- The live XY path owns exactly 1024 samples of delay, including pitch-vocoder
  scheduling and its wet/ready mask.
- The current processor-wide output delay must be moved to fixed-size delay rings
  for the Dry and preset branches plus the sample-aligned mix controls. Do not add
  a second delay after the final mix.
- At final mix, delayed Dry, delayed preset, delayed depth/bypass/preset-active
  controls, and the XY signal/mask must refer to the same source sample.
- Preserve the existing mix coefficients, preset DSP, public latency declaration,
  1024-sample reported total latency, and delayed-Dry `DEPTH=0` behavior.
- All memory is allocated in `prepareToPlay` / `prepare`; process callbacks remain
  allocation-, lock-, I/O-, logging-, and ValueTree-free.

The live Y path must use the Plan's streaming 1024-point FFT / 256-sample-hop
phase vocoder, 5 ms smoothing, linked stereo phase treatment, and UI mapping
`semitones = 24 * (y - 0.5)`. It must not alter the live X position, direction,
grain timing, or BAR phase.

## Deterministic verification required

Add executed tests for pitch direction/range, stereo linkage and finite output,
independence from X/transport, zero allocations, and exact end-to-end 1024-sample
latency for Dry, preset-only, XY, bypass, and variable block sizes. Run the XY
engine test and Runtime Smoke targets. If no local compiler exists, report them
as `NOT RUN` and use the committed Windows CI run before Jev or promotion.

No UI/state/serialization/parameter/CMake/workflow/preset-algorithm changes.
No candidate or Program Files mutation in this task slice.
