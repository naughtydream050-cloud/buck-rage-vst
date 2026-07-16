#include "PluginEditor.h"
#include "GeneratedLayout.h"

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
#endif

juce::Image LaoziBuckRawShitEditor::loadImage(const char* name)
{
    int size = 0;
    if (auto* data = BinaryData::getNamedResource(name, size))
        return juce::ImageCache::getFromMemory(data, size);
    return {};
}

LaoziBuckRawShitEditor::LaoziBuckRawShitEditor(LaoziBuckRawShitProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      processor(processorToUse),
      meter(processor.leftPeak, processor.rightPeak),
      pressureAttachment(processor.apvts, LaoziBuckRawShitProcessor::pressureParamId, pressureKnob),
      kickAttachment(processor.apvts, LaoziBuckRawShitProcessor::kickParamId, kickKnob),
      auraAttachment(processor.apvts, LaoziBuckRawShitProcessor::auraParamId, auraKnob),
      glueAttachment(processor.apvts, LaoziBuckRawShitProcessor::glueParamId, glueKnob),
      outputAttachment(processor.apvts, LaoziBuckRawShitProcessor::outputParamId, outputKnob),
      presetAttachment(processor.apvts, LaoziBuckRawShitProcessor::presetIndexParamId, presetSelector),
      bypassAttachment(processor.apvts, LaoziBuckRawShitProcessor::bypassParamId, bypassButton)
{
    faceplate = loadImage("faceplate_laozi_buck_raw_shit_png");
    setupKnob(pressureKnob, loadImage("knob_pressure_png"), 0.50f);
    setupKnob(kickKnob, loadImage("knob_kick_png"), 0.55f);
    setupKnob(auraKnob, loadImage("knob_aura_png"), 0.45f);
    setupKnob(glueKnob, loadImage("knob_glue_png"), 0.50f);
    setupKnob(outputKnob, loadImage("knob_output_png"), 2.0f / 3.0f);

    for (int index = 0; index < 5; ++index)
        presetSelector.addItem(presets[index].name, index + 1);
    presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colours::black.withAlpha(0.20f));
    presetSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffd0c8bf));
    presetSelector.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    presetSelector.onChange = [this] { applyPreset(presetSelector.getSelectedItemIndex()); };

    const auto styleButton = [] (juce::TextButton& button)
    {
        button.setColour(juce::TextButton::buttonColourId, juce::Colours::black.withAlpha(0.08f));
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff9d3c4f).withAlpha(0.45f));
        button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd0c8bf));
        button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    };
    styleButton(previousPreset); styleButton(nextPreset); styleButton(oversampleButton); styleButton(bypassButton);
    previousPreset.onClick = [this] { setChoiceParameter(LaoziBuckRawShitProcessor::presetIndexParamId, (presetSelector.getSelectedItemIndex() + 4) % 5, 5); };
    nextPreset.onClick = [this] { setChoiceParameter(LaoziBuckRawShitProcessor::presetIndexParamId, (presetSelector.getSelectedItemIndex() + 1) % 5, 5); };
    oversampleButton.onClick = [this]
    {
        const auto current = static_cast<int>(processor.apvts.getRawParameterValue(LaoziBuckRawShitProcessor::oversampleParamId)->load());
        setChoiceParameter(LaoziBuckRawShitProcessor::oversampleParamId, (current + 1) % 3, 3);
    };

    addAndMakeVisible(pressureKnob); addAndMakeVisible(kickKnob); addAndMakeVisible(auraKnob); addAndMakeVisible(glueKnob); addAndMakeVisible(outputKnob);
    addAndMakeVisible(meter); addAndMakeVisible(presetSelector); addAndMakeVisible(previousPreset); addAndMakeVisible(nextPreset);
    addAndMakeVisible(oversampleButton); addAndMakeVisible(bypassButton);
    setSize(LaoziLayout::displayWidth, LaoziLayout::displayHeight);
    startTimerHz(15);
}

void LaoziBuckRawShitEditor::setupKnob(ImageKnob& knob, juce::Image image, float defaultValue)
{
    knob.setImage(std::move(image));
    knob.setAngleRange(LaoziLayout::knobStartDegrees, LaoziLayout::knobEndDegrees);
    knob.setDefaultNormalised(defaultValue);
}

void LaoziBuckRawShitEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    if (faceplate.isValid()) g.drawImage(faceplate, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit, false);
}

void LaoziBuckRawShitEditor::resized()
{
    pressureKnob.setBounds(LaoziLayout::pressureBounds()); kickKnob.setBounds(LaoziLayout::kickBounds());
    auraKnob.setBounds(LaoziLayout::auraBounds()); glueKnob.setBounds(LaoziLayout::glueBounds()); outputKnob.setBounds(LaoziLayout::outputBounds());
    presetSelector.setBounds(LaoziLayout::presetBounds()); previousPreset.setBounds(LaoziLayout::previousPresetBounds()); nextPreset.setBounds(LaoziLayout::nextPresetBounds());
    meter.setBounds(LaoziLayout::meterBounds()); oversampleButton.setBounds(LaoziLayout::oversampleBounds()); bypassButton.setBounds(LaoziLayout::bypassBounds());
}

void LaoziBuckRawShitEditor::setFloatParameter(const char* id, float value)
{
    if (auto* parameter = processor.apvts.getParameter(id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    }
}

void LaoziBuckRawShitEditor::setChoiceParameter(const char* id, int index, int count)
{
    if (auto* parameter = processor.apvts.getParameter(id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(static_cast<float>(index) / static_cast<float>(count - 1));
        parameter->endChangeGesture();
    }
}

void LaoziBuckRawShitEditor::applyPreset(int index)
{
    if (! juce::isPositiveAndBelow(index, 5)) return;
    const auto& preset = presets[index];
    setFloatParameter(LaoziBuckRawShitProcessor::pressureParamId, preset.pressure);
    setFloatParameter(LaoziBuckRawShitProcessor::kickParamId, preset.kick);
    setFloatParameter(LaoziBuckRawShitProcessor::auraParamId, preset.aura);
    setFloatParameter(LaoziBuckRawShitProcessor::glueParamId, preset.glue);
}

void LaoziBuckRawShitEditor::timerCallback()
{
    const auto oversample = static_cast<int>(processor.apvts.getRawParameterValue(LaoziBuckRawShitProcessor::oversampleParamId)->load());
    if (oversample != displayedOversample)
    {
        displayedOversample = oversample;
        oversampleButton.setButtonText(oversample == 0 ? "OVERSAMPLE\nOFF" : (oversample == 1 ? "OVERSAMPLE\n2x" : "OVERSAMPLE\n4x"));
    }
}
