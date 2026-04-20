#pragma once
#include <JuceHeader.h>

class BuckRageProcessor : public juce::AudioProcessor
{
public:
    BuckRageProcessor();
    ~BuckRageProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BUCK RAGE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& d) override;
    void setStateInformation(const void* d, int s) override;

    juce::AudioProcessorValueTreeState apvts;

    static float asymSat(float x, float drive) noexcept {
        return x >= 0.f ? std::tanh(drive * x) : std::tanh(0.65f * drive * x);
    }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    float lfoPhase { 0.f };
    float lpfZ[2] { 0.f, 0.f };
    double sampleRate_ { 44100.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BuckRageProcessor)
};
