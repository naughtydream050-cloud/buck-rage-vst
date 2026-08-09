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
    int backgroundBytes = 0, quoteBytes = 0;
    const auto* backgroundData = BinaryData::getNamedResource ("Adefault1280x853_png", backgroundBytes);
    const auto* quoteData = BinaryData::getNamedResource ("quotepanel512x360_png", quoteBytes);
    const auto background = backgroundData != nullptr ? juce::ImageFileFormat::loadFrom (backgroundData, static_cast<size_t> (backgroundBytes)) : juce::Image {};
    const auto quote = quoteData != nullptr ? juce::ImageFileFormat::loadFrom (quoteData, static_cast<size_t> (quoteBytes)) : juce::Image {};
    passed &= require (background.isValid() && background.getWidth() == 1280 && background.getHeight() == 853, "master-background-loaded");
    passed &= require (quote.isValid() && quote.getWidth() == 512 && quote.getHeight() == 360, "quote-decoration-loaded-native-count-grid-size");
#else
    passed &= require (false, "binarydata-unavailable");
#endif

    ToyotomiHideyoshiAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    passed &= require (editor != nullptr, "editor-created");
    if (editor == nullptr) return 1;
    editor->setSize (1280, 853);
    passed &= require (editor->getLocalBounds() == juce::Rectangle<int> (0, 0, 1280, 853), "fixed-editor-size");

    UiSpec spec;
    passed &= require (spec.getRegion ("quotePanel") == juce::Rectangle<int> (332, 432, 512, 360), "quote-replaces-count-grid-region");
    auto full = juce::Image (juce::Image::ARGB, 1280, 853, true);
    { juce::Graphics graphics (full); editor->paintEntireComponent (graphics, true); }
    passed &= require (writePng (full, "toyotomi-timeline-editor-full.png"), "full-ui-render");
    passed &= require (writePng (full.getClippedImage ({ 332, 432, 512, 360 }), "toyotomi-quote-panel.png"), "quote-panel-render");

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
