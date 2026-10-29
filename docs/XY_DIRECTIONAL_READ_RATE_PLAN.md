# XY directional read-rate correction

## Scope

Make an X trajectory that travels from left to right across one bar audibly
forward, and the reverse path audibly backward.  This change affects only the
live XY grain read increment.  It does not add Reverb, activate Y audio,
change the UI, alter Preset DSP, or change the fixed output latency.

## Cause

The live increment currently multiplies the normalized X slope by both the
recent-source length and the host phase increment.  A full-bar horizontal
trajectory therefore reads a 125 ms source at about 0.07x at 130 BPM, which
sounds close to a hold instead of a directional scrub.

## Approved behavior

One normalized horizontal sweep per bar maps to a signed 1.0x live-grain
read increment.  Positive X slope is forward; negative X slope is reverse.
The live source placement reserves a complete 20 ms grain in the requested
direction before its final source-range clamp.

## Acceptance criteria

1. A +1.0 normalized X slope launches a +1.0x grain read; a -1.0 slope
   launches -1.0x, before the existing source-range clamp.
2. The rate mapping is independent of host sample rate and bar duration.
3. Still X keeps the existing hold path; Y remains audio-neutral.
4. Forward/reverse transitions remain finite and use the existing crossfade,
   with no audio-thread allocation, lock, logging, or I/O.
5. Existing XY and state tests remain green.
