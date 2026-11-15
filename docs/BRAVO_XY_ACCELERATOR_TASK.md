# BRAVO XY Accelerator — bounded preset-data/menu task

Base: 0406e86. User chose a separate BRAVO-style XY trajectory preset. This is
an authored example, not a reconstruction of the two source songs. Scope is
message-thread preset authoring only; no changes to realtime DSP, COUNT preset
DSP, BRAVO 1/2, layout/assets, latency, or serialized format.

Allowed changes: Source/PluginEditorV2.cpp, Source/PluginStateModel.cpp,
Source/PluginStateModel.h, tests/StateModelTests.cpp. Do not stage, commit, push,
deploy, or touch existing unrelated changes. Leave task and evidence files alone.

- AC-01: Add a separately named right-click item "Load BRAVO XY Accelerator".
  Enable only when Y axis Speed is ON, REC is off, and the selected BAR has no
  existing XY recording. In the async callback recheck those guards and ensure
  it is still the same BAR; use JUCE SafePointer for editor lifetime. Opening or
  cancelling the menu never writes state; preserve the existing Speed toggle.
- AC-02: StateModel loads only the chosen empty valid BAR when Y Speed is on.
  Use a single trajectory publication and existing storage. Preserve all other
  slots, global toggles, and COUNT settings (DEPTH included). Store points with
  (phase, x, y): (0,.45,.50), (.50,.70,.50), (.5001,.45,.50),
  (.70,.70,.65), (.7001,.45,.65), (.83,.70,.80), (.8301,.45,.80),
  (.92,.70,.93), (.9201,.45,.93), (1,.70,1). The X return events are
  0.20 / 0.13 / 0.09 BAR apart. Keep timeSeconds monotonic with these
  normalized times; XY already scales by the entire trajectory span.
- AC-03: Add state-model tests: guard invalid BAR, Speed OFF and overwrite;
  verify one publication, exact point times, round-trip, no unrelated state
  changes. Report native tests NOT RUN if compiler unavailable. Launcher
  verification git diff --check is a whitespace check only, not AC acceptance.

Read only this task and the four scoped files in relevant slices. Report compact
file list, checks, and potential limitations.
