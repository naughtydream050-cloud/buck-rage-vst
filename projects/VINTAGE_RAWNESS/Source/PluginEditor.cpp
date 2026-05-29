#include "PluginEditor.h"
#include "GeneratedLayout.h"

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
 #define VINTAGE_RAWNESS_HAS_BINARY_DATA 1
#else
 #define VINTAGE_RAWNESS_HAS_BINARY_DATA 0
#endif

namespace
{
constexpr auto kExternalReferenceImagePath = "C:/Users/razor/Downloads/S__46284845.jpg";

juce::Image loadBinaryImage(const char* name)
{
#if VINTAGE_RAWNESS_HAS_BINARY_DATA
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
    if (auto image = loadBinaryImage("faceplate_vintage_rawness_png"); image.isValid())
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

juce::Image cropButtonFromFaceplate(const juce::Image& faceplate, juce::Rectangle<float> bounds)
{
    if (!faceplate.isValid())
        return {};

    return faceplate.getClippedImage(bounds.toNearestInt()).convertedToFormat(juce::Image::ARGB);
}
}

VintageRawnessEditor::VintageRawnessEditor(VintageRawnessProcessor& p)
    : AudioProcessorEditor(&p),
      proc(p),
      dirtAttach(p.apvts, VintageRawnessProcessor::dirtParamId, dirtKnob),
      crushAttach(p.apvts, VintageRawnessProcessor::crushParamId, crushKnob),
      wobbleAttach(p.apvts, VintageRawnessProcessor::wobbleParamId, wobbleKnob)
{
    setSize(static_cast<int>(std::round(VintageRawnessGeneratedLayout::displayWidth)),
            static_cast<int>(std::round(VintageRawnessGeneratedLayout::displayHeight)));

    faceplateImage = loadReferenceFaceplate();

    auto dirtImage = loadBinaryImage("knob_dirt_png");
    if (!dirtImage.isValid())
        dirtImage = cropKnobFromFaceplate(faceplateImage, VintageRawnessGeneratedLayout::dirtBounds());

    auto crushImage = loadBinaryImage("knob_crush_png");
    if (!crushImage.isValid())
        crushImage = cropKnobFromFaceplate(faceplateImage, VintageRawnessGeneratedLayout::crushBounds());

    auto wobbleImage = loadBinaryImage("knob_wobble_png");
    if (!wobbleImage.isValid())
        wobbleImage = cropKnobFromFaceplate(faceplateImage, VintageRawnessGeneratedLayout::wobbleBounds());

    setupKnob(dirtKnob, std::move(dirtImage), VintageRawnessGeneratedLayout::dirtDefault);
    setupKnob(crushKnob, std::move(crushImage), VintageRawnessGeneratedLayout::crushDefault);
    setupKnob(wobbleKnob, std::move(wobbleImage), VintageRawnessGeneratedLayout::wobbleDefault);

    preset1.onClick = [this] { applyPreset(0); };
    preset2.onClick = [this] { applyPreset(1); };
    preset3.onClick = [this] { applyPreset(2); };
    preset4.onClick = [this] { applyPreset(3); };

    preset1.setButtonImage(cropButtonFromFaceplate(faceplateImage, VintageRawnessGeneratedLayout::preset1Bounds()));
    preset2.setButtonImage(cropButtonFromFaceplate(faceplateImage, VintageRawnessGeneratedLayout::preset2Bounds()));
    preset3.setButtonImage(cropButtonFromFaceplate(faceplateImage, VintageRawnessGeneratedLayout::preset3Bounds()));
    preset4.setButtonImage(cropButtonFromFaceplate(faceplateImage, VintageRawnessGeneratedLayout::preset4Bounds()));

    addAndMakeVisible(dirtKnob);
    addAndMakeVisible(crushKnob);
    addAndMakeVisible(wobbleKnob);
    addAndMakeVisible(preset1);
    addAndMakeVisible(preset2);
    addAndMakeVisible(preset3);
    addAndMakeVisible(preset4);

    updatePresetSelection(0);
}

void VintageRawnessEditor::setupKnob(ImageKnobSlider& knob, juce::Image image, float neutralValue)
{
    knob.setOpaque(false);
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters(juce::degreesToRadians(VintageRawnessGeneratedLayout::knobAngleStartDeg),
                             juce::degreesToRadians(VintageRawnessGeneratedLayout::knobAngleEndDeg),
                             true);
    knob.setAngleRange(VintageRawnessGeneratedLayout::knobAngleStartDeg,
                       VintageRawnessGeneratedLayout::knobAngleEndDeg);
    knob.setNeutralValue(neutralValue);
    knob.setKnobImage(std::move(image));
}

void VintageRawnessEditor::resized()
{
    dirtKnob.setBounds(VintageRawnessGeneratedLayout::dirtDisplayBounds().toNearestInt());
    crushKnob.setBounds(VintageRawnessGeneratedLayout::crushDisplayBounds().toNearestInt());
    wobbleKnob.setBounds(VintageRawnessGeneratedLayout::wobbleDisplayBounds().toNearestInt());

    preset1.setBounds(VintageRawnessGeneratedLayout::preset1DisplayBounds().toNearestInt());
    preset2.setBounds(VintageRawnessGeneratedLayout::preset2DisplayBounds().toNearestInt());
    preset3.setBounds(VintageRawnessGeneratedLayout::preset3DisplayBounds().toNearestInt());
    preset4.setBounds(VintageRawnessGeneratedLayout::preset4DisplayBounds().toNearestInt());
}

void VintageRawnessEditor::paint(juce::Graphics& g)
{
    if (faceplateImage.isValid())
    {
        g.drawImage(faceplateImage, getLocalBounds().toFloat());
        return;
    }

    g.fillAll(juce::Colour(0xff11110f));
}

void VintageRawnessEditor::applyPreset(int presetIndex)
{
    if (!juce::isPositiveAndBelow(presetIndex, kPresetCount))
        return;

    const auto& preset = presets[presetIndex];
    setParameterValue(VintageRawnessProcessor::dirtParamId, preset.dirt);
    setParameterValue(VintageRawnessProcessor::crushParamId, preset.crush);
    setParameterValue(VintageRawnessProcessor::wobbleParamId, preset.wobble);
    updatePresetSelection(presetIndex);
}

void VintageRawnessEditor::setParameterValue(const char* parameterId, float value)
{
    if (auto* parameter = proc.apvts.getParameter(parameterId))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
        parameter->endChangeGesture();
    }
}

void VintageRawnessEditor::updatePresetSelection(int presetIndex)
{
    selectedPreset = presetIndex;
    preset1.setSelected(selectedPreset == 0);
    preset2.setSelected(selectedPreset == 1);
    preset3.setSelected(selectedPreset == 2);
    preset4.setSelected(selectedPreset == 3);
}
