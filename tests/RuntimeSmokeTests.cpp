#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "UIComponents.h"
#include "UiSpec.h"
#include <algorithm>
#include <iostream>
#include <tuple>

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

uint64_t imageHash (const juce::Image& image)
{
    uint64_t hash = 1469598103934665603ULL;
    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto colour = image.getPixelAt (x, y).getARGB();
            hash ^= static_cast<uint64_t> (colour);
            hash *= 1099511628211ULL;
        }
    return hash;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    bool passed = require (ToyotomiUi::validateEmbeddedImageAssets(), "embedded-image-assets");

    const auto writePng = [] (const juce::Image& source, const juce::String& name)
    {
        const auto output = juce::File::getCurrentWorkingDirectory().getChildFile (name);
        juce::FileOutputStream stream (output);
        return stream.openedOk() && juce::PNGImageFormat().writeImageToStream (source, stream);
    };

#if TOYOTOMI_HAS_BINARY_DATA
    int backgroundBytes = 0;
    const auto* backgroundData = BinaryData::getNamedResource ("A_default_1280x853_png", backgroundBytes);
    const auto backgroundOnly = backgroundData != nullptr
                              ? juce::ImageFileFormat::loadFrom (backgroundData, static_cast<size_t> (backgroundBytes))
                              : juce::Image {};
    passed &= require (backgroundOnly.isValid() && backgroundOnly.getWidth() == 1280 && backgroundOnly.getHeight() == 853,
                       "master-default-background-loaded");
    passed &= require (writePng (backgroundOnly, "toyotomi-editor-background-only.png"), "background-only-preview");
#else
    passed &= require (false, "master-default-background-binarydata-unavailable");
#endif

    ToyotomiHideyoshiAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    passed &= require (true, "processor-created");

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    passed &= require (editor != nullptr, "editor-created");
    if (editor == nullptr)
        return 1;

    editor->setSize (1280, 853);
    const auto bounds = editor->getLocalBounds();
    passed &= require (bounds.getWidth() == 1280 && bounds.getHeight() == 853, "editor-size");

    UiSpec uiSpec;
    passed &= require (uiSpec.getRegion ("barTabs") == juce::Rectangle<int> (317, 99, 542, 34)
                       && uiSpec.getRegion ("barMap") == juce::Rectangle<int> (316, 143, 608, 266),
                       "bar-ui-reference-regions");
    passed &= require (uiSpec.scaledBounds (uiSpec.getRegion ("barTabs"), { 0, 0, 960, 640 })
                           == uiSpec.getRegion ("barTabs"),
                       "fixed-canvas-never-scales-hit-regions");

    BarTabComponent tabs;
    tabs.setSize (542, 34);
    for (int tab = 0; tab < 4; ++tab)
    {
        tabs.setSelectedPage (tab);
        auto tabImage = juce::Image (juce::Image::ARGB, 542, 34, true);
        juce::Graphics tabGraphics (tabImage);
        tabs.paintEntireComponent (tabGraphics, true);
        passed &= require (tabImage.isValid(), ("tab-strip-native-" + juce::String (tab)).toRawUTF8());
    }

    auto image = juce::Image (juce::Image::ARGB, 1280, 853, true);
    juce::Graphics graphics (image);
    editor->paintEntireComponent (graphics, true);
    passed &= require (image.isValid(), "editor-painted");

    passed &= require (writePng (image, "toyotomi-editor-full.png"), "editor-full-preview");

    const std::array<std::pair<juce::Rectangle<int>, const char*>, 4> editorCrops {{
        { { 317,  99, 542,  34 }, "toyotomi-editor-bar-tabs.png" },
        { { 316, 143, 608, 266 }, "toyotomi-editor-bar-map.png" },
        { { 332, 432, 512, 360 }, "toyotomi-editor-count-grid.png" },
        { { 866, 465, 251, 327 }, "toyotomi-editor-knob-panel.png" }
    }};
    for (const auto& [crop, name] : editorCrops)
    {
        auto cropImage = image.getClippedImage (crop);
        passed &= require (writePng (cropImage, name), name);
    }

    const std::array<std::tuple<int, int, int, const char*>, 5> barMapStates {{
        { 0, 10,  5, "bar-map-render-01-selected-11-playing-06.png" },
        { 0,  5,  5, "bar-map-render-02-selected-playing-06.png" },
        { 1, 20, 29, "bar-map-render-03-selected-21-playing-30.png" },
        { 2, 39, -1, "bar-map-render-04-selected-40-playing-outside.png" },
        { 3, 63, 48, "bar-map-render-05-selected-64-playing-49.png" }
    }};
    BarMapComponent barMap;
    barMap.setSize (608, 266);
    passed &= require (barMap.hasReferenceCellBounds(), "bar-map-16-reference-cell-bounds");
    for (const auto& [tab, selectedBar, playingBar, filename] : barMapStates)
    {
        barMap.setDisplayState (tab, selectedBar, playingBar);
        auto barMapImage = juce::Image (juce::Image::ARGB, 608, 266, true);
        juce::Graphics barMapGraphics (barMapImage);
        barMap.paintEntireComponent (barMapGraphics, true);
        auto output = juce::File::getCurrentWorkingDirectory().getChildFile (filename);
        bool rendered = false;
        {
            juce::FileOutputStream stream (output);
            rendered = stream.openedOk()
                       && juce::PNGImageFormat().writeImageToStream (barMapImage, stream);
            stream.flush();
        }

        // The BAR MAP now has an opaque image frame.  Each BarCell still clips
        // its own state image, label and playing badge to getLocalBounds(); the
        // smoke test verifies that every state can be rendered into that frame.
        passed &= require (rendered && output.existsAsFile() && output.getSize() > 0, filename);
    }

    const std::array<int, 25> labelBars {{ 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
                                            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63 }};
    auto labelSheet = juce::Image (juce::Image::ARGB, 5 * 72, 5 * 94, true);
    juce::Graphics labelGraphics (labelSheet);
    for (int i = 0; i < static_cast<int> (labelBars.size()); ++i)
    {
        BarCellComponent cell;
        cell.configure (labelBars[static_cast<size_t> (i)], false, false);
        cell.setSize (72, 94);
        auto cellImage = juce::Image (juce::Image::ARGB, 72, 94, true);
        juce::Graphics cellGraphics (cellImage);
        cell.paintEntireComponent (cellGraphics, true);
        labelGraphics.drawImageAt (cellImage, (i % 5) * 72, (i / 5) * 94);
    }
    passed &= require (writePng (labelSheet, "toyotomi-bar-labels-40-64.png"), "bar-labels-40-64-preview");

    const std::array<int, 9> waveformBars {{ 0, 1, 2, 3, 4, 5, 39, 48, 63 }};
    auto waveformSheet = juce::Image (juce::Image::ARGB, 3 * 72, 3 * 94, true);
    juce::Graphics waveformGraphics (waveformSheet);
    std::array<uint64_t, waveformBars.size()> waveformHashes {};
    for (int i = 0; i < static_cast<int> (waveformBars.size()); ++i)
    {
        BarCellComponent cell;
        cell.configure (waveformBars[static_cast<size_t> (i)], false, false);
        cell.setSize (72, 94);
        auto cellImage = juce::Image (juce::Image::ARGB, 72, 94, true);
        juce::Graphics cellGraphics (cellImage);
        cell.paintEntireComponent (cellGraphics, true);
        waveformHashes[static_cast<size_t> (i)] = imageHash (cellImage.getClippedImage ({ 5, 33, 62, 30 }));
        waveformGraphics.drawImageAt (cellImage, (i % 3) * 72, (i / 3) * 94);
    }
    std::sort (waveformHashes.begin(), waveformHashes.end());
    const auto uniqueWaveforms = std::distance (waveformHashes.begin(), std::unique (waveformHashes.begin(), waveformHashes.end()));
    passed &= require (uniqueWaveforms == static_cast<ptrdiff_t> (waveformBars.size()), "bar-waveforms-not-all-identical");
    passed &= require (writePng (waveformSheet, "toyotomi-bar-waveform-comparison.png"), "bar-waveform-comparison-preview");

    editor.reset();
    processor.releaseResources();
    passed &= require (true, "editor-destroyed");
    return passed ? 0 : 1;
}
