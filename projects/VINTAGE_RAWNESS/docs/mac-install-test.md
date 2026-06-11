# VINTAGE RAWNESS Mac Test Install

This file is for private test distribution of the unsigned macOS builds.

## What This Fixes

The Mac test package is built to remove the four usual blockers:

1. Intel Mac vs Apple Silicon mismatch: the CI build is universal `x86_64 + arm64`.
2. Host format mismatch: the CI build ships both `VINTAGE RAWNESS.vst3` and `VINTAGE RAWNESS.component`.
3. Gatekeeper quarantine after download: remove quarantine locally with the commands below.
4. Unsigned bundle rejection during test installs: CI applies ad-hoc codesign and verifies the bundle.

Public release still needs Developer ID signing, notarization, stapling, and real host validation.

## Install Paths

Use the VST3 for FL Studio, Ableton, Reaper, Studio One, Cubase, and other VST3 hosts:

```bash
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
cp -R "VINTAGE RAWNESS.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/"
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/VINTAGE RAWNESS.vst3"
```

Use the AU for Logic and AU hosts:

```bash
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
cp -R "VINTAGE RAWNESS.component" "$HOME/Library/Audio/Plug-Ins/Components/"
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/VINTAGE RAWNESS.component"
```

## Reset Host Cache

After replacing the plugin, restart the DAW. If Logic does not rescan the AU, run:

```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -f "$HOME/Library/Caches/AudioUnitCache/com.apple.audiounits.cache"
rm -f "$HOME/Library/Caches/com.apple.audiounits.cache"
```

Then reopen Logic and rescan plugins.

## Optional Local Checks

```bash
lipo -archs "$HOME/Library/Audio/Plug-Ins/VST3/VINTAGE RAWNESS.vst3/Contents/MacOS/VINTAGE RAWNESS"
codesign --verify --deep --strict "$HOME/Library/Audio/Plug-Ins/VST3/VINTAGE RAWNESS.vst3"
```

Expected architectures:

```text
x86_64 arm64
```

