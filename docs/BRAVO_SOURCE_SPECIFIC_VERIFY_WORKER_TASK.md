# Bounded BRAVO source-specific schedule verification

Read only the approved source-specific schedule plan, criteria, event map, and
the listed implementation/test slices. Do not edit any file.

Verify that BRAVO 1 uses exactly BAR 3, 53, 54, 56, 70 and 99; BRAVO 2 uses
exactly BAR 2, 4, 7, 19, 20, 24, 31 and 33; empty BARs are dry; and no
four-BAR repeating rule or modulo-64 alias can apply to BRAVO events. Confirm
that manual 64-BAR state, XY priority and 1024-sample latency remain outside
the BRAVO schedule change.

Allowed reads only:

- `AGENTS.md`
- `docs/BRAVO_SOURCE_SPECIFIC_SCHEDULE_PLAN.md`
- `docs/BRAVO_SOURCE_SPECIFIC_SCHEDULE_CRITERIA.json`
- `reports/bravodomo-preset-analysis/source-specific-event-map.json`
- `Source/BravoProjectPresets.h`
- `Source/PluginProcessor.cpp`
- `Source/TimelineScratchEngine.h`
- `Source/TimelineScratchEngine.cpp`
- `tests/TimelineScratchEngineTests.cpp`

Do not read source audio, historical reports, unrelated UI, credentials or
other projects. Do not claim native tests, CI or listening were executed.
Return a compact evidence table for AC-01 through AC-05 and a literal
`WORKER COMPACT RESULT` heading.
