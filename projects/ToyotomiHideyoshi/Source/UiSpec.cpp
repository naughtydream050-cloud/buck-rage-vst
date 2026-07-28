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
        reinterpret_cast<const char*> (BinaryData::ui_spec_json),
        BinaryData::ui_spec_jsonSize);
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

    // Runtime fallback mirrors ui/spec/ui-spec.json. The editor must remain
    // visible even when the design-time JSON is not embedded as BinaryData.
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
