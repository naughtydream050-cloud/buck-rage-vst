# Bounded GLM task: XY horizontal direction / vertical speed

Implement exactly `docs/XY_PAD_DIRECTION_SPEED_PLAN.md` and satisfy every
criterion in `docs/XY_PAD_DIRECTION_SPEED_AC.json`.

Allowed files: `Source/XYPadEngine.h`, `Source/XYPadEngine.cpp`,
`Source/PluginStateModel.h`, `Source/PluginStateModel.cpp`,
`Source/PluginProcessor.cpp`, `Source/PluginEditorV2.h`, `Source/PluginEditorV2.cpp`,
`tests/XYPadEngineTests.cpp` and `tests/StateModelTests.cpp`.

Do not edit UI artwork/spec, BRAVO schedules, COUNT DSP, parameter IDs,
latency configuration, CMake, workflow, candidate/final/Program Files, or
unrelated files. Do not commit or deploy.

Use persistent fractional live-grain and hold-loop positions so Y affects
reads continuously. Keep all audio-thread work allocation-free and lock-free.
Replace the old opt-in tests with deterministic tests for the formula,
directions, vertical default/retained reverse direction, state migration and
existing regression behaviour. If a native build tool is unavailable, run the
available source-level checks, leave an explicit gap report, and stop; do not
invent a PASS.
