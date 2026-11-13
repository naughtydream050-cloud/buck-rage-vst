# XY Y-axis Speed — approved implementation plan

Source baseline: `e6321dc` on `codex/xy-preset-priority-switch`.  Preserve all
pre-existing dirty BRAVO reports and documents.  This is a Phase 4 realtime DSP
change; only the files named in the worker task may change.

## Behaviour

`Y axis Speed` is an XY-pad right-click toggle, persisted in project state and
defaulting to off when absent from old or new state.  With it off, saved XY
motion and audio output remain the baseline behaviour.  With it on, Y is sampled
with the existing X trajectory: bottom maps to 0.5, centre to 1.0, and top to
2.0.  That multiplier applies once when a live grain is launched to the already
computed signed X reading increment.  It does not change X jump detection,
direction, hold/micro-loop, grain hop/retrigger timing, serial-range checks or
crossfades.  Therefore its audible effect is natural fragment speed and pitch,
not a separate FFT pitch shifter.

The toggle must reach the audio thread only through a lock-free atomic snapshot.
No APVTS IDs, existing preset algorithms, XY points, artwork/layout, or the
1024-sample latency may change.  The BRAVO schedules are out of scope.

## Verification contract

AC-01 compatibility: old state defaults off; save/restore round-trips the
toggle; off and centre-Y output match the previous X-only result; right click
does not mutate a trajectory.

AC-02 audio: fixed source proves 0.5/1/2 signed increment ratios, forward and
reverse direction, stereo-common reading position, and unchanged X hold.  Add a
deterministic X-retrigger plus Y-ramp fixture/WAV evidence if the existing test
support provides it.

AC-03 safety: endpoints/abrupt Y, no-history, seek/loop/stop, depth zero and
bypass remain finite/safe.  No allocation, mutex, ValueTree, I/O or logging is
introduced into `processBlock`; existing 18-condition performance thresholds are
not relaxed.

AC-04 release: focused existing regressions plus Windows CI pass on the same
source.  Jev reviews each AC from bounded evidence; Codex reviews the final diff.
No candidate, final or Program Files writes are allowed in this task.  A manual
test bundle may be placed only after every gate passes and provenance is checked.
