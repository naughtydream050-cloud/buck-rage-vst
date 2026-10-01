#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstdint>
#include <vector>

// One realtime-safe engine serves the currently playing timeline BAR.  The
// 64 BARs are configuration snapshots, never 64 DSP instances.
class TimelineScratchEngine final
{
public:
    static constexpr double maxHistorySeconds = 10.0;
    enum class Preset : uint8_t { off, forwardCut, backspin, chirp, baby, transform, drag, zigzag, tapeBrake, custom };
    enum class Length : uint8_t { sixteenth, eighth, quarter, half, oneBar };
    enum class DiscontinuityReason : uint8_t { none, stopped, start, samplePositionJump, ppqJump };

    struct Slot final
    {
        Preset preset = Preset::off;
        Length length = Length::oneBar;
        float speed = 1.0f, pitch = 0.0f, depth = 1.0f;
    };

    struct Transport final
    {
        bool playing = false, discontinuity = false;
        DiscontinuityReason discontinuityReason = DiscontinuityReason::none;
        int startBar = -1;
        double startBarPhase = 0.0, barPhasePerSample = 0.0, quartersPerBar = 4.0,
               secondsPerQuarter = 0.5;
    };

    struct BackspinReadState final
    {
        bool active = false, crossfading = false;
        double windowSamples = 0.0, windowStartSerial = 0.0, windowEndSerial = 0.0,
               primaryReadSerial = 0.0, secondaryReadSerial = 0.0,
               historyValidSamples = 0.0,
               reverseReadRate = 0.0;
        float wetRamp = 0.0f;
        uint32_t completedWraps = 0;
    };

    struct Diagnostics final
    {
        int playingBar = -1;
        Preset preset = Preset::off;
        double effectPhase = 0.0, historyValidSamples = 0.0,
               captureWindowSamples = 0.0, readPosition = 0.0, readRate = 0.0;
        uint32_t wrapCount = 0, transportResetCount = 0;
        float effectiveWet = 0.0f;
        DiscontinuityReason discontinuityReason = DiscontinuityReason::none;
        double forwardCycleSamples = 0.0, forwardReadSamples = 0.0,
               forwardCyclePosition = 0.0;
        uint32_t forwardRetriggerCount = 0;
        bool forwardSourceEnded = false;
    };

    void prepare (double sampleRate, int maxBlockSize, int channels);
    void release();
    void resetTransport() noexcept;
    void process (juce::AudioBuffer<float>&, const Transport&, const std::array<Slot, 64>&) noexcept;
    int getAllocatedHistorySamples() const noexcept { return historySamples; }
    BackspinReadState getBackspinReadState() const noexcept;
    Diagnostics getDiagnostics() const noexcept;
    static double tapeBrakePlaybackRate (double effectPhase, float speed) noexcept;

private:
    static double lengthInQuarters (Length, double quartersPerBar) noexcept;
    bool canRead (double serial) const noexcept;
    float read (const std::vector<float>&, double serial) const noexcept;
    void write (int channel, float sample) noexcept;
    void beginBar (int bar, const Slot&, double quartersPerBar, double secondsPerQuarter) noexcept;
    bool beginBackspinCapture() noexcept;
    bool beginTapeBrakeCapture() noexcept;
    bool beginBabyCapture() noexcept;
    bool beginForwardCutCapture() noexcept;
    bool beginDragCapture() noexcept;
    float wetSample (int channel, double barPhase, double quartersPerBar) noexcept;
    void advanceBackspinReadHeads() noexcept;
    void advanceBabyReadHead() noexcept;
    void advanceForwardCutReadHead() noexcept;
    double historyValidSamples() const noexcept;
    bool isWetReady() const noexcept;

    double sampleRateHz = 0.0;
    int historySamples = 0, channelCount = 0;
    std::array<std::vector<float>, 2> history;
    uint64_t writeSerial = 0, minimumReadableSerial = 0;
    int activeBar = -1;
    Slot activeSlot {};
    double activeDurationQuarters = 0.25, activeDurationSamples = 0.0,
           tapeReadSerial = 0.0, tapeWindowStartSerial = 0.0, tapeWindowEndSerial = 0.0,
           backspinWindowSamples = 0.0, backspinWindowStartSerial = 0.0,
           backspinWindowEndSerial = 0.0, backspinPrimaryReadSerial = 0.0,
           backspinSecondaryReadSerial = 0.0;
    double babyWindowSamples = 0.0, babyWindowStartSerial = 0.0,
           babyWindowEndSerial = 0.0, babyReadSerial = 0.0, babyCycleSample = 0.0;
    double dragWindowSamples = 0.0, dragWindowStartSerial = 0.0,
           dragWindowEndSerial = 0.0, dragReadSerial = 0.0;
    double forwardWindowSamples = 0.0, forwardWindowStartSerial = 0.0,
           forwardWindowEndSerial = 0.0, forwardReadSerial = 0.0,
           forwardSecondaryReadSerial = 0.0;
    double forwardCycleSamples = 0.0, forwardReadSamples = 0.0,
           forwardFadeSamples = 0.0, forwardCyclePosition = 0.0,
           forwardSourceSilenceSamples = 0.0;
    int backspinWrapSamples = 1, backspinWrapProgress = 1;
    int forwardWrapSamples = 1, forwardWrapProgress = 1;
    uint32_t backspinCompletedWraps = 0, forwardCompletedWraps = 0;
    float wetRamp = 0.0f;
    bool waitingForDry = false, backspinCaptured = false, tapeBrakeCaptured = false,
         babyCaptured = false, forwardCaptured = false, forwardSourceEnded = false,
         forwardSourceHasSignal = false, dragCaptured = false;
    double diagnosticEffectPhase = 0.0, diagnosticReadPosition = 0.0, diagnosticReadRate = 0.0;
    float diagnosticEffectiveWet = 0.0f;
    uint32_t transportResetCount = 0;
    DiscontinuityReason lastDiscontinuityReason = DiscontinuityReason::none;
};
