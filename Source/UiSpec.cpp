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
    int size = 0;
    const auto* data = BinaryData::getNamedResource ("runtime_1024_layout_json", size);
    const auto json = data != nullptr ? juce::String::fromUTF8 (reinterpret_cast<const char*> (data), size)
                                      : juce::String {};
    const auto parsed = juce::JSON::parse (json);
    return parsed.isObject() ? parsed : juce::var {};
#else
    return {};
#endif
}
}

UiSpec::UiSpec()
{
    // The canonical 1024 native layout is compiled into the VST3, so FL Studio
    // never depends on a sidecar file or a 1280 coordinate conversion.
    root = loadEmbeddedUiSpec();

    if (auto* rootObject = root.getDynamicObject())
        if (auto* canvas = rootObject->getProperty ("canvas").getDynamicObject())
        {
            canvasWidth = static_cast<int> (canvas->getProperty ("width"));
            canvasHeight = static_cast<int> (canvas->getProperty ("height"));
            valid = canvasWidth > 0 && canvasHeight > 0
                 && rootObject->getProperty ("components").getDynamicObject() != nullptr;
        }

    // Keep a safe native canvas if a future packaged manifest is invalid.
    if (! valid)
    {
        canvasWidth = 1024;
        canvasHeight = 683;
    }
}

juce::Rectangle<int> UiSpec::getComponent (const juce::String& name) const
{
    if (auto* rootObject = root.getDynamicObject())
        if (auto* components = rootObject->getProperty ("components").getDynamicObject())
            if (auto* component = components->getProperty (juce::Identifier (name)).getDynamicObject())
                return { static_cast<int> (component->getProperty ("x")),
                         static_cast<int> (component->getProperty ("y")),
                         static_cast<int> (component->getProperty ("w")),
                         static_cast<int> (component->getProperty ("h")) };
    return {};
}

namespace RuntimeLayout
{
juce::Rectangle<int> bounds (const juce::String& name)
{
    static const UiSpec runtimeSpec;
    return runtimeSpec.getComponent (name);
}

juce::Rectangle<int> localBounds (const juce::String& child, const juce::String& parent)
{
    const auto parentBounds = bounds (parent);
    return bounds (child).translated (-parentBounds.getX(), -parentBounds.getY());
}
}
