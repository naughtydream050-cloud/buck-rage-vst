# GLM task: BRAVO XY Accelerator verification

Read only the context bundle selected by the observe router plus the following bounded files: `Source/PluginEditorV2.cpp`, `Source/PluginStateModel.cpp`, `Source/PluginStateModel.h`, `tests/StateModelTests.cpp`, `docs/BRAVO_XY_ACCELERATOR_TASK.md`, `docs/BRAVO_COUNT_VERIFICATION_PLAN.md`, and `docs/BRAVO_COUNT_VERIFICATION_AC.json`.

Preserve existing uncommitted work. Do not alter realtime DSP, COUNT DSP, BRAVO 1/2, UI geometry/assets, serialization schema, parameter IDs, latency, candidate, final, Program Files, or unrelated files. Do not stage, commit, push, or deploy.

Validate and, only when a deterministic defect exists, minimally repair the existing BRAVO XY Accelerator implementation. Expand `tests/StateModelTests.cpp` only as needed to prove:

1. Y Speed OFF, invalid BAR, recording/overwrite guard conditions leave state unchanged.
2. A valid empty selected BAR loads exactly one 10-point trajectory and has one DSP publication.
3. Opening/cancelling or a stale menu selection cannot write a different BAR; preserve SafePointer and async guard behavior.
4. Existing COUNT preset/depth, BRAVO project-preset ID, adjacent BAR and all other slots remain unchanged.
5. State round-trip preserves the authored points and the required 0.20/0.13/0.09 X-return timing.

If an editor-level behavior cannot be exercised by the existing test harness, add a compact deterministic test seam only if it does not alter plugin behavior; otherwise report the limitation clearly. Run the available focused StateModel target if a compiler exists; otherwise run `git diff --check` and report native test `NOT RUN`. Finish with `WORKER COMPACT RESULT`.
