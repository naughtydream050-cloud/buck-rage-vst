#include "UiSpec.h"

UiSpec::UiSpec()
{
    // ui/spec/ui-spec.json is the design-time SSOT. The initial Phase 1
    // skeleton only needs its fixed canvas contract at runtime; this avoids
    // coupling component construction to generated BinaryData symbol names.
    canvasWidth = 1280;
    canvasHeight = 853;
    valid = true;
}

juce::Rectangle<int> UiSpec::getRegion (const juce::String& name) const
{
    if (auto* rootObject = root.getDynamicObject())
        if (auto* regions = rootObject->getProperty ("regions").getDynamicObject())
            if (auto* region = regions->getProperty (juce::Identifier (name)).getDynamicObject())
                return { static_cast<int> (region->getProperty ("x")),
                         static_cast<int> (region->getProperty ("y")),
                         static_cast<int> (region->getProperty ("w")),
                         static_cast<int> (region->getProperty ("h")) };

    return {};
}

juce::Rectangle<int> UiSpec::scaleRegion (const juce::String& name,
                                          juce::Rectangle<int> viewport) const
{
    const auto scale = juce::jmin (viewport.getWidth() / static_cast<float> (canvasWidth),
                                   viewport.getHeight() / static_cast<float> (canvasHeight));
    const auto fittedWidth = juce::roundToInt (canvasWidth * scale);
    const auto fittedHeight = juce::roundToInt (canvasHeight * scale);
    const auto originX = viewport.getX() + (viewport.getWidth() - fittedWidth) / 2;
    const auto originY = viewport.getY() + (viewport.getHeight() - fittedHeight) / 2;
    const auto source = getRegion (name);

    return { originX + juce::roundToInt (source.getX() * scale),
             originY + juce::roundToInt (source.getY() * scale),
             juce::roundToInt (source.getWidth() * scale),
             juce::roundToInt (source.getHeight() * scale) };
}
