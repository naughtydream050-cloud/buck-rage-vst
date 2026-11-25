# Bounded Worker task 01

Edit only `tests/RuntimeSmokeTests.cpp`. Do not edit DSP, WAV writing, CMake,
workflow, state, UI or any other file.

In the existing `testXyDoesNotSpillIntoNextBar`, parameterize the existing render
so it deterministically runs all combinations of 44.1/48/96 kHz, next BAR OFF/DRAG,
and a block alignment that both starts on and crosses the BAR boundary. Keep the
existing reference comparison from the first sample of the next BAR, finite check,
pre-boundary XY evidence and tail threshold. Give each failed metric enough labels
to identify rate, preset and alignment. Do not change production code. Run only a
focused test if an available local toolchain permits it; otherwise report NOT RUN.
