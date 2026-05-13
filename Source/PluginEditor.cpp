#include "PluginEditor.h"
#include "GeneratedLayout.h"

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
 #define RUDE_HYPE_HAS_BINARY_DATA 1
#else
 #define RUDE_HYPE_HAS_BINARY_DATA 0
#endif

namespace
{
constexpr auto kExternalReferenceImagePath = "C:/Users/razor/Downloads/S__45752322.jpg";

juce::Image loadBinaryImage(const char* name)
{
#if RUDE_HYPE_HAS_BINARY_DATA
    int size = 0;
    if (auto* data = BinaryData::getNamedResource(name, size))
        return juce::ImageCache::getFromMemory(data, size);
#else
    juce::ignoreUnused(name);
#endif
    return {};
}

juce::Image loadReferenceFaceplate()
{
    if (auto image = loadBinaryImage("faceplate_rude_hype_png"); image.isValid())
        return image;

    return juce::ImageFileFormat::loadFrom(juce::File(kExternalReferenceImagePath));
}

juce::Image cropKnobFromFaceplate(const juce::Image& faceplate, juce::Rectangle<float> bounds)
{
    if (!faceplate.isValid())
        return {};

    return faceplate.getClippedImage(bounds.toNearestInt());
}

void setupKnob(ImageKnobSlider& knob, juce::Image image)
{
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters(juce::degreesToRadians(RudeHypeGeneratedLayout::knobAngleStartDeg),
                             juce::degreesToRadians(RudeHypeGeneratedLayout::knobAngleEndDeg),
                             true);
    knob.setAngleRange(RudeHypeGeneratedLayout::knobAngleStartDeg,
                       RudeHypeGeneratedLayout::knobAngleEndDeg);
    knob.setKnobImage(std::move(image));
}
}

RudeHypeEditor::RudeHypeEditor(RudeHypeProcessor& p)
    : AudioProcessorEditor(&p),
      proc(p),
      shoutAttach(p.apvts, "shout", shoutKnob),
      burnAttach(p.apvts, "burn", burnKnob)
{
    setSize(static_cast<int>(RudeHypeGeneratedLayout::canvasWidth),
            static_cast<int>(RudeHypeGeneratedLayout::canvasHeight));

    faceplateImage = loadReferenceFaceplate();

    auto shoutImage = loadBinaryImage("knob_shout_png");
    if (!shoutImage.isValid())
        shoutImage = cropKnobFromFaceplate(faceplateImage, RudeHypeGeneratedLayout::shoutBounds());

    auto burnImage = loadBinaryImage("knob_burn_png");
    if (!burnImage.isValid())
        burnImage = cropKnobFromFaceplate(faceplateImage, RudeHypeGeneratedLayout::burnBounds());

    setupKnob(shoutKnob, std::move(shoutImage));
    setupKnob(burnKnob, std::move(burnImage));

    addAndMakeVisible(shoutKnob);
    addAndMakeVisible(burnKnob);
}

void RudeHypeEditor::resized()
{
    shoutKnob.setBounds(RudeHypeGeneratedLayout::shoutBounds().toNearestInt());
    burnKnob.setBounds(RudeHypeGeneratedLayout::burnBounds().toNearestInt());
}

void RudeHypeEditor::paint(juce::Graphics& g)
{
    if (faceplateImage.isValid())
    {
        g.drawImageAt(faceplateImage, 0, 0);
        return;
    }

    g.fillAll(juce::Colour(0xff11110f));
    g.setColour(juce::Colours::red);
    g.setFont(18.0f);
    g.drawText("RUDE HYPE reference image load failed", getLocalBounds(), juce::Justification::centred);
}
