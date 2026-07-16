#pragma once
#include <JuceHeader.h>
#include "ImageKnob.h"
#include "PluginProcessor.h"
#include "StereoMeter.h"

class LaoziBuckRawShitEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LaoziBuckRawShitEditor(LaoziBuckRawShitProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Preset { const char* name; float pressure; float kick; float aura; float glue; };
    static constexpr Preset presets[5] {
        { "BUCK MASTER", 0.50f, 0.55f, 0.45f, 0.50f },
        { "MORF FORCE", 0.70f, 0.85f, 0.65f, 0.55f },
        { "GRAND KICK", 0.55f, 1.00f, 0.40f, 0.45f },
        { "DARK CEREMONY", 0.45f, 0.60f, 0.90f, 0.65f },
        { "RAW SHIT", 0.90f, 0.80f, 0.55f, 0.75f }
    };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    static juce::Image loadImage(const char* name);
    void setupKnob(ImageKnob&, juce::Image, float defaultValue);
    void applyPreset(int index);
    void setFloatParameter(const char* id, float value);
    void setChoiceParameter(const char* id, int index, int count);
    void timerCallback() override;

    LaoziBuckRawShitProcessor& processor;
    juce::Image faceplate;
    ImageKnob pressureKnob, kickKnob, auraKnob, glueKnob, outputKnob;
    StereoMeter meter;
    juce::ComboBox presetSelector;
    juce::TextButton previousPreset { "<" }, nextPreset { ">" }, oversampleButton, bypassButton { "BYPASS" };
    SliderAttachment pressureAttachment, kickAttachment, auraAttachment, glueAttachment, outputAttachment;
    ComboBoxAttachment presetAttachment;
    ButtonAttachment bypassAttachment;
    int displayedOversample { -1 };
};
