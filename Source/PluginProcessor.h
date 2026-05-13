#pragma once
#include <JuceHeader.h>

class RudeHypeProcessor : public juce::AudioProcessor
{
public:
    RudeHypeProcessor();
    ~RudeHypeProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "RUDE HYPE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(const int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& d) override;
    void setStateInformation(const void* d, int s) override;

    juce::AudioProcessorValueTreeState apvts;

    static float asymSat(float x, float drive) noexcept
    {
        return x >= 0.0f ? std::tanh(drive * x) : std::tanh(0.65f * drive * x);
    }

private:
    static constexpr int kStereoChannels = 2;
    static constexpr float kReferenceSampleRate = 44100.0f;
    static constexpr float kDcBlockerPole = 0.995f;
    static constexpr float kOutputCeiling = 0.98f;
    static constexpr float kShoutFocusLowHz = 1800.0f;
    static constexpr float kShoutFocusHighHz = 6800.0f;
    static constexpr float kBurnLowSplitHz = 160.0f;
    static constexpr float kBurnFizzLowHz = 6200.0f;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static float onePoleCoefficient(float hz, double sampleRate) noexcept;
    static float softClip(float x) noexcept;
    static float transistorSat(float x, float drive) noexcept;
    static float wavefold(float x, float amount) noexcept;

    float lfoPhase { 0.0f };
    float shoutLowZ[kStereoChannels] { 0.0f, 0.0f };
    float shoutHighZ[kStereoChannels] { 0.0f, 0.0f };
    float burnLowZ[kStereoChannels] { 0.0f, 0.0f };
    float fizzLowZ[kStereoChannels] { 0.0f, 0.0f };
    float dcX1[kStereoChannels] { 0.0f, 0.0f };
    float dcY1[kStereoChannels] { 0.0f, 0.0f };
    float prevSat[kStereoChannels] { 0.0f, 0.0f };
    float tapeEnv { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> shoutSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> burnSmooth;
    double sampleRate_ { 44100.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RudeHypeProcessor)
};
