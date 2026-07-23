#include "PluginEditor.h"
#include "GeneratedLayout.h"

#include <cmath>

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
      bypassAttachment(processor.apvts, LaoziBuckRawShitProcessor::bypassParamId, bypassButton)
{
    faceplate = loadImage("faceplate_laozi_buck_raw_shit_png");
    setupKnob(pressureKnob, loadImage("knob_pressure_png"), 0.50f);
    setupKnob(kickKnob, loadImage("knob_kick_png"), 0.55f);
    setupKnob(auraKnob, loadImage("knob_aura_png"), 0.45f);
    setupKnob(glueKnob, loadImage("knob_glue_png"), 0.50f);
    setupKnob(outputKnob, loadImage("knob_output_png"), 2.0f / 3.0f);
    for (auto* knob : { &pressureKnob, &kickKnob, &auraKnob, &glueKnob, &outputKnob })
        knob->onUserEdit = [this] { markPresetCustom(); };

    bypassButton.setClickingTogglesState(true);
    presetHit.onClick = [this]
    {
        juce::PopupMenu menu;
        for (int index = 0; index < static_cast<int>(LaoziPresetLibrary::presets.size()); ++index) menu.addItem(index + 1, LaoziPresetLibrary::presets[static_cast<size_t>(index)].name);
        auto safeThis = juce::Component::SafePointer<LaoziBuckRawShitEditor>(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetHit), [safeThis] (int result)
        {
            if (safeThis != nullptr && result > 0)
            {
                safeThis->setChoiceParameter(LaoziBuckRawShitProcessor::presetIndexParamId, result - 1, presetChoiceCount);
                safeThis->applyPreset(result - 1);
            }
        });
    };
    previousPreset.onClick = [this]
    {
        constexpr auto presetCount = static_cast<int>(LaoziPresetLibrary::presets.size());
        const auto current = choiceIndex(LaoziBuckRawShitProcessor::presetIndexParamId, presetChoiceCount);
        const auto next = current == customPresetIndex ? presetCount - 1 : (current + presetCount - 1) % presetCount;
        setChoiceParameter(LaoziBuckRawShitProcessor::presetIndexParamId, next, presetChoiceCount); applyPreset(next);
    };
    nextPreset.onClick = [this]
    {
        constexpr auto presetCount = static_cast<int>(LaoziPresetLibrary::presets.size());
        const auto current = choiceIndex(LaoziBuckRawShitProcessor::presetIndexParamId, presetChoiceCount);
        const auto next = current == customPresetIndex ? 0 : (current + 1) % presetCount;
        setChoiceParameter(LaoziBuckRawShitProcessor::presetIndexParamId, next, presetChoiceCount); applyPreset(next);
    };
    oversampleButton.onClick = [this]
    {
        const auto current = choiceIndex(LaoziBuckRawShitProcessor::oversampleParamId, 3);
        setChoiceParameter(LaoziBuckRawShitProcessor::oversampleParamId, (current + 1) % 3, 3);
    };

    addAndMakeVisible(pressureKnob); addAndMakeVisible(kickKnob); addAndMakeVisible(auraKnob); addAndMakeVisible(glueKnob); addAndMakeVisible(outputKnob);
    addAndMakeVisible(meter); addAndMakeVisible(presetHit); addAndMakeVisible(previousPreset); addAndMakeVisible(nextPreset);
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
    const auto presetIndex = choiceIndex(LaoziBuckRawShitProcessor::presetIndexParamId, presetChoiceCount);
    g.setColour(juce::Colour(0xffc9c0b7));
    g.setFont(juce::Font(6.8f, juce::Font::plain));
    const auto presetName = presetIndex == customPresetIndex ? "CUSTOM" : LaoziPresetLibrary::presets[static_cast<size_t>(presetIndex)].name;
    g.drawFittedText(presetName, LaoziLayout::presetTextBounds(), juce::Justification::centred, 1);

    const auto oversampleIndex = choiceIndex(LaoziBuckRawShitProcessor::oversampleParamId, 3);
    const char* oversampleText[] { "OFF", "2x", "4x" };
    g.setColour(juce::Colour(0xffc3616c));
    g.setFont(juce::Font(6.2f, juce::Font::plain));
    g.drawFittedText(oversampleText[oversampleIndex], LaoziLayout::oversampleStatusBounds(), juce::Justification::centred, 1);

    if (processor.apvts.getRawParameterValue(LaoziBuckRawShitProcessor::bypassParamId)->load() > 0.5f)
    {
        g.setColour(juce::Colour(0xffd85b69));
        g.fillEllipse(LaoziLayout::bypassIndicatorBounds().toFloat().reduced(2.0f));
    }
}

void LaoziBuckRawShitEditor::resized()
{
    pressureKnob.setBounds(LaoziLayout::pressureBounds()); kickKnob.setBounds(LaoziLayout::kickBounds());
    auraKnob.setBounds(LaoziLayout::auraBounds()); glueKnob.setBounds(LaoziLayout::glueBounds()); outputKnob.setBounds(LaoziLayout::outputBounds());
    presetHit.setBounds(LaoziLayout::presetBounds()); previousPreset.setBounds(LaoziLayout::previousPresetBounds()); nextPreset.setBounds(LaoziLayout::nextPresetBounds());
    meter.setBounds(LaoziLayout::meterBounds()); oversampleButton.setBounds(LaoziLayout::oversampleBounds()); bypassButton.setBounds(LaoziLayout::bypassBounds());
}

int LaoziBuckRawShitEditor::choiceIndex(const char* id, int count) const noexcept
{
    // APVTS raw values are denormalised. For AudioParameterChoice this is the
    // actual discrete item index, not a 0..1 proportion.
    const auto rawIndex = processor.apvts.getRawParameterValue(id)->load();
    return juce::jlimit(0, count - 1, static_cast<int>(std::lround(rawIndex)));
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
    if (! juce::isPositiveAndBelow(index, static_cast<int>(LaoziPresetLibrary::presets.size()))) return;
    const auto& preset = LaoziPresetLibrary::presets[static_cast<size_t>(index)];
    setFloatParameter(LaoziBuckRawShitProcessor::pressureParamId, preset.pressure);
    setFloatParameter(LaoziBuckRawShitProcessor::kickParamId, preset.kick);
    setFloatParameter(LaoziBuckRawShitProcessor::auraParamId, preset.aura);
    setFloatParameter(LaoziBuckRawShitProcessor::glueParamId, preset.glue);
    setFloatParameter(LaoziBuckRawShitProcessor::outputParamId, preset.outputDb);
}

void LaoziBuckRawShitEditor::markPresetCustom()
{
    if (choiceIndex(LaoziBuckRawShitProcessor::presetIndexParamId, presetChoiceCount) != customPresetIndex)
        setChoiceParameter(LaoziBuckRawShitProcessor::presetIndexParamId, customPresetIndex, presetChoiceCount);
}

void LaoziBuckRawShitEditor::timerCallback()
{
    const auto oversample = choiceIndex(LaoziBuckRawShitProcessor::oversampleParamId, 3);
    const auto preset = choiceIndex(LaoziBuckRawShitProcessor::presetIndexParamId, presetChoiceCount);
    if (oversample != displayedOversample || preset != displayedPreset)
    {
        // Editor recreation must never overwrite user-adjusted APVTS values.
        // Preset values are applied only by the explicit menu/previous/next actions.
        displayedOversample = oversample;
        displayedPreset = preset;
        repaint();
    }
}
