#pragma once
#include <JuceHeader.h>

class ImageKnobSlider final : public juce::Slider
{
public:
    void setKnobImage(juce::Image imageToUse)
    {
        knobImage = std::move(imageToUse);
        repaint();
    }

    void setAngleRange(float startDegrees, float endDegrees)
    {
        startAngleDegrees = startDegrees;
        endAngleDegrees = endDegrees;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        if (!knobImage.isValid())
            return;

        const auto bounds = getLocalBounds().toFloat();
        const auto centre = bounds.getCentre();
        const auto normalised = static_cast<float>(juce::jlimit(0.0, 1.0, valueToProportionOfLength(getValue())));
        const auto angleDegrees = juce::jmap(normalised, startAngleDegrees, endAngleDegrees);

        juce::Graphics::ScopedSaveState state(g);
        g.addTransform(juce::AffineTransform::rotation(
            juce::degreesToRadians(angleDegrees), centre.x, centre.y));
        g.drawImage(knobImage, bounds);
    }

private:
    juce::Image knobImage;
    float startAngleDegrees { -150.0f };
    float endAngleDegrees { 150.0f };
};
