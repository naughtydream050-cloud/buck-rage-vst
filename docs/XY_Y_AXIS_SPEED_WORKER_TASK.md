# Bounded worker task: XY Y-axis Speed

Implement only `docs/XY_Y_AXIS_SPEED_PLAN.md` and satisfy
`docs/XY_Y_AXIS_SPEED_AC.json`.  Read only the listed source/test files and the
two task documents.  Preserve pre-existing dirty BRAVO files.  Do not change
BRAVO schedules, artwork/layout, APVTS IDs, preset algorithms, latency, build
configuration, candidate/final/Program Files, credentials, budgets or routing.

Allowed implementation files: `Source/XYPadEngine.h`, `Source/XYPadEngine.cpp`,
`Source/PluginStateModel.h`, `Source/PluginStateModel.cpp`,
`Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/PluginEditorV2.cpp`,
`Source/UIComponents.h`, `Source/UIComponents.cpp`, `tests/XYPadEngineTests.cpp`,
`tests/StateModelTests.cpp`, `tests/RuntimeSmokeTests.cpp`.

Use an atomic boolean/snapshot for the audio-thread setting.  Right-click must
show `Y axis Speed`, must not clear/add/move XY points, and must persist the
setting.  The multiplier is 0.5 bottom, 1.0 centre, 2.0 top and applies only to
the signed X-derived increment at live grain launch.  Keep X timing and hold.
No FFT.  Add focused deterministic assertions for the criteria.  Run only the
existing focused test/build commands discoverable from CMake/CI; report commands
and results compactly.  Do not self-approve or deploy.
