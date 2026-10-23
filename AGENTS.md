# Toyotomi Hideyoshi Project Rules

- Follow the root `01_PLUGINS/AGENTS.md` and QUAD-BOOT phase gates.
- Current approved task is XY Pad Phase 2 live-processing re-entrant transition repair at base `2322502e7946e46b85c74d061452861fd76d4efc`, retaining original AC-01..16 and adding approved R2-AC-17..21.
- `ui/spec/ui-spec.json` is the visual and layout SSOT. C++ consumes it.
- The attached 1280 x 853 reference is visual truth. Do not redesign it.
- Keep static artwork separate from JUCE-drawn controls and status.
- The approved XY Plan and amendment are the DSP source of truth. UI, serialized State, parameter IDs, existing Preset algorithms, absolute BAR synchronization, and 1024-sample latency remain frozen.
- Never allocate, lock, update ValueTree, access UI, perform file I/O, or log heavily in `processBlock`.
- Do not push directly to `main`. Candidate promotion requires all deterministic checks, Jev AC PASS, and Codex/Sol approval. FL Studio validation remains `NOT TESTED` until the user performs it.

