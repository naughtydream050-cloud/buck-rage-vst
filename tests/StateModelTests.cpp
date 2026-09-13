#include "PluginStateModel.h"
#include <algorithm>
#include <array>
#include <iostream>

namespace
{
bool ok = true;
void check (bool value, const char* label) { std::cout << (value ? "PASS " : "FAIL ") << label << '\n'; ok &= value; }
}

int main()
{
    PluginStateModel model;
    check (model.getUiState().selectedBar == PluginStateModel::kNoSelectedBar,
           "fresh-default-has-no-selected-bar");
    check (model.getUiState().selectedTab == 0 && model.getUiState().tabHighlight == -1,
           "fresh-default-has-no-selected-tab-highlight");
    model.setSelectedPreset (PluginStateModel::ScratchPreset::backspin);
    model.setSelectedLength (PluginStateModel::NoteLength::quarter);
    check (model.getSlot (0).preset == PluginStateModel::ScratchPreset::off
           && model.getSlot (0).length == PluginStateModel::NoteLength::sixteenth,
           "fresh-default-selection-actions-do-not-mutate-slot-zero");
    model.selectBar (4);
    model.setSelectedPreset (PluginStateModel::ScratchPreset::backspin);
    model.setSelectedLength (PluginStateModel::NoteLength::quarter);
    model.setSelectedSpeed (1.75f);
    model.setSelectedPitch (-3.0f);
    model.setSelectedDepth (0.70f);
    model.setSelectedMotion ({ { .1f, .2f }, { .4f, .5f }, { .8f, .9f } });
    model.selectBar (5);
    model.setSelectedPreset (PluginStateModel::ScratchPreset::chirp);
    check (model.getSlot (4).preset == PluginStateModel::ScratchPreset::custom
           && model.getSlot (5).preset == PluginStateModel::ScratchPreset::chirp,
           "64-timeline-slots-are-independent");

    const auto beforeTab = model.getSlot (4);
    for (const int tab : { 1, 2, 3, 0 }) model.selectTab (tab);
    const auto ui = model.getUiState();
    check (ui.selectedTab == 0 && ui.selectedBar == 5
           && ui.tabHighlight == 0
           && model.getSlot (4).preset == beforeTab.preset
           && model.getSlot (4).motion.size() == beforeTab.motion.size(),
           "tab-switch-is-view-only");

    const auto bypassBefore = model.getUiState().bypass;
    std::array<bool, 10> selectedImages {};
    for (int preset = 0; preset < 10; ++preset)
    {
        model.setSelectedPreset (static_cast<PluginStateModel::ScratchPreset> (preset));
        selectedImages.fill (false);
        selectedImages[static_cast<size_t> (static_cast<int> (model.getSlot (5).preset))] = true;
        check (model.getSlot (5).preset == static_cast<PluginStateModel::ScratchPreset> (preset)
               && std::count (selectedImages.begin(), selectedImages.end(), true) == 1
               && selectedImages[static_cast<size_t> (preset)]
               && model.getUiState().bypass == bypassBefore,
               "preset-single-selection-and-bypass-isolation");
    }
    model.setBypass (true);
    check (model.getSlot (5).preset == PluginStateModel::ScratchPreset::custom && model.getUiState().bypass,
           "bypass-does-not-alter-timeline-slot");
    model.setHostSync (false);
    model.setInternalBpm (133.25);
    model.setInternalTimeSignature (6, 8);
    model.setProjectPresetId (0);

    const auto state = model.toValueTree();
    PluginStateModel restored;
    check (restored.fromValueTree (state), "v2-round-trip");
    check (restored.getSlot (4).customMotion && restored.getSlot (4).motion.size() == 3
           && restored.getSlot (4).length == PluginStateModel::NoteLength::quarter
           && restored.getSlot (4).speed == 1.75f && restored.getSlot (4).pitch == -3.0f,
           "timeline-slot-parameters-round-trip");
    check (! restored.getUiState().hostSync && restored.getUiState().internalBpm == 133.25
           && restored.getUiState().internalTimeSigNumerator == 6
           && restored.getUiState().internalTimeSigDenominator == 8
           && restored.getUiState().projectPresetId == 0,
           "top-controls-round-trip");
    PluginStateModel noSelectionRestored;
    PluginStateModel freshState;
    check (noSelectionRestored.fromValueTree (freshState.toValueTree())
           && noSelectionRestored.getUiState().selectedBar == PluginStateModel::kNoSelectedBar
           && noSelectionRestored.getUiState().tabHighlight == -1,
           "none-selection-round-trips");

    // XY recording is intentionally separate from existing preset/custom
    // motion, and always uses absolute BAR indices.
    check (! model.getSlot (0).xyMotionExists, "fresh-default-has-no-xy-motion");
    model.selectBar (10); // BAR 11
    model.beginSelectedXYMotion (.10f, .20f);
    model.appendSelectedXYMotion (.45f, .60f, .25);
    model.appendSelectedXYMotion (.80f, .90f, .75);
    const auto bar11Motion = model.getSlot (10).xyMotion;
    model.selectBar (11); // BAR 12
    check (! model.getSlot (11).xyMotionExists && model.getSlot (10).xyMotion == bar11Motion,
           "xy-bar-11-and-12-are-independent");
    model.beginSelectedXYMotion (.30f, .40f);
    model.appendSelectedXYMotion (.60f, .30f, .50);
    model.selectBar (10);
    check (model.getSlot (10).xyMotionExists && model.getSlot (10).xyMotion == bar11Motion,
           "xy-bar-switch-restores-absolute-bar-11");
    model.resetSelectedBarXYPosition();
    check (model.getSlot (10).currentX == .5f && model.getSlot (10).currentY == .5f
           && model.getSlot (10).xyMotion == bar11Motion, "xy-reset-keeps-recorded-motion");
    model.clearSelectedBarXYMotion();
    check (! model.getSlot (10).xyMotionExists && model.getSlot (11).xyMotionExists,
           "xy-clear-is-local-and-keeps-current-position");
    for (const auto bar : { 10, 26, 42, 58 })
    {
        model.beginSlotXYMotion (bar, (float) bar / 64.0f, .25f);
        model.appendSlotXYMotion (bar, .75f, (float) bar / 64.0f, .5);
    }
    const auto xyState = model.toValueTree();
    PluginStateModel xyRestored;
    check (xyRestored.fromValueTree (xyState), "xy-save-restore-succeeds");
    bool separateXY = true;
    for (const auto bar : { 10, 26, 42, 58 })
        separateXY &= xyRestored.getSlot (bar).xyMotionExists
                   && xyRestored.getSlot (bar).xyMotion.size() == 2
                   && xyRestored.getSlot (bar).xyMotion[0].x == (float) bar / 64.0f
                   && xyRestored.getSlot (bar).xyMotion[1].timeSeconds == .5;
    check (separateXY && ! xyRestored.getSlot (9).xyMotionExists,
           "xy-bars-11-27-43-59-have-zero-cross-bar-contamination");

    // v1 migration deliberately takes each legacy BAR's Count 0 as its new
    // timeline slot: the former 1024-slot hierarchy no longer exists in UI.
    juce::ValueTree legacy ("ToyotomiHideyoshiState");
    legacy.setProperty ("stateVersion", 1, nullptr);
    juce::ValueTree global ("Global"); global.setProperty ("selectedBar", 3, nullptr); legacy.addChild (global, -1, nullptr);
    juce::ValueTree bars ("Bars"), bar ("Bar"), count ("Count");
    bar.setProperty ("index", 3, nullptr); count.setProperty ("index", 0, nullptr);
    count.setProperty ("preset", static_cast<int> (PluginStateModel::ScratchPreset::drag), nullptr);
    bar.addChild (count, -1, nullptr); bars.addChild (bar, -1, nullptr); legacy.addChild (bars, -1, nullptr);
    PluginStateModel migrated;
    check (migrated.fromValueTree (legacy) && migrated.getSlot (3).preset == PluginStateModel::ScratchPreset::drag,
           "v1-first-count-migrates-to-timeline-slot");
    check (migrated.getUiState().hostSync && migrated.getUiState().internalBpm == 120.0
           && migrated.getUiState().internalTimeSigNumerator == 4
           && migrated.getUiState().internalTimeSigDenominator == 4,
           "legacy-top-controls-defaults-preserved");
    juce::ValueTree legacyTabState ("ToyotomiHideyoshiState"); legacyTabState.setProperty ("stateVersion", 3, nullptr);
    juce::ValueTree legacyGlobal ("Global"); legacyGlobal.setProperty ("selectedTab", 2, nullptr);
    legacyTabState.addChild (legacyGlobal, -1, nullptr);
    PluginStateModel legacyTabRestored;
    check (legacyTabRestored.fromValueTree (legacyTabState)
           && legacyTabRestored.getUiState().selectedTab == 2
           && legacyTabRestored.getUiState().tabHighlight == -1,
           "legacy-tab-page-restores-without-gold-highlight");

    PluginStateModel preserved = restored;
    juce::ValueTree future ("ToyotomiHideyoshiState"); future.setProperty ("stateVersion", 99, nullptr);
    check (! restored.fromValueTree (future) && restored.getSlot (4).speed == preserved.getSlot (4).speed,
           "future-version-preserves-state");
    return ok ? 0 : 1;
}
