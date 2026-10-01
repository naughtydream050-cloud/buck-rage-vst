# FORWARD CUT — approved Sol Plan

This critical DSP change is limited to FORWARD CUT and its deterministic tests.
UI, state schema, parameter IDs, and BABY, BACKSPIN, TAPE BRAKE, and OFF DSP
behavior are locked.

At effect entry, capture one safe fixed 125 ms window. Keep it fixed for the
BAR/LENGTH. Read only forward at native rate `+1.0`; SPEED does not change the
read rate and only sets `cycleMs = 125 ms / SPEED`. Each cycle reads from the
capture head, fades in for about 5 ms, reads forward, fades out for about 5 ms,
then outputs complete Wet silence for the remaining cycle. The read head does
not advance during silence and the next cycle starts at the capture head.

LENGTH is the total effect time. The final about 8 ms transitions Wet to Dry;
there are no retriggers after LENGTH. DEPTH is only the Dry/Wet mix.

Transport STOP invalidates capture/read/retrigger state and returns Dry. It is
separate from source-ended detection. On process-before-effect Dry input, both
channels must remain below `1e-4` for 30 ms continuously before source-ended is
latched. A zero crossing, a gap of 20 ms or less, or a short drum gap does not
end the source. Source-ended fades Wet to Dry for about 8 ms and prevents all
further retriggers in the current BAR, even if input resumes. The latch clears
only at the next BAR effect entry. A BAR beginning with silence must not start an
old capture.

SPEED values 0.25, 1.0, and 4.0, every LENGTH, DEPTH 0/100, Host Sync ON/OFF,
64 BAR continuity, source silence, STOP, LENGTH boundary, retrigger click,
finite output, and realtime safety are required deterministic evidence.
