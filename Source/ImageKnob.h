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

    void paint(juce::Graphics& g) override
    {
        if (!knobImage.isValid())
            return;

        const auto bounds = getLocalBounds().toFloat();
        const auto centre = bounds.getCentre();
        const auto normalised = static_cast<float>(juce::jlimit(0.0, 1.0, valueToProportionOfLength(getValue())));
        const auto angleDegrees = juce::jmap(normalised, startAngleDegrees, endAngleDegrees);

        const auto imageToBounds = juce::AffineTransform::scale(bounds.getWidth() / static_cast<float>(knobImage.getWidth()),
                                                               bounds.getHeight() / static_cast<float>(knobImage.getHeight()))
                                       .translated(bounds.getX(), bounds.getY());
        const auto rotation = juce::AffineTransform::rotation(juce::degreesToRadians(angleDegrees), centre.x, centre.y);

        g.drawImageTransformed(knobImage, imageToBounds.followedBy(rotation), false);
    }

private:
    juce::Image knobImage;
    float startAngleDegrees { -150.0f };
    float endAngleDegrees { 150.0f };
};
