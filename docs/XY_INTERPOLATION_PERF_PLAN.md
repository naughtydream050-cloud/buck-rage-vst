# XY trajectory lookup worst-case optimization

Base: 45398787674c32f05da6a0278ba1fce95e5c64e4. Preserve unrelated dirty files and the BAR-boundary fix.

The live XY renderer scans up to 256 trajectory timestamps from index 1 on every sample. Replace only that search with a bounded lower-bound binary search returning exactly the same first timestamp >= query, including duplicate timestamps, first/last endpoints, reverse strokes and discontinuities. Do not change interpolation arithmetic, PCM, timing, DSP routing, UI, state, COUNT, BRAVO or 1024-sample latency. No allocation, lock or I/O on the audio thread.

Before/after deterministic tests must compare returned segment and X/Y/slope exactly over dense, duplicate, forward/backward and endpoint queries. Run all native regressions, same-source Windows CI and 18-condition 2-second warmup/30-second benchmark. Preserve all earlier adverse measurements. For release, two fresh normal-priority 18-condition runs must each satisfy p99 <= 50% and maximum <= 80% of callback deadline, including 96 kHz/64 maximum <= 0.533 ms. Jev individual AC review and Codex final review precede SHA-verified placement in a new D manual-test folder. Existing candidate, final, Program Files and old manual versions remain untouched; FL listening is NOT TESTED.
