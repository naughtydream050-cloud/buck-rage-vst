# BRAVO XY Accelerator and COUNT 5 measurement verification

Base source: `0406e863cfab7ac62b3de238b03dfc11ca834ac7` plus the existing uncommitted BRAVO XY Accelerator diff.

Scope: verify and, only if necessary, correct the bounded message-thread BRAVO authoring path. Add deterministic COUNT 5 measurement coverage. Do not alter COUNT DSP, XY DSP, BRAVO 1/2 schedules, UI geometry, parameter IDs, state schema, or the 1024-sample latency.

The Jev Context Router remains `observe`. Candidate metadata consists of the BRAVO authoring, COUNT measurement, and CI/artifact bundles. A low-confidence, abstaining, invalid, unavailable, or budget-blocked decision falls back to the existing minimum safety bundle; this does not claim filesystem isolation.

GLM is the primary bounded worker for two sequential tasks: BRAVO guard/persistence evidence, then COUNT all-preset measurement tests. DeepSeek may run once for the same task only after a confirmed GLM transport/provider failure. Test failure, judge UNKNOWN, and timeout alone do not authorize fallback. Codex reviews the final diff and evidence.

Manual-test placement is allowed only after all numbered acceptance criteria, focused native tests, Windows CI, and Jev review are PASS on one source SHA. Candidate, final, and Program Files are excluded. FL Studio listening remains `NOT TESTED`.
