# BRAVO 1/2 generic preset plan

Status: APPROVED.  This replaces source-song reconstruction only; it does not
replace the frozen UI, state, XY, latency, or realtime constraints.

`BRAVO 1` and `BRAVO 2` are generic four-BAR patterns, not claims about the
two reference songs.  They override only the manual preset audio layer while
selected.  Manual 64-BAR slots and XY data remain serialized untouched; `Init`
uses the same manual slots immediately.

- BRAVO 1: dry, dry, `transform` at 90% depth, then `forwardCut` at 90% depth.
  Both active BARs use a one-BAR duration and speed 1.0.
- BRAVO 2: dry, dry, `zigzag` at 85% depth and speed 1.0, then `tapeBrake` at
  85% depth and speed 1.0.  The audible pitch movement is only the existing
  read-speed consequence; no independent pitch shifter is added.

The profile arrays are fixed-capacity POD data created before audio processing.
The audio callback only atomically selects manual / BRAVO1 / BRAVO2 slots; it
allocates, locks, logs, and persists nothing.  Existing 1024-sample alignment,
DEPTH, bypass, XY priority, stereo read position, transport recovery, UI,
parameter IDs, and state schema remain unchanged.

Validation must prove exact profile shape, Init/manual identity, state restore,
finite stereo output, depth-zero/bypass behavior, existing regression tests,
and Windows CI.  No manual-test bundle is produced until those gates plus Jev
and Codex review pass.  FL validation remains NOT TESTED.
