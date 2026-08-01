#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "UIComponents.h"
#include <iostream>

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

    auto image = juce::Image (juce::Image::ARGB, 1280, 853, true);
    juce::Graphics graphics (image);
    editor->paint (graphics);
    passed &= require (image.isValid(), "editor-painted");

    editor.reset();
    processor.releaseResources();
    passed &= require (true, "editor-destroyed");
    return passed ? 0 : 1;
}
