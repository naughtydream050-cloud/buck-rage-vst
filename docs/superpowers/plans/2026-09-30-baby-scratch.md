# BABY Scratch Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make BABY audibly play a short fixed capture forward and backward throughout its LENGTH.

**Architecture:** Extend the one existing `TimelineScratchEngine` with a BABY capture window and continuous read-head motion. Reuse history, wet ramp and BAR timing; expose fixed numeric read state for tests.

**Tech Stack:** JUCE C++17, CMake/CTest, Windows GitHub Actions.

---

### Task 1: Prove the missing behavior

**Files:** `tests/TimelineScratchEngineTests.cpp`

- [ ] Add a test using prefilled stereo history, BABY one-BAR, SPEED=1, DEPTH=1; assert non-Dry sound at early/mid/late phases and a fixed valid capture window.
- [ ] Assert alternating positive and negative read rates over contiguous blocks, bounded finite samples, and no hard jumps at turnarounds.
- [ ] Run `TimelineScratchEngineTests`; confirm the new assertion fails because BABY currently stays Dry.

### Task 2: Add only the BABY runtime path

**Files:** `Source/TimelineScratchEngine.h`, `Source/TimelineScratchEngine.cpp`

- [ ] Add scalar capture/read position, phase and diagnostics fields; reset them on prepare/release/transport reset/BAR entry.
- [ ] At BABY entry, capture a fixed 250 ms window after enough history exists; otherwise remain Dry.
- [ ] During LENGTH, advance at SPEED and map a cosine cycle to forward/backward read position, smoothly reaching zero velocity at both ends. Reuse existing read interpolation and wet ramp.
- [ ] Run `TimelineScratchEngineTests` and verify the new test passes with existing tests unchanged.

### Task 3: Regression and delivery evidence

**Files:** `tests/TimelineScratchEngineTests.cpp` plus the two engine files above.

- [ ] Add LENGTH-end Dry, no recapture, SPEED 0.25/1.0/4.0, history-insufficient and OFF regression assertions.
- [ ] Run RuntimeSmokeTests, StateModelTests, TimelineScratchEngineTests and aggregate validation using the existing build route. If local compiler is unavailable, report that and use task-scoped branch CI.
- [ ] Inspect `git diff --check` and changed paths. Report CI artifact and FL Studio as NOT TESTED until the user listens.
