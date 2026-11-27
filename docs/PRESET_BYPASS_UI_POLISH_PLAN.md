# Preset selector and bypass polish

## Approved scope

- Project-preset selector displays and steps `Init`, `BRAVO 1`, and `BRAVO 2` from the existing `projectPresetId` state.
- Previous/next controls cycle the existing three IDs; popup selection, restore, and editor reopen use that same state.
- Replace only the two native 80 x 31 BYPASS state sprites with the approved dark-metal horizontal switch: dark OFF lamp; restrained amber ON lamp.
- Keep the existing BYPASS state, hit rectangle, audio behavior, serialization, DSP, XY, COUNT and layout unchanged.

## Acceptance

1. Direct menu selection and arrows render the matching project-preset text and do not change a manual BAR or XY data.
2. Project-preset state round-trips through the existing state model.
3. Bypass state is isolated from project presets and renders exactly one opaque native 80 x 31 sprite at the existing rectangle in each state.
4. Existing StateModel and RuntimeSmoke tests pass. Host listening remains NOT TESTED.

## Approved bypass asset contract

| State | Source | Runtime sprite | Native and hit bounds | SHA-256 |
| --- | --- | --- | --- | --- |
| OFF | `Resources/ui-v2/source/generated/preset-bypass-polish-20261127/approved-toolbar-comparison.png` | `Resources/ui-v2/runtime-1024/standalone/bypass_off.png` | `931,14,80,31` | `743A75545C039DA5055B809369C1D1934508E5470FD748664FD5EC34A6559669` |
| ON | `Resources/ui-v2/source/generated/preset-bypass-polish-20261127/approved-toolbar-comparison.png` | `Resources/ui-v2/runtime-1024/standalone/bypass_on.png` | `931,14,80,31` | `75E556D149386A62323AD5DEF97EDB234ED7AF36868FD4C0363F76023868DDDA` |

Both sprites are fully opaque and are selected exclusively from the existing `UiState::bypass` field.
