# CHIRP / TRANSFORM / ZIGZAG Sol Plan

Base: `7a62c7a6f1c6642d23bb317ba5827c764e65ffe5`

Implement the three previously unimplemented Count 5 presets in the single
`TimelineScratchEngine`. Each effect captures one fixed 125 ms history window
at BAR entry and keeps it for the selected LENGTH. SPEED controls only the
motion cycle: `cycleSeconds = secondsPerQuarter * 0.5 / SPEED`, with SPEED in
the existing 0.25..4.0 range. DEPTH remains the existing Dry/Wet mix and the
existing 8 ms transition ramp is retained.

CHIRP uses a smooth forward/reverse round trip with gate open for the first
80% of each half-cycle and closed for the final 20%. TRANSFORM uses the same
round trip with four cuts per cycle, each gate open for 50% and closed for
50%. ZIGZAG uses the smooth path points 0, 1, 0.25, 0.75, 0 and has no gate.

All three use process-before-effect Dry input for source-silence detection:
both channels below 1e-4 for 30 ms latches that BAR dry after the transition;
short gaps and zero crossings do not latch; the next BAR is the only relatch
point. Transport STOP remains independent and invalidates the runtime state.

UI, state schema, parameter IDs, PITCH, and existing preset behavior are out
of scope. Candidate promotion requires deterministic checks, successful CI,
Jev 1.13 PASS for every criterion, and Sol final review.
