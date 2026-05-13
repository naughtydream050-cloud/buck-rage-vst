# RUDE HYPE Mac Build Notes

## GPT Consultation Result

Status: `APPROVE`

The approved first Mac step is a staged macOS VST3 build gate:

- GitHub Actions `macos-14`
- Release build only
- VST3 only
- preserve image-first UI
- no `PluginEditor` doctrine changes
- no AU/codesign/notarization/universal work until the required credentials and bundle strategy exist

## Required Guards

- Keep `ui/spec/ui-spec.json` as UI SSOT.
- Keep `PluginEditor.cpp` as an implementation consumer.
- Keep image knobs on `AffineTransform` rotation.
- Keep circular alpha requirements.
- Add macOS toolchain checks:
  - `cmake`
  - `xcodebuild`
  - `xcode-select`
  - `JUCE` submodule presence
- Treat missing `.vst3` artifact as CI failure.

## Current Limitation

The current GitHub CI route is still a lightweight text-first route. It builds the plugin, but final Mac distribution should embed the RUDE HYPE image assets instead of depending on local runtime fallback paths.

## Deferred Gates

- AU target
- universal binary
- codesign
- notarization
- `auval`

These require a stable bundle ID, packaging plan, and Apple Developer credentials.
