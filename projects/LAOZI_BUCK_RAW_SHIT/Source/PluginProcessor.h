#pragma once
#include <JuceHeader.h>
#include <atomic>

class LaoziBuckRawShitProcessor final : public juce::AudioProcessor
{
public:
    LaoziBuckRawShitProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "老子-BUCK RAW SHIT-"; }
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
    std::atomic<float> leftPeak { 0.0f };
    std::atomic<float> rightPeak { 0.0f };

    static constexpr auto pressureParamId = "pressure";
    static constexpr auto kickParamId = "kick";
    static constexpr auto auraParamId = "aura";
    static constexpr auto glueParamId = "glue";
    static constexpr auto outputParamId = "output";
    static constexpr auto bypassParamId = "bypass";
    static constexpr auto oversampleParamId = "oversample";
    static constexpr auto presetIndexParamId = "presetIndex";

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static void publishPeak(std::atomic<float>& destination, float value) noexcept;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LaoziBuckRawShitProcessor)
};
