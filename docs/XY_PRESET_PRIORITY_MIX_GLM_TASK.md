# Bounded GLM task: XY-preset exclusive output switch

Implement only the user-approved contract in `docs/XY_PRESET_PRIORITY_MIX_PLAN.md`
and `docs/XY_PRESET_PRIORITY_MIX_CRITERIA.json`, bound by
`docs/XY_PRESET_PRIORITY_MIX_CODEX_APPROVAL.json`.

Edit only `Source/XYPadEngine.cpp` and `tests/XYPadEngineTests.cpp`. Do not
edit the Processor, UI, State, parameters, latency, timeline/transport,
existing preset algorithms, CMake, workflows, artifact folders, candidate,
final, or Program Files.

Replace only the preset-active branch of `XYPadEngine::mixLayers` with the
approved switch law. At `xyBlend=0` retain `presetOutput` bit-exact. At
`xyBlend=1`, return `dry*(1-depth) + xyWet*depth`, independent of presetOutput.
Use the existing clamped blend and preserve the preset-OFF path. Do not change
the function signature.

Update focused tests for MIX-01 through MIX-07. Existing `parallel-equal-power`
expectations must be replaced rather than left as passing legacy behavior.
Tests must cover exact endpoints, an intermediate linear crossfade, depth-zero,
two distinct preset outputs at full XY, finite stereo values, and the unchanged
preset-OFF law. Keep tests deterministic. Do not claim that this removes the
unconfirmed `boon` sound.

Run `git diff --check` and the smallest existing XY test command discoverable in
the repository. Report commands, exit codes, changed files, and any blocker.
Do not commit, push, promote, deploy, or call another model. End with literal
`WORKER COMPACT RESULT` and `WORKER STATUS: COMPLETE` only if all requested
steps finish.
