#pragma once

#include <JuceHeader.h>

// Native 1024x683 bounds generated from the approved 1280x853 SSOT.
// This is Toyotomi's sole runtime layout source, following Buck Raw Shit's
// GeneratedLayout.h pattern.
namespace GeneratedLayout
{
inline juce::Rectangle<int> speedKnobBounds()       { return { 744, 513, 48, 48 }; }
inline juce::Rectangle<int> pitchKnobBounds()       { return { 793, 513, 48, 48 }; }
inline juce::Rectangle<int> depthKnobBounds()       { return { 848, 513, 48, 48 }; }
inline juce::Rectangle<int> speedReadoutBounds()    { return { 742, 563, 48, 16 }; }
inline juce::Rectangle<int> pitchReadoutBounds()    { return { 793, 563, 48, 16 }; }
inline juce::Rectangle<int> depthReadoutBounds()    { return { 848, 563, 48, 16 }; }
inline juce::Rectangle<int> outputPanelBounds()     { return { 907, 364, 107, 270 }; }
inline int outputLLabelCenterX()                    { return 943; }
inline int outputRLabelCenterX()                    { return 979; }
inline juce::Rectangle<int> outputLBounds()         { return { 937, 419, 12, 174 }; }
inline juce::Rectangle<int> outputRBounds()         { return { 973, 419, 12, 174 }; }
inline juce::Rectangle<int> outputLReadoutBounds()  { return { 923, 601, 39, 21 }; }
inline juce::Rectangle<int> outputRReadoutBounds()  { return { 960, 601, 39, 21 }; }
}
