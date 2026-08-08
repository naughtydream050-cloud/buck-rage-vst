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
        { "barTabs", 317, 99, 542, 34 }, { "barMap", 316, 143, 608, 266 },
        { "presetPalette", 942, 87, 333, 350 }, { "xyPad", 18, 520, 289, 249 },
        { "countGrid", 332, 432, 512, 360 }, { "countParameters", 866, 465, 251, 327 },
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
    juce::ignoreUnused (editorBounds);
    return { 0, 0, canvasWidth, canvasHeight };
}

juce::Rectangle<int> UiSpec::scaledBounds (juce::Rectangle<int> referenceBounds,
                                           juce::Rectangle<int> editorBounds) const
{
    juce::ignoreUnused (editorBounds);
    return referenceBounds;
}

juce::Rectangle<int> UiSpec::scaleRegion (const juce::String& name,
                                          juce::Rectangle<int> viewport) const
{
    return scaledBounds (getRegion (name), viewport);
}
