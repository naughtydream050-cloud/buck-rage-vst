#pragma once

#include <JuceHeader.h>
#include <array>

// The only coordinates below are measured from the 1280 x 853 visual SSOT.
// Runtime rectangles are derived by scaling their edges once to 1024 x 683.
struct ParameterLayout final
{
    static juce::Rectangle<int> toRuntime (juce::Rectangle<int> master)
    {
        constexpr float sx = 1024.0f / 1280.0f;
        constexpr float sy = 683.0f / 853.0f;
        const auto left = juce::roundToInt ((float) master.getX() * sx);
        const auto top = juce::roundToInt ((float) master.getY() * sy);
        const auto right = juce::roundToInt ((float) master.getRight() * sx);
        const auto bottom = juce::roundToInt ((float) master.getBottom() * sy);
        return { left, top, right - left, bottom - top };
    }

    static juce::Rectangle<int> parameterPanelBounds() { return toRuntime ({ 914, 455, 209, 337 }); }
    static juce::Rectangle<int> outputPanelBounds()    { return toRuntime ({ 1134, 455, 134, 337 }); }

    static const std::array<juce::Rectangle<int>, 3>& knobBounds()
    {
        static const std::array<juce::Rectangle<int>, 3> bounds {{
            toRuntime ({ 930, 641, 60, 60 }),
            toRuntime ({ 991, 641, 60, 60 }),
            toRuntime ({ 1060, 641, 60, 60 })
        }};
        return bounds;
    }

    static const std::array<juce::Rectangle<int>, 3>& readoutBounds()
    {
        static const std::array<juce::Rectangle<int>, 3> bounds {{
            toRuntime ({ 928, 703, 60, 20 }),
            toRuntime ({ 991, 703, 60, 20 }),
            toRuntime ({ 1060, 703, 60, 20 })
        }};
        return bounds;
    }

    static const std::array<juce::Rectangle<int>, 2>& outputMeterBounds()
    {
        static const std::array<juce::Rectangle<int>, 2> bounds {{
            toRuntime ({ 1174, 523, 15, 218 }),
            toRuntime ({ 1217, 523, 15, 218 })
        }};
        return bounds;
    }

    static const std::array<juce::Rectangle<int>, 2>& outputReadoutBounds()
    {
        static const std::array<juce::Rectangle<int>, 2> bounds {{
            toRuntime ({ 1154, 751, 48, 26 }),
            toRuntime ({ 1204, 751, 48, 26 })
        }};
        return bounds;
    }

    static std::array<juce::Rectangle<int>, 2> outputMeterLocalBounds()
    {
        const auto parent = outputPanelBounds().getPosition();
        auto bounds = outputMeterBounds();
        for (auto& bound : bounds) bound.translate (-parent.x, -parent.y);
        return bounds;
    }

    static std::array<juce::Rectangle<int>, 2> outputReadoutLocalBounds()
    {
        const auto parent = outputPanelBounds().getPosition();
        auto bounds = outputReadoutBounds();
        for (auto& bound : bounds) bound.translate (-parent.x, -parent.y);
        return bounds;
    }
};
