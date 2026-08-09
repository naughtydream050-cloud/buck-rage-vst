#include "PluginEditor.h"
#include <iostream>

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
 #define TOYOTOMI_HAS_BINARY_DATA 1
#else
 #define TOYOTOMI_HAS_BINARY_DATA 0
#endif

namespace
{
juce::Image loadMasterDefault()
{
#if TOYOTOMI_HAS_BINARY_DATA
    int size = 0;
    // juce_add_binary_data converts the filename into an identifier.  The
    // previous literal filename lookup always returned nullptr in the VST3,
    // leaving the editor with its opaque black clear colour.
    const auto* data = BinaryData::getNamedResource ("A_default_1280x853_png", size);
    const auto image = data != nullptr ? juce::ImageFileFormat::loadFrom (data, static_cast<size_t> (size)) : juce::Image {};
    if (! image.isValid() || image.getWidth() != 1280 || image.getHeight() != 853)
        std::cerr << "BACKGROUND_ASSET_FAIL resource=A_default_1280x853_png bytes=" << size << '\n';
    return image;
#else
    return {};
    return {};
#endif
}
}

ToyotomiHideyoshiAudioProcessorEditor::ToyotomiHideyoshiAudioProcessorEditor (
    ToyotomiHideyoshiAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      referenceImage (loadMasterDefault()),
      topBar (p),
      artwork (referenceImage),
      countParameters (p),
      outputMeter (p)
{
    auto& state = processor.getStateModel();
    const auto uiState = state.getUiState();
    barTabs.setSelectedPage (uiState.selectedTab);
    // Page selection is a view-only choice.  It must never mutate the active
    // BAR/COUNT slot or any parameter state.
    barMap.setDisplayState (uiState.selectedTab, uiState.selectedBar, -1);
    countGrid.setSelectedCount (uiState.selectedCount);
    presetPalette.setSelectedPreset (static_cast<int> (state.getCount (uiState.selectedBar, uiState.selectedCount).preset));
    barTabs.onSelectedPage = [this, &state] (int page)
    {
        state.selectTab (page);
        const auto ui = state.getUiState();
        barMap.setDisplayState (ui.selectedTab, ui.selectedBar, -1);
    };
    barMap.onSelectedBar = [this, &state] (int bar)
    {
        state.selectBar (bar);
        const auto ui = state.getUiState();
        barMap.setDisplayState (ui.selectedTab, ui.selectedBar, -1);
    };
    countGrid.onSelectedCount = [&state] (int count) { state.selectCount (count); };
    presetPalette.onPresetSelected = [&state] (int preset) { state.setSelectedPreset ((PluginStateModel::ScratchPreset) preset); };
    countParameters.onLengthSelected = [&state] (int length) { state.setSelectedLength ((PluginStateModel::NoteLength) length); };
    xyPad.onMotionChanged = [&state] (const std::vector<PluginStateModel::MotionPoint>& motion) { state.setSelectedMotion (motion); };
    xyPad.onClearMotion = [&state] { state.clearSelectedMotion(); };
    xyPad.onResetCount = [&state] { const auto ui = state.getUiState(); state.resetCountSlot (ui.selectedBar, ui.selectedCount); };
    for (auto* component : std::array<juce::Component*, 10> {
             &topBar, &artwork, &barTabs, &barMap, &countGrid,
             &xyPad, &presetPalette, &countParameters, &outputMeter, &bottomStatus })
    {
        addAndMakeVisible (*component);

        component->setAlpha (1.0f);
    }

    artwork.toBack();
    topBar.toFront (false); barTabs.toFront (false); barMap.toFront (false);
    countGrid.toFront (false); xyPad.toFront (false); presetPalette.toFront (false);
    countParameters.toFront (false); outputMeter.toFront (false); bottomStatus.toFront (false);

    setOpaque (true);
    // Image cutouts are authored at native pixels.  A fitted/scaled editor
    // makes visual and hit-test geometry diverge in FL Studio.
    setResizable (false, false);
    setSize (uiSpec.getCanvasWidth(), uiSpec.getCanvasHeight());
}

void ToyotomiHideyoshiAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (referenceImage.isValid())
    {
        g.drawImageAt (referenceImage, 0, 0);
    }
    else
    {
        static bool logged = false;
        if (! logged)
        {
            std::cerr << "BACKGROUND_PAINT_SKIPPED invalid-master-default\n";
            logged = true;
        }
    }
}

void ToyotomiHideyoshiAudioProcessorEditor::resized()
{
    const auto viewport = getLocalBounds();
    topBar.setBounds          (uiSpec.scaleRegion ("topBar", viewport));
    artwork.setBounds         (uiSpec.scaleRegion ("artwork", viewport));
    barTabs.setBounds         (uiSpec.scaleRegion ("barTabs", viewport));
    barMap.setBounds          (uiSpec.scaleRegion ("barMap", viewport));
    countGrid.setBounds       (uiSpec.scaleRegion ("countGrid", viewport));
    xyPad.setBounds           (uiSpec.scaleRegion ("xyPad", viewport));
    presetPalette.setBounds   (uiSpec.scaleRegion ("presetPalette", viewport));
    countParameters.setBounds (uiSpec.scaleRegion ("countParameters", viewport));
    outputMeter.setBounds     (uiSpec.scaleRegion ("outputMeter", viewport));
    bottomStatus.setBounds    (uiSpec.scaleRegion ("bottomStatus", viewport));

}
