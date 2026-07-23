#include "ImageKnob.h"

ImageKnob::ImageKnob()
{
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setOpaque(false);
    setMouseClickGrabsKeyboardFocus(false);
}

void ImageKnob::setImage(juce::Image imageToUse)
{
    knobImage = std::move(imageToUse);
    repaint();
}

void ImageKnob::setAngleRange(float startDegrees, float endDegrees)
{
    startAngleDegrees = startDegrees;
    endAngleDegrees = endDegrees;
    repaint();
}

void ImageKnob::setDefaultNormalised(float value) noexcept
{
    defaultNormalised = juce::jlimit(0.0f, 1.0f, value);
    setDoubleClickReturnValue(true, proportionOfLengthToValue(defaultNormalised));
}

void ImageKnob::mouseDown(const juce::MouseEvent& event)
{
    if (onUserEdit) onUserEdit();
    juce::Slider::mouseDown(event);
}

void ImageKnob::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (onUserEdit) onUserEdit();
    juce::Slider::mouseWheelMove(event, wheel);
}

void ImageKnob::paint(juce::Graphics& g)
{
    if (! knobImage.isValid())
        return;

    const auto bounds = getLocalBounds().toFloat();
    const auto side = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto square = juce::Rectangle<float>(side, side).withCentre(bounds.getCentre());
    const auto normalised = static_cast<float>(juce::jlimit(0.0, 1.0, valueToProportionOfLength(getValue())));
    const auto angle = juce::jmap(normalised, startAngleDegrees, endAngleDegrees)
                     - juce::jmap(defaultNormalised, startAngleDegrees, endAngleDegrees);
    const auto scale = juce::AffineTransform::scale(square.getWidth() / static_cast<float>(knobImage.getWidth()),
                                                     square.getHeight() / static_cast<float>(knobImage.getHeight()))
                           .translated(square.getX(), square.getY());
    const auto rotation = juce::AffineTransform::rotation(juce::degreesToRadians(angle), square.getCentreX(), square.getCentreY());
    g.drawImageTransformed(knobImage, scale.followedBy(rotation), false);
}
