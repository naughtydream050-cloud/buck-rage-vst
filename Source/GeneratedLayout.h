#pragma once
#include <JuceHeader.h>

// Generated from ui/spec/ui-spec.json. Do not edit by hand.
namespace RudeHypeGeneratedLayout
{
inline constexpr float canvasWidth = 1592.0f;
inline constexpr float canvasHeight = 988.0f;
inline constexpr float displayScale = 0.42f;
inline constexpr float displayWidth = canvasWidth * displayScale;
inline constexpr float displayHeight = canvasHeight * displayScale;
inline constexpr float knobAngleStartDeg = -150.0f;
inline constexpr float knobAngleEndDeg = 150.0f;

inline juce::Rectangle<float> scaled(juce::Rectangle<float> bounds)
{
    return { bounds.getX() * displayScale,
             bounds.getY() * displayScale,
             bounds.getWidth() * displayScale,
             bounds.getHeight() * displayScale };
}

inline constexpr float shoutX = 205.0f;
inline constexpr float shoutY = 378.0f;
inline constexpr float shoutWidth = 390.0f;
inline constexpr float shoutHeight = 390.0f;
inline constexpr float shoutCenterX = 400.0f;
inline constexpr float shoutCenterY = 573.0f;
inline constexpr float shoutRadius = 195.0f;
inline juce::Rectangle<float> shoutBounds() { return { shoutX, shoutY, shoutWidth, shoutHeight }; }
inline juce::Rectangle<float> shoutDisplayBounds() { return scaled(shoutBounds()); }

inline constexpr float burnX = 1010.0f;
inline constexpr float burnY = 380.0f;
inline constexpr float burnWidth = 382.0f;
inline constexpr float burnHeight = 382.0f;
inline constexpr float burnCenterX = 1201.0f;
inline constexpr float burnCenterY = 571.0f;
inline constexpr float burnRadius = 191.0f;
inline juce::Rectangle<float> burnBounds() { return { burnX, burnY, burnWidth, burnHeight }; }
inline juce::Rectangle<float> burnDisplayBounds() { return scaled(burnBounds()); }
}
