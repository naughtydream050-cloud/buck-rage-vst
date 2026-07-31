# UI Asset Separation

`Resources/reference_ui.jpg` is the 1280 x 853 visual truth. `faceplate_static.png` keeps only permanent artwork, frames, labels, ornament and meter scale. Variable wells are intentionally blanked before the Editor draws state.

| Region | Static asset | Dynamic layer |
| --- | --- | --- |
| Samurai / crest / texture | faceplate | none |
| BAR / COUNT panel frames | faceplate | cut-out cell surface, text, selected / playing overlay |
| SPEED / PITCH / DEPTH | faceplate panel | knob ring asset, rotated pointer, value |
| LENGTH / action buttons | faceplate panel | state overlay and matching hit bounds |
| XY PAD | faceplate glass / axes | clipped motion path and node |
| OUTPUT | faceplate frame / dB scale | meter-well asset, L/R LED bars, peak values |
| Top / bottom status | faceplate labels | live values and bypass state |

All bounds originate from the 1280 x 853 `ui/spec/ui-spec.json` control entries and use the Editor's shared fitted-canvas transform.

