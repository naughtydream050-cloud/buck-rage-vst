# XY Live R2 RED-test contract

## Authority and fixed inputs

- Approved plan: `docs/XY_PAD_LIVE_R2_PLAN.md`, SHA-256 `C873F27A6AA107AE87C5A4686820B1CB3D9A9D02F2C2C0E5FEEB9396895BB5F0`.
- Acceptance criteria: `docs/XY_PAD_LIVE_R2_AC.json`; its hash is recorded in the critical-task receipt.
- Baseline source is the current committed HEAD named in that receipt.
- This is only the first RED-test phase. Do not modify production source or any non-test file.

## Allowed scope

- `tests/XYPadEngineTests.cpp` only.

## Required work

Add deterministic tests against the current live XY engine that demonstrate the two approved R2 defects before repair:

1. Ramped X trajectories at normalized rates `+0.01`, `+0.1`, `+0.5`, `-0.01`, `-0.1`, and `-0.5` units/second, across 44.1, 48, and 96 kHz, must expose the current per-sample delta threshold treating normal motion as hold. Expected moving/hold state must be computed independently from normalized trajectory velocity and the Plan's 0.001 hold threshold; do not derive expected values from engine flags.
2. A transition from an old grain to a retargeted grain must be measured from rendered audio samples and compared to an independent equal-power reference using the approved 3 ms envelope. The test must detect the current non-equal-power/incorrectly normalized blend. Include a deterministic mutation probe or equivalent test self-check proving that a deliberately altered envelope fails.

Tests must inspect actual audio/state behavior, not merely source strings or the presence of a counter. Avoid cumulative-OR assertions: check each scheduled trajectory and each sample-window result independently. Keep test runtime bounded and deterministic.

## Verification and stop conditions

- Run the narrowest available test/build command for `XYPadEngineTests` if the configured toolchain exists.
- Run `git diff --check`.
- Do not change production code, Plan, AC, public parameters, UI, State, or transport behavior.
- Do not commit, push, promote, or change the candidate.
- If the existing test seams cannot observe either required behavior without a production-code change, stop and report the exact seam and evidence needed; do not broaden scope.
- Report exact changed paths, commands, exit codes, baseline RED assertions, and any unavailable local toolchain. Do not claim RED unless an executed test fails for the targeted behavioral assertion rather than setup/compile failure.
