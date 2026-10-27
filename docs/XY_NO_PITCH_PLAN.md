# XY live independent pitch removal: approved contract

Source baseline: de111a418ad3da2a4d0df764a31c08a64420f32f. User approved removal of XY FFT/phase-vocoder independent pitch; Y remains recorded, restored and displayed, but has no audio effect. Do not substitute another effect. Preserve X scratch, its rate-dependent pitch, and all existing non-XY preset processing. Preserve UI, state, IDs, transport, audio-thread safety and existing untracked work. Retain 1024-sample alignment unless a separately verified safe change preserves the preset and dry timing contract. No candidate promotion without deterministic checks, CI, Jev and Codex approval.

## Acceptance criteria

1. Remove XY independent pitch from the audio path.
2. Preserve X forward/reverse.
3. Verify X rate-dependent pitch using independent fixed input.
4. Y changes no audio; REC, display and state round trip remain.
5. Preserve 64 BAR isolation and synchronization.
6. Preserve capture, generation, micro-loop and crossfade.
7. Preset-only output is bit-exact against baseline at matching timing.
8. Test combinations, insufficient capture, depth zero and bypass.
9. Verify reported latency and dry/wet alignment.
10. At 96 kHz/64 samples maximum DSP time <= 0.533 ms; retain all measurements.
11. Test 44.1/48/96 kHz and variable blocks.
12. Verify finite output, peak and transition clicks.
13. No audio-thread allocation, lock or I/O.
14. StateModel, TimelineScratchEngine, RuntimeSmoke and Windows CI pass.
15. Produce and evaluate XY, preset, combined and stereo diagnostic WAVs.
16. Jev 1.13 reviews every AC; Codex reviews evidence and full diff.

Use the approved bounded GLM route; DeepSeek is allowed only for actual GLM provider/transport failure under the current CRITICAL rules. No budget changes. Candidate deployment is authorized only after all gates and source/artifact verification; Program Files and final remain untouched. FL validation remains NOT TESTED until the user confirms it.

## X speed design and verification

For a fixed, valid source window of N samples, the ideal source position is p[n] = x[n] * (N - 1). Its signed natural playback rate is r[n] = p[n] - p[n-1] source samples per output sample: positive is forward, negative reverse. For a tone in the fixed window, constant rates 0.5, 1 and 2 must yield frequencies 0.5, 1 and 2 times the source frequency without a separate pitch shifter. A stationary X uses the existing bounded micro-loop rather than a repeated DC sample. The source window's anchor and size must remain fixed for each grain, so writer motion is not mistaken for X speed. Use stereo-linked linear interpolation, retain the existing transition envelope, and never read future or expired samples. If a requested rate cannot be read safely for an entire grain, transition safely rather than accessing invalid history; report the limitation. Validate abrupt changes, direction reversals, stationary transitions, and sample-rate/block-size invariance against an independent fixed-PCM reference. This is a design contract, not evidence of implementation or performance.
