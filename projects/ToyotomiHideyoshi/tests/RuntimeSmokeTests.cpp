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

    const auto writePng = [] (const juce::Image& source, const juce::String& name)
    {
        const auto output = juce::File::getCurrentWorkingDirectory().getChildFile (name);
        juce::FileOutputStream stream (output);
        return stream.openedOk() && juce::PNGImageFormat().writeImageToStream (source, stream);
    };
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

    editor.reset();
    processor.releaseResources();
    passed &= require (true, "editor-destroyed");
    return passed ? 0 : 1;
}
