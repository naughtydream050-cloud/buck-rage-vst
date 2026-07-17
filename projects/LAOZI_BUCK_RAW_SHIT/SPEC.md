# 老子-BUCK RAW SHIT- Spec

- Visual truth: `ui/reference/reference.png`.
- UI SSOT: `ui/spec/ui-spec.json`.
- Phase 2: Windows VST3 mastering DSP and sound-character preset implementation.
- Audio path: input → KICK low-band impact → AURA high-band/M-S width → PRESSURE saturation → GLUE linked compression → DC blocker → OUTPUT gain → stereo meter → output. Bypass leaves input unchanged.
- Presets use sound-character names only; artist reference names never appear in the product UI.
- Oversample selection remains state/UI-only until a separate latency and host-validation pass.
- macOS build, signing, and FL Studio host-validation claims remain out of scope.
