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
    static constexpr float kDryWetMix = 0.78f;
    static constexpr float kMaxDelayMs = 145.0f;
    static constexpr float kBaseDelayMs = 18.0f;
    static constexpr float kDelaySmooth = 0.045f;
    static constexpr float kPreEqLowHz = 180.0f;
    static constexpr float kPreEqMidHz = 340.0f;
    static constexpr float kPostHighPassMinHz = 150.0f;
    static constexpr float kPostHighPassMaxHz = 250.0f;
    static constexpr float kPostLowPassMinHz = 3000.0f;
    static constexpr float kPostLowPassMaxHz = 5000.0f;
    static constexpr float kWowMinHz = 0.5f;
    static constexpr float kWowMaxHz = 2.0f;
    static constexpr float kFlutterMinHz = 8.0f;
    static constexpr float kFlutterMaxHz = 25.0f;
    static constexpr float kJitterMinMs = 2.0f;
    static constexpr float kJitterMaxMs = 8.0f;
    static constexpr float kParamSmoothingSeconds = 0.025f;

    static float sanitize(float value) noexcept;
    static float fastClip(float value) noexcept;
    float preEq(float sample, float dirt, int channel) noexcept;
    static float saturate(float sample, float dirt, float envelope) noexcept;
    float crushSample(float sample, float crush, float envelope, int channel) noexcept;
    static float dcBlock(float sample, int channel, float* x1, float* y1) noexcept;
    static float onePoleCoefficient(float cutoffHz, float sampleRate) noexcept;
    static float nextRandomBipolar(uint32_t& state) noexcept;

    float readModulatedDelay(int channel, float input, float wobble, float envelope) noexcept;
    float postBandpass(float sample, float dirt, float crush, int channel) noexcept;

    juce::AudioBuffer<float> delayBuffer;
    int delayBufferSamples { 1 };
    int writeIndex { 0 };
    int activeChannels { 0 };
    float sampleRateHz { kDefaultSampleRate };
    float attackCoeff { 0.0f };
    float releaseCoeff { 0.0f };
    float envelopeState { 0.0f };
    float delayTimeState[kMaxChannels] { 0.0f, 0.0f };
    float wowNoise[kMaxChannels] { 0.0f, 0.0f };
    float flutterNoise[kMaxChannels] { 0.0f, 0.0f };
    float slopNoise[kMaxChannels] { 0.0f, 0.0f };
    float heldSample[kMaxChannels] { 0.0f, 0.0f };
    int holdCounter[kMaxChannels] { 0, 0 };
    float preLowState[kMaxChannels] { 0.0f, 0.0f };
    float preMidState[kMaxChannels] { 0.0f, 0.0f };
    float postHighPassLowState[kMaxChannels] { 0.0f, 0.0f };
    float postLowPass1[kMaxChannels] { 0.0f, 0.0f };
    float postLowPass2[kMaxChannels] { 0.0f, 0.0f };
    float postLowPass3[kMaxChannels] { 0.0f, 0.0f };
    float postLowPass4[kMaxChannels] { 0.0f, 0.0f };
    float dcX1[kMaxChannels] { 0.0f, 0.0f };
    float dcY1[kMaxChannels] { 0.0f, 0.0f };
    uint32_t randomState { 0x5f3759dfu };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dirtSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> crushSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> wobbleSmooth;
};
