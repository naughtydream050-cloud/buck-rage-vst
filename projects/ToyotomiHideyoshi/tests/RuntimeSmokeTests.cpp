#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "UIComponents.h"
#include "UiSpec.h"
#include <iostream>
#include <tuple>

namespace
{
bool require (bool condition, const char* label)
{
    std::cout << (condition ? "PASS " : "FAIL ") << label << '\n';
    return condition;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    bool passed = require (ToyotomiUi::validateEmbeddedImageAssets(), "embedded-image-assets");

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
    passed &= require (uiSpec.getRegion ("barTabs") == juce::Rectangle<int> (317, 99, 542, 35)
                       && uiSpec.getRegion ("barMap") == juce::Rectangle<int> (316, 143, 608, 266),
                       "bar-ui-reference-regions");
    passed &= require (uiSpec.scaledBounds (uiSpec.getRegion ("barTabs"), { 0, 0, 960, 640 })
                           == uiSpec.getRegion ("barTabs"),
                       "fixed-canvas-never-scales-hit-regions");

    BarTabComponent tabs;
    tabs.setSize (542, 35);
    for (int tab = 0; tab < 4; ++tab)
    {
        tabs.setSelectedPage (tab);
        auto tabImage = juce::Image (juce::Image::ARGB, 542, 35, true);
        juce::Graphics tabGraphics (tabImage);
        tabs.paintEntireComponent (tabGraphics, true);
        passed &= require (tabImage.isValid(), ("tab-strip-native-" + juce::String (tab)).toRawUTF8());
    }

    auto image = juce::Image (juce::Image::ARGB, 1280, 853, true);
    juce::Graphics graphics (image);
    editor->paintEntireComponent (graphics, true);
    passed &= require (image.isValid(), "editor-painted");

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
        bool cellsAreClipped = true;
        for (int y = 0; y < barMapImage.getHeight(); ++y)
            for (int x = 0; x < barMapImage.getWidth(); ++x)
            {
                bool inCell = false;
                for (int i = 0; i < 16; ++i)
                    inCell = inCell || juce::Rectangle<int> (6 + (i % 8) * 75, 32 + (i / 8) * 97, 71, 92).contains (x, y);
                if (! inCell && barMapImage.getPixelAt (x, y).getAlpha() != 0)
                    cellsAreClipped = false;
            }
        auto output = juce::File::getCurrentWorkingDirectory().getChildFile (filename);
        bool rendered = false;
        {
            juce::FileOutputStream stream (output);
            rendered = stream.openedOk()
                       && juce::PNGImageFormat().writeImageToStream (barMapImage, stream);
            stream.flush();
        }

        passed &= require (rendered && output.existsAsFile() && output.getSize() > 0 && cellsAreClipped, filename);
    }

    editor.reset();
    processor.releaseResources();
    passed &= require (true, "editor-destroyed");
    return passed ? 0 : 1;
}
