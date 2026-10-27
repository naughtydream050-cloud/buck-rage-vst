# Critical bounded implementation: XY independent-pitch removal

Implement only the approved `docs/XY_NO_PITCH_PLAN.md` and `docs/XY_NO_PITCH_CRITERIA.json` from source `de111a418ad3da2a4d0df764a31c08a64420f32f`.

Allowed files: `Source/XYPadEngine.cpp`, `Source/XYPadEngine.h`, `tests/XYPadEngineTests.cpp`, and `tests/XYLegacyPitchReference.h` if a test-only reference needs changing. Do not edit UI, State, parameters, Processor, preset engine, CMake, workflow, artifacts, candidate, Program Files, or final output. Preserve pre-existing untracked files.

Work in this order:
1. Add deterministic RED-capable tests proving current Y changes XY audio and X grain reading is fixed at plus/minus one sample per output sample. Tests must use independent fixed PCM and must not accept cumulative success.
2. Remove the XY live FFT/phase-vocoder processing and its pitch-only allocations/state. Y remains an audio-neutral stored/displayed coordinate. Keep fixed 1024-sample XY output alignment using only preallocated delay state; do not alter `PluginProcessor` or its reported latency.
3. Change each live grain to own a fixed valid source span and a fractional source read increment calculated from the normalized X slope. Read with stereo-linked linear interpolation. A constant X path must be stationary/micro-loop; increasing and decreasing X must naturally produce forward/reverse rates. Preserve current tail/crossfade safety; never read outside retained history.
4. Extend tests for Y neutrality, X ±0.5/±1/±2 rates, direction, finite/stereo output, prior capture/hold/re-entry behavior and no audio-thread allocation.
5. Run `git diff --check` and the smallest existing local test command if discoverable; report every command, its exit code, changed files, and unresolved test/build requirements.

No candidate deployment, commit, push, model calls, budget changes or redesign. Finish with literal `WORKER COMPACT RESULT` and `WORKER STATUS: COMPLETE` only if every allowed implementation step is completed; otherwise report `INCOMPLETE` with the exact blocker.
