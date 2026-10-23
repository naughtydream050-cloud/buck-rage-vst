# XY Live R2: Grain Reference Retention Optimization

This implementation note supplements the approved XY Live R2 Plan and does not
change its audio behavior or acceptance criteria.

## Change

Live grains reference immutable sample serials in the preallocated stereo history
ring instead of copying a complete grain on every launch. The user-selectable
history remains capped at 125 ms. Physical ring capacity adds one 20 ms grain
lifetime so a grain launched from the oldest selectable position remains valid
until its final sample is read, including across ring wrap.

The source serial tags remain checked on every render read. A missing or overwritten
serial rejects the read; no stale slot is treated as valid. Hold material and the
3 ms re-entry tail remain separately preallocated buffers.

## Memory bound

At 96 kHz the stereo float history is approximately 13920 frames, plus two frame
guards; the serial array has one `uint64_t` per frame. This is fixed at prepare
time, with no process-thread allocation. The 125 ms selection window is unchanged.

## Required verification

`XYPadEngineTests` checks forward/reverse fractional mapping, ring wrap, and that
the oldest selectable grain remains readable for its complete 20 ms lifetime.
The same-source Windows CI and D-machine re-entrant benchmark remain required;
this note is not evidence that those gates passed.
