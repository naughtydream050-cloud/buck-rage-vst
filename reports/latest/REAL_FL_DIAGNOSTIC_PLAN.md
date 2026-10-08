# Real FL diagnostic stage (repair remains pending)

Repair Plan AC-01 through AC-09 remain unchanged and required. No repair claim or
normal candidate promotion is authorized by this diagnostic stage.

Instrument the currently failing deployed source fdef59fe so the user can capture
actual FL transport and exact Dry/Wet/output PCM. DSP formulas, capture windows,
source latch, parameter IDs, State and UI stay unchanged.

Diagnostic acceptance criteria:

- DIAG-01: diagnostic observer/processor enabled vs disabled output is bit-exact,
  tested over every sample and all existing preset paths and Host Sync ON/OFF.
- DIAG-02: one record per sample, matching PCM clock/writer serial/output sample;
  all sample slots checked over three four-BAR passes, with sustained gate-open
  Wet counts per individual pass and BAR.
- DIAG-03: raw PPQ/sample presence and values, host loop metadata, origin,
  requested and active BAR, entry, capture bounds, gate, source-ended, read
  position/rate, reset and numeric PCM recorded without changing audio behavior.
- DIAG-04: fixed preallocated SPSC producer, no RT allocation/lock/I/O/string/UI;
  background writer, bounded 180-second sample budget, flush and lifecycle tests.
- DIAG-05: any dropped record invalidates evidence; forced-overflow test rejects
  evidence. Lossless little-endian float PCM and explicit 164-byte sample schema.
- DIAG-06: activation marker absent by default; diagnostic artifact stored in
  output/windows/diagnostic, no replacement of candidate or Program Files.

Parent repair promotion stays BLOCKED until actual real-FL fixture reproduces
before-FAIL and after-PASS, all original nine AC pass, Jev and Sol final approval.

Activation contract: before loading diagnostic bundle, create
output/windows/diagnostic/enable-real-fl-diagnostic.txt with exact content ENABLED.
At prepareToPlay, the marker is checked off the audio thread. Each instance creates
a unique captures/session folder. Close FL after recording to flush the session.
Analyzer: node scripts/analyze-real-host-diagnostic.mjs <session-folder>.
4 BAR and 8 BAR with identical PCM, HOST SYNC ON, 130 BPM, 4/4, LENGTH=1 BAR,
SPEED=1.0, DEPTH=100%, three passes each. CHIRP/TRANSFORM/ZIGZAG separately.

Source roles: GLM/DeepSeek attempted bounded instrumentation but did not complete
it; Codex completed the escalated diagnostic implementation. Worker launcher
PASS with NOT IMPLEMENTED is rejected. No measured Sol baseline: savings UNKNOWN.
