#pragma once

#include <JuceHeader.h>

class UiSpec final
{
public:
    UiSpec();

    bool isValid() const noexcept { return valid; }
    juce::Rectangle<int> getComponent (const juce::String& name) const;
    int getCanvasWidth() const noexcept { return canvasWidth; }
    int getCanvasHeight() const noexcept { return canvasHeight; }

private:
    juce::var root;
    bool valid = false;
    int canvasWidth = 1024;
    int canvasHeight = 683;
};

// Runtime UI geometry is compiled from ui/spec/runtime-1024-layout.json.
// Components use these final integer rectangles directly; this helper never
// scales mouse positions or image bounds at runtime.
namespace RuntimeLayout
{
juce::Rectangle<int> bounds (const juce::String& name);
juce::Rectangle<int> localBounds (const juce::String& child, const juce::String& parent);
}
