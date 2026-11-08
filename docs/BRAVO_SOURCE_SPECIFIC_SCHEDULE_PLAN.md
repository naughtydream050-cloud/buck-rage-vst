# BRAVO source-specific schedule

Replace only the generic every-four-BAR BRAVO profiles with immutable schedules
derived from `reports/bravodomo-preset-analysis/source-specific-event-map.json`.
BRAVO 1 events: BAR 3, 53, 54, 56, 70, 99. BRAVO 2 events: BAR 2, 4, 7,
19, 20, 24, 31, 33. Empty song BARs are dry. Event locations are one-based
absolute host BARs, not 64-BAR modulo locations.

Keep IDs 0/1/2, manual 64-BAR data, XY, UI geometry, parameter IDs, latency and
existing DSP algorithms unchanged. The first 64 bars remain projectable into
BAR MAP; no event after BAR 64 may alias back into it. The existing absolute
transport fields must be used. No allocation, lock, ValueTree access, I/O or
logging may be added to processBlock/audio processing.

The mapping is a measurement-backed approximation; it does not claim effect
identity or listening similarity. No candidate, final, Program Files or commit.
