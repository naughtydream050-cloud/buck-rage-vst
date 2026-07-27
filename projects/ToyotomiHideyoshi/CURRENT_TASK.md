# CURRENT TASK

## Phase

Phase 1 UI — Stitch.Capture followed by Stitch.Draft.

## Goal

Create the image-first Windows VST3 UI skeleton for the 64-bar, 16-count-per-bar Buck/Krump scratch sequencer “Toyotomi Hideyoshi”.

## Current checkpoint

- Project initialization: complete
- Specification files: complete
- Stitch.Capture: complete
- UI skeleton: implemented; build verification pending
- Build environment investigation: complete
- Existing-project build trace: complete
- CMake/JUCE static review: complete with project-local fixes
- Persistent state model: intentionally not started until the requested checkpoint

## Allowed scope

Only files under this project. Phase 1 permits UI, state management, host display, output metering, and pass-through audio.

## Validation truth

- Source implementation: UI skeleton and pass-through/meter transport implemented
- CMake configuration: NOT RUN — CMake is not installed
- VST3 compile/link/bundle: NOT RUN
- Local build: NOT RUN — no existing local toolchain trace was found; established project builds are CI artifacts
- CI build: NOT TESTED
- FL Studio host validation: NOT TESTED

## Next action

Use an already-existing local CMake/MSVC shell when its path is supplied, or obtain approval for the smallest shared CI workflow addition. Do not begin state-model work before configure/compile/link pass.
