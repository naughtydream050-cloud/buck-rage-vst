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

    const auto cropBounds = bounds.toNearestInt();
    const auto cropped = faceplate.getClippedImage(cropBounds).convertedToFormat(juce::Image::ARGB);
    auto masked = juce::Image(juce::Image::ARGB, cropBounds.getWidth(), cropBounds.getHeight(), true);

    juce::Graphics g(masked);
    juce::Path circularMask;
    circularMask.addEllipse(masked.getBounds().toFloat());
    g.reduceClipRegion(circularMask);
    g.drawImageAt(cropped, 0, 0);

    return masked;
}

void setupKnob(ImageKnobSlider& knob, juce::Image image)
{
    knob.setOpaque(false);
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
    setSize(static_cast<int>(std::round(RudeHypeGeneratedLayout::displayWidth)),
            static_cast<int>(std::round(RudeHypeGeneratedLayout::displayHeight)));

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
    shoutKnob.setBounds(RudeHypeGeneratedLayout::shoutDisplayBounds().toNearestInt());
    burnKnob.setBounds(RudeHypeGeneratedLayout::burnDisplayBounds().toNearestInt());
}

void RudeHypeEditor::paint(juce::Graphics& g)
{
    if (faceplateImage.isValid())
    {
        g.drawImage(faceplateImage, getLocalBounds().toFloat());
        return;
    }

    g.fillAll(juce::Colour(0xff11110f));
    g.setColour(juce::Colours::red);
    g.setFont(18.0f);
    g.drawText("RUDE HYPE BinaryData load failed", getLocalBounds(), juce::Justification::centred);
}
