#pragma once
#include <JuceHeader.h>
#include <atomic>

class StereoMeter final : public juce::Component, private juce::Timer
{
public:
    StereoMeter(std::atomic<float>& leftPeakSource, std::atomic<float>& rightPeakSource);
    void paint(juce::Graphics&) override;

private:
    void timerCallback() override;
    static float toDb(float linear) noexcept;

    std::atomic<float>& leftPeak;
    std::atomic<float>& rightPeak;
    float displayedLeftDb { -60.0f };
    float displayedRightDb { -60.0f };
};
