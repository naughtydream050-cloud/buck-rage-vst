# Toyotomi Hideyoshi State

## Current phase

Phase 1 — Windows VST3 UI / state / host-display validation. UI assets are frozen pending the user's FL Studio review; no Scratch DSP or Phase 2 work is authorized.

## Canonical paths

- SSOT: `D:\Development\RAZOR_FACE_COMPANY\01_PLUGINS\projects\ToyotomiHideyoshi`
- Candidate: `output\fl-candidate\Toyotomi Hideyoshi.vst3`
- Final (user approval required): `output\final\Toyotomi Hideyoshi.vst3`

## Latest validated candidate

- GitHub Actions run: `31250870603`
- Source HEAD: `a49df568f72da9f63af9f08df666f0b24690703c`
- Internal VST3 SHA-256: `F591F0207B58D2B0080CDC9421F530225C41AD950FBEDF11350952C9683F2B2F`
- CI configure / compile / link / StateModelTests / Runtime Smoke Test: passed
- FL Studio host validation: pending user test

## Next action

The user copies the candidate bundle to an FL Studio scan location, rescans it, and reports the actual host result. Do not update `output\final` until that approval.
