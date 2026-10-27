# XY no-pitch physical source cleanup

Base: `de111a418ad3da2a4d0df764a31c08a64420f32f` plus the currently unstaged no-pitch diff.

Only edit `Source/XYPadEngine.cpp` and `tests/XYPadEngineTests.cpp`.

1. Physically delete the two `#if 0` blocks containing the removed live and offline XY phase-vocoder implementations. Do not leave disabled FFT/pitch symbols in either file.
2. Replace obsolete Y-pitch tests with deterministic tests that prove Y=0, .5, and 1 yield identical XY audio for the same X trajectory, and that the fixed output delay is exactly 1024 samples.
3. Preserve X Forward/Reverse, fixed output delay, all public APIs, UI/state/parameter IDs, and every non-XY preset. No changes outside the two allowed files.
4. Do not use an external API, do not commit, push, reset, or clean. Finish with `git diff --check`.

Success is source with no `livePitch`, `processLivePitch`, `kLivePitch`, `XYPadLiveFFT`, `fftWork`, or phase-vocoder source in either allowed file, plus updated deterministic tests. Do not change CI/candidate artifacts.
