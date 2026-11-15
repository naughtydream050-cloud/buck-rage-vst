# GLM task: COUNT 5 all-preset end measurement

Read only the context bundle selected by the observe router plus `Source/TimelineScratchEngine.cpp`, `Source/PluginProcessor.cpp`, `tests/RuntimeSmokeTests.cpp`, `docs/COUNT_PRESET_END_TAIL_AUDIT.md`, `docs/BRAVO_COUNT_VERIFICATION_PLAN.md`, and `docs/BRAVO_COUNT_VERIFICATION_AC.json`.

Add measurement-only deterministic coverage. Production files `Source/TimelineScratchEngine.cpp` and `Source/PluginProcessor.cpp` are read-only for this task. Do not change COUNT DSP, XY DSP, BRAVO schedules, UI, state/schema, parameters, latency, candidate/final/Program Files, or unrelated changes. Do not stage, commit, push, or deploy.

Use a phase-coded stereo input and an independently rendered dry baseline, aligned by the existing fixed 1024-sample latency. Cover all ten preset enum values (OFF, CUSTOM and the eight factory presets) with XY absent. For each preset cover: (a) a LENGTH ending inside a BAR and (b) a one-BAR effect reaching the BAR boundary; then vary next BAR OFF versus active and DEPTH 0 versus 100. Record, in a compact JSON/CSV report written by the existing test only, preset, condition, residual classification (`old_processed_pcm`, `temporary_dry_attenuation`, `no_difference`), residual duration samples and channels. Do not suppress unfavorable measurements.

The test must distinguish source history/processed PCM from mere dry-level attenuation by comparison with both the delayed dry baseline and prior processed output. Validate that 1024 samples is the common path delay, not a special effect tail. Ensure all test output is bounded and deterministic. Run the focused RuntimeSmoke target if available; otherwise report native test `NOT RUN` and run `git diff --check`. Finish with `WORKER COMPACT RESULT`.
