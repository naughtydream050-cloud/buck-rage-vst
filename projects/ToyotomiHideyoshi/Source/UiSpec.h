#pragma once

#include <JuceHeader.h>

class UiSpec final
{
public:
    UiSpec();

    bool isValid() const noexcept { return valid; }
    juce::Rectangle<int> getRegion (const juce::String& name) const;
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

