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
    const auto* data = BinaryData::getNamedResource ("master_default_no_count_grid_title_1280x853_png", size);
    const auto image = data != nullptr ? juce::ImageFileFormat::loadFrom (data, static_cast<size_t> (size)) : juce::Image {};
    if (! image.isValid() || image.getWidth() != 1280 || image.getHeight() != 853)
        std::cerr << "BACKGROUND_ASSET_FAIL resource=master_default_no_count_grid_title_1280x853_png bytes=" << size << '\n';
    return image;
#else
    return {};
#endif
}

juce::Image loadQuoteImage()
{
#if TOYOTOMI_HAS_BINARY_DATA
    int size = 0;
    const auto* data = BinaryData::getNamedResource ("quote_panel_user_20260814_512x360_png", size);
    return data != nullptr ? juce::ImageFileFormat::loadFrom (data, static_cast<size_t> (size)) : juce::Image {};
#else
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
      quotePanel (loadQuoteImage()),
      countParameters (p),
      outputMeter (p)
{
    auto& state = processor.getStateModel();
    const auto uiState = state.getUiState();
    displayTab = uiState.selectedTab;
    barTabs.setSelectedPage (displayTab);
    // Page selection is a view-only choice.  It must never mutate the active
    // BAR/COUNT slot or any parameter state.
    barMap.setDisplayState (displayTab, uiState.selectedBar, processor.getCurrentTimelineSlot());
    barMap.setSlotPreview ([&state] (int bar) { return state.getSlot (bar); });
    presetPalette.setPresetProvider ([&state]
    {
        const auto ui = state.getUiState();
        return static_cast<int> (state.getSlot (ui.selectedBar).preset);
    });
    barTabs.onSelectedPage = [this, &state] (int page)
    {
        state.selectTab (page);
        displayTab = page;
        const auto ui = state.getUiState();
        barMap.setDisplayState (displayTab, ui.selectedBar, processor.getCurrentTimelineSlot());
    };
    barMap.onSelectedBar = [this, &state] (int bar)
    {
        state.selectBar (bar);
        const auto ui = state.getUiState();
        barMap.setDisplayState (displayTab, ui.selectedBar, processor.getCurrentTimelineSlot());
    };
    presetPalette.onPresetSelected = [this, &state] (int preset) { state.setSelectedPreset ((PluginStateModel::ScratchPreset) preset); barMap.setSlotPreview ([&state] (int bar) { return state.getSlot (bar); }); };
    countParameters.onLengthSelected = [&state] (int length) { state.setSelectedLength ((PluginStateModel::NoteLength) length); };
    xyPad.onMotionChanged = [this, &state] (const std::vector<PluginStateModel::MotionPoint>& motion) { state.setSelectedMotion (motion); barMap.setSlotPreview ([&state] (int bar) { return state.getSlot (bar); }); };
    xyPad.onClearMotion = [this, &state] { state.clearSelectedMotion(); barMap.setSlotPreview ([&state] (int bar) { return state.getSlot (bar); }); };
    xyPad.onResetSlot = [this, &state] { state.resetSelectedSlot(); barMap.setSlotPreview ([&state] (int bar) { return state.getSlot (bar); }); };
    for (auto* component : std::array<juce::Component*, 10> {
             &topBar, &artwork, &barTabs, &barMap, &quotePanel,
             &xyPad, &presetPalette, &countParameters, &outputMeter, &bottomStatus })
    {
        addAndMakeVisible (*component);

        component->setAlpha (1.0f);
    }

    artwork.toBack();
    topBar.toFront (false); barTabs.toFront (false); barMap.toFront (false);
    quotePanel.toFront (false); xyPad.toFront (false); presetPalette.toFront (false);
    countParameters.toFront (false); outputMeter.toFront (false); bottomStatus.toFront (false);
    quotePanel.setInterceptsMouseClicks (false, false);

    setOpaque (true);
    // Image cutouts are authored at native pixels.  A fitted/scaled editor
    // makes visual and hit-test geometry diverge in FL Studio.
    setResizable (false, false);
    setSize (uiSpec.getCanvasWidth(), uiSpec.getCanvasHeight());
    refreshSelectedSlotViews();
    startTimerHz (30);
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
    quotePanel.setBounds      (uiSpec.scaleRegion ("quotePanel", viewport));
    xyPad.setBounds           (uiSpec.scaleRegion ("xyPad", viewport));
    presetPalette.setBounds   (uiSpec.scaleRegion ("presetPalette", viewport));
    countParameters.setBounds (uiSpec.scaleRegion ("countParameters", viewport));
    outputMeter.setBounds     (uiSpec.scaleRegion ("outputMeter", viewport));
    bottomStatus.setBounds    (uiSpec.scaleRegion ("bottomStatus", viewport));

}

void ToyotomiHideyoshiAudioProcessorEditor::refreshSelectedSlotViews()
{
    const auto ui = processor.getStateModel().getUiState();
    const auto& slot = processor.getStateModel().getSlot (ui.selectedBar);
    countParameters.setSelectedBar (ui.selectedBar);
    xyPad.setSelectedBar (ui.selectedBar);
    xyPad.setMotion (slot.motion);
    countParameters.repaint();
}

void ToyotomiHideyoshiAudioProcessorEditor::timerCallback()
{
    const auto playhead = processor.getCurrentTimelineSlot();
    const auto ui = processor.getStateModel().getUiState();
    barMap.setDisplayState (displayTab, ui.selectedBar, playhead);
    refreshSelectedSlotViews();
}
