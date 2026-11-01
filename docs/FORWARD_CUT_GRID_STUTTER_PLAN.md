# FORWARD CUT — Grid Stutter implementation contract

## Authorized scope

Convert only the existing `FORWARD CUT` DSP from a fixed-seconds retrigger to a
host-tempo-synchronised, fixed-capture Grid Stutter.  Keep the existing preset
ID, UI, saved State, parameter IDs, XY engine, all other preset algorithms and
1024-sample output alignment unchanged.

`LENGTH` remains the active effect duration. `SPEED` remains in `[0.25, 4.0]`
and controls repeat density. No beat/onset/Kick detection is introduced.

## DSP contract

At BAR entry, `FORWARD CUT` derives a repeat cycle in samples:

`cycleSamples = sampleRate * secondsPerQuarter * 0.25 / clamp(speed, .25, 4)`.

This is a sixteenth-note cycle at Speed 1, scaled by Speed and host tempo.  On
the first safe capture after entry, lock one source interval equal to the wet
part of that cycle from the existing history ring. Re-read this same interval
at native forward rate each cycle; retain the existing fade-in/fade-out and
silent tail. Never read outside the fixed ring/generation range.

The input is still written once per sample and the wet path makes one bounded
interpolated read per channel/sample. No new allocation, lock, file I/O or
additional history storage is allowed in `process`.

## Acceptance criteria

1. At 90/120/180 BPM, Speed 1 cycle is respectively one sixteenth-note in
   samples within one sample of the formula above.
2. Speed .25/1/4 produces strictly decreasing cycle sizes and increasing
   retrigger counts for the same active duration.
3. Capture is fixed after first safe capture; the writer never advances the
   loop source and both stereo channels use the same sample positions.
4. `LENGTH` still limits active duration; all five LENGTH values remain
   covered by the existing regression.
5. Dry fallback, transport discontinuity, silence latching, finite output and
   click-safe fades retain existing deterministic coverage.
6. No UI, State, APVTS ID, XY or non-FORWARD-CUT preset source changes.

## Checks

Extend `TimelineScratchEngineTests` with deterministic tempo/cycle/capture
assertions. Run the project's existing TimelineScratchEngine test command and
the focused forward-cut AC checker. Candidate promotion and host installation
are out of scope.
