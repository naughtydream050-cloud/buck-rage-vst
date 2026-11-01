# Bounded GLM task — FORWARD CUT Grid Stutter

Read `AGENTS.md`, `docs/FORWARD_CUT_GRID_STUTTER_PLAN.md`,
`Source/TimelineScratchEngine.h`, `Source/TimelineScratchEngine.cpp`, and the
FORWARD CUT section of `tests/TimelineScratchEngineTests.cpp` before editing.

Implement only the approved contract. Edit only:

- `Source/TimelineScratchEngine.cpp`
- `tests/TimelineScratchEngineTests.cpp`

Do not modify headers, UI, State, parameter IDs, `PluginProcessor`, XY engine,
other presets, build files, or generated reports. Preserve unrelated changes.

Use the existing fixed history ring. `FORWARD CUT` cycle timing must be
`sampleRate * secondsPerQuarter * .25 / speed`; the source interval must be
fixed once safely captured. Retain the established fade/silence behavior,
native forward read rate, source-silence latch and all realtime safety rules.

Add focused deterministic tests for 90/120/180 BPM cycle timing, speed density,
fixed capture under advancing writer, stereo linked source positions, and
existing all-LENGTH behavior. No test may rely on wall-clock timing.

Run the exact relevant existing CMake/CTest command you discover plus
`powershell -ExecutionPolicy Bypass -File tests/forward-cut-ac-check.ps1 -Ac AC-01`
against the generated TimelineScratchEngine log when available. Report actual
commands/results only. Do not commit, push, build a candidate, install a VST or
dispatch another worker. Finish with `WORKER COMPACT RESULT`.
