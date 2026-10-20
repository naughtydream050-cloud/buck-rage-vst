# XY Pad Phase 2 — approved implementation contract

Status: Codex/Sol implementation plan for the user-authorized Phase 2 work on 2026-10-20.
Baseline: `62c61490607197d13ad2c8028da443bc18df87dc` (verified against the successful Windows artifact for run `39215052454`).

## Goal and protected behavior

Connect the existing saved per-BAR XY trajectory to a stereo scratch layer. X controls
the source position (left=reverse, right=forward); Y controls independent pitch
(top=+12 semitones, middle=0, bottom=-12). Reuse the existing 64-bar map and saved
trajectory format. Do not change UI artwork/layout, APVTS IDs, serialized state,
existing preset algorithms, preset-only output, host BAR mapping, or reported host
latency. Reverb is out of scope.

## PCM capture and pitch architecture

Each BAR with a non-empty saved XY trajectory captures that BAR's incoming stereo PCM
on its first complete pass. XY is dry while a capture is incomplete or being rendered.
The completed BAR buffer is immutable while published and is reused on subsequent
visits in the same transport epoch. A fixed-capacity background renderer consumes a
POD job containing the BAR capture generation and the bounded trajectory snapshot.
It copies the input BAR into one preallocated work buffer, renders X scrubbing followed
by independent Y pitch shifting into the BAR buffer, then atomically publishes READY.
If the renderer is late or any generation/state/queue check fails, only XY is dry;
the existing preset continues normally.

Allocate one fixed PCM bank in `prepareToPlay`, not in `processBlock`. Cap the BAR
duration at 4/4, 90 BPM: `ceil(sampleRate * 4 * 60 / 90)` frames per BAR. At 96 kHz,
64 stereo float BARs require 131,072,000 bytes (125 MiB); the renderer's one stereo
BAR scratch adds at most 2,048,000 bytes (1.96 MiB), plus bounded FFT/trajectory data.
If allocation fails, sample rate exceeds 96 kHz, or a BAR exceeds the configured
duration, disable XY capture safely and preserve all existing presets. No runtime
allocation, lock, ValueTree, file I/O, or logging is allowed on the audio thread.

Use fixed states EMPTY -> CAPTURING -> QUEUED -> RENDERING -> READY, guarded by
atomics and generation IDs. The audio thread is the sole PCM capture writer and
playback reader; the render worker owns a slot only while it is RENDERING. Stop,
restart, and seek abort only an incomplete capture; completed READY BARs survive so a
one-pass capture can be auditioned after stop/restart or direct seek. Confirmed loop
wraps and normal BAR transitions also preserve READY BARs. Sample-rate, tempo,
time-signature, or that BAR's trajectory revision changes invalidate the affected
rendered audio (and any incomplete job); XY falls back dry until recaptured/rendered.
Stale render jobs may finish only into their owned slot and must fail the generation
check before publication. No slot may be overwritten while a worker or reader owns it.
Preset DSP is untouched. Source-arrangement edits are not exposed by JUCE's transport
position; captures therefore persist until that BAR's trajectory changes, the sample
rate/tempo/time signature changes, or plugin state is reloaded.

The renderer operates on a complete, already-captured BAR, so its FFT window is not
streaming latency. Use a 2048-sample Hann phase-vocoder window with 512-sample hop,
centered frames and explicit edge padding; output length remains exactly the captured
BAR length. Host-reported processor latency remains 0. Validate impulse alignment
within one sample and frequency shift with deterministic sine fixtures. L/R use the
same time/pitch controls and frame timing; preserve their inter-channel phase
relationship within the deterministic tolerance.

## XY mapping and click control

Normalize saved point times to BAR phase `u = clamp(t / max(duration, epsilon), 0, 1)`.
At sample `n`, query `u = n / max(barFrames - 1, 1)` and linearly interpolate X and Y
between the surrounding points; hold the first/last point outside their time span.
Map X to source position `p = clamp(x, 0, 1) * (barFrames - 1)`. Use linear
interpolation between adjacent source samples; never wrap and never read outside the
captured range. Positive X delta is forward, negative is reverse.

Do not clamp X velocity and let it miss the requested position. Position fidelity is
primary: the target is reached at each trajectory point. At direction reversal or a
read jump whose implied rate exceeds 4 samples/output-sample, crossfade old/new reads
for 3 ms with equal-power weights; outside that bounded transition use the exact
trajectory target. When X is stationary, use a 20 ms micro-loop centered on the
selected position with a 2 ms equal-power seam crossfade, clamped to available source
frames. These are fixed constants and are tested at all supported sample rates.

Y semitones are `24 * (0.5 - y)`; pitch ratio is `2^(semitones / 12)`. Interpolate
the ratio per phase-vocoder frame from the same normalized BAR trajectory. The
phase-vocoder renderer preserves BAR duration and does not change X's source-position
path. No host PDC is added because rendering uses the complete BAR before playback.

## Layer independence and mix

The existing preset-only path remains byte-for-byte selected whenever no READY XY BAR
is available. Never gate or dry the preset because XY capture/render is unavailable.
When only XY is ready, output `dry * (1-depth) + xy * depth`. When both layers are
ready, run the preset wet at full internal depth and output
`dry * (1-depth) + depth * (0.70710678 * presetWet + 0.70710678 * xyWet)`.
This is the approved equal-power parallel blend; do not add a limiter. Measure
correlated-signal peak growth and document it. At depth 0, bypass all arithmetic and
leave input samples bit-exact. Preset-only without XY keeps its existing output path
and depth law. Bypass and stopped transport do not play stale XY PCM.

## State/UI/transport bridge

Keep `PluginStateModel` serialization and existing `MotionPoint` schema unchanged.
Add only a non-audio-thread change callback in `PluginStateModel` so the processor can
convert the changed BAR's vector into a preallocated atomic POD snapshot. Each point
field is stored atomically; a sequence counter is odd during publication and even when
complete. The audio thread reads once and adopts only a stable even-generation copy,
otherwise retaining its prior copy. Require lock-free 32-bit atomics at compile time.
`processBlock` may not query UI or `ValueTree`. The audio thread adopts a trajectory at
BAR entry and holds that immutable copy until the next entry; a recording/edit made
mid-BAR applies at the next entry. A BAR without a trajectory at entry remains
XY-dry for that visit. XY uses the
same host absolute BAR and phase already supplied to the DSP engine when Host Sync is
on; Host Sync off uses the existing internal timeline. Tempo changes invalidate the
current XY render and recapture under the new BAR duration; no phase reset is allowed
at ordinary BAR boundaries. Host-position loss means XY dry until a valid position
resumes. Existing preset transport behavior must not be changed.

## Scope

Allowed: new `XYPadEngine` implementation/header, narrow Processor snapshot/mix
connection, the bounded change-callback in `PluginStateModel.*` (no serialized schema
change), project source list/build registration if required, deterministic XY DSP
tests, Runtime Smoke/StateModel tests for existing trajectory round-trip, CI/Gate
evidence. Do not edit `PluginEditorV2.*`, `UIComponents.*`, visual assets, APVTS IDs,
parameter ranges, serialized schema, existing preset algorithms, `TimelineScratchEngine`
behavior, final output, or Program Files. A required scope expansion returns to
Codex/Sol before editing.

## Acceptance criteria (fixed)

AC-01. BARs 1..64 own independent captures/render states; deterministic distinct
  fixtures prove no cross-BAR leakage.
AC-02. Existing 64-BAR trajectory save/restore remains point/time/coordinate exact;
  state schema and UI selection are unchanged.
AC-03. Host Sync ON/OFF maps phase sample-accurately; block sizes 64..1024, variable
  blocks and tempo updates create no BAR-boundary phase reset.
AC-04. X rising reads forward and X falling reads reverse; sample interpolation is
  within `2e-6` of the scalar reference and never reads out of range.
AC-05. Y produces -12/0/+12 st within 1% frequency error, duration unchanged, and
  holds X fixed; pitch state is independent of X direction/slope.
AC-06. X hold produces the specified 20 ms loop, not a constant held sample; the
  2 ms seam is continuous within the click threshold.
AC-07. Reversal and >4x implied jumps use a 3 ms bounded crossfade, reach the target,
  remain finite and have no out-of-range read.
AC-08. Incomplete first-pass capture and render-pending are XY-only dry; complete
  capture is tied to the current BAR/epoch and only replays after READY.
AC-09. DEPTH 0 is bit-exact dry. XY-only depth mix, preset-only legacy path, LENGTH,
  SPEED and PITCH application scope match this plan.
AC-10. XY-only, preset-only, both, preset OFF+XY, no trajectory, missing capture,
  bypass and STOP are separately asserted; XY failure never changes preset output.
AC-11. Stereo uses linked trajectory/read positions; L/R identity and phase tests meet
  documented deterministic tolerances.
AC-12. Loop/seek/STOP/restart/time-signature/sample-rate changes never read stale or
  partially rendered PCM; confirmed loop preserves valid captures.
AC-13. No NaN/Inf, audio-thread allocation, lock, ValueTree, file I/O or logging; bank
  allocation is bounded and fails safely; worker queue overflow is XY-only dry.
AC-14. Deterministic XY tests, TimelineScratchEngineTests, RuntimeSmokeTests,
  StateModelTests and Windows VST3 CI pass on the same source HEAD; diagnostic WAVs
  cover forward, reverse, hold, fold/jump, and pitch extremes.
AC-15. Jev 1.13 reviews all AC individually against this exact plan/hash and evidence;
  any FAIL/UNKNOWN/unavailable blocks candidate promotion.
AC-16. Before promotion, verify that the successful CI artifact is bound to the
  exact source HEAD/run and record ZIP, bundle-tree, AMD64 PE, and internal binary
  SHA-256 values. The current candidate remains untouched during this review;
  FL Studio listening remains NOT TESTED until the user tests.

### Post-promotion verification (required, after AC-01..AC-16 PASS and Sol approval)

Sol/Codex approval is a separate release decision after the Jev gate; it is not
evidence that Jev can judge before that decision exists. Only after the approved
receipt is issued may the official promotion command update candidate. Then verify
that the promoted candidate's complete bundle tree and internal binary SHA-256
match the same CI artifact, and preserve the prior candidate backup. A failed
post-promotion hash check blocks D testing and requires a controlled rollback/review.
