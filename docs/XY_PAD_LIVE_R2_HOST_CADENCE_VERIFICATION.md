# AC-20 host-cadence verification correction

Scope: benchmark scheduling and evidence parsing only. All 21 approved AC,
DSP behavior, thresholds (p99 <= .50, worst <= .80), maximum event rate,
18 conditions, 2 s warm-up and 30 s measured audio per condition are retained.

ETL `xy-ac20-scheduler-20261024-150606.etl` has 4,210,943 events and zero
reported lost events. Target process 16268, main thread 7512, has Ready-state
descheduling gaps up to 2.8229 ms. The longest gaps begin at scheduler priority 1.
Microsoft documents MMCSS priorities 1..7 for threads that consumed their CPU
quota: https://learn.microsoft.com/en-us/windows/win32/procthread/multimedia-class-scheduler-service

The old benchmark runs host-sized callbacks without any inter-callback wait.
This observation supports MMCSS quota demotion in the synthetic tight loop.
It does not prove which historical callback outlier coincides with each gap:
the old metric rows have no callback QPC timestamps.

The corrected test host dispatches blocks on a monotonic sample clock using a
high-resolution waitable timer outside the measured processLive call. Every
callback remains measured; no outlier is filtered, dropped or CPU-time adjusted.
Dispatch lateness and wall duration are separately reported. This measures
processing cost under host cadence; it does not certify end-to-end DAW scheduling.
The DSP production path receives no timer, sleep or scheduling changes.

The existing parser also predates the three max-wall diagnostic fields in the
current executable. Update its strict allowlisted row grammar and mutation
fixtures; do not accept unrecognized fields or relax deadline rejection.

Qualification still requires the exact CI executable and source hashes, local
Ryzen 3 2200U results, all 21 deterministic AC, Jev and Sol approval. A remaining
deadline failure blocks promotion.
