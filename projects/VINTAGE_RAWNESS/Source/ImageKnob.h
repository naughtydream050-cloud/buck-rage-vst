#pragma once
#include <JuceHeader.h>

class ImageKnobSlider final : public juce::Slider
{
public:
    ImageKnobSlider()
    {
        setOpaque(false);
    }

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

    void setNeutralValue(float value) noexcept
    {
        neutralValue = juce::jlimit(0.0f, 1.0f, value);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        if (!knobImage.isValid())
            return;

        const auto bounds = getLocalBounds().toFloat();
        const auto side = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const auto square = juce::Rectangle<float>(side, side).withCentre(bounds.getCentre());
        const auto centre = square.getCentre();
        const auto normalised = static_cast<float>(juce::jlimit(0.0, 1.0, valueToProportionOfLength(getValue())));
        const auto angleDegrees = valueToAngle(normalised) - valueToAngle(neutralValue);

        const auto imageToBounds = juce::AffineTransform::scale(square.getWidth() / static_cast<float>(knobImage.getWidth()),
                                                               square.getHeight() / static_cast<float>(knobImage.getHeight()))
                                       .translated(square.getX(), square.getY());
        const auto rotation = juce::AffineTransform::rotation(juce::degreesToRadians(angleDegrees), centre.x, centre.y);

        g.drawImageTransformed(knobImage, imageToBounds.followedBy(rotation), false);
    }

private:
    float valueToAngle(float value) const noexcept
    {
        return juce::jmap(value, startAngleDegrees, endAngleDegrees);
    }

    juce::Image knobImage;
    float startAngleDegrees { -150.0f };
    float endAngleDegrees { 150.0f };
    float neutralValue { 0.5f };
};
