# Live XY fixed-size lock-free FFT contract

Codex-approved bounded implementation, 2026-10-24. Original 21 AC and hashes,
1024-point FFT / 256-hop phase-vocoder equations, Pitch range, stereo linking,
Preset behavior, UI/State/IDs and 1024-sample latency remain unchanged.

JUCE 7.0.12's Windows fallback FFT acquires `processLock` in `perform()`, reached
by both real transform methods (upstream modules/juce_dsp/frequency/juce_FFT.cpp
lines 95..118 and 123..183). CMake enables no alternative Windows FFT backend.
This is a confirmed conflict with AC-14 / R2-AC-20, independent of timing.

Replace only the live XY FFT instance with a private fixed-1024 radix-2 transform.
Constructor precomputes bit-reversal and per-stage float sine/cosine tables.
Scratch/tables live in the instance allocated once during prepare. Each transform
uses fixed loops and that instance's scratch, with no locks, heap allocation,
shared state, logging or I/O. SSE2 vector butterflies are optional acceleration;
the scalar fallback implements the same equations.

Preserve JUCE's interleaved complex forward buffer and real inverse packing,
including conjugate reconstruction and 1/N inverse scale. Do not change the
phase-vocoder's bin mapping or phase equations as part of this backend change.
The existing background-only offline FFT is outside this replacement.

Mandatory checks before acceptance: impulse, DC, Nyquist, bin tones and fixed
noise against JUCE forward/inverse; normalized forward max error <=2e-6,
normalized-input inverse sample error <=2e-6; roundtrip <=2e-6. Run all existing
Pitch frequency, direction independence, stereo, latency, finite and active-Pitch
allocation tests. Compare diagnostic PCM and record numerical differences.
Re-run the exact new CI executable over all 18 host-performance conditions.
No deadline/quality threshold may be relaxed; any failure blocks promotion.
