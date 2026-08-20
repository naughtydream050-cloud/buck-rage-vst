#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// UI V2 is intentionally independent from the retired PluginEditor/UIComponents
// paint path.  It owns only image layers and transparent, co-bounded inputs.
class ToyotomiHideyoshiAudioProcessorEditorV2 final : public juce::AudioProcessorEditor,
                                                       private juce::Timer
{
public:
    explicit ToyotomiHideyoshiAudioProcessorEditorV2 (ToyotomiHideyoshiAudioProcessor&);
    ~ToyotomiHideyoshiAudioProcessorEditorV2() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Surface;
    class HitRegion;
    class KnobRegion;
    class XYRegion;
    ToyotomiHideyoshiAudioProcessor& processor;
    std::unique_ptr<Surface> surface;
    juce::OwnedArray<HitRegion> hitRegions;
    std::array<std::unique_ptr<KnobRegion>, 3> knobs;
    std::unique_ptr<XYRegion> xyInput;
    void timerCallback() override;
    void addImageHit (juce::Rectangle<int>, std::function<void()>);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToyotomiHideyoshiAudioProcessorEditorV2)
};
