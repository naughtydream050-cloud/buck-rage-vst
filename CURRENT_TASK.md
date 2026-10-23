# CURRENT TASK

## Phase

Approved XY Pad Phase 2 live processing — re-entrant transition repair and AC-20 performance qualification.

## Canonical working location

`D:\Development\_CODEX_TOOLS\worktrees\toyotomi-xy-ac10-verification`

Branch: `codex/xy-live-ac10-verification`
Base source: `2322502e7946e46b85c74d061452861fd76d4efc`
Current committed source: `baad7660e5679d68338912edd2e5066b95077d1d`

The original 16 AC and approved R2-AC-17..21 remain required. The current D candidate is preserved as rollback; no new candidate has been promoted.

## Validation truth

- Latest Windows CI Run `39982014270`: successful build and deterministic checks for the current committed source.
- AC-20 on Ryzen 3 2200U: **FAIL**. The 18-condition Release benchmark still exceeds the specified p99 or maximum audio-deadline ratios in multiple sample-rate/block-size conditions. Thresholds have not been relaxed.
- Experimental AVX2 tail-rendering optimization is in progress and is not yet built or validated.
- Jev 1.13: not run for the current uncommitted optimization.
- FL Studio host validation: `NOT TESTED`.

## Frozen scope

Do not change UI, saved State, parameter IDs, existing Preset algorithms, absolute BAR synchronization, or the 1024-sample latency. Do not update `output\\final` or Program Files. Candidate promotion requires all 21 AC PASS, Jev PASS, and Codex/Sol approval bound to the exact artifact.

## Next action

Finish and correctness-test the bounded tail-rendering optimization, then run the full Windows CI and the exact 18-condition AC-20 benchmark. If any required performance case remains over threshold, do not promote the artifact.
