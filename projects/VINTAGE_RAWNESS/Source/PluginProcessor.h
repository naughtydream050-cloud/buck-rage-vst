#pragma once
#include <JuceHeader.h>
#include "KrumpWarpEffect.h"

class VintageRawnessProcessor final : public juce::AudioProcessor
{
public:
    VintageRawnessProcessor();
    ~VintageRawnessProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VINTAGE RAWNESS"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    static constexpr auto dirtParamId = "dirt";
    static constexpr auto crushParamId = "crush";
    static constexpr auto wobbleParamId = "wobble";

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    KrumpWarpEffect krumpWarp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VintageRawnessProcessor)
};
