#pragma once
#include <JuceHeader.h>

namespace VintageRawnessGeneratedLayout
{
constexpr float canvasWidth = 1448.0f;
constexpr float canvasHeight = 1086.0f;
constexpr float displayScale = 0.50f;
constexpr float displayWidth = canvasWidth * displayScale;
constexpr float displayHeight = canvasHeight * displayScale;

constexpr float knobAngleStartDeg = -150.0f;
constexpr float knobAngleEndDeg = 150.0f;

constexpr float dirtDefault = 0.30f;
constexpr float crushDefault = 0.20f;
constexpr float wobbleDefault = 0.10f;

inline juce::Rectangle<float> scale(juce::Rectangle<float> r) noexcept
{
    return { r.getX() * displayScale, r.getY() * displayScale,
             r.getWidth() * displayScale, r.getHeight() * displayScale };
}

inline juce::Rectangle<float> dirtBounds() noexcept { return { 210.0f, 456.0f, 224.0f, 224.0f }; }
inline juce::Rectangle<float> crushBounds() noexcept { return { 612.0f, 456.0f, 224.0f, 224.0f }; }
inline juce::Rectangle<float> wobbleBounds() noexcept { return { 1014.0f, 456.0f, 224.0f, 224.0f }; }

inline juce::Rectangle<float> dirtDisplayBounds() noexcept { return scale(dirtBounds()); }
inline juce::Rectangle<float> crushDisplayBounds() noexcept { return scale(crushBounds()); }
inline juce::Rectangle<float> wobbleDisplayBounds() noexcept { return scale(wobbleBounds()); }

inline juce::Rectangle<float> preset1Bounds() noexcept { return { 178.0f, 944.0f, 270.0f, 78.0f }; }
inline juce::Rectangle<float> preset2Bounds() noexcept { return { 464.0f, 944.0f, 270.0f, 78.0f }; }
inline juce::Rectangle<float> preset3Bounds() noexcept { return { 748.0f, 944.0f, 270.0f, 78.0f }; }
inline juce::Rectangle<float> preset4Bounds() noexcept { return { 1022.0f, 944.0f, 270.0f, 78.0f }; }

inline juce::Rectangle<float> preset1DisplayBounds() noexcept { return scale(preset1Bounds()); }
inline juce::Rectangle<float> preset2DisplayBounds() noexcept { return scale(preset2Bounds()); }
inline juce::Rectangle<float> preset3DisplayBounds() noexcept { return scale(preset3Bounds()); }
inline juce::Rectangle<float> preset4DisplayBounds() noexcept { return scale(preset4Bounds()); }
}
