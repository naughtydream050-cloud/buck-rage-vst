#pragma once
#include <JuceHeader.h>

namespace LaoziLayout
{
constexpr float canvasWidth = 1280.0f;
constexpr float canvasHeight = 905.0f;
constexpr float displayScale = 0.5f;
constexpr int displayWidth = 640;
constexpr int displayHeight = 453;
constexpr float knobStartDegrees = -150.0f;
constexpr float knobEndDegrees = 150.0f;

inline juce::Rectangle<int> scale(juce::Rectangle<float> bounds)
{
    return bounds.withPosition(bounds.getX() * displayScale, bounds.getY() * displayScale)
                 .withSize(bounds.getWidth() * displayScale, bounds.getHeight() * displayScale)
                 .toNearestInt();
}

inline juce::Rectangle<int> pressureBounds() { return scale({ 112.0f, 428.0f, 196.0f, 196.0f }); }
inline juce::Rectangle<int> kickBounds() { return scale({ 360.0f, 428.0f, 196.0f, 196.0f }); }
inline juce::Rectangle<int> auraBounds() { return scale({ 593.0f, 428.0f, 196.0f, 196.0f }); }
inline juce::Rectangle<int> glueBounds() { return scale({ 820.0f, 428.0f, 196.0f, 196.0f }); }
inline juce::Rectangle<int> outputBounds() { return scale({ 1105.0f, 637.0f, 100.0f, 100.0f }); }
inline juce::Rectangle<int> presetBounds() { return scale({ 94.0f, 39.0f, 154.0f, 28.0f }); }
inline juce::Rectangle<int> previousPresetBounds() { return scale({ 250.0f, 39.0f, 20.0f, 28.0f }); }
inline juce::Rectangle<int> nextPresetBounds() { return scale({ 272.0f, 39.0f, 20.0f, 28.0f }); }
inline juce::Rectangle<int> meterBounds() { return scale({ 1127.0f, 429.0f, 56.0f, 197.0f }); }
inline juce::Rectangle<int> meterLeftBounds() { return scale({ 1127.0f, 429.0f, 19.0f, 197.0f }); }
inline juce::Rectangle<int> meterRightBounds() { return scale({ 1164.0f, 429.0f, 19.0f, 197.0f }); }
inline juce::Rectangle<int> oversampleBounds() { return scale({ 50.0f, 843.0f, 120.0f, 41.0f }); }
inline juce::Rectangle<int> bypassBounds() { return scale({ 1110.0f, 843.0f, 119.0f, 41.0f }); }
inline juce::Rectangle<int> presetTextBounds() { return scale({ 101.0f, 44.0f, 140.0f, 18.0f }); }
inline juce::Rectangle<int> oversampleStatusBounds() { return scale({ 118.0f, 855.0f, 37.0f, 17.0f }); }
inline juce::Rectangle<int> bypassIndicatorBounds() { return scale({ 1120.0f, 854.0f, 15.0f, 15.0f }); }
}
