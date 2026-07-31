#include "PluginEditor.h"

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
 #define TOYOTOMI_HAS_BINARY_DATA 1
#else
 #define TOYOTOMI_HAS_BINARY_DATA 0
#endif

namespace
{
juce::Image loadFaceplateImage()
{
#if TOYOTOMI_HAS_BINARY_DATA
    return juce::ImageFileFormat::loadFrom (BinaryData::faceplate_static_png,
                                            BinaryData::faceplate_static_pngSize);
#else
    return {};
#endif
}
}

ToyotomiHideyoshiAudioProcessorEditor::ToyotomiHideyoshiAudioProcessorEditor (
    ToyotomiHideyoshiAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      referenceImage (loadFaceplateImage()),
      topBar (p),
      artwork (referenceImage),
      countParameters (p),
      outputMeter (p)
{
    auto& state = processor.getStateModel();
    barTabs.onSelectedPage = [&state] (int page) { state.selectTab (page); };
    barMap.onSelectedBar = [&state] (int bar) { state.selectBar (bar); };
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

        // Static texture remains image-first, while each control paints its
        // live state as a restrained overlay on top of the faceplate.
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
    g.fillAll (ToyotomiUi::background());

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

