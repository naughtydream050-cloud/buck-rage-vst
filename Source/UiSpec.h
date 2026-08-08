#pragma once

#include <JuceHeader.h>

class UiSpec final
{
public:
    UiSpec();

    bool isValid() const noexcept { return valid; }
    juce::Rectangle<int> getRegion (const juce::String& name) const;
    juce::Rectangle<int> getControl (const juce::String& name) const;
    juce::Rectangle<int> scaledBounds (juce::Rectangle<int> referenceBounds,
                                      juce::Rectangle<int> editorBounds) const;
    juce::Rectangle<int> getScaledCanvasBounds (juce::Rectangle<int> editorBounds) const;
    juce::Rectangle<int> scaleRegion (const juce::String& name,
                                      juce::Rectangle<int> viewport) const;
    int getCanvasWidth() const noexcept { return canvasWidth; }
    int getCanvasHeight() const noexcept { return canvasHeight; }

private:
    juce::var root;
    bool valid = false;
    int canvasWidth = 1280;
    int canvasHeight = 853;
};
