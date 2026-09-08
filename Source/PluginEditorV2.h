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
    ~ToyotomiHideyoshiAudioProcessorEditorV2() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    // BAR MAP owns the alpha-cut-out region of the faceplate. Keep its asset
    // contract independently observable by the smoke test.
    bool hasValidBarMapAssets() const;
    bool validateInteractiveBounds() const;
    bool debugClickAt (juce::Point<int> point);
    bool debugMouseDownAt (juce::Point<int> point);
    bool debugMouseUpAt (juce::Point<int> point);
    bool debugXYAt (juce::Point<int> point, double elapsedSeconds = -1.0);
    void debugSetOutputMeterDb (float left, float right);

private:
    class Surface;
    class OutputMeter;
    class HitRegion;
    class KnobRegion;
    class BpmRegion;
    class XYRegion;
    ToyotomiHideyoshiAudioProcessor& processor;
    // Editor-local controls: recording must never be restored from a project.
    bool xyRecording = false, xyRecordingHasPoint = false;
    int xyPressedButton = -1;
    int xyRecordingBar = PluginStateModel::kNoSelectedBar;
    double xyRecordingStartMilliseconds = 0.0;
    std::unique_ptr<Surface> surface;
    std::unique_ptr<OutputMeter> outputMeter;
    juce::OwnedArray<HitRegion> hitRegions;
    std::array<std::unique_ptr<KnobRegion>, 3> knobs;
    std::unique_ptr<BpmRegion> bpmInput;
    std::unique_ptr<juce::TextEditor> bpmEditor;
    std::unique_ptr<XYRegion> xyInput;
    void toggleXYRecording();
    void stopXYRecording();
    void updateXYFromPad (juce::Point<float>, double elapsedSeconds = -1.0);
    void adjustInternalBpm (double delta);
    void beginBpmTextEdit();
    void commitBpmTextEdit();
    void showProjectPresetMenu();
    void stepProjectPreset (int direction);
    void timerCallback() override;
    void addImageHit (juce::Rectangle<int>, std::function<void()>);
    void addXYButtonImageHit (juce::Rectangle<int>, int, std::function<void()>);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToyotomiHideyoshiAudioProcessorEditorV2)
};
