# Bounded paired sine/cosine evaluation for live XY

Codex-approved equivalent arithmetic optimization, 2026-10-24.
The 21 AC, phase-vocoder equations, X/motion curves, coefficients, parameters,
stereo link and 1024-sample latency remain the approved specification.

CI 40077183709 passed FFT references and all DSP/runtime suites. Its exact D
benchmark still failed AC-20: 96 kHz/64 p99 .58035, worst 2.44545. Keep this RED
measurement and do not change thresholds or discard outliers.

Evaluate paired sine/cosine from the same phase in one private, stateless helper.
For finite |phase|<=1e9, reduce using an integer multiple of 2*pi split into exact
201/32 and its residual; reflect to [-pi/2,pi/2]; evaluate degree-17 sine and
degree-16 cosine in double precision. Larger/nonfinite phases use the original
standard-library functions. Phase accumulators and frequency equations are
unchanged. No tables, allocation, locks, I/O or latency are added.

Use the helper only for live Pitch synthesis and live equal-power X/hold
coefficients. Constructor/window preparation and the background offline renderer
remain unchanged. A paired evaluation removes duplicate calls for stereo.

Before acceptance, compare against std::sin/cos on at least 200,000 deterministic
phases across the fast domain, exact quadrant boundaries, large cycle counts,
and the fallback domain. Maximum absolute coefficient error must be <=2e-9.
All existing independent X/transition references must retain <=2e-6 error, and
all Pitch/stereo/latency/allocation tests must pass. Re-measure the exact new
Release executable over all 18 original conditions; remaining failure blocks
promotion. This is not authority to approximate beyond those bounds.
