# Approved bounded AC-10 verification contract

Task: xy-live-ac10-verification-20261023
Source HEAD: 24ebeb066d637e2d65445eeac2eb0f8404f5a60a
Approved Plan: docs/XY_PAD_LIVE_LOW_LATENCY_PLAN.md
Approved AC: docs/XY_PAD_LIVE_LOW_LATENCY_AC.json, all sixteen unchanged.

Sol authorizes tests only. Do not modify production DSP, UI, state, parameters,
Plan, AC, shared Harness, budgets, models, CI workflow, or git history. No commit,
push, release approval or promotion by the worker.

Allowed edit: tests/RuntimeSmokeTests.cpp only. Use apply_patch. Preserve every
existing assertion. If production behavior contradicts the expectations below,
report the conflict; do not weaken the test or repair production code.

Add a compact processor-level AC-10 test alongside
testXyLayerIndependenceAtProcessorBoundary, called by main. Existing cases cover
XY-only (preset OFF), combined, no trajectory, bypass and STOP. The missing proof
is that unavailable XY never disables or attenuates the existing preset:

1. Render two fresh processors with identical DRAG configuration and stereo input;
   only one has an XY trajectory. Use 48 kHz, host sync ON, a long BAR, depth 0.8,
   and deterministic input. The trajectory processor's delayed ready mask must be
   exactly zero through cold history collection. Every corresponding output sample
   in this initial unavailable interval must equal the preset-only processor
   bit-for-bit. Require nonzero preset-versus-delayed-Dry difference in at least one
   eligible short window; prevent a vacuous all-Dry PASS.
2. Continue signal long enough for XY mask to reach one and assert nonzero XY
   contribution. Insert at least 100 ms of both-channel silence, then resume input
   in the SAME BAR. After the 30 ms hold + 8 ms ramp + 1024-sample pipeline drain,
   compare EVERY sample while the XY mask is zero to the matching preset-only
   processor bit-for-bit. Require the silent interval was actually exercised and
   the preset retains its expected nonzero history tail, where available.
3. Resume input without BAR reset. Assert the mask reacquires after valid new
   history and the output again differs from preset-only. Use per-window counts
   and finite checks; never OR one successful sample across a whole BAR.
4. Add a no-trajectory + active-preset paired baseline if useful; compare it to
   existing legacy delayed-preset test, rather than inventing a gain correction.

Add only test-access helpers needed for per-sample ready-mask reads inside the
existing RuntimeSmokeTestAccess. All test buffers and assertions run outside the
audio thread. Print compact PASS names and numeric METRIC counts/errors for each
state (warmup, active, silence-unavailable, resumed) so CI evidence is auditable.

Use a preallocated AudioBuffer with fixed blocks and an independent expected
sample clock. Account explicitly for the existing 1024-sample output latency.
Do not compare silence/transition/zero-crossing samples using an audible threshold
alone. Fail if a required interval is never reached. Add no dependencies.

Local C++ toolchain is absent. Run git diff --check as the worker's preliminary
verification. Report C++ execution NOT RUN; Windows CI will execute real assertions
before any AC PASS or candidate decision. Final compact result must state changed
file, preliminary check, and the pending CI requirement. Do not claim the entire
project or AC-10 passed from git diff --check.
