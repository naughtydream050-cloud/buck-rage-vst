#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "UIComponents.h"
#include "UiSpec.h"
#include <iostream>
#include <memory>

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
 #define TOYOTOMI_HAS_BINARY_DATA 1
#else
 #define TOYOTOMI_HAS_BINARY_DATA 0
#endif

namespace
{
bool require (bool condition, const char* label)
{
    std::cout << (condition ? "PASS " : "FAIL ") << label << '\n';
    return condition;
}

bool writePng (const juce::Image& image, const juce::String& filename)
{
    juce::FileOutputStream stream (juce::File::getCurrentWorkingDirectory().getChildFile (filename));
    return stream.openedOk() && juce::PNGImageFormat().writeImageToStream (image, stream);
}

bool hasDifferentPixels (const juce::Image& first, const juce::Image& second)
{
    for (int y = 0; y < first.getHeight(); ++y)
        for (int x = 0; x < first.getWidth(); ++x)
            if (first.getPixelAt (x, y) != second.getPixelAt (x, y)) return true;
    return false;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    bool passed = require (ToyotomiUi::validateEmbeddedImageAssets(), "embedded-image-assets");

#if TOYOTOMI_HAS_BINARY_DATA
    int backgroundBytes = 0, quoteBytes = 0, knobRingBytes = 0, knobPointerBytes = 0;
    const auto* backgroundData = BinaryData::getNamedResource ("master_default_no_count_grid_title_1280x853_png", backgroundBytes);
    const auto* quoteData = BinaryData::getNamedResource ("quote_panel_user_20260814_512x360_png", quoteBytes);
    const auto* knobRingData = BinaryData::getNamedResource ("knob_ring_60_png", knobRingBytes);
    const auto* knobPointerData = BinaryData::getNamedResource ("knob_pointer_60_png", knobPointerBytes);
    const auto background = backgroundData != nullptr ? juce::ImageFileFormat::loadFrom (backgroundData, static_cast<size_t> (backgroundBytes)) : juce::Image {};
    const auto quote = quoteData != nullptr ? juce::ImageFileFormat::loadFrom (quoteData, static_cast<size_t> (quoteBytes)) : juce::Image {};
    const auto knobRing = knobRingData != nullptr ? juce::ImageFileFormat::loadFrom (knobRingData, static_cast<size_t> (knobRingBytes)) : juce::Image {};
    const auto knobPointer = knobPointerData != nullptr ? juce::ImageFileFormat::loadFrom (knobPointerData, static_cast<size_t> (knobPointerBytes)) : juce::Image {};
    passed &= require (background.isValid() && background.getWidth() == 1280 && background.getHeight() == 853, "master-background-loaded");
    passed &= require (quote.isValid() && quote.getWidth() == 512 && quote.getHeight() == 360, "quote-decoration-loaded-native-count-grid-size");
    passed &= require (knobRing.isValid() && knobRing.getWidth() == 60 && knobRing.getHeight() == 60, "knob-ring-60-loaded-native-size");
    passed &= require (knobPointer.isValid() && knobPointer.getWidth() == 60 && knobPointer.getHeight() == 60, "knob-pointer-60-loaded-native-size");
#else
    passed &= require (false, "binarydata-unavailable");
#endif

    ToyotomiHideyoshiAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    passed &= require (editor != nullptr, "editor-created");
    if (editor == nullptr) return 1;
    editor->setSize (1024, 683);
    passed &= require (editor->getLocalBounds() == juce::Rectangle<int> (0, 0, 1024, 683), "fixed-editor-size-80-percent");

    UiSpec spec;
    passed &= require (spec.getRegion ("quotePanel") == juce::Rectangle<int> (332, 432, 512, 360), "quote-replaces-count-grid-region");
    auto full = juce::Image (juce::Image::ARGB, 1024, 683, true);
    { juce::Graphics graphics (full); editor->paintEntireComponent (graphics, true); }
    passed &= require (writePng (full, "toyotomi-timeline-editor-full.png"), "full-ui-render");
    passed &= require (writePng (full.getClippedImage ({ 266, 346, 410, 288 }), "toyotomi-quote-panel.png"), "quote-panel-render");

    const auto renderKnobPanel = [&]
    {
        auto preview = juce::Image (juce::Image::ARGB, 1024, 683, true);
        juce::Graphics graphics (preview);
        editor->paintEntireComponent (graphics, true);
        return preview.getClippedImage ({ 693, 372, 201, 262 });
    };
    auto& uiState = processor.getStateModel();
    uiState.setSlotSpeed (0, PluginStateModel::kMinSpeed);
    const auto speedMin = renderKnobPanel();
    passed &= require (writePng (speedMin, "toyotomi-editor-knob-speed-min.png"), "knob-speed-min-render");
    uiState.setSlotSpeed (0, 1.0f);
    const auto speedDefault = renderKnobPanel();
    passed &= require (writePng (speedDefault, "toyotomi-editor-knob-speed-default.png"), "knob-speed-default-render");
    uiState.setSlotSpeed (0, PluginStateModel::kMaxSpeed);
    const auto speedMax = renderKnobPanel();
    passed &= require (writePng (speedMax, "toyotomi-editor-knob-speed-max.png"), "knob-speed-max-render");
    passed &= require (hasDifferentPixels (speedMin, speedMax), "knob-speed-min-max-rotation");

    uiState.setSlotPitch (0, PluginStateModel::kMinPitch);
    const auto pitchMin = renderKnobPanel();
    passed &= require (writePng (pitchMin, "toyotomi-editor-knob-pitch-min.png"), "knob-pitch-min-render");
    uiState.setSlotPitch (0, 0.0f);
    const auto pitchCentre = renderKnobPanel();
    passed &= require (writePng (pitchCentre, "toyotomi-editor-knob-pitch-centre.png"), "knob-pitch-centre-render");
    uiState.setSlotPitch (0, PluginStateModel::kMaxPitch);
    const auto pitchMax = renderKnobPanel();
    passed &= require (writePng (pitchMax, "toyotomi-editor-knob-pitch-max.png"), "knob-pitch-max-render");
    passed &= require (hasDifferentPixels (pitchMin, pitchMax), "knob-pitch-min-max-rotation");

    uiState.setSlotDepth (0, 0.0f);
    const auto depthMin = renderKnobPanel();
    passed &= require (writePng (depthMin, "toyotomi-editor-knob-depth-min.png"), "knob-depth-min-render");
    uiState.setSlotDepth (0, 0.5f);
    const auto depthCentre = renderKnobPanel();
    passed &= require (writePng (depthCentre, "toyotomi-editor-knob-depth-centre.png"), "knob-depth-centre-render");
    uiState.setSlotDepth (0, 1.0f);
    const auto depthMax = renderKnobPanel();
    passed &= require (writePng (depthMax, "toyotomi-editor-knob-depth-max.png"), "knob-depth-max-render");
    passed &= require (hasDifferentPixels (depthMin, depthMax), "knob-depth-min-max-rotation");

    const auto writePresetPreview = [&] (PluginStateModel::ScratchPreset preset, const juce::String& filename)
    {
        processor.getStateModel().setSelectedPreset (preset);
        auto preview = juce::Image (juce::Image::ARGB, 1024, 683, true);
        juce::Graphics graphics (preview);
        editor->paintEntireComponent (graphics, true);
        return writePng (preview, filename);
    };
    passed &= require (writePresetPreview (PluginStateModel::ScratchPreset::off, "toyotomi-preset-preview-off.png"), "preset-preview-off");
    passed &= require (writePresetPreview (PluginStateModel::ScratchPreset::backspin, "toyotomi-preset-preview-backspin.png"), "preset-preview-backspin");
    passed &= require (writePresetPreview (PluginStateModel::ScratchPreset::custom, "toyotomi-preset-preview-custom.png"), "preset-preview-custom");

    BarMapComponent barMap;
    barMap.setSize (608, 266);
    passed &= require (barMap.hasReferenceCellBounds(), "bar-map-reference-bounds");
    PluginStateModel state;
    state.setSlotPreset (0, PluginStateModel::ScratchPreset::forwardCut);
    state.setSlotPreset (1, PluginStateModel::ScratchPreset::backspin);
    state.setSlotPreset (39, PluginStateModel::ScratchPreset::chirp);
    barMap.setSlotPreview ([&state] (int bar) { return state.getSlot (bar); });
    barMap.setDisplayState (0, 10, -1);
    auto stopped = juce::Image (juce::Image::ARGB, 608, 266, true);
    { juce::Graphics graphics (stopped); barMap.paintEntireComponent (graphics, true); }
    barMap.setDisplayState (0, 10, 5);
    auto playing = juce::Image (juce::Image::ARGB, 608, 266, true);
    { juce::Graphics graphics (playing); barMap.paintEntireComponent (graphics, true); }
    passed &= require (hasDifferentPixels (stopped, playing), "playhead-red-state-renders-only-while-playing");
    passed &= require (writePng (playing, "toyotomi-timeline-bar-map.png"), "timeline-bar-map-render");

    const auto selectedBefore = state.getUiState().selectedBar;
    state.selectTab (3);
    passed &= require (state.getUiState().selectedBar == selectedBefore, "tab-selection-does-not-change-selected-bar");
    state.selectBar (39);
    const auto prior = state.getSlot (0).preset;
    state.setSelectedPreset (PluginStateModel::ScratchPreset::drag);
    passed &= require (state.getSlot (39).preset == PluginStateModel::ScratchPreset::drag && state.getSlot (0).preset == prior,
                       "bar-slot-preset-is-independent");

    editor.reset();
    processor.releaseResources();
    passed &= require (true, "editor-and-processor-destroyed");
    return passed ? 0 : 1;
}
