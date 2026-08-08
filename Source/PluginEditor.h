#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UiSpec.h"
#include "UIComponents.h"

class ToyotomiHideyoshiAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ToyotomiHideyoshiAudioProcessorEditor (ToyotomiHideyoshiAudioProcessor&);
    ~ToyotomiHideyoshiAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ToyotomiHideyoshiAudioProcessor& processor;
    UiSpec uiSpec;
    juce::Image referenceImage;

    TopBarComponent topBar;
    ArtworkPanel artwork;
    BarTabComponent barTabs;
    BarMapComponent barMap;
    CountGridComponent countGrid;
    XYMotionPad xyPad;
    ScratchPresetPalette presetPalette;
    CountParameterPanel countParameters;
    OutputMeterComponent outputMeter;
    BottomStatusBar bottomStatus;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToyotomiHideyoshiAudioProcessorEditor)
};

