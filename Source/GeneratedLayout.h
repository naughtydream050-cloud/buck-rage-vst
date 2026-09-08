#pragma once

#include <JuceHeader.h>

// Native 1024x683 bounds generated from the approved 1280x853 SSOT.
// This is Toyotomi's sole runtime layout source, following Buck Raw Shit's
// GeneratedLayout.h pattern.
namespace GeneratedLayout
{
inline juce::Rectangle<int> hostSyncBounds()        { return { 375, 14, 88, 31 }; }
inline juce::Rectangle<int> bpmBounds()             { return { 469, 14, 62, 31 }; }
inline juce::Rectangle<int> timeSigBounds()         { return { 541, 14, 70, 31 }; }
inline juce::Rectangle<int> presetSelectorBounds()  { return { 697, 14, 155, 31 }; }
inline juce::Rectangle<int> presetPrevBounds()      { return { 862, 14, 29, 31 }; }
inline juce::Rectangle<int> presetNextBounds()      { return { 894, 14, 29, 31 }; }
inline juce::Rectangle<int> speedKnobBounds()       { return { 744, 513, 48, 48 }; }
inline juce::Rectangle<int> pitchKnobBounds()       { return { 793, 513, 48, 48 }; }
inline juce::Rectangle<int> depthKnobBounds()       { return { 848, 513, 48, 48 }; }
inline juce::Rectangle<int> speedReadoutBounds()    { return { 745, 571, 43, 17 }; }
inline juce::Rectangle<int> pitchReadoutBounds()    { return { 797, 571, 45, 17 }; }
inline juce::Rectangle<int> depthReadoutBounds()    { return { 849, 571, 43, 17 }; }
inline juce::Rectangle<int> outputPanelBounds()     { return { 907, 364, 107, 270 }; }
// Visible slot pixels measured from the approved reference, not label centres.
// Channel PNGs are prepared at these native sizes with no dark side margins.
inline juce::Rectangle<int> outputLBounds()         { return { 940, 419, 16, 174 }; }
inline juce::Rectangle<int> outputRBounds()         { return { 974, 419, 15, 174 }; }
inline juce::Rectangle<int> outputLReadoutBounds()  { return { 923, 601, 39, 21 }; }
inline juce::Rectangle<int> outputRReadoutBounds()  { return { 963, 601, 39, 21 }; }
// XY uses this one native coordinate contract for paint, input and tests.
inline juce::Rectangle<int> xyPadBounds()            { return { 38, 438, 195, 141 }; }
// Exact native crops from the approved 1024 reference. These are the single
// source for both state sprites and transparent input regions.
inline juce::Rectangle<int> xyRecBounds()            { return { 26, 596, 60, 30 }; }
inline juce::Rectangle<int> xyClearBounds()          { return { 94, 596, 60, 30 }; }
inline juce::Rectangle<int> xyResetViewBounds()      { return { 158, 596, 81, 30 }; }
}
