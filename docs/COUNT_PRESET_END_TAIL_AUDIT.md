# COUNT 5 preset ending — source audit

Source inspected: 0406e863cfab7ac62b3de238b03dfc11ca834ac7. Applies to
manual COUNT presets, XY absent, normal continuous playback and nonzero DEPTH.
No COUNT DSP was changed. This is a control-flow finding, not an FL listening
result or an exhaustive new native render.

| Preset | LENGTH ends within BAR | BAR changes while old wet is active |
|---|---|---|
| OFF | No own wet source | Incoming OFF routes to dry when XY is inactive; the internal tail is masked |
| CUSTOM | No own COUNT wet source | Incoming CUSTOM is not OFF and can expose the previous wet tail |
| BABY | Last readable wet sample remains during the ~8 ms wet ramp | Old wet can spill into new BAR |
| FORWARD CUT, BACKSPIN, TAPE BRAKE, DRAG, CHIRP, TRANSFORM, ZIGZAG | Wet reader returns zero at expiry; the common ramp can briefly attenuate dry | Old wet can spill into new BAR if the reader returns a nonzero sample |

Evidence: TimelineScratchEngine::wetSample checks LENGTH before preset dispatch
and exempts only BABY with a captured window and positive wet ramp. At a BAR
change process() sets waitingForDry before replacing activeSlot. Phase already
belongs to the new BAR, so the old wet reader can pass the LENGTH check again
during the handoff. The exact result depends on remaining wet gain, capture,
gate/read state, DEPTH, and transport reset. Continuous 1 BAR effects are the
natural repro; a short LENGTH that expired well before the boundary will
usually have no remaining ramp. The processor delays dry and preset lanes
equally; 1024 samples is not an extra effect-only tail. The final mix gates
the COUNT lane with the incoming BAR's delayed Preset-active flag and DEPTH.
OFF (or DEPTH 0) therefore masks the internal COUNT tail.

Native reproduction: run a phase-coded stereo source into BAR N with each
factory COUNT preset, followed by BAR N+1 OFF and by a non-OFF preset in
separate renders. Capture both engine trace and final output. Compare compensated output
against a dry render around LENGTH expiry and the BAR boundary. Test short
LENGTH and 1 BAR, DEPTH 0/100, and log whether the residual is old PCM or just
dry gain reduction. The existing BABY constant-input test checks release
continuity, not all-preset audibility.
