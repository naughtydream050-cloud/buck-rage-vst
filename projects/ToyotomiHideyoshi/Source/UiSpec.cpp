#include "UiSpec.h"

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
 #define TOYOTOMI_HAS_BINARY_DATA 1
#else
 #define TOYOTOMI_HAS_BINARY_DATA 0
#endif

namespace
{
juce::var loadEmbeddedUiSpec()
{
#if TOYOTOMI_HAS_BINARY_DATA
    const auto json = juce::String::fromUTF8 (
        reinterpret_cast<const char*> (BinaryData::uispec_json),
        BinaryData::uispec_jsonSize);
    const auto parsed = juce::JSON::parse (json);
    return parsed.isObject() ? parsed : juce::var {};
#else
    return {};
#endif
}
}

UiSpec::UiSpec()
{
    // ui/spec/ui-spec.json is the single source of truth. It is compiled into
    // the VST3, so FL Studio never depends on a sidecar file being present.
    root = loadEmbeddedUiSpec();

    if (auto* rootObject = root.getDynamicObject())
        if (auto* canvas = rootObject->getProperty ("canvas").getDynamicObject())
        {
            canvasWidth = static_cast<int> (canvas->getProperty ("width"));
            canvasHeight = static_cast<int> (canvas->getProperty ("height"));
            valid = canvasWidth > 0 && canvasHeight > 0
                 && rootObject->getProperty ("regions").getDynamicObject() != nullptr;
        }

    // Keep the canonical Phase 1 canvas available if an invalid future asset
    // is packaged. getRegion() supplies safe non-zero fallback regions.
    if (! valid)
    {
        canvasWidth = 1280;
        canvasHeight = 853;
    }
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

    struct Region { const char* id; int x, y, w, h; };
    static constexpr Region regions[] {
        { "topBar", 5, 5, 1270, 79 }, { "artwork", 5, 89, 307, 416 },
        { "barTabs", 316, 98, 514, 36 }, { "barMap", 316, 143, 618, 268 },
        { "presetPalette", 942, 88, 333, 350 }, { "xyPad", 15, 510, 297, 285 },
        { "countGrid", 330, 414, 523, 381 }, { "countParameters", 866, 465, 251, 327 },
        { "outputMeter", 1126, 449, 140, 343 }, { "bottomStatus", 5, 806, 1270, 42 }
    };
    for (const auto& region : regions)
        if (name == region.id) return { region.x, region.y, region.w, region.h };

    return {};
}

juce::Rectangle<int> UiSpec::getControl (const juce::String& name) const
{
    if (auto* rootObject = root.getDynamicObject())
        if (auto* controls = rootObject->getProperty ("controls").getDynamicObject())
            if (auto* control = controls->getProperty (juce::Identifier (name)).getDynamicObject())
                return { static_cast<int> (control->getProperty ("x")),
                         static_cast<int> (control->getProperty ("y")),
                         static_cast<int> (control->getProperty ("w")),
                         static_cast<int> (control->getProperty ("h")) };

    return {};
}

juce::Rectangle<int> UiSpec::getScaledCanvasBounds (juce::Rectangle<int> editorBounds) const
{
    // The editor is aspect-constrained, but retaining one fitted canvas here
    // keeps the faceplate and every component on precisely the same transform.
    const auto scaleX = editorBounds.getWidth() / static_cast<float> (canvasWidth);
    const auto scaleY = editorBounds.getHeight() / static_cast<float> (canvasHeight);
    const auto scale = juce::jmin (scaleX, scaleY);
    const auto width = juce::roundToInt (canvasWidth * scale);
    const auto height = juce::roundToInt (canvasHeight * scale);
    return { editorBounds.getCentreX() - width / 2, editorBounds.getCentreY() - height / 2, width, height };
}

juce::Rectangle<int> UiSpec::scaledBounds (juce::Rectangle<int> referenceBounds,
                                           juce::Rectangle<int> editorBounds) const
{
    const auto canvas = getScaledCanvasBounds (editorBounds);
    const auto scaleX = canvas.getWidth() / static_cast<float> (canvasWidth);
    const auto scaleY = canvas.getHeight() / static_cast<float> (canvasHeight);
    return { canvas.getX() + juce::roundToInt (referenceBounds.getX() * scaleX),
             canvas.getY() + juce::roundToInt (referenceBounds.getY() * scaleY),
             juce::roundToInt (referenceBounds.getWidth() * scaleX),
             juce::roundToInt (referenceBounds.getHeight() * scaleY) };
}

juce::Rectangle<int> UiSpec::scaleRegion (const juce::String& name,
                                          juce::Rectangle<int> viewport) const
{
    return scaledBounds (getRegion (name), viewport);
}
