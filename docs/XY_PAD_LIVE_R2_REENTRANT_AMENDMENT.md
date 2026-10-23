# XY Pad Live R2 — Re-entrant Transition Amendment

Status: User-selected design option 1 on 2026-10-23; implementation has not started.
Base source: `2322502e7946e46b85c74d061452861fd76d4efc`.
Base Plan SHA-256: `C873F27A6AA107AE87C5A4686820B1CB3D9A9D02F2C2C0E5FEEB9396895BB5F0`.
Base AC SHA-256: `42F874D962B75400BC8AE8F0CBE737E13AD05753D60E51ED7999AAE50C8C9195`.

This amendment narrows the approved R2 transition rule for an event that arrives
while another transition is in progress. It does not replace any original
AC-01..AC-16. The existing rule for an uninterrupted transition remains the
equal-power law in `XY_PAD_LIVE_R2_PLAN.md`.

## Required behavior for an interrupted transition

At the sample where a new jump, fold, hold entry/exit, or live reacquisition event
arrives during a transition:

1. Before reusing any source grain slot, copy the current X/hold renderer's POD
   state and render its next T pre-pitch stereo samples into the inactive one of
   two preallocated 3 ms tail buffers. The copy includes active grain ages,
   transition phase, hold phase/mix, and the source snapshots or fixed references
   needed to render them. It must not mutate live renderer state or read future
   input. The tail's sample zero is the continuation that would have been
   rendered at the event sample.
2. Start the latest bounded target on that same sample. Do not queue or coalesce
   it until an earlier fade finishes. Restart the transition phase at zero.
   Retire superseded grains only after their continuation has been rendered into
   the tail.
3. For this re-entry only, use a linear convex blend:
   `y[k] = (1-q)*tail[k] + q*target[k]`, `q=k/(T-1)`,
   `T=max(2, round(0.003*sampleRate))`.
4. The first transition sample is the exact captured continuation; the final
   sample is the target. Coefficients are nonnegative and sum to one, so repeated
   retargets cannot increase a bounded input's peak through recursive equal-power
   gain alone. Do not add limiter, gain compensation, extra voices, or latency.
5. The uninterrupted equal-power law, 2 ms hold seam, target mapping, source
   bounds, stereo link, pitch stage, and existing 1024-sample pipeline stay as
   already approved. Retarget tails are rendered before the Y pitch stage so the
   existing pitch pipeline remains single and unchanged.

If a sample arrives with no active transition, use the approved equal-power
transition. If a sample arrives during the transition, use the linear rule above
for that re-entry. A subsequent event during that re-entry is also an interrupted
transition and uses the same bounded linear rule.

## Added testable acceptance criteria

The original sixteen AC remain unchanged. These additions are mandatory before
the existing AC-15/16 release checks can pass:

- **R2-AC-17 — Immediate re-entry:** Inject a target event at every offset
  1..T-1 of an active transition. The transition state restarts on the event
  sample, the old branch is the captured audible continuation, and no target is
  delayed until the previous fade ends. Compare every output sample to an
  independently implemented scalar reference; maximum error <= 2e-6.
- **R2-AC-18 — Bounded peak under repeated retargets:** Exercise a new event on
  every sample for at least 10*T samples using DC, correlated, anti-correlated,
  impulse, and phase-offset stereo inputs. No NaN/Inf; each output sample's
  absolute value must not exceed the larger absolute value of that sample's
  captured-tail and target branch values by more than 2e-6, per channel. Record
  peak and RMS per channel. Verify the interrupted rule has no recursive
  equal-power coefficient accumulation.
- **R2-AC-19 — Hold and source-slot safety:** Interrupt transitions during hold
  entry, hold exit, seam crossfade, fold, and source-ended reacquisition. Assert
  the tail equals the audible pre-pitch continuation at the boundary, old grains
  never revive, new target reads remain within valid serials, and the 2 ms seam
  is not restarted or doubled accidentally.
- **R2-AC-20 — Realtime bounded work:** Under the R2 sample-rate, block-size,
  and variable-block matrix, inject the maximum supported event rate. Record
  operations per sample, maximum tail render length, and block processing time.
  No allocation, lock, ValueTree access, I/O, logging, or blocking. The
  controlled Windows reference host is the current D-development machine:
  AMD Ryzen 3 2200U, 4 logical processors, Windows 11 Home. Record CPU power
  mode, sample rate, block size, event count, compiler/build mode, and test
  executable hash. The p99 processing time must be <= 50% and the worst observed
  block <= 80% of that block's audio deadline. CI records timing but does not
  replace this controlled-host deadline check. If the deadline is missed, return
  to Sol; do not coalesce, queue, drop, or lengthen transitions.
- **R2-AC-21 — Mutation sensitivity:** Deliberately delay event handling,
  overwrite the tail before capturing it, use the wrong blend law, allow a stale
  grain to revive, and inject an over-bound peak. Every mutation must fail the
  corresponding deterministic assertion.

## Execution and gate

1. Hash this amendment and a supplemental AC manifest alongside the unchanged
   base Plan and AC hashes.
2. Add only the interrupted-transition deterministic tests. Run them against
   base source and preserve their RED evidence. No DSP source edits are allowed
   in this phase.
3. After RED is established, implement only the behavior above in the existing
   XY engine and its tests. UI, state, parameter IDs, transport, other presets,
   and candidate are outside scope.
4. Run XY DSP, TimelineScratchEngine, Runtime Smoke, StateModel, Aggregate,
   Windows VST3 CI, all original sixteen AC, and R2-AC-17..21 against the same
   committed source and artifact.
5. Jev judges every original and added criterion from hash-bound execution
   evidence. Sol reviews the complete diff and worst-case measurements. Candidate
   promotion requires all deterministic checks, CI, Jev PASS, and Sol approval.

The amendment fixes the design choice for interrupted transitions but does not
claim that the implementation or any gate has passed.
