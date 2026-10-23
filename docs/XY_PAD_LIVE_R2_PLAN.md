# XY live R2: rate-independent hold and equal-power transitions

Status: APPROVED BY SOL/CODEX for bounded implementation after user approval on 2026-10-23.
Baseline: `2a3408f253faf1655a8c7eb94e7e0a77e2d562e3`.
Supersedes only the conflicting X stationary threshold and transition-law details
in `XY_PAD_LIVE_LOW_LATENCY_PLAN.md`. Its other behavior and all sixteen original
AC requirements remain in force. Historical verdicts stay bound to their old plan.

## Findings that require this amendment

The old Plan uses `abs(deltaX)<=1e-4` on each sample, while current source uses
`1e-6`. A 0-to-1 X ramp over two seconds at 48 kHz changes approximately
`1.04e-5` per sample, so the literal old threshold would turn ordinary slow
movement into hold. Direction detection using per-sample deltas is also rate
dependent. The current grain implementation does not establish the Plan's
equal-power transition: linear gain factors are included in overlap-add and
then normalized by their sum. These discrepancies are the bounded R2 scope.

## Fixed live behavior

Keep the first-visit live XY path; 125 ms maximum recent stereo history; 20 ms
frozen grains at the existing 10 ms hop; exact X target mapping; Y pitch from
-12 to +12 semitones; 20 ms cold-history readiness; 8 ms XY wet ramps; 30 ms
current-input silence detection and same-BAR reacquisition; fixed 1024-sample
pipeline latency; 64 independent trajectories; existing transport, preset,
mix, depth, UI, state, and parameter contracts. R2 adds no output FIFO or
latency-report change. Preset output remains independent of XY availability.

## X movement and hold contract

For the active linear trajectory segment `(x0,p0)..(x1,p1)`, calculate in
double precision normalized X velocity:

`vX = ((x1-x0)/(p1-p0)) * d(barPhase)/dt` (normalized X units per second).

Use the existing sample-accurate BAR clock to obtain `d(barPhase)/dt`; do not
infer velocity from block size or float-rounded consecutive sample differences.
The interpolated X target remains unchanged. Constant and one-point segments
have zero velocity. Duplicate-time endpoints follow the existing deterministic
ordering; never divide by zero or pass a non-finite velocity to the renderer.
A discontinuous target jump is handled as a jump event.

Approved thresholds:

- Enter hold when `abs(vX)<=0.001` continuously for `ceil(0.002*sampleRate)` samples.
- Resume movement on the first sample with `abs(vX)>=0.002`, or on a discrete
  target jump. Start its 3 ms transition immediately, without waiting for a hop.
- In the interval `(0.001,0.002)`, retain the prior hold/moving mode. Any sample
  above the hold-entry threshold cancels an incomplete hold counter.
- Outside hold, positive velocity reads forward and negative velocity reads
  reverse. Pitch Y never changes X direction, position, or timing.
- Constant X holds. Continuous ramps at +/-0.01, +/-0.1, and +/-0.5 normalized
  units per second remain moving at 44.1/48/96 kHz. Velocities inside the explicit
  deadband are defined as stationary; arbitrary sub-threshold motion is not promised.
- Tempo updates change velocity from the current BAR clock without resetting
  BAR phase, live history, or the X target.

## Equal-power transition contract

Normalize grain overlap-add separately within each old/new stereo branch. Blend
the two normalized branches before the existing Y pitch stage, so no later
overlap normalization can cancel the transition law:

`T=max(2,round(0.003*sampleRate)); q=k/(T-1); a=cos(pi*q/2); b=sin(pi*q/2)`

`xyX = a*oldBranch + b*newBranch`, for each linked stereo channel, `k=0..T-1`.
The first sample uses `(a,b)=(1,0)`; the last `(0,1)`. Check `a*a+b*b=1` and
scalar/output reference error <=2e-6 at quarter points. Do not divide by `a+b`
or by a normalization term containing the transition weights. Retire the old
branch at completion so it cannot return to full gain. Apply this law to folds,
jumps, hold entry/exit, and live reacquisition; keep exact target mapping and the
existing 2 ms hold-seam law.

If a new event arrives during a transition, render the audible frozen-grain/hold
continuation into one of two preallocated 3 ms stereo tail buffers from a POD
state copy before reusing its source slots. Use that captured tail as the old
branch and the latest exact bounded target as the new branch; restart `q=0`.
No unbounded voices, delayed event queue, future input reads, or extra output
latency. At 96 kHz the two tail buffers together require 4,608 bytes. Retarget
stress must measure worst-case work; fixed memory alone is not a realtime proof.
If the bounded path misses its controlled realtime budget, return the design to
Sol rather than queueing, dropping targets, or lengthening the fade.

Equal-power is not peak normalization: identical branches reach sqrt(2), or
+3.0103 dB, at the midpoint. Measure transition, hold seam, linked stereo, and
final existing Preset blend peaks independently. No limiter, Preset gain change,
or unapproved compensation.

## RED/GREEN and full verification

First add tests only and show the R2 movement and coefficient assertions fail
against this baseline. Do not call a passing baseline test RED. Preserve prior
boundary RED evidence as a distinct historical result. The scalar reference
must implement the equation independently of production helpers.

Test signed ramps +/-0.01/0.1/0.5; constant hold; entry/exit boundaries;
hysteresis; interrupted hold; jump during transition; 44.1/48/96 kHz;
64/128/256/512/1024 and variable blocks; tempo changes; block offsets 1/N/2/N-1;
position-coded grains; DC, correlated/anti-correlated stereo and phase offsets;
finite/bounded reads; and injected wrong threshold, linear law, extra
normalization, hop-delay, seam rewind, and grain revival. Check every sample in
the transition interval. Save diagnostic WAVs and worst-case processing metrics.

Run XY DSP, TimelineScratchEngine, Runtime Smoke, StateModel, Aggregate, and
Windows VST3 validation on the exact committed source. Keep all original
AC-01..AC-16 and classify each with Jev 1.13 using hash-bound evidence.
Jev additionally checks implementation/test relevance, non-vacuity, evidence
sufficiency, Plan conformance, regression risks, and promotion prerequisites.
Supplementary judgments never replace the original sixteen. UNKNOWN means inspect
and add bounded evidence for the same source; an unresolved required FAIL or
UNKNOWN blocks promotion. AC-16 judges the gate's pre-review behavior; an
independent post-review audit then requires AC-01..16 exactly once and all PASS.
Sol/Codex performs the final complete diff and evidence review.

After all gates, verify SOURCE_HEAD, downloaded ZIP, AMD64 PE, complete bundle
tree, and internal VST3 SHA. Use the official promotion command to preserve the
old candidate and install the verified bundle at D's candidate path. Never
update Program Files or `final`. FL Studio audio/PDC validation remains
`NOT TESTED` until the user tests the deployed candidate.

## Worker boundary

GLM implements the explicitly approved tests and then only the code required by
their RED result. DeepSeek may run once for the identical scope only after an
actual allowed GLM provider/transport failure. Neither worker may revise this
Plan, change unrelated algorithms, approve, or promote. Record actual provider,
fallback and Jev costs; token savings stays UNKNOWN without a measured baseline.
