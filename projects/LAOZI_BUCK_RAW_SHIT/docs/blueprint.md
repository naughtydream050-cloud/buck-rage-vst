# 老子-BUCK RAW SHIT- Blueprint

The faceplate comes directly from the supplied reference. Five circular RGBA knob crops cover their matching reference knobs completely, avoiding double rendering while preserving the original visual language. `ui/spec/ui-spec.json` owns placement; editor code only consumes generated layout constants.

The Windows tester package is created only after CI verifies a non-empty VST3 bundle. FL Studio validation remains a user-host gate.

## Phase 2 DSP

PRESSURE is a normalized soft-saturation stage; KICK adds a controlled 125 Hz impact band; AURA combines upper-band excitation with moderate M/S side expansion; GLUE applies linked peak compression. A DC blocker follows the nonlinear chain. All state is preallocated in the processor and `processBlock` uses no allocation or locks. Oversample selection remains UI/state-only pending a dedicated latency pass. Preset names describe sound character rather than reference artists. (Trust: 9)
