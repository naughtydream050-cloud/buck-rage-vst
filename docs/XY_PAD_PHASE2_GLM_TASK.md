# Bounded CRITICAL implementation task: XY Pad Phase 2

Implement only the Codex-approved plan `docs/XY_PAD_PHASE2_PLAN.md`.

- Base/source HEAD: `62c61490607197d13ad2c8028da443bc18df87dc`
- Plan SHA-256: `33e4f62f645dc3b79b8a79f53d5844689675368e46a93ea8fa59972feb26e981`
- Criteria SHA-256: `c48befa1e5d5ff74bffc37dc1b87e2dda06d8eced6ef2550c5ddc23ec9d2123a`
- Implement AC-01 through AC-14 only. AC-15/16 are Jev/Codex release gates and must not be self-approved.

Allowed files:

- `Source/XYPadEngine.h` and `.cpp` (new)
- `Source/PluginProcessor.h` and `.cpp`
- `Source/PluginStateModel.h` and `.cpp` only for a non-serialized XY snapshot change callback
- `CMakeLists.txt`
- `tests/XYPadEngineTests.cpp` (new)
- `tests/RuntimeSmokeTests.cpp`
- `tests/StateModelTests.cpp`
- `.github/workflows/toyotomi-hideyoshi-windows.yml` only to run/upload XY test diagnostics

Forbidden:

- Any edits to `PluginEditorV2.*`, `UIComponents.*`, artwork, UI manifests, APVTS IDs,
  serialized model schema, `TimelineScratchEngine.*` behavior, or any existing preset DSP.
- Reinterpretation/change of the fixed plan, memory budget, capture rules, pitch range,
  X direction, mix, latency, transport, or fallback behavior.
- Candidate/final/Program Files changes, commit, push, merge, or promotion.
- Audio-thread allocation, locks, ValueTree, I/O, logging, blocking waits, or FFT work.

Implementation contract:

1. Add fixed one-bank stereo per-BAR capture capacity and an atomic BAR state/generation
   machine exactly as documented. Allocate only before processing. Handle allocation
   failure by disabling XY only.
2. Add the fixed bounded background render worker and SPSC job queue. Capture PCM on
   the audio thread; never perform phase-vocoder work there. Publish only if the slot
   generation still matches. Keep READY captures across loop/seek/stop/restart and
   invalidate according to the plan.
3. Implement sample-exact normalized trajectory interpolation, forward/reverse source
   lookup, 20ms hold micro-loop, 2ms seam, 3ms reversal/jump crossfade, and the
   independent -12..+12 semitone offline phase-vocoder renderer with 2048 Hann/512 hop.
   Output is exactly BAR length and processor latency stays zero.
4. Keep preset-only processing byte-for-byte on the legacy path whenever XY is absent
   or not READY. Mix only when XY is READY using the exact plan equations. Depth zero
   must return original input bits.
5. Add real deterministic execution assertions for AC-01..14, including render-worker
   lifecycle, stale generation, queue saturation, allocation/lock instrumentation,
   frequency/duration/pitch independence and diagnostic WAV fixtures. Do not use
   cumulative-OR success checks or source-text presence checks.
6. Configure/build and run the XY tests plus existing TimelineScratchEngineTests,
   RuntimeSmokeTests and StateModelTests. Report actual commands and exits. If any
   behavior, source boundary, test, or scope is ambiguous, stop without guessing and
   return the exact gap to Codex/Sol.

Do not edit the plan/criteria or expand scope. Do not call other models, issue a
DeepSeek call directly, or perform any candidate/promotion operation. The outer
Harness owns provider routing and may use the single approved fallback only for an
actual GLM provider/transport failure.
