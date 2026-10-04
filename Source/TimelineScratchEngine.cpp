#include "TimelineScratchEngine.h"
#include <cmath>

namespace
{
constexpr float kWetRampSeconds = 0.008f;
constexpr double kMinimumReadableDistance = 2.0;
constexpr double kBackspinWrapSeconds = 0.008;
constexpr double kTapeBrakeStartRate = 0.92;
constexpr double kForwardCutWindowSeconds = 0.125;
constexpr double kForwardCutFadeSeconds = 0.005;
constexpr double kForwardCutSilenceHoldSeconds = 0.030;
constexpr float kForwardCutSilenceThreshold = 1.0e-4f;
constexpr double kDragMinimumWindowSeconds = 0.125;
constexpr double kMotionCaptureWindowSeconds = 0.125;
constexpr double kMotionGateFadeSeconds = 0.003;
}

void TimelineScratchEngine::prepare (double rate, int, int channels)
{
    sampleRateHz = juce::jmax (1.0, rate);
    channelCount = juce::jlimit (1, 2, channels);
    historySamples = juce::jmax (8, juce::roundToInt (sampleRateHz * maxHistorySeconds));
    for (auto& channel : history) channel.assign ((size_t) historySamples, 0.0f);
    writeSerial = minimumReadableSerial = 0;
    activeBar = -1; wetRamp = 0.0f; waitingForDry = false;
    backspinCaptured = tapeBrakeCaptured = babyCaptured = forwardCaptured = dragCaptured = false;
    backspinWindowSamples = backspinWindowStartSerial = backspinWindowEndSerial = 0.0;
    backspinPrimaryReadSerial = backspinSecondaryReadSerial = 0.0;
    tapeReadSerial = tapeWindowStartSerial = tapeWindowEndSerial = 0.0;
    babyWindowSamples = babyWindowStartSerial = babyWindowEndSerial = 0.0;
    babyReadSerial = babyCycleSample = 0.0;
    dragWindowSamples = dragWindowStartSerial = dragWindowEndSerial = dragReadSerial = 0.0;
    forwardWindowSamples = forwardWindowStartSerial = forwardWindowEndSerial = forwardReadSerial = 0.0;
    forwardSecondaryReadSerial = 0.0;
    forwardCycleSamples = forwardReadSamples = forwardFadeSamples = forwardCyclePosition = 0.0;
    forwardSourceSilenceSamples = 0.0;
    motionWindowSamples = motionWindowStartSerial = motionWindowEndSerial = 0.0;
    motionPhase = motionCycleSamples = motionSourceSilenceSamples = 0.0;
    motionCaptured = motionSourceEnded = motionSourceHasSignal = false;
    forwardSourceEnded = false;
    forwardSourceHasSignal = false;
    backspinWrapSamples = backspinWrapProgress = 1;
    forwardWrapSamples = forwardWrapProgress = 1;
    backspinCompletedWraps = forwardCompletedWraps = 0;
}

void TimelineScratchEngine::release()
{
    for (auto& channel : history) { channel.clear(); channel.shrink_to_fit(); }
    historySamples = channelCount = 0; sampleRateHz = 0.0;
    writeSerial = minimumReadableSerial = 0; activeBar = -1; wetRamp = 0.0f;
    backspinCaptured = tapeBrakeCaptured = babyCaptured = forwardCaptured = dragCaptured = false;
    backspinWindowSamples = backspinWindowStartSerial = backspinWindowEndSerial = 0.0;
    backspinPrimaryReadSerial = backspinSecondaryReadSerial = 0.0;
    tapeReadSerial = tapeWindowStartSerial = tapeWindowEndSerial = 0.0;
    babyWindowSamples = babyWindowStartSerial = babyWindowEndSerial = 0.0;
    babyReadSerial = babyCycleSample = 0.0;
    dragWindowSamples = dragWindowStartSerial = dragWindowEndSerial = dragReadSerial = 0.0;
    forwardWindowSamples = forwardWindowStartSerial = forwardWindowEndSerial = forwardReadSerial = 0.0;
    forwardSecondaryReadSerial = 0.0;
    forwardCycleSamples = forwardReadSamples = forwardFadeSamples = forwardCyclePosition = 0.0;
    forwardSourceSilenceSamples = 0.0;
    motionWindowSamples = motionWindowStartSerial = motionWindowEndSerial = 0.0;
    motionPhase = motionCycleSamples = motionSourceSilenceSamples = 0.0;
    motionCaptured = motionSourceEnded = motionSourceHasSignal = false;
    forwardSourceEnded = false;
    forwardSourceHasSignal = false;
    backspinWrapSamples = backspinWrapProgress = 1;
    forwardWrapSamples = forwardWrapProgress = 1;
    backspinCompletedWraps = forwardCompletedWraps = 0;
}

void TimelineScratchEngine::resetTransport (bool preserveHistory) noexcept
{
    if (! preserveHistory)
        minimumReadableSerial = writeSerial;
    activeBar = -1; wetRamp = 0.0f; waitingForDry = false;
    backspinCaptured = tapeBrakeCaptured = babyCaptured = forwardCaptured = dragCaptured = false;
    forwardSourceEnded = false;
    forwardSourceHasSignal = false;
    forwardSourceSilenceSamples = 0.0;
    motionSourceSilenceSamples = 0.0;
    motionSourceEnded = motionSourceHasSignal = motionCaptured = false;
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
    backspinCaptured = tapeBrakeCaptured = babyCaptured = forwardCaptured = dragCaptured = false;
    forwardSourceEnded = false;
    forwardSourceHasSignal = false;
    forwardSourceSilenceSamples = 0.0;
    backspinCompletedWraps = 0;
    const auto speed = juce::jlimit (0.25f, 4.0f, slot.speed);
    backspinWindowSamples = juce::jlimit (sampleRateHz * 0.125, sampleRateHz * 0.500,
        sampleRateHz * (0.250 / std::sqrt ((double) speed)));
    backspinWrapSamples = juce::jmax (1, juce::roundToInt (sampleRateHz * kBackspinWrapSeconds));
    backspinWrapProgress = backspinWrapSamples;
    babyWindowSamples = sampleRateHz * 0.250;
    babyCycleSample = 0.0;
    const auto dragSpeed = juce::jlimit (0.25f, 4.0f, slot.speed);
    const auto dragRate = juce::jlimit (0.25, 0.85, 0.45 * std::sqrt ((double) dragSpeed));
    dragWindowSamples = juce::jmin ((double) historySamples - 4.0,
                                    juce::jmax (sampleRateHz * kDragMinimumWindowSeconds,
                                                activeDurationSamples * dragRate + kMinimumReadableDistance + 1.0));
    dragWindowStartSerial = dragWindowEndSerial = dragReadSerial = 0.0;
    forwardWindowSamples = sampleRateHz * kForwardCutWindowSeconds;
    const auto cycleSeconds = 0.125 / (double) speed;
    forwardCycleSamples = juce::jmax (1.0, sampleRateHz * cycleSeconds);
    const auto minimumSilenceSamples = juce::jmax (sampleRateHz * 0.005,
                                                   forwardCycleSamples * 0.15);
    forwardReadSamples = juce::jmax (0.0, juce::jmin (forwardWindowSamples,
                                                      forwardCycleSamples - minimumSilenceSamples));
    forwardFadeSamples = juce::jmin (sampleRateHz * kForwardCutFadeSeconds,
                                     juce::jmax (1.0, forwardReadSamples));
    forwardCyclePosition = 0.0;
    forwardSourceSilenceSamples = 0.0;
    forwardSourceEnded = false;
    forwardSourceHasSignal = false;
    forwardWrapSamples = juce::jmax (1, juce::roundToInt (sampleRateHz * kBackspinWrapSeconds));
    forwardWrapProgress = forwardWrapSamples;
    forwardCompletedWraps = 0;
    motionWindowSamples = sampleRateHz * kMotionCaptureWindowSeconds;
    motionWindowStartSerial = motionWindowEndSerial = 0.0;
    motionPhase = 0.0;
    motionCycleSamples = juce::jmax (1.0,
        secondsPerQuarter * 0.5 * sampleRateHz / (double) speed);
    motionSourceSilenceSamples = 0.0;
    motionCaptured = motionSourceEnded = motionSourceHasSignal = false;
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

bool TimelineScratchEngine::beginBabyCapture() noexcept
{
    if (babyCaptured || activeSlot.preset != Preset::baby) return babyCaptured;
    if (historyValidSamples() < babyWindowSamples + kMinimumReadableDistance + 2.0) return false;

    babyWindowEndSerial = (double) writeSerial - kMinimumReadableDistance;
    babyWindowStartSerial = babyWindowEndSerial - babyWindowSamples;
    if (! canRead (babyWindowStartSerial) || ! canRead (babyWindowEndSerial)) return false;

    babyReadSerial = babyWindowStartSerial;
    babyCycleSample = 0.0;
    babyCaptured = true;
    return true;
}

bool TimelineScratchEngine::beginForwardCutCapture() noexcept
{
    if (forwardCaptured || activeSlot.preset != Preset::forwardCut) return forwardCaptured;
    const auto required = forwardWindowSamples + kMinimumReadableDistance + 2.0;
    if (! forwardSourceHasSignal || historyValidSamples() < required) return false;

    forwardWindowEndSerial = (double) writeSerial - kMinimumReadableDistance;
    forwardWindowStartSerial = forwardWindowEndSerial - forwardWindowSamples;
    if (! canRead (forwardWindowStartSerial) || ! canRead (forwardWindowEndSerial - 1.0)) return false;

    forwardReadSerial = forwardWindowStartSerial;
    forwardSecondaryReadSerial = forwardWindowStartSerial;
    forwardCyclePosition = 0.0;
    forwardWrapProgress = forwardWrapSamples;
    forwardCaptured = true;
    return true;
}

bool TimelineScratchEngine::beginDragCapture() noexcept
{
    if (dragCaptured || activeSlot.preset != Preset::drag) return dragCaptured;
    if (dragWindowSamples <= 0.0) return false;

    // DRAG is anchored at the effect entry, not at a long retrospective
    // snapshot.  This prevents a BAR loop from replaying the previous BAR's
    // entire history while keeping the two-sample realtime safety margin.
    dragWindowStartSerial = (double) writeSerial - kMinimumReadableDistance;
    dragWindowEndSerial = dragWindowStartSerial + dragWindowSamples;
    if (! canRead (dragWindowStartSerial)) return false;

    dragReadSerial = dragWindowStartSerial;
    dragCaptured = true;
    return true;
}

bool TimelineScratchEngine::beginMotionCapture() noexcept
{
    const auto presetId = static_cast<int> (activeSlot.preset);
    if (motionCaptured || presetId < static_cast<int> (Preset::chirp)
        || presetId > static_cast<int> (Preset::zigzag))
        return motionCaptured;
    const auto required = motionWindowSamples + kMinimumReadableDistance + 2.0;
    if (! motionSourceHasSignal || historyValidSamples() < required) return false;

    motionWindowEndSerial = (double) writeSerial - kMinimumReadableDistance;
    motionWindowStartSerial = motionWindowEndSerial - motionWindowSamples;
    if (! canRead (motionWindowStartSerial) || ! canRead (motionWindowEndSerial - 1.0))
        return false;

    motionPhase = 0.0;
    motionCaptured = true;
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
    const auto captureWindow = activeSlot.preset == Preset::baby ? babyWindowSamples
        : activeSlot.preset == Preset::forwardCut ? forwardWindowSamples
        : activeSlot.preset == Preset::drag ? dragWindowSamples
        : (static_cast<int> (activeSlot.preset) >= static_cast<int> (Preset::chirp)
            && static_cast<int> (activeSlot.preset) <= static_cast<int> (Preset::zigzag))
            ? motionWindowSamples : backspinWindowSamples;
    return { activeBar, activeSlot.preset, diagnosticEffectPhase, historyValidSamples(),
             captureWindow,
             diagnosticReadPosition, diagnosticReadRate,
             backspinCompletedWraps, transportResetCount, diagnosticEffectiveWet,
             lastDiscontinuityReason, forwardCycleSamples, forwardReadSamples,
             forwardCyclePosition, forwardCompletedWraps, forwardSourceEnded,
             motionSourceEnded };
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
    return (activeSlot.preset == Preset::forwardCut && forwardCaptured)
        || (activeSlot.preset == Preset::backspin && backspinCaptured)
        || (activeSlot.preset == Preset::tapeBrake && tapeBrakeCaptured)
        || (activeSlot.preset == Preset::baby && babyCaptured
            && canRead (babyWindowStartSerial) && canRead (babyWindowEndSerial))
        || (activeSlot.preset == Preset::drag && dragCaptured
            && canRead (dragReadSerial))
        || ((static_cast<int> (activeSlot.preset) >= static_cast<int> (Preset::chirp)
             && static_cast<int> (activeSlot.preset) <= static_cast<int> (Preset::zigzag))
            && motionCaptured && canRead (motionWindowStartSerial));
}

float TimelineScratchEngine::wetSample (int channel, double barPhase, double quartersPerBar) noexcept
{
    const auto elapsedQuarters = barPhase * quartersPerBar;
    // BABY keeps its last valid wet sample during the short release ramp.
    // Returning zero here would cause a full-scale dip at LENGTH expiry.
    if ((elapsedQuarters >= activeDurationQuarters || activeDurationQuarters <= 0.0)
        && ! (activeSlot.preset == Preset::baby && babyCaptured && wetRamp > 0.0f))
        return 0.0f;

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

    if (activeSlot.preset == Preset::forwardCut && forwardCaptured)
    {
        diagnosticReadPosition = forwardReadSerial;
        diagnosticReadRate = 1.0;
        if (forwardCyclePosition >= forwardReadSamples)
            return 0.0f;

        const auto source = read (history[(size_t) channel], forwardReadSerial);
        if (forwardCyclePosition < forwardFadeSamples)
        {
            const auto t = (float) (forwardCyclePosition / juce::jmax (1.0, forwardFadeSamples));
            return source * std::sin (t * juce::MathConstants<float>::halfPi);
        }
        const auto fadeStart = juce::jmax (0.0, forwardReadSamples - forwardFadeSamples);
        if (forwardCyclePosition >= fadeStart)
        {
            const auto t = (float) ((forwardCyclePosition - fadeStart)
                                    / juce::jmax (1.0, forwardFadeSamples));
            return source * std::cos (t * juce::MathConstants<float>::halfPi);
        }
        return source;
    }

    if (activeSlot.preset == Preset::tapeBrake && tapeBrakeCaptured)
    {
        diagnosticReadPosition = tapeReadSerial;
        diagnosticReadRate = tapeBrakePlaybackRate (activeDurationQuarters > 0.0
            ? elapsedQuarters / activeDurationQuarters : 1.0, activeSlot.speed);
        return read (history[(size_t) channel], tapeReadSerial);
    }
    if (activeSlot.preset == Preset::baby && babyCaptured)
    {
        diagnosticReadPosition = babyReadSerial;
        const auto angle = juce::MathConstants<double>::pi * babyCycleSample / babyWindowSamples;
        diagnosticReadRate = 0.5 * juce::MathConstants<double>::pi
            * (double) juce::jlimit (0.25f, 4.0f, activeSlot.speed) * std::sin (angle);
        return read (history[(size_t) channel], babyReadSerial);
    }
    if (activeSlot.preset == Preset::drag && dragCaptured)
    {
        const auto speed = (double) juce::jlimit (0.25f, 4.0f, activeSlot.speed);
        diagnosticReadPosition = dragReadSerial;
        diagnosticReadRate = juce::jlimit (0.25, 0.85, 0.45 * std::sqrt (speed));
        return read (history[(size_t) channel], dragReadSerial);
    }
    const auto motionPreset = static_cast<int> (activeSlot.preset) >= static_cast<int> (Preset::chirp)
        && static_cast<int> (activeSlot.preset) <= static_cast<int> (Preset::zigzag);
    if (motionPreset && motionCaptured)
    {
        const auto u = juce::jlimit (0.0, 1.0,
                                     motionCycleSamples > 0.0
                                         ? motionPhase / motionCycleSamples : 0.0);
        double position = 0.0;
        double positionRate = 0.0;
        float gate = 1.0f;

        if (activeSlot.preset == Preset::zigzag)
        {
            const auto segment = juce::jmin (3, (int) std::floor (u * 4.0));
            const auto t = u * 4.0 - (double) segment;
            constexpr double points[] { 0.0, 1.0, 0.25, 0.75, 0.0 };
            const auto eased = 0.5 * (1.0 - std::cos (juce::MathConstants<double>::pi * t));
            position = points[segment] + (points[segment + 1] - points[segment]) * eased;
            positionRate = (points[segment + 1] - points[segment])
                * 2.0 * juce::MathConstants<double>::pi * std::sin (juce::MathConstants<double>::pi * t)
                * juce::jmax (0.0, motionWindowSamples - 1.0)
                / juce::jmax (1.0, motionCycleSamples);
        }
        else
        {
            position = 0.5 * (1.0 - std::cos (2.0 * juce::MathConstants<double>::pi * u));
            positionRate = juce::MathConstants<double>::pi
                * std::sin (2.0 * juce::MathConstants<double>::pi * u)
                * juce::jmax (0.0, motionWindowSamples - 1.0)
                / juce::jmax (1.0, motionCycleSamples);
            const auto gateUnits = activeSlot.preset == Preset::chirp ? 2.0 : 8.0;
            const auto local = std::fmod (u * gateUnits, 1.0);
            const auto openLimit = activeSlot.preset == Preset::chirp ? 0.8 : 0.5;
            const auto segmentSamples = motionCycleSamples / gateUnits;
            const auto fadeSamples = juce::jmin (sampleRateHz * kMotionGateFadeSeconds,
                                                  segmentSamples * 0.25);
            if (local >= openLimit)
                gate = 0.0f;
            else if (fadeSamples > 1.0 && local * segmentSamples < fadeSamples)
                gate = (float) (local * segmentSamples / fadeSamples);
            else if (fadeSamples > 1.0 && (openLimit - local) * segmentSamples < fadeSamples)
                gate = (float) ((openLimit - local) * segmentSamples / fadeSamples);
        }

        const auto serial = motionWindowStartSerial
            + position * juce::jmax (0.0, motionWindowSamples - 1.0);
        diagnosticReadPosition = serial;
        diagnosticReadRate = positionRate;
        return read (history[(size_t) channel], serial) * gate;
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

void TimelineScratchEngine::advanceBabyReadHead() noexcept
{
    const auto speed = (double) juce::jlimit (0.25f, 4.0f, activeSlot.speed);
    babyCycleSample = std::fmod (babyCycleSample + speed, 2.0 * babyWindowSamples);
    const auto angle = juce::MathConstants<double>::pi * babyCycleSample / babyWindowSamples;
    babyReadSerial = babyWindowStartSerial + 0.5 * babyWindowSamples * (1.0 - std::cos (angle));
}

void TimelineScratchEngine::advanceForwardCutReadHead() noexcept
{
    if (forwardCycleSamples <= 0.0) return;
    ++forwardCyclePosition;
    if (forwardCyclePosition >= forwardCycleSamples)
    {
        forwardCyclePosition = 0.0;
        forwardReadSerial = forwardWindowStartSerial;
        forwardSecondaryReadSerial = forwardWindowStartSerial;
        ++forwardCompletedWraps;
    }
    else if (forwardCyclePosition < forwardReadSamples)
        forwardReadSerial = juce::jmin (forwardWindowEndSerial - 1.0,
                                        forwardWindowStartSerial + forwardCyclePosition);
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
        resetTransport (transport.discontinuityReason == DiscontinuityReason::loopWrap);
    }

    const auto rampStep = 1.0f / juce::jmax (1.0f, (float) sampleRateHz * kWetRampSeconds);
    const auto channels = juce::jmin (channelCount, buffer.getNumChannels());
    bool loopBoundaryApplied = false;
    for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        if (! loopBoundaryApplied && transport.loopBoundarySample >= 0
            && sampleIndex >= transport.loopBoundarySample)
        {
            resetTransport (true);
            loopBoundaryApplied = true;
        }
        const auto dryLeft = channels > 0 ? buffer.getSample (0, sampleIndex) : 0.0f;
        const auto dryRight = channels > 1 ? buffer.getSample (1, sampleIndex) : dryLeft;
        if (activeSlot.preset == Preset::forwardCut && ! forwardSourceEnded && ! waitingForDry)
        {
            if (std::abs (dryLeft) < kForwardCutSilenceThreshold
                && std::abs (dryRight) < kForwardCutSilenceThreshold)
                forwardSourceSilenceSamples += 1.0;
            else
            {
                forwardSourceHasSignal = true;
                forwardSourceSilenceSamples = 0.0;
            }

            if (forwardSourceSilenceSamples >= sampleRateHz * kForwardCutSilenceHoldSeconds)
                forwardSourceEnded = true;
        }
        const auto localSample = loopBoundaryApplied
            ? sampleIndex - transport.loopBoundarySample : sampleIndex;
        const auto progressed = (loopBoundaryApplied ? transport.loopStartPhase
                                                      : transport.startBarPhase)
            + transport.barPhasePerSample * (double) localSample;
        const auto wholeBars = (int) std::floor (progressed);
        const auto phase = progressed - std::floor (progressed);
        const auto bar = transport.playing
            ? ((loopBoundaryApplied ? transport.loopStartBar : transport.startBar) + wholeBars + 64) % 64
            : -1;
        if (bar != activeBar && bar >= 0)
        {
            if (wetRamp > 0.001f) waitingForDry = true;
            else beginBar (bar, slots[(size_t) bar], transport.quartersPerBar, transport.secondsPerQuarter);
        }

        const auto motionPreset = static_cast<int> (activeSlot.preset) >= static_cast<int> (Preset::chirp)
            && static_cast<int> (activeSlot.preset) <= static_cast<int> (Preset::zigzag);
        if (motionPreset && ! motionSourceEnded && ! waitingForDry)
        {
            if (std::abs (dryLeft) < kForwardCutSilenceThreshold
                && std::abs (dryRight) < kForwardCutSilenceThreshold)
                motionSourceSilenceSamples += 1.0;
            else
            {
                motionSourceHasSignal = true;
                motionSourceSilenceSamples = 0.0;
            }

            if (motionSourceSilenceSamples >= sampleRateHz * kForwardCutSilenceHoldSeconds)
                motionSourceEnded = true;
        }

        const auto elapsedQuarters = phase * transport.quartersPerBar;
        diagnosticEffectPhase = activeDurationQuarters > 0.0 ? elapsedQuarters / activeDurationQuarters : 0.0;
        const auto effectActive = activeBar >= 0 && activeSlot.preset != Preset::off
            && elapsedQuarters < activeDurationQuarters;
        if (effectActive && ! waitingForDry && ! (activeSlot.preset == Preset::forwardCut && forwardSourceEnded))
        {
            if (activeSlot.preset == Preset::forwardCut) beginForwardCutCapture();
            if (activeSlot.preset == Preset::backspin) beginBackspinCapture();
            if (activeSlot.preset == Preset::tapeBrake) beginTapeBrakeCapture();
            if (activeSlot.preset == Preset::baby) beginBabyCapture();
            if (activeSlot.preset == Preset::drag) beginDragCapture();
            if (motionPreset) beginMotionCapture();
        }
        const auto sourceEnded = (activeSlot.preset == Preset::forwardCut && forwardSourceEnded)
            || (motionPreset && motionSourceEnded);
        const auto targetWet = effectActive && ! waitingForDry && ! sourceEnded && isWetReady() ? 1.0f : 0.0f;
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
        if (activeSlot.preset == Preset::baby && effectActive && babyCaptured && ! waitingForDry)
            advanceBabyReadHead();
        if (activeSlot.preset == Preset::forwardCut && effectActive && forwardCaptured
            && ! waitingForDry && ! forwardSourceEnded)
            advanceForwardCutReadHead();
        if (activeSlot.preset == Preset::drag && effectActive && dragCaptured && ! waitingForDry)
        {
            const auto dragRate = juce::jlimit (0.25, 0.85,
                                                0.45 * std::sqrt ((double) juce::jlimit (0.25f, 4.0f, activeSlot.speed)));
            dragReadSerial = juce::jmin (dragWindowEndSerial - 1.0,
                                         dragReadSerial + dragRate);
        }
        if (motionPreset && effectActive && motionCaptured && ! waitingForDry && ! motionSourceEnded)
            motionPhase = std::fmod (motionPhase + 1.0, juce::jmax (1.0, motionCycleSamples));
        ++writeSerial;
    }
}
