# Bounded test-first task: natural X-speed playback

Baseline de111a418ad3da2a4d0df764a31c08a64420f32f. Read docs/XY_NO_PITCH_PLAN.md and docs/XY_NO_PITCH_CRITERIA.json. Add deterministic tests only to tests/XYPadEngineTests.cpp. Do not modify production code, the build, state, UI, parameters, latency, existing tests or candidate. Preserve existing untracked files.

Create a fixed-PCM reference that independently calculates p[n] = x[n] * (N-1) with a fixed source window and linearly interpolates source samples. Exercise constant X-derived source rates +0.5, +1, +2, -0.5, -1 and -2; check read direction and expected tone frequency, and report numeric results. Include Y-only changes that must be audio-neutral after the intended change. Demonstrate and retain the expected failing results on the current implementation if build tools are available; otherwise clearly say NOT RUN. Do not weaken assertions or claim the current behavior matches the new design. No gain normalization or arbitrary pitch correction. No model calls, commits, pushes or candidate updates. Finish with WORKER COMPACT RESULT and accurately report changed files and tests.

Verification command: git diff --check. This alone is not proof of DSP behavior. CI or an available local toolchain must execute the tests separately.
