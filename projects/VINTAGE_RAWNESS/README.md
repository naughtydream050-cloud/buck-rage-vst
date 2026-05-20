# VINTAGE RAWNESS

Image-first JUCE/VST3 effect plugin for Krump and Buck track melody processing.

The generated UI image is the visual source of truth. JUCE draws the faceplate image, overlays three circular RGBA image knobs, and exposes four preset hit areas.

## Controls

- DIRT: wave shaping and saturation
- CRUSH: bit depth and sample hold reduction
- WOBBLE: irregular random-walk modulation

## Presets

- Vintage Hype: `DIRT 0.30`, `CRUSH 0.20`, `WOBBLE 0.10`
- Nasty Chain: `DIRT 0.80`, `CRUSH 0.70`, `WOBBLE 0.40`
- MF Heaveness: `DIRT 0.40`, `CRUSH 0.30`, `WOBBLE 0.90`
- Bout: `DIRT 0.60`, `CRUSH 0.50`, `WOBBLE 0.20`

## Build

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

macOS builds add AU automatically and use `CMAKE_OSX_DEPLOYMENT_TARGET=11.0`.
