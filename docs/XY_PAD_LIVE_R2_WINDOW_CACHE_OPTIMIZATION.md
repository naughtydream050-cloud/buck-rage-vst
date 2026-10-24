# XY R2 bounded corrections approved by Codex, 2026-10-24

All original Plan/AC hashes and 21 requirements remain unchanged.

RED: test-only commit ba3bf79e9d07d4962270ec600be44ef8063fd1ae,
CI 40074225237, XY build exit 0, XY test exit -1073741819 (0xC0000005).
The added cases differ only in reverse physical ring offsets 1..4 and AVX/SSE
selection. The predecessor 64833c0 passes existing DSP tests. The SSE reverse
pair increments use unchecked unsigned decrement; offset 1 crosses below zero.
Fix only this decrement to wrap zero to capacity-1, matching scalar reads.

Performance RED: CI 40073863430 executable SHA256
3BA821167E9CCEEB8C9E656A11DB7747657B02FF0D205E206D59FDD6AA7909F1,
44.1 kHz / 64 samples, +12 st, paced maximum-event-rate workload:
p99 deadline ratio .398760, worst .911423 (> .80), 30.0037 wall seconds,
1,317,840 events / 1,323,008 samples. Full baseline matrix remains recorded.

Approved equivalent optimization: precompute the 1024 float Hann values during
prepare using exactly the existing expression and float cast. The analysis and
synthesis stages read these values instead of recomputing the same cosine four
times per stereo FFT hop. Keep all bin math, phase processing, FFT size/hop,
pitch control, stereo linking, output timing and the 1024-sample delay unchanged.

Store the window in one fixed heap allocation during prepare, not inline in the
processor object. The initial inline-array build passed the new XY checks but
Runtime Smoke exited with 0xC00000FD (stack overflow) in CI 40074989967. Preserve
the processor's stack footprint and reset/check the allocation on all prepare,
failure and release paths. Process-time allocation remains prohibited.

Verification: compare every cached value to the prior expression with exact float
equality at 44.1/48/96 kHz, rerun all pitch/delay/stereo tests and ring-offset
tests, preserve RED/GREEN source/run IDs, compare diagnostic PCM, then measure
the exact new Release executable across all 18 conditions. No threshold or
outlier exclusion is permitted. Remaining failure blocks candidate promotion.
