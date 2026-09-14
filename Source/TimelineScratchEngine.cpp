#include "TimelineScratchEngine.h"
#include <cmath>

namespace
{
constexpr float kRampSeconds = 0.008f;
constexpr double kMinimumReadableDistance = 2.0;
constexpr double kBackspinWindowSeconds = 0.35;
constexpr double kBackspinLaunchSeconds = 0.04;
}

void TimelineScratchEngine::prepare (double rate, int, int channels)
{
    sampleRateHz = juce::jmax (1.0, rate);
    channelCount = juce::jlimit (1, 2, channels);
    historySamples = juce::jmax (8, juce::roundToInt (sampleRateHz * maxHistorySeconds));
    for (auto& channel : history) channel.assign ((size_t) historySamples, 0.0f);
    writeSerial = minimumReadableSerial = 0;
    activeBar = -1; wetRamp = 0.0f; waitingForDry = false;
    backspinWindowSamples = backspinLaunchOffsetSamples = 0.0;
    backspinPrimaryOffsetSamples = backspinSecondaryOffsetSamples = 0.0;
    backspinWrapSamples = backspinWrapProgress = 1;
    backspinCompletedWraps = 0;
}

void TimelineScratchEngine::release()
{
    for (auto& channel : history) { channel.clear(); channel.shrink_to_fit(); }
    historySamples = channelCount = 0; sampleRateHz = 0.0;
    writeSerial = minimumReadableSerial = 0; activeBar = -1; wetRamp = 0.0f;
    backspinWindowSamples = backspinLaunchOffsetSamples = 0.0;
    backspinPrimaryOffsetSamples = backspinSecondaryOffsetSamples = 0.0;
    backspinWrapSamples = backspinWrapProgress = 1;
    backspinCompletedWraps = 0;
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
    activeDurationSamples = juce::jmin ((double) historySamples - 4.0,
        activeDurationQuarters * secondsPerQuarter * sampleRateHz);
    // TAPE BRAKE retains its existing initial source position.
    const auto initialHistoryOffset = juce::jmin (sampleRateHz * 0.08,
        juce::jmax (2.0, activeDurationSamples * 0.15));
    anchorSerial = (double) writeSerial - initialHistoryOffset;
    tapeReadSerial = anchorSerial;

    // BACKSPIN loops a moving, recent-history window.  It must not use BAR
    // duration as a read distance: LENGTH controls only how long it is active.
    backspinWindowSamples = juce::jmin (sampleRateHz * kBackspinWindowSeconds,
        juce::jmax (8.0, (double) historySamples - 4.0));
    backspinLaunchOffsetSamples = juce::jmin (backspinWindowSamples * 0.25,
        juce::jmax (kMinimumReadableDistance, sampleRateHz * kBackspinLaunchSeconds));
    const auto maximumOffsetAdvance = 1.0 + 4.0;
    backspinWrapSamples = juce::jmax (1, juce::jmin (
        juce::roundToInt (sampleRateHz * kRampSeconds),
        juce::jmax (1, static_cast<int> (std::floor ((backspinWindowSamples - backspinLaunchOffsetSamples)
                                                      / maximumOffsetAdvance)))));
    backspinWrapProgress = backspinWrapSamples;
    backspinPrimaryOffsetSamples = backspinLaunchOffsetSamples;
    backspinSecondaryOffsetSamples = backspinLaunchOffsetSamples;
    backspinCompletedWraps = 0;
}

TimelineScratchEngine::BackspinReadState TimelineScratchEngine::getBackspinReadState() const noexcept
{
    const auto speed = juce::jlimit (0.25f, 4.0f, activeSlot.speed);
    return { activeSlot.preset == Preset::backspin,
             backspinWrapProgress < backspinWrapSamples,
             backspinWindowSamples,
             backspinLaunchOffsetSamples,
             backspinPrimaryOffsetSamples,
             backspinSecondaryOffsetSamples,
             -(double) speed,
             wetRamp,
             backspinCompletedWraps };
}

double TimelineScratchEngine::tapeBrakePlaybackRate (double effectPhase, float speed) noexcept
{
    const auto safePhase = juce::jlimit (0.0, 1.0, effectPhase);
    const auto safeSpeed = juce::jlimit (0.25f, 4.0f, speed);
    // Even 0.25x SPEED begins below unity; SPEED changes both the initial
    // braking amount and the curve without altering the parameter range.
    const auto startRate = juce::jlimit (0.40, 0.88, 0.72 / std::sqrt ((double) safeSpeed));
    return startRate * std::pow (1.0 - safePhase, 1.0 + 0.25 * (double) safeSpeed);
}

float TimelineScratchEngine::wetSample (int channel, double barPhase, double quartersPerBar) noexcept
{
    if (activeSlot.preset == Preset::off || activeSlot.preset == Preset::custom)
        return 0.0f;
    const auto elapsedQuarters = barPhase * quartersPerBar;
    if (elapsedQuarters >= activeDurationQuarters || activeDurationQuarters <= 0.0)
        return 0.0f;
    if (activeSlot.preset == Preset::backspin)
    {
        const auto primary = read (history[(size_t) channel],
                                   (double) writeSerial - backspinPrimaryOffsetSamples);
        if (backspinWrapProgress >= backspinWrapSamples)
            return primary;

        const auto secondary = read (history[(size_t) channel],
                                     (double) writeSerial - backspinSecondaryOffsetSamples);
        const auto blend = (float) backspinWrapProgress / (float) backspinWrapSamples;
        return primary * (1.0f - blend) + secondary * blend;
    }
    if (activeSlot.preset == Preset::tapeBrake)
    {
        return read (history[(size_t) channel], tapeReadSerial);
    }
    return 0.0f;
}

void TimelineScratchEngine::advanceBackspinReadHeads() noexcept
{
    const auto speed = juce::jlimit (0.25f, 4.0f, activeSlot.speed);
    const auto offsetAdvance = 1.0 + (double) speed;

    if (backspinWrapProgress >= backspinWrapSamples
        && backspinPrimaryOffsetSamples + offsetAdvance * (double) backspinWrapSamples >= backspinWindowSamples)
    {
        backspinSecondaryOffsetSamples = backspinLaunchOffsetSamples;
        backspinWrapProgress = 0;
    }

    if (backspinWrapProgress < backspinWrapSamples)
    {
        backspinPrimaryOffsetSamples = juce::jmin (backspinWindowSamples,
                                                    backspinPrimaryOffsetSamples + offsetAdvance);
        backspinSecondaryOffsetSamples += offsetAdvance;
        if (++backspinWrapProgress >= backspinWrapSamples)
        {
            backspinPrimaryOffsetSamples = backspinSecondaryOffsetSamples;
            ++backspinCompletedWraps;
        }
        return;
    }

    backspinPrimaryOffsetSamples = juce::jmin (backspinWindowSamples,
                                                backspinPrimaryOffsetSamples + offsetAdvance);
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
            tapeReadSerial += tapeBrakePlaybackRate (elapsed / activeDurationQuarters, activeSlot.speed);
        }
        if (activeSlot.preset == Preset::backspin && effectActive && ! waitingForDry)
            advanceBackspinReadHeads();
        ++writeSerial;
    }
}
