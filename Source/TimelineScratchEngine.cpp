#include "TimelineScratchEngine.h"
#include <cmath>

namespace
{
constexpr float kRampSeconds = 0.008f;
constexpr double kMinimumReadableDistance = 2.0;
}

void TimelineScratchEngine::prepare (double rate, int, int channels)
{
    sampleRateHz = juce::jmax (1.0, rate);
    channelCount = juce::jlimit (1, 2, channels);
    historySamples = juce::jmax (8, juce::roundToInt (sampleRateHz * maxHistorySeconds));
    for (auto& channel : history) channel.assign ((size_t) historySamples, 0.0f);
    writeSerial = minimumReadableSerial = 0;
    activeBar = -1; wetRamp = 0.0f; waitingForDry = false;
}

void TimelineScratchEngine::release()
{
    for (auto& channel : history) { channel.clear(); channel.shrink_to_fit(); }
    historySamples = channelCount = 0; sampleRateHz = 0.0;
    writeSerial = minimumReadableSerial = 0; activeBar = -1; wetRamp = 0.0f;
}

void TimelineScratchEngine::resetTransport() noexcept
{
    // Old ring contents remain allocated but cannot be read after a transport
    // discontinuity. This is O(1) and safe on the audio thread.
    minimumReadableSerial = writeSerial;
    activeBar = -1; wetRamp = 0.0f; waitingForDry = false;
}

double TimelineScratchEngine::lengthInQuarters (Length length, double quartersPerBar) noexcept
{
    switch (length)
    {
        case Length::sixteenth: return 0.25;
        case Length::eighth:    return 0.5;
        case Length::quarter:   return 1.0;
        case Length::half:      return 2.0;
        case Length::oneBar:    return quartersPerBar;
    }
    return 0.25;
}

bool TimelineScratchEngine::canRead (double serial) const noexcept
{
    const auto oldest = juce::jmax ((double) minimumReadableSerial, (double) writeSerial - (double) historySamples);
    return historySamples > 3 && serial >= oldest && serial <= (double) writeSerial - kMinimumReadableDistance;
}

float TimelineScratchEngine::read (const std::vector<float>& channel, double serial) const noexcept
{
    if (! canRead (serial)) return 0.0f;
    const auto whole = (int64_t) std::floor (serial);
    const auto fraction = (float) (serial - (double) whole);
    const auto first = (int) (whole % historySamples);
    const auto second = (first + 1) % historySamples;
    return channel[(size_t) first] + (channel[(size_t) second] - channel[(size_t) first]) * fraction;
}

void TimelineScratchEngine::write (int channel, float sample) noexcept
{
    if (channel < channelCount && historySamples > 0)
        history[(size_t) channel][(size_t) (writeSerial % (uint64_t) historySamples)] = sample;
}

void TimelineScratchEngine::beginBar (int bar, const Slot& slot, double quartersPerBar, double secondsPerQuarter) noexcept
{
    activeBar = bar;
    activeSlot = slot;
    activeDurationQuarters = juce::jmin (lengthInQuarters (slot.length, quartersPerBar), quartersPerBar);
    // Clamp requested duration to the explicit 10-second history ceiling.
    activeDurationQuarters = juce::jmin (activeDurationQuarters,
        maxHistorySeconds / juce::jmax (0.000001, secondsPerQuarter));
    const auto durationSamples = juce::jmin ((double) historySamples - 4.0,
        activeDurationQuarters * secondsPerQuarter * sampleRateHz);
    anchorSerial = (double) writeSerial - juce::jmax (2.0, durationSamples);
    tapeReadSerial = anchorSerial;
}

float TimelineScratchEngine::wetSample (int channel, double barPhase, double quartersPerBar) noexcept
{
    if (activeSlot.preset == Preset::off || activeSlot.preset == Preset::custom)
        return 0.0f;
    const auto elapsedQuarters = barPhase * quartersPerBar;
    if (elapsedQuarters >= activeDurationQuarters || activeDurationQuarters <= 0.0)
        return 0.0f;
    const auto effectPhase = juce::jlimit (0.0, 1.0, elapsedQuarters / activeDurationQuarters);
    const auto speed = juce::jlimit (0.25f, 4.0f, activeSlot.speed);
    if (activeSlot.preset == Preset::backspin)
    {
        const auto travel = effectPhase * speed * juce::jmax (2.0, (double) historySamples * 0.45);
        return read (history[(size_t) channel], (double) writeSerial - 2.0 - travel);
    }
    if (activeSlot.preset == Preset::tapeBrake)
    {
        const auto curve = juce::jlimit (0.0, 1.0, effectPhase * speed);
        const auto rate = (1.0 - curve) * (1.0 - curve);
        const auto sample = read (history[(size_t) channel], tapeReadSerial);
        return sample;
    }
    return 0.0f;
}

void TimelineScratchEngine::process (juce::AudioBuffer<float>& buffer, const Transport& transport,
                                     const std::array<Slot, 64>& slots) noexcept
{
    if (historySamples <= 0 || sampleRateHz <= 0.0) return;
    if (transport.discontinuity || ! transport.playing || transport.startBar < 0)
        resetTransport();
    const auto rampStep = 1.0f / juce::jmax (1.0f, (float) sampleRateHz * kRampSeconds);
    const auto channels = juce::jmin (channelCount, buffer.getNumChannels());
    for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        const auto progressed = transport.startBarPhase + transport.barPhasePerSample * (double) sampleIndex;
        const auto wholeBars = (int) std::floor (progressed);
        const auto phase = progressed - std::floor (progressed);
        const auto bar = transport.playing ? (transport.startBar + wholeBars + 64) % 64 : -1;
        if (bar != activeBar && bar >= 0)
        {
            // Fade to dry before moving a read head. This avoids an abrupt
            // source jump at a BAR boundary without a second always-on engine.
            if (wetRamp > 0.001f) waitingForDry = true;
            else beginBar (bar, slots[(size_t) bar], transport.quartersPerBar, transport.secondsPerQuarter);
        }
        const auto effectActive = activeBar >= 0 && activeSlot.preset != Preset::off
            && phase * transport.quartersPerBar < activeDurationQuarters;
        const auto targetWet = effectActive && ! waitingForDry ? 1.0f : 0.0f;
        wetRamp += juce::jlimit (-rampStep, rampStep, targetWet - wetRamp);
        if (waitingForDry && wetRamp <= 0.001f && bar >= 0)
        {
            waitingForDry = false;
            beginBar (bar, slots[(size_t) bar], transport.quartersPerBar, transport.secondsPerQuarter);
        }
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto dry = buffer.getSample (channel, sampleIndex);
            const auto wet = wetSample (channel, phase, transport.quartersPerBar);
            const auto effectiveWet = juce::jlimit (0.0f, 1.0f, activeSlot.depth) * wetRamp;
            buffer.setSample (channel, sampleIndex, dry * (1.0f - effectiveWet) + wet * effectiveWet);
            write (channel, dry);
        }
        if (activeSlot.preset == Preset::tapeBrake && effectActive)
        {
            const auto elapsed = phase * transport.quartersPerBar;
            const auto curve = juce::jlimit (0.0, 1.0, (elapsed / activeDurationQuarters) * activeSlot.speed);
            const auto rate = (1.0 - curve) * (1.0 - curve);
            tapeReadSerial += rate;
        }
        ++writeSerial;
    }
}
