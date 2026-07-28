#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "PluginStateModel.h"

class ToyotomiHideyoshiAudioProcessor final : public juce::AudioProcessor
{
public:
    ToyotomiHideyoshiAudioProcessor();
    ~ToyotomiHideyoshiAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    float consumeOutputPeak (int channel) noexcept;
    double getHostBpm() const noexcept { return hostBpm.load (std::memory_order_relaxed); }
    int getTimeSignatureNumerator() const noexcept { return timeSignatureNumerator.load (std::memory_order_relaxed); }
    int getTimeSignatureDenominator() const noexcept { return timeSignatureDenominator.load (std::memory_order_relaxed); }
    bool getHostSyncAvailable() const noexcept { return hostSyncAvailable.load (std::memory_order_relaxed); }
    bool getHostPlaying() const noexcept { return hostPlaying.load (std::memory_order_relaxed); }
    PluginStateModel& getStateModel() noexcept { return stateModel; }
    const PluginStateModel& getStateModel() const noexcept { return stateModel; }

private:
    static void publishPeak (std::atomic<float>& destination, float value) noexcept;

    std::atomic<float> outputPeakLeft { 0.0f };
    std::atomic<float> outputPeakRight { 0.0f };
    std::atomic<double> hostBpm { 120.0 };
    std::atomic<int> timeSignatureNumerator { 4 };
    std::atomic<int> timeSignatureDenominator { 4 };
    std::atomic<bool> hostSyncAvailable { false };
    std::atomic<bool> hostPlaying { false };
    PluginStateModel stateModel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToyotomiHideyoshiAudioProcessor)
};
