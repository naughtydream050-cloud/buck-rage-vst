# Phase 1 Blueprint

## Architecture

`PluginProcessor` owns host-visible globals, lock-free meter transport, minimal playhead snapshot, and later the serialized pattern state. `MainPluginEditor` composes focused UI components and reads the fixed 1280 x 853 geometry contract from the UI specification.

## Static versus dynamic

The reference JPEG remains the source for the samurai artwork, crest, distressed texture, brush lettering, and decoration. Title/status text, tabs, bar cells, count cells, preset cells, parameters, XY trajectory, meters, and bottom status are JUCE components; the complete reference image is never used as a live UI background.

## Meter contract

The tap point is output after pass-through. The audio thread publishes independent channel maxima with `std::atomic<float>` compare-exchange. The UI consumes them with `exchange(0)` at approximately 30 Hz and applies fast attack/slow release in the declared -60 dB to +6 dB range.

## CI build gate

The dedicated `windows-2022` workflow uses the complete repository `JUCE` 7.0.12 tree, CMake 3.31.6, and MSVC 19.44.35228. Run `30315369457` configured and built `ToyotomiHideyoshi_VST3`, verified the complete VST3 bundle, and uploaded `ToyotomiHideyoshi-Windows-VST3`. The bundle was `4004247` bytes at `ToyotomiHideyoshi_artefacts/Release/VST3/Toyotomi Hideyoshi.vst3`.

## Phase order

1. Capture reference geometry and style.
2. Draft component skeleton.
3. Complete the minimum CI build gate.
4. Implement fixed pattern state and serialization only when authorized.
5. Validate output meters and pass-through at runtime.
6. Record state restore and host-validation facts separately.
