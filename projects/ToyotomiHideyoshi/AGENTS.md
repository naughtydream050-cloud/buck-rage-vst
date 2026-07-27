# Toyotomi Hideyoshi Project Rules

- Follow the root `01_PLUGINS/AGENTS.md` and QUAD-BOOT phase gates.
- Phase 1 is Windows VST3 UI, state, host display, output meter, and audio pass-through only.
- `ui/spec/ui-spec.json` is the visual and layout SSOT. C++ consumes it.
- The attached 1280 x 853 reference is visual truth. Do not redesign it.
- Keep static artwork separate from JUCE-drawn controls and status.
- Do not add scratch, reverse, pitch-shift, time-stretch, tape-stop, filter, gate, stutter, oversampling, or post-FX DSP in Phase 1.
- Never allocate, lock, update ValueTree, access UI, perform file I/O, or log heavily in `processBlock`.
- Do not push directly to `main`. FL Studio validation must be reported as `NOT TESTED` until performed by the user.

