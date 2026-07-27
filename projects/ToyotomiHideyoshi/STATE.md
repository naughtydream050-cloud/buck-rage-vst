# Toyotomi Hideyoshi State

## Current phase

Phase 1 UI / Stitch.Capture → Stitch.Draft.

## Product identity

- Display name: Toyotomi Hideyoshi
- Internal target: ToyotomiHideyoshi
- Format: Windows VST3
- Version: 0.1.0
- Manufacturer code: `Rzfc`
- Plugin code: `TyHd`
- Bundle identifier: `com.razorfacecompany.toyotomihideyoshi`

## Current status

Project initialization, Phase 1 specifications, Stitch.Capture, UI skeleton, build-environment investigation, and CMake/JUCE static review are complete. No product DSP has been imported or started. The fixed pattern state model has not started.

## Validation

- CMake configure: NOT RUN — CMake unavailable
- VST3 compile: NOT RUN
- VST3 link: NOT RUN
- VST3 bundle: NOT GENERATED
- State restore: NOT TESTED
- Audio pass-through: NOT TESTED
- FL Studio host validation: NOT TESTED

## Blockers

Visual Studio Installer, Visual Studio 2022 Build Tools/Community, Developer Command Prompt, CMake, MSBuild, Ninja, and a compiler were not found. No installation was performed. Existing VINTAGE_RAWNESS, BUCK_RAGE, and LAOZI Windows VST3 evidence identifies GitHub Actions (`windows-latest`, CMake configure/build) as the established build route; local folders retain downloaded artifacts rather than CMake build trees. The shared root JUCE checkout is structurally incomplete for CMake; a complete JUCE 7.0.12 tree exists only in `.codex_tmp` and is a local fallback. Toyotomi Hideyoshi has no matching workflow, and project policy forbids modifying shared workflows without approval.
