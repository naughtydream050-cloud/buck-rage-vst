#pragma once
#include <JuceHeader.h>

class KrumpWarpEffect final
{
public:
    void prepare(double sampleRate, int maximumBlockSize, int maximumChannels);
    void reset() noexcept;
    void processBlock(juce::AudioBuffer<float>& buffer, float dirt, float crush, float wobble) noexcept;

private:
    static constexpr int kMaxChannels = 2;
    static constexpr float kDefaultSampleRate = 44100.0f;
    static constexpr float kAttackMs = 4.0f;
    static constexpr float kReleaseMs = 120.0f;
    static constexpr float kOutputCeiling = 0.98f;
    static constexpr float kDcBlockPole = 0.995f;
    static constexpr float kDryWetMix = 0.72f;
    static constexpr float kMaxDelayMs = 55.0f;
    static constexpr float kBaseDelayMs = 7.0f;
    static constexpr float kDelaySmooth = 0.018f;
    static constexpr float kPostFilterMinHz = 2600.0f;
    static constexpr float kPostFilterMaxHz = 14500.0f;
    static constexpr float kParamSmoothingSeconds = 0.025f;

    static float sanitize(float value) noexcept;
    static float fastClip(float value) noexcept;
    static float saturate(float sample, float dirt, float envelope) noexcept;
    static float crushSample(float sample, float crush, float envelope, int channel) noexcept;
    static float dcBlock(float sample, int channel, float* x1, float* y1) noexcept;
    static float onePoleCoefficient(float cutoffHz, float sampleRate) noexcept;
    static float nextRandomBipolar(uint32_t& state) noexcept;

    float readWarpedDelay(int channel, float input, float wobble, float envelope) noexcept;

    juce::AudioBuffer<float> delayBuffer;
    int delayBufferSamples { 1 };
    int writeIndex { 0 };
    int activeChannels { 0 };
    float sampleRateHz { kDefaultSampleRate };
    float attackCoeff { 0.0f };
    float releaseCoeff { 0.0f };
    float envelopeState { 0.0f };
    float delayTimeState[kMaxChannels] { 0.0f, 0.0f };
    float heldSample[kMaxChannels] { 0.0f, 0.0f };
    int holdCounter[kMaxChannels] { 0, 0 };
    float postFilterState[kMaxChannels] { 0.0f, 0.0f };
    float dcX1[kMaxChannels] { 0.0f, 0.0f };
    float dcY1[kMaxChannels] { 0.0f, 0.0f };
    float instability[kMaxChannels] { 0.0f, 0.0f };
    uint32_t randomState { 0x5f3759dfu };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dirtSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> crushSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> wobbleSmooth;
};
