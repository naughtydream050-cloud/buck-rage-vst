#include "UiSpec.h"

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
 #define TOYOTOMI_HAS_BINARY_DATA 1
#else
 #define TOYOTOMI_HAS_BINARY_DATA 0
#endif

UiSpec::UiSpec()
{
#if TOYOTOMI_HAS_BINARY_DATA
    const auto json = juce::String::fromUTF8 (BinaryData::ui_spec_json,
                                              BinaryData::ui_spec_jsonSize);
    root = juce::JSON::parse (json);

    if (auto* rootObject = root.getDynamicObject())
    {
        if (auto* canvas = rootObject->getProperty ("canvas").getDynamicObject())
        {
            canvasWidth = static_cast<int> (canvas->getProperty ("width"));
            canvasHeight = static_cast<int> (canvas->getProperty ("height"));
            valid = canvasWidth > 0 && canvasHeight > 0;
        }
    }
#endif
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
