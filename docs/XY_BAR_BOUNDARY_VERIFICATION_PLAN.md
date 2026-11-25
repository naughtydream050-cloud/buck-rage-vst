# XY BAR boundary verification extension

Base source: `0567c88eeef75c7fb22bdc78e4c55e5f6c7b2d9f` (contains boundary fix `4539878`).

Verify only the observed symptom: a preceding BAR's XY audio must not reach the
next BAR/COUNT. Extend the processor smoke test to render reference/no-XY and XY
paths at 44.1, 48 and 96 kHz, stereo, with host blocks both aligned to and crossing
the BAR boundary. Cover next BAR OFF and active preset. Emit paired comparison WAVs
for the boundary cases into the test diagnostics directory.

Do not edit `Source/`, UI, state, parameter IDs, COUNT 5 algorithms, BRAVO,
trajectory semantics or 1024-sample latency. If the expanded test exposes a real
failure, stop after recording the deterministic reproduction; no DSP repair is in
this task. Preserve all existing performance observations. New D placement requires
the exact-source CI, performance gate, Jev AC verdicts and Codex review.
