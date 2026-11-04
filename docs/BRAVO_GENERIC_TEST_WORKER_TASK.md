# Bounded task: generic BRAVO profile test review

Review only `Source/BravoProjectPresets.h`, the BRAVO selection lines in
`Source/PluginProcessor.{h,cpp}`, and `tests/TimelineScratchEngineTests.cpp`.
The approved profile is:

- ID 1: dry, dry, transform 1 BAR / speed 1.0 / depth .90, forwardCut 1 BAR /
  speed 1.0 / depth .90.
- ID 2: dry, dry, zigzag 1 BAR / speed 1.0 / depth .85, tapeBrake 1 BAR /
  speed 1.0 / depth .85.

Confirm that the profile does not write manual state and that the selected
array is used consistently for scratch rendering and alignment metadata.
Make only a minimal test correction if a stated shape assertion is missing.
Do not alter the profile, UI, state schema, DSP algorithms, artifacts,
credentials, CI, or any unrelated file. Run `git diff --check` and report the
result compactly.
