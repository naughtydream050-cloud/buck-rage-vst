#include "PluginStateModel.h"
#include <iostream>

namespace
{
bool ok = true;
void check (bool value, const char* label) { std::cout << (value ? "PASS " : "FAIL ") << label << '\n'; ok &= value; }
}

int main()
{
    PluginStateModel model;
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
           && model.getSlot (4).preset == beforeTab.preset
           && model.getSlot (4).motion.size() == beforeTab.motion.size(),
           "tab-switch-is-view-only");

    const auto bypassBefore = model.getUiState().bypass;
    for (int preset = 0; preset < 10; ++preset)
    {
        model.setSelectedPreset (static_cast<PluginStateModel::ScratchPreset> (preset));
        check (model.getSlot (5).preset == static_cast<PluginStateModel::ScratchPreset> (preset)
               && model.getUiState().bypass == bypassBefore,
               "preset-single-selection-and-bypass-isolation");
    }
    model.setBypass (true);
    check (model.getSlot (5).preset == PluginStateModel::ScratchPreset::custom && model.getUiState().bypass,
           "bypass-does-not-alter-timeline-slot");

    const auto state = model.toValueTree();
    PluginStateModel restored;
    check (restored.fromValueTree (state), "v2-round-trip");
    check (restored.getSlot (4).customMotion && restored.getSlot (4).motion.size() == 3
           && restored.getSlot (4).length == PluginStateModel::NoteLength::quarter
           && restored.getSlot (4).speed == 1.75f && restored.getSlot (4).pitch == -3.0f,
           "timeline-slot-parameters-round-trip");

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

    PluginStateModel preserved = restored;
    juce::ValueTree future ("ToyotomiHideyoshiState"); future.setProperty ("stateVersion", 99, nullptr);
    check (! restored.fromValueTree (future) && restored.getSlot (4).speed == preserved.getSlot (4).speed,
           "future-version-preserves-state");
    return ok ? 0 : 1;
}
