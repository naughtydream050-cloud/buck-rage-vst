# Toyotomi Hideyoshi State

## Current phase

Phase 1 UI / Stitch.Capture and Stitch.Draft. The minimum Windows VST3 CI build gate is complete; fixed 64 x 16 state work has not started.

## Product identity

- Display name: Toyotomi Hideyoshi
- Internal target: ToyotomiHideyoshi
- Format: Windows VST3
- Version: 0.1.0
- Manufacturer code: `Rzfc`
- Plugin code: `TyHd`
- Bundle identifier: `com.razorfacecompany.toyotomihideyoshi`

## Validation

- CMake configure: PASS (GitHub Actions run 30315369457)
- VST3 compile: PASS
- VST3 link: PASS
- VST3 bundle: GENERATED
- Artifact upload: PASS (`ToyotomiHideyoshi-Windows-VST3`)
- State restore: NOT TESTED
- Audio pass-through runtime: NOT TESTED
- FL Studio host validation: NOT TESTED

## Blockers

No build blocker remains for the Phase 1 state-model gate. Visual UI approval, runtime pass-through verification, state restore verification, and FL Studio validation remain intentionally untested.
