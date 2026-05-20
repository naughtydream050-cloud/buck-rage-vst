#pragma once
#include <JuceHeader.h>

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
    static constexpr int kMaxTrackedChannels = 2;
    static constexpr float kOutputCeiling = 0.98f;
    static constexpr float kDcBlockerPole = 0.995f;
    static constexpr float kCrushMinHoldHz = 2600.0f;
    static constexpr float kCrushMaxHoldHz = 26000.0f;
    static constexpr float kWobbleMinRateHz = 0.35f;
    static constexpr float kWobbleMaxRateHz = 4.50f;
    static constexpr float kWobbleMaxBlend = 0.42f;
    static constexpr float kSmoothingSeconds = 0.025f;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static float saturate(float sample, float drive) noexcept;
    static float bitReduce(float sample, float amount) noexcept;
    static float dcBlock(float sample, int channel, float* x1, float* y1) noexcept;

    double currentSampleRate { 44100.0 };
    float sampleHold[kMaxTrackedChannels] { 0.0f, 0.0f };
    int holdCounter[kMaxTrackedChannels] { 0, 0 };
    float wobbleState[kMaxTrackedChannels] { 0.0f, 0.0f };
    float previousSample[kMaxTrackedChannels] { 0.0f, 0.0f };
    float dcX1[kMaxTrackedChannels] { 0.0f, 0.0f };
    float dcY1[kMaxTrackedChannels] { 0.0f, 0.0f };
    uint32_t randomState { 0x6d2b79f5u };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dirtSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> crushSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> wobbleSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VintageRawnessProcessor)
};
