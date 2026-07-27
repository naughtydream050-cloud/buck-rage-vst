# Phase 1 Blueprint

## Architecture

`PluginProcessor` owns host-visible globals, lock-free meter transport, minimal playhead snapshot, and later the serialized pattern state. `MainPluginEditor` composes focused UI components and reads geometry from `ui/spec/ui-spec.json`.

The persistent model will use a fixed 64 × 16 array in memory. Serialization will use a versioned ValueTree with validated numeric ranges and bounded motion-point counts. It will not register 1024 slots as APVTS parameters.

## Static versus dynamic

Static reference-derived rendering is limited to the samurai artwork, crest, distressed texture, brush lettering, and decorative marks. The title/status text, tabs, bar cells, count cells, preset cells, parameter controls, XY trajectory, meters, and bottom status are JUCE components.

The Phase 1 skeleton keeps the original JPEG as a visual-truth/source atlas. It draws only approved static source regions; it does not place the complete screenshot behind live controls.

## Meter contract

The tap point is the output after pass-through. The audio thread publishes independent channel maxima with `std::atomic<float>` compare-exchange. The UI consumes with `exchange(0)` at approximately 30 Hz and applies fast attack/slow release in the declared -60 dB to +6 dB visual range.

## Build gate

The project uses CMake 3.22+, C++17, JUCE 7.0.12, and the VST3 `Fx` category. `Rzfc` and `TyHd` are four-character JUCE identifiers. CMake validates a complete JUCE source tree (`CMakeLists.txt` plus `modules/CMakeLists.txt`) before adding it; `TOYOTOMI_JUCE_DIR` can supply a canonical checkout.

`tools/build_windows.ps1` does not alter PATH and can use a pre-existing CMake/MSVC shell if one is supplied. Traced Windows VST3 evidence for VINTAGE_RAWNESS, BUCK_RAGE, and LAOZI uses GitHub Actions (`windows-latest`, `cmake`, then `cmake --build`) and downloads the resulting artifact locally; no local CMake cache, generated solution, compiler path, or build log remains. Build status remains NOT RUN until configure, compile, link, and bundle creation are independently observed.

## Phase order

1. Capture reference geometry and style.
2. Draft component skeleton.
3. Implement fixed pattern state and serialization.
4. Connect host display and global state.
5. Validate output meters and pass-through.
6. Configure/build Windows VST3.
7. Record state restore and host-validation truth separately.
