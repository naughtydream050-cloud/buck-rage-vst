# Toyotomi Hideyoshi State

## Current phase

Approved XY Pad Phase 2 live processing — re-entrant transition repair.

## Canonical paths

- Project SSOT: `D:\Development\RAZOR_FACE_COMPANY\01_PLUGINS\projects\ToyotomiHideyoshi`
- Active worktree: `D:\Development\_CODEX_TOOLS\worktrees\toyotomi-xy-ac10-verification`
- Candidate: `output\windows\candidate\Toyotomi Hideyoshi.vst3`
- Final is outside this task and remains unchanged.

## Latest validated candidate

- Base source: `2322502e7946e46b85c74d061452861fd76d4efc`
- Base Plan SHA-256: `C873F27A6AA107AE87C5A4686820B1CB3D9A9D02F2C2C0E5FEEB9396895BB5F0`
- Base AC SHA-256: `42F874D962B75400BC8AE8F0CBE737E13AD05753D60E51ED7999AAE50C8C9195`
- Re-entrant amendment SHA-256: `CF00BC6FF342D8734E0CCCAB1C82711242AC45E2401093D1EDC0E3A47FDBB640`
- Criteria: original AC-01..16 plus approved R2-AC-17..21.
- D candidate: previous XY build; preserve as rollback until the new candidate passes all gates.
- FL Studio validation: `NOT TESTED`.

## Next action

Establish RED against unchanged DSP, then implement the approved transition amendment, complete deterministic/CI/Jev/Sol gates, and promote only the verified artifact to the D candidate. Do not update Program Files or `output\final` in this task.
