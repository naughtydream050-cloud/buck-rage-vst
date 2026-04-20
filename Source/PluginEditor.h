#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "KnobLookAndFeel.h"

class BuckRageEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit BuckRageEditor(BuckRageProcessor&);
    ~BuckRageEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override {}

    BuckRageProcessor& proc;
    SilverKnobLAF knobLAF;

    juce::Slider buckKnob, rageKnob;
    using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
    Attach buckAttach, rageAttach;

    juce::Image bgImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BuckRageEditor)
};
