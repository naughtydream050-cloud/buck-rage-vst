#include "PluginEditor.h"

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
 #define TOYOTOMI_HAS_BINARY_DATA 1
#else
 #define TOYOTOMI_HAS_BINARY_DATA 0
#endif

namespace
{
juce::Image loadReferenceImage()
{
#if TOYOTOMI_HAS_BINARY_DATA
    return juce::ImageFileFormat::loadFrom (BinaryData::reference_ui_jpg,
                                            BinaryData::reference_ui_jpgSize);
#else
    return {};
#endif
}
}

ToyotomiHideyoshiAudioProcessorEditor::ToyotomiHideyoshiAudioProcessorEditor (
    ToyotomiHideyoshiAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      referenceImage (loadReferenceImage()),
      topBar (p),
      artwork (referenceImage),
      countParameters (p),
      outputMeter (p)
{
    auto& state = processor.getStateModel();
    const auto uiState = state.getUiState();
    barTabs.setSelectedPage (uiState.selectedTab);
    barMap.setDisplayState (uiState.selectedTab, uiState.selectedBar, 5);
    barTabs.onSelectedPage = [this, &state] (int page)
    {
        state.selectTab (page);
        const auto ui = state.getUiState();
        barMap.setDisplayState (ui.selectedTab, ui.selectedBar, 5);
    };
    barMap.onSelectedBar = [this, &state] (int bar)
    {
        state.selectBar (bar);
        const auto ui = state.getUiState();
        barMap.setDisplayState (ui.selectedTab, ui.selectedBar, 5);
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
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (1280.0 / 853.0);
    setResizeLimits (960, 640, 1920, 1280);
    setSize (uiSpec.getCanvasWidth(), uiSpec.getCanvasHeight());
}

void ToyotomiHideyoshiAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (referenceImage.isValid())
    {
        const auto canvas = uiSpec.getScaledCanvasBounds (getLocalBounds());
        g.drawImageWithin (referenceImage, canvas.getX(), canvas.getY(),
                           canvas.getWidth(), canvas.getHeight(),
                           juce::RectanglePlacement::centred);
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
