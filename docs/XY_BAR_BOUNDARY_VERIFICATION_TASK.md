# Bounded Worker task

Implement only the approved verification extension in `tests/RuntimeSmokeTests.cpp`.
Expand `testXyDoesNotSpillIntoNextBar` or its immediate helpers to test 44.1/48/96
kHz; stereo; host blocks aligned to and crossing a BAR boundary; and next BAR OFF
or DRAG. Compare each XY render to its no-XY reference from the first next-BAR
sample, retain the existing 3 ms tail/finite/earlier-XY metrics, and write paired
input/reference/XY-output WAVs for the cases. Do not edit `Source/`, CMake,
workflow, UI, state, params, COUNT, BRAVO, or docs other than no files. If a case
fails, retain a deterministic failing test and report it; do not repair DSP.

Run the focused RuntimeSmoke test if the toolchain is available. Return a compact
result with changed files, exact cases, WAV names, commands and failures.
