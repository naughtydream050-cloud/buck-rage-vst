# XY Live R2 RED-test repair contract

## Authority

- Approved plan: `docs/XY_PAD_LIVE_R2_PLAN.md`, SHA-256 `C873F27A6AA107AE87C5A4686820B1CB3D9A9D02F2C2C0E5FEEB9396895BB5F0`.
- Approved AC manifest: `docs/XY_PAD_LIVE_R2_AC.json`, SHA-256 `42F874D962B75400BC8AE8F0CBE737E13AD05753D60E51ED7999AAE50C8C9195`.
- Current committed base is recorded in the dispatch receipt. Replace the previous invalid uncommitted test block; it is known not to compile and is not evidence.

## Scope

- Modify only `tests/XYPadEngineTests.cpp`.
- Production implementation is explicitly out of scope for this phase.
- Preserve existing tests and project changes outside the R2 helper block.

## Tests to implement

The test file already defines `XYPadEngineTestAccess` as a friend. Use the actual current fields and public `processLive` path; do not invent engine APIs. Relevant existing members include `XYPadEngine::Transport::phasePerSample`, `liveStationaryFrames`, `liveHoldReady`, `liveDirection`, `liveWriteSerial`, `liveGrains`, `liveGrainSnapshots`, and `liveTransitionFrames`.

### 1. Live normalized-velocity RED test

- Drive `processLive` with deterministic finite stereo input and a two-point BAR trajectory for each signed normalized X velocity `0.01`, `0.1`, and `0.5` units/second at sample rates 44100, 48000, and 96000 Hz.
- Set the independent host transport clock to one normalized BAR per second (`phasePerSample = 1 / sampleRate`); construct each trajectory so its independently computed derivative `dX/dphase * phasePerSecond` is exactly the tested velocity. Keep every trajectory separate; do not use a cumulative OR result.
- Expected moving/hold is computed independently from the approved Plan thresholds and required hold duration `ceil(0.002 * sampleRate)`. Observe actual `liveStationaryFrames`, `liveHoldReady`, and `liveDirection` only after processing samples.
- In the current baseline, the 0.01 unit/second ramps must demonstrably enter the old hold detector because per-sample delta is below `1e-6`; the 0.1 and 0.5 ramps must remain moving. Preserve this targeted baseline failure as RED evidence. Include both positive and negative direction.

### 2. Actual live equal-power RED test

- Drive the public `processLive` path with primed, non-silent constant stereo input; produce a stable old grain, then a deterministic X discontinuity that triggers the production live retarget/crossfade.
- Capture rendered audio and mask sample-by-sample, accounting for the fixed 1024-sample pipeline delay. Compare the actual transition region to an independent branch-normalized reference using `T=max(2,round(0.003*sampleRate))`, `q=k/(T-1)`, `a=cos(pi*q/2)`, `b=sin(pi*q/2)`. For identical constant branch inputs, assert the midpoint gain is `sqrt(2)` relative to one branch, as specified (measure/print the expected +3.0103 dB peak; do not add a limiter or gain correction).
- If needed, seed private fixed-capacity live state through `XYPadEngineTestAccess`, but still call `processLive` to render the transition. Do not test only the formula or `renderScrubAndPitch`, which is not the live path.
- Add a deterministic mutation self-check: perturb the reference envelope and assert that the same measured-audio comparison rejects it. No source-string-presence check is acceptable.

## Test integration and truthful reporting

- Replace/remove all invalid helpers added by the prior incomplete attempt; no undefined API, unbalanced namespace, unused helper-only assertion, or cumulative-OR success condition.
- Register and execute the tests from `main()` using the existing `check()` runner.
- Run the narrowest available `ToyotomiHideyoshiXYEngineTests` build/test. A compile/setup failure is not RED. A RED claim is valid only if the executable ran and the named behavior assertions failed while unrelated setup/build passed.
- Run `git diff --check` and report each exact command and exit status.
- Do not alter production source, Plan/AC, UI, State, transport, parameters, commit, push, or candidate.
- If actual audio behavior cannot be made observable via the current seams in this bounded task, stop and report a specific observation seam needed; do not fabricate results.
