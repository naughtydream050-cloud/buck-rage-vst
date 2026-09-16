#include "TimelineScratchEngine.h"
#include <cmath>

namespace
{
constexpr float kWetRampSeconds = 0.008f;
constexpr double kMinimumReadableDistance = 2.0;
constexpr double kBackspinWrapSeconds = 0.008;
constexpr double kTapeBrakeStartRate = 0.92;
}

void TimelineScratchEngine::prepare (double rate, int, int channels)
{
    sampleRateHz = juce::jmax (1.0, rate);
    channelCount = juce::jlimit (1, 2, channels);
    historySamples = juce::jmax (8, juce::roundToInt (sampleRateHz * maxHistorySeconds));
    for (auto& channel : history) channel.assign ((size_t) historySamples, 0.0f);
    writeSerial = minimumReadableSerial = 0;
    activeBar = -1; wetRamp = 0.0f; waitingForDry = false;
    backspinCaptured = tapeBrakeCaptured = false;
    backspinWindowSamples = backspinWindowStartSerial = backspinWindowEndSerial = 0.0;
    backspinPrimaryReadSerial = backspinSecondaryReadSerial = 0.0;
    tapeReadSerial = tapeWindowStartSerial = tapeWindowEndSerial = 0.0;
    backspinWrapSamples = backspinWrapProgress = 1;
    backspinCompletedWraps = 0;
}

void TimelineScratchEngine::release()
{
    for (auto& channel : history) { channel.clear(); channel.shrink_to_fit(); }
    historySamples = channelCount = 0; sampleRateHz = 0.0;
    writeSerial = minimumReadableSerial = 0; activeBar = -1; wetRamp = 0.0f;
    backspinCaptured = tapeBrakeCaptured = false;
    backspinWindowSamples = backspinWindowStartSerial = backspinWindowEndSerial = 0.0;
    backspinPrimaryReadSerial = backspinSecondaryReadSerial = 0.0;
    tapeReadSerial = tapeWindowStartSerial = tapeWindowEndSerial = 0.0;
    backspinWrapSamples = backspinWrapProgress = 1;
    backspinCompletedWraps = 0;
}

void TimelineScratchEngine::resetTransport() noexcept
{
    minimumReadableSerial = writeSerial;
    activeBar = -1; wetRamp = 0.0f; waitingForDry = false;
    backspinCaptured = tapeBrakeCaptured = false;
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

double TimelineScratchEngine::historyValidSamples() const noexcept
{
    return juce::jmin ((double) historySamples,
                       juce::jmax (0.0, (double) writeSerial - (double) minimumReadableSerial));
}

bool TimelineScratchEngine::canRead (double serial) const noexcept
{
    const auto oldest = juce::jmax ((double) minimumReadableSerial,
                                    (double) writeSerial - (double) historySamples);
    return historySamples > 3 && serial >= oldest
        && serial <= (double) writeSerial - kMinimumReadableDistance;
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

void TimelineScratchEngine::beginBar (int bar, const Slot& slot, double quartersPerBar,
                                      double secondsPerQuarter) noexcept
{
    activeBar = bar;
    activeSlot = slot;
    activeDurationQuarters = juce::jmin (lengthInQuarters (slot.length, quartersPerBar), quartersPerBar);
    activeDurationQuarters = juce::jmin (activeDurationQuarters,
        maxHistorySeconds / juce::jmax (0.000001, secondsPerQuarter));
    activeDurationSamples = juce::jmin ((double) historySamples - 4.0,
        activeDurationQuarters * secondsPerQuarter * sampleRateHz);
    backspinCaptured = tapeBrakeCaptured = false;
    backspinCompletedWraps = 0;
    const auto speed = juce::jlimit (0.25f, 4.0f, slot.speed);
    backspinWindowSamples = juce::jlimit (sampleRateHz * 0.125, sampleRateHz * 0.500,
        sampleRateHz * (0.250 / std::sqrt ((double) speed)));
    backspinWrapSamples = juce::jmax (1, juce::roundToInt (sampleRateHz * kBackspinWrapSeconds));
    backspinWrapProgress = backspinWrapSamples;
}

bool TimelineScratchEngine::beginBackspinCapture() noexcept
{
    if (backspinCaptured || activeSlot.preset != Preset::backspin) return backspinCaptured;
    const auto required = backspinWindowSamples + kMinimumReadableDistance + 2.0;
    if (historyValidSamples() < required) return false;

    backspinWindowEndSerial = (double) writeSerial - kMinimumReadableDistance;
    backspinWindowStartSerial = backspinWindowEndSerial - backspinWindowSamples;
    if (! canRead (backspinWindowStartSerial) || ! canRead (backspinWindowEndSerial - 1.0)) return false;

    backspinPrimaryReadSerial = backspinWindowEndSerial - 1.0;
    backspinSecondaryReadSerial = backspinPrimaryReadSerial;
    backspinWrapProgress = backspinWrapSamples;
    backspinCaptured = true;
    return true;
}

bool TimelineScratchEngine::beginTapeBrakeCapture() noexcept
{
    if (tapeBrakeCaptured || activeSlot.preset != Preset::tapeBrake) return tapeBrakeCaptured;
    const auto exponent = 0.75 + 0.25 * (double) juce::jlimit (0.25f, 4.0f, activeSlot.speed);
    const auto requiredTravel = activeDurationSamples * kTapeBrakeStartRate / (exponent + 1.0);
    const auto requiredWindow = juce::jmax (sampleRateHz * 0.250,
                                             requiredTravel + kMinimumReadableDistance + 2.0);
    if (historyValidSamples() < requiredWindow) return false;

    tapeWindowEndSerial = (double) writeSerial - kMinimumReadableDistance;
    tapeWindowStartSerial = tapeWindowEndSerial - requiredWindow;
    if (! canRead (tapeWindowStartSerial) || ! canRead (tapeWindowEndSerial - 1.0)) return false;

    tapeReadSerial = tapeWindowStartSerial;
    tapeBrakeCaptured = true;
    return true;
}

TimelineScratchEngine::BackspinReadState TimelineScratchEngine::getBackspinReadState() const noexcept
{
    const auto speed = juce::jlimit (0.25f, 4.0f, activeSlot.speed);
    return { activeSlot.preset == Preset::backspin && backspinCaptured,
             backspinWrapProgress < backspinWrapSamples,
             backspinWindowSamples,
             backspinWindowStartSerial,
             backspinWindowEndSerial,
             backspinPrimaryReadSerial,
             backspinSecondaryReadSerial,
             historyValidSamples(),
             -(double) speed,
             wetRamp,
             backspinCompletedWraps };
}

TimelineScratchEngine::Diagnostics TimelineScratchEngine::getDiagnostics() const noexcept
{
    return { activeBar, activeSlot.preset, diagnosticEffectPhase, historyValidSamples(),
             backspinWindowSamples, diagnosticReadPosition, diagnosticReadRate,
             backspinCompletedWraps, transportResetCount, diagnosticEffectiveWet,
             lastDiscontinuityReason };
}

double TimelineScratchEngine::tapeBrakePlaybackRate (double effectPhase, float speed) noexcept
{
    const auto phase = juce::jlimit (0.0, 1.0, effectPhase);
    const auto safeSpeed = juce::jlimit (0.25f, 4.0f, speed);
    const auto exponent = 0.75 + 0.25 * (double) safeSpeed;
    return kTapeBrakeStartRate * std::pow (1.0 - phase, exponent);
}

bool TimelineScratchEngine::isWetReady() const noexcept
{
    return (activeSlot.preset == Preset::backspin && backspinCaptured)
        || (activeSlot.preset == Preset::tapeBrake && tapeBrakeCaptured);
}

float TimelineScratchEngine::wetSample (int channel, double barPhase, double quartersPerBar) noexcept
{
    const auto elapsedQuarters = barPhase * quartersPerBar;
    if (elapsedQuarters >= activeDurationQuarters || activeDurationQuarters <= 0.0) return 0.0f;

    if (activeSlot.preset == Preset::backspin && backspinCaptured)
    {
        diagnosticReadPosition = backspinPrimaryReadSerial;
        diagnosticReadRate = -(double) juce::jlimit (0.25f, 4.0f, activeSlot.speed);
        const auto primary = read (history[(size_t) channel], backspinPrimaryReadSerial);
        if (backspinWrapProgress >= backspinWrapSamples) return primary;
        const auto secondary = read (history[(size_t) channel], backspinSecondaryReadSerial);
        const auto t = (float) backspinWrapProgress / (float) backspinWrapSamples;
        const auto gainA = std::cos (t * juce::MathConstants<float>::halfPi);
        const auto gainB = std::sin (t * juce::MathConstants<float>::halfPi);
        return primary * gainA + secondary * gainB;
    }

    if (activeSlot.preset == Preset::tapeBrake && tapeBrakeCaptured)
    {
        diagnosticReadPosition = tapeReadSerial;
        diagnosticReadRate = tapeBrakePlaybackRate (activeDurationQuarters > 0.0
            ? elapsedQuarters / activeDurationQuarters : 1.0, activeSlot.speed);
        return read (history[(size_t) channel], tapeReadSerial);
    }
    return 0.0f;
}

void TimelineScratchEngine::advanceBackspinReadHeads() noexcept
{
    const auto speed = (double) juce::jlimit (0.25f, 4.0f, activeSlot.speed);
    if (backspinWrapProgress >= backspinWrapSamples
        && backspinPrimaryReadSerial - speed * (double) backspinWrapSamples <= backspinWindowStartSerial)
    {
        backspinSecondaryReadSerial = backspinWindowEndSerial - 1.0;
        backspinWrapProgress = 0;
    }

    backspinPrimaryReadSerial = juce::jmax (backspinWindowStartSerial,
                                             backspinPrimaryReadSerial - speed);
    if (backspinWrapProgress < backspinWrapSamples)
    {
        backspinSecondaryReadSerial = juce::jmax (backspinWindowStartSerial,
                                                   backspinSecondaryReadSerial - speed);
        if (++backspinWrapProgress >= backspinWrapSamples)
        {
            backspinPrimaryReadSerial = backspinSecondaryReadSerial;
            ++backspinCompletedWraps;
        }
    }
}

void TimelineScratchEngine::process (juce::AudioBuffer<float>& buffer, const Transport& transport,
                                     const std::array<Slot, 64>& slots) noexcept
{
    if (historySamples <= 0 || sampleRateHz <= 0.0) return;
    if (transport.discontinuity || ! transport.playing || transport.startBar < 0)
    {
        ++transportResetCount;
        lastDiscontinuityReason = transport.discontinuity ? transport.discontinuityReason
            : DiscontinuityReason::stopped;
        resetTransport();
    }

    const auto rampStep = 1.0f / juce::jmax (1.0f, (float) sampleRateHz * kWetRampSeconds);
    const auto channels = juce::jmin (channelCount, buffer.getNumChannels());
    for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        const auto progressed = transport.startBarPhase + transport.barPhasePerSample * (double) sampleIndex;
        const auto wholeBars = (int) std::floor (progressed);
        const auto phase = progressed - std::floor (progressed);
        const auto bar = transport.playing ? (transport.startBar + wholeBars + 64) % 64 : -1;
        if (bar != activeBar && bar >= 0)
        {
            if (wetRamp > 0.001f) waitingForDry = true;
            else beginBar (bar, slots[(size_t) bar], transport.quartersPerBar, transport.secondsPerQuarter);
        }

        const auto elapsedQuarters = phase * transport.quartersPerBar;
        diagnosticEffectPhase = activeDurationQuarters > 0.0 ? elapsedQuarters / activeDurationQuarters : 0.0;
        const auto effectActive = activeBar >= 0 && activeSlot.preset != Preset::off
            && elapsedQuarters < activeDurationQuarters;
        if (effectActive && ! waitingForDry)
        {
            if (activeSlot.preset == Preset::backspin) beginBackspinCapture();
            if (activeSlot.preset == Preset::tapeBrake) beginTapeBrakeCapture();
        }
        const auto targetWet = effectActive && ! waitingForDry && isWetReady() ? 1.0f : 0.0f;
        wetRamp += juce::jlimit (-rampStep, rampStep, targetWet - wetRamp);

        if (waitingForDry && wetRamp <= 0.0f && bar >= 0)
        {
            waitingForDry = false;
            beginBar (bar, slots[(size_t) bar], transport.quartersPerBar, transport.secondsPerQuarter);
        }

        for (int channel = 0; channel < channels; ++channel)
        {
            const auto dry = buffer.getSample (channel, sampleIndex);
            const auto effectiveWet = juce::jlimit (0.0f, 1.0f, activeSlot.depth) * wetRamp;
            diagnosticEffectiveWet = effectiveWet;
            if (effectiveWet > 0.0f)
            {
                const auto wet = wetSample (channel, phase, transport.quartersPerBar);
                buffer.setSample (channel, sampleIndex, dry * (1.0f - effectiveWet) + wet * effectiveWet);
            }
            write (channel, dry);
        }

        if (activeSlot.preset == Preset::tapeBrake && effectActive && tapeBrakeCaptured && ! waitingForDry)
        {
            const auto effectPhase = activeDurationQuarters > 0.0 ? elapsedQuarters / activeDurationQuarters : 1.0;
            tapeReadSerial = juce::jmin (tapeWindowEndSerial - 1.0,
                                         tapeReadSerial + tapeBrakePlaybackRate (effectPhase, activeSlot.speed));
        }
        if (activeSlot.preset == Preset::backspin && effectActive && backspinCaptured && ! waitingForDry)
            advanceBackspinReadHeads();
        ++writeSerial;
    }
}
