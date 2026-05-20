#pragma once
#include <JuceHeader.h>
#include "ImageKnob.h"
#include "PluginProcessor.h"

class RudeHypeEditor final : public juce::AudioProcessorEditor
{
public:
    explicit RudeHypeEditor(RudeHypeProcessor&);
    ~RudeHypeEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    RudeHypeProcessor& proc;

    ImageKnobSlider shoutKnob;
    ImageKnobSlider burnKnob;

    using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
    Attach shoutAttach;
    Attach burnAttach;

    juce::Image faceplateImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RudeHypeEditor)
};
