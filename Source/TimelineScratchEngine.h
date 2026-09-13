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

    struct Slot final
    {
        Preset preset = Preset::off;
        Length length = Length::sixteenth;
        float speed = 1.0f, pitch = 0.0f, depth = 0.5f;
    };

    struct Transport final
    {
        bool playing = false, discontinuity = false;
        int startBar = -1;
        double startBarPhase = 0.0, barPhasePerSample = 0.0, quartersPerBar = 4.0,
               secondsPerQuarter = 0.5;
    };

    void prepare (double sampleRate, int maxBlockSize, int channels);
    void release();
    void resetTransport() noexcept;
    void process (juce::AudioBuffer<float>&, const Transport&, const std::array<Slot, 64>&) noexcept;
    int getAllocatedHistorySamples() const noexcept { return historySamples; }

private:
    static double lengthInQuarters (Length, double quartersPerBar) noexcept;
    bool canRead (double serial) const noexcept;
    float read (const std::vector<float>&, double serial) const noexcept;
    void write (int channel, float sample) noexcept;
    void beginBar (int bar, const Slot&, double quartersPerBar, double secondsPerQuarter) noexcept;
    float wetSample (int channel, double barPhase, double quartersPerBar) noexcept;

    double sampleRateHz = 0.0;
    int historySamples = 0, channelCount = 0;
    std::array<std::vector<float>, 2> history;
    uint64_t writeSerial = 0, minimumReadableSerial = 0;
    int activeBar = -1;
    Slot activeSlot {};
    double activeDurationQuarters = 0.25, anchorSerial = 0.0, tapeReadSerial = 0.0;
    float wetRamp = 0.0f;
    bool waitingForDry = false;
};
