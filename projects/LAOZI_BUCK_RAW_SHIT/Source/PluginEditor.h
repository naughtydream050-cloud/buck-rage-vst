#pragma once
#include <JuceHeader.h>
#include "ImageKnob.h"
#include "PluginProcessor.h"
#include "PresetLibrary.h"
#include "StereoMeter.h"

class UiHitTargetButton final : public juce::Button
{
public:
    UiHitTargetButton() : juce::Button("") { setOpaque(false); setWantsKeyboardFocus(false); }
    void paintButton(juce::Graphics&, bool, bool) override {}
};

class LaoziBuckRawShitEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LaoziBuckRawShitEditor(LaoziBuckRawShitProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    static juce::Image loadImage(const char* name);
    void setupKnob(ImageKnob&, juce::Image, float defaultValue);
    void applyPreset(int index);
    void markPresetCustom();
    void setFloatParameter(const char* id, float value);
    void setChoiceParameter(const char* id, int index, int count);
    int choiceIndex(const char* id, int count) const noexcept;
    static constexpr int presetChoiceCount = static_cast<int>(LaoziPresetLibrary::presets.size()) + 1;
    static constexpr int customPresetIndex = static_cast<int>(LaoziPresetLibrary::presets.size());
    void timerCallback() override;

    LaoziBuckRawShitProcessor& processor;
    juce::Image faceplate;
    ImageKnob pressureKnob, kickKnob, auraKnob, glueKnob, outputKnob;
    StereoMeter meter;
    UiHitTargetButton presetHit, previousPreset, nextPreset, oversampleButton, bypassButton;
    SliderAttachment pressureAttachment, kickAttachment, auraAttachment, glueAttachment, outputAttachment;
    ButtonAttachment bypassAttachment;
    int displayedOversample { -1 };
    int displayedPreset { -1 };
};
