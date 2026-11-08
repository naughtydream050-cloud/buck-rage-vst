# Bounded critical BRAVO schedule completion task

Use only the approved BRAVO plan, criteria and measured event map. Verify the
existing implementation and make a minimal correction only if a deterministic
test proves a mismatch. Do not redesign DSP or infer additional source events.

Allowed edits: `Source/BravoProjectPresets.h`, `Source/PluginProcessor.cpp`,
`Source/TimelineScratchEngine.h`, `Source/TimelineScratchEngine.cpp`, and
`tests/TimelineScratchEngineTests.cpp`.

Required facts: BRAVO 1 exactly uses BAR 3,53,54,56,70,99; BRAVO 2 exactly
uses BAR 2,4,7,19,20,24,31,33; no every-four-BAR schedule; BAR 70 does not
alias to BAR 6; and the existing manual 64-BAR map, XY priority and 1024-sample
latency remain untouched. Do not edit UI, state schema, parameter IDs, CMake,
CI, artifacts, candidate, final or Program Files. No allocation, lock,
ValueTree, I/O or logging in audio processing.

Run the two supplied deterministic checks. If native compilation is unavailable
locally, report it as an environment limitation and do not claim CI or listening.
Finish with `WORKER STATUS: COMPLETE` only if no required code correction is
left and with a `WORKER COMPACT RESULT` heading.
