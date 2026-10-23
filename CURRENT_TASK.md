# CURRENT TASK

## Phase

Approved XY Pad Phase 2 live processing — re-entrant transition repair and AC-20 performance qualification.

## Canonical working location

`D:\Development\_CODEX_TOOLS\worktrees\toyotomi-xy-ac10-verification`

Branch: `codex/xy-live-ac10-verification`
Base source: `2322502e7946e46b85c74d061452861fd76d4efc`
Current committed source: `7df10e51b6a7c0944ca3272cb439f686ab709b56`

The original 16 AC and approved R2-AC-17..21 remain required. The current D candidate is preserved as rollback; no new candidate has been promoted.

## Validation truth

- Latest Windows CI Run `39984485818`: successful Release build and deterministic checks for the current committed source.
- AVX2 tail renderer: compiled and validated by CI; independent reference max error `1.19209e-7` (limit `2e-6`), AVX2 available on D device.
- AC-20 prior measurement: **RETEST REQUIRED**. The prior harness repeatedly called one-sample processing rather than one host-sized callback, so those ratios are diagnostic only and do not qualify AC-20. No threshold changed.
- Corrected 18-condition benchmark now processes one host-sized callback with a 256-point alternating trajectory to retain near-every-sample re-entry; not yet measured.
- Jev 1.13: not run for the current source.
- FL Studio host validation: `NOT TESTED`.

## Frozen scope

Do not change UI, saved State, parameter IDs, existing Preset algorithms, absolute BAR synchronization, or the 1024-sample latency. Do not update `output\\final` or Program Files. Candidate promotion requires all 21 AC PASS, Jev PASS, and Codex/Sol approval bound to the exact artifact.

## Next action

Run Windows CI for the corrected benchmark harness, then measure its exact 18 conditions on the Ryzen 3 2200U. If any required performance case remains over threshold, do not promote the artifact.
