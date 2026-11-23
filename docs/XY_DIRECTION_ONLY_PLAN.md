# XY direction-only / continuous Y speed

Base: 414305cac99fd906e6d01e9a797b528f0e9825bf. Current D manual-test is preserved.

X changes only direction and start/reversal/jump source position. The magnitude of dX/dBAR is never a source-read increment. Forward for increasing X; reverse for decreasing X. A truly vertical path retains prior direction, or forward if none. Y controls the signed read increment continuously: magnitude = 0.5+0.5Y+Y^2. No optional mode. A horizontal/diagonal active path has one continuously advancing fractional source cursor; no 10-ms re-pick or grain restart for ordinary motion. Existing 3-ms transition applies only on reversal or discontinuous X jump. Vertical motion loops the X-selected short sound slice with seam crossfade.

Preallocate an XY-only bounded history of up to 10 seconds in prepare. Read only valid past PCM, never future/expired data. On exhaustion or lack of material fade XY to dry with existing mask; do not fake continuity by jumping to a new slice. Reacquire after valid material returns. Keep the 1024-sample alignment and existing XY priority, stereo shared read position, BAR 1-64, BRAVO 1/2, COUNT 5 DSP, UI, saved trajectories, parameter IDs and public state unchanged. No audio-thread allocations, locks, I/O, ValueTree or logs.

Use old-vs-new deterministic read-position traces and actual-path WAVs from a sustained tone and drum loop. Do not claim listening success from tests alone. Same-SHA CI, 18-condition benchmark, Jev per AC and Sol diff review gate a new manual-test D bundle; old bundle, candidate, final and Program Files are untouched. FL Studio listening remains NOT TESTED pending user confirmation.
