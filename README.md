# RUDE HYPE

JUCE VST3 dirty saturation plugin with image-first RUDE HYPE UI.

- UI source: `C:\Users\razor\Downloads\S__45752322.jpg`
- Canvas: `1592x988`
- Display scale: `0.42` (`669x415` editor size)
- Faceplate: `Resources/faceplate_rude_hype.png`
- Rotating knobs: `Resources/knob_shout.png`, `Resources/knob_burn.png`
- Knob alpha: RGBA PNG with fully transparent pixels outside the circle
- UI SSOT: `ui/spec/ui-spec.json`
- CI fallback: when embedded BinaryData is unavailable, the editor loads `C:\Users\razor\Downloads\S__45752322.jpg`, crops SHOUT/BURN at runtime, and applies circular alpha masks.

## Design

The full panel remains image-first. SHOUT and BURN use circular cropped image knobs that rotate in JUCE, so the knob texture and marker rotate as captured from the flat reference image without rectangular backgrounds.

## Verify

Run from `D:\Development\RAZOR_FACE_COMPANY\01_PLUGINS`:

```powershell
node projects/RUDE_HYPE/tools/validate_knob_rotation.mjs --out reports/latest/rude-hype/knob-rotation-report.json
node projects/RUDE_HYPE/tools/validate_knob_alpha.mjs --out reports/latest/rude-hype/knob-alpha-report.json
node projects/RUDE_HYPE/tools/validate_hit_areas.mjs --out reports/latest/rude-hype/hit-area-report.json --tolerance 1
node projects/RUDE_HYPE/tools/validate_render_capture.mjs --out reports/latest/rude-hype/render-capture-report.json
node tools/harness/run_harness.mjs --project projects/RUDE_HYPE --out reports/latest/rude-hype --reference projects/RUDE_HYPE/ui/reference/reference.png --rendered projects/RUDE_HYPE/ui/rendered/rendered.png
node tools/harness/make_context_pack.mjs --project projects/RUDE_HYPE --reports reports/latest/rude-hype --out reports/latest/rude-hype-context-pack.json
```
