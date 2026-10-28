# XY preset-priority switch: approved implementation contract

Source baseline: `d03647f1d11cffddc2110f645a8fcdfb22355cfb`.

When XY is valid, route the time-aligned XY layer to the output and fade the
time-aligned preset output out with the existing XY blend ramp. The preset DSP
continues processing off-route, so it can resume without a reset. When XY has
no trajectory, is warming up, has insufficient PCM, or has ended after input
silence, the same ramp returns output to the preset path. This applies to saved
trajectory playback and hold, not only live mouse input.

For `b = clamp(xyBlend, 0, 1)` and `d = clamp(depth, 0, 1)`:

```
xyOutput = dry * (1 - d) + xyWet * d
final    = presetOutput * (1 - b) + xyOutput * b
```

With the preset OFF, the existing XY-only warm-up law remains: dry at `b = 0`
and the same dry-to-XY ramp as before. Depth zero remains bit-exact dry.

Do not change UI, saved state, parameter IDs, Y audio neutrality, 1024-sample
latency, timeline/transport handling, or any preset algorithm. Do not add a
limiter, gain compensation, queue, extra latency, allocation, lock, I/O, or
logging on the audio thread. The reported `boon` sound has no confirmed cause;
this change must add a diagnostic test that distinguishes XY-only output from
the previous parallel-preset mix, but does not claim to remove it.

This is a D manual-test build contract only. It does not authorize candidate,
final, or Program Files deployment.
