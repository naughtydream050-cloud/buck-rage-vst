#pragma once
#include <JuceHeader.h>

class ImageKnob final : public juce::Slider
{
public:
    ImageKnob();
    void setImage(juce::Image imageToUse);
    void setAngleRange(float startDegrees, float endDegrees);
    void setDefaultNormalised(float value) noexcept;
    void paint(juce::Graphics&) override;

private:
    juce::Image knobImage;
    float startAngleDegrees { -150.0f };
    float endAngleDegrees { 150.0f };
    float defaultNormalised { 0.5f };
};
