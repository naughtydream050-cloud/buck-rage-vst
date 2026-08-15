#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UiSpec.h"
#include "UIComponents.h"

class ToyotomiHideyoshiAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                    private juce::Timer
{
public:
    explicit ToyotomiHideyoshiAudioProcessorEditor (ToyotomiHideyoshiAudioProcessor&);
    ~ToyotomiHideyoshiAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // The artwork and all hit regions remain authored on the 1280 x 853
    // reference canvas.  This one transform keeps visual and input geometry
    // together in the smaller fixed FL Studio editor.
    static constexpr float kEditorScale = 0.9f;
    static constexpr int kEditorWidth = 1152;
    static constexpr int kEditorHeight = 768;

    ToyotomiHideyoshiAudioProcessor& processor;
    UiSpec uiSpec;
    juce::Image referenceImage;

    juce::Component content;
    TopBarComponent topBar;
    ArtworkPanel artwork;
    BarTabComponent barTabs;
    BarMapComponent barMap;
    QuotePanel quotePanel;
    XYMotionPad xyPad;
    ScratchPresetPalette presetPalette;
    CountParameterPanel countParameters;
    OutputMeterComponent outputMeter;
    BottomStatusBar bottomStatus;
    int displayTab = 0;

    void timerCallback() override;
    void refreshSelectedSlotViews();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToyotomiHideyoshiAudioProcessorEditor)
};

