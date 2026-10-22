# XY Pad Live Low-Latency Processing Plan

Status: APPROVED by Sol for bounded implementation on 2026-10-22.
Baseline source: `0e5852058cc9e8b3717ecddcc141bb9600b10b38`.

## Goal and fixed behavior

Replace first-pass full-BAR capture/render with low-latency live XY processing. A
trajectory assigned to BAR 4 must affect the first BAR 4 visit after a bounded
cold-start warmup; no next-loop pass is required. Preserve the 64-BAR trajectory
data, serialization, UI artwork/layout, and parameter IDs. Keep preset algorithms
and absolute-BAR mapping unchanged.

## Live XY source and X motion

Allocate fixed-capacity stereo history and processing storage in `prepareToPlay`.
Keep at most the latest 125 ms of source samples. Generate 20 ms grains every
10 ms. At each grain launch, freeze its source-range serial bounds; never follow
the moving writer while rendering that grain. Map normalized X to the valid recent
source window; positive X movement reads forward and negative movement reads in
reverse. Use bounded linear interpolation and never read unwritten or expired
serials. Use the existing 3 ms equal-power transition for direction folds and
abrupt target jumps. Preserve exact trajectory position as the target; smooth only
the transition, not the target path.

When `abs(deltaX) <= 1e-4` continuously for 2 ms, hold a frozen 20 ms grain with a
2 ms equal-power seam crossfade. On resumed movement, crossfade back to live grains
over 3 ms. Before 20 ms of valid source history exists, XY alone remains dry; then
wet rises over 8 ms. Expand the valid source window gradually up to 125 ms. Normal
BAR and loop boundaries retain history. When both current dry input channels stay
below `1e-4` for 30 ms, ramp only XY to dry over 8 ms and stop any held-grain output.
When input resumes, live XY may resume in the same BAR; do not replay stale hold
audio.

## Independent Y pitch

Convert stored UI Y to semitones as `24 * (y - 0.5)`: top +12 st, center 0 st,
bottom -12 st. Put a streaming 1024-point FFT / 256-sample-hop phase vocoder after
the X grain stage. Smooth its target over 5 ms. Link stereo analysis timing and
phase treatment while preserving L/R relationship. Pitch must not affect X read
position, direction, or BAR phase. At Y center, use the same-latency identity
path and crossfade safely into/out of pitch processing.

Implement and measure a fixed 1024-sample pipeline latency, including all FIFOs and
the phase-vocoder schedule; FFT size alone is not evidence of latency. Declare
`setLatencySamples(1024)`. Validate with impulse/block-size tests. This explicitly
changes the previous no-latency contract: Dry, preset, XY, bypass and mix-control
timing all receive the same 1024-sample delay. DEPTH 0 is bit-exact delayed Dry;
preset-only is its legacy output delayed by exactly 1024 samples. Do not change
preset DSP or make XY unavailability force the preset dry.

## Transport, mix and realtime constraints

Normalize saved trajectory time over each BAR and interpolate coordinates per
sample. Use only the active playback BAR trajectory; keep edit-selected yellow
BAR independent from playback red BAR. Preserve phase across tempo and block-size
changes. Keep history across ordinary BAR and confirmed loop transitions. On seek,
STOP/restart, discard stale XY grain/pitch state and resume only from valid live
history. Host Sync ON uses the existing host timeline; OFF uses the existing
internal timeline. Missing host position makes XY dry, not preset dry.

Preserve current XY/preset mix coefficients and the existing depth semantics except
for the explicitly fixed 1024-sample global latency. XY-only, preset-only, both,
preset OFF plus XY, no trajectory, insufficient history, silence, DEPTH 0, bypass,
and STOP must be separately testable. Do not add a limiter.

No audio-thread allocation, lock, ValueTree access, UI access, file I/O, logging,
or blocking operation. Use preallocated fixed-capacity buffers and lock-free or
single-owner state transfer. No UI, state schema, parameter ID, or preset algorithm
changes. If the implementation requires behavior outside this plan, stop and return
to Sol for a plan amendment.

## Verification and release gates

Run deterministic XY DSP tests, TimelineScratchEngineTests, RuntimeSmokeTests,
StateModelTests, Windows VST3 CI, and diagnostic WAV generation for first-BAR,
forward/reverse, hold, fold/jump, and pitch extremes. Verify 44.1/48/96 kHz and
fixed/variable blocks 64/128/256/512/1024. Test all 64 BARs, Host Sync ON/OFF,
tempo change, loop, seek, STOP/restart, silence, preset independence, peaks, and
finite output. Jev 1.13 classifies each AC against this exact plan and evidence.
Any FAIL/UNKNOWN blocks candidate promotion. Sol reviews the complete diff and
test/CI/Jev evidence before issuing an approval receipt. Only the official gate may
update the D candidate after approval and artifact/source/tree/binary SHA checks.
Program Files is never updated. FL Studio listening/PDC feel stays `NOT TESTED`
until user verification.
