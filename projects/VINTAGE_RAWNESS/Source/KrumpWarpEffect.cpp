#include "KrumpWarpEffect.h"

void KrumpWarpEffect::prepare(double sampleRate, int maximumBlockSize, int maximumChannels)
{
    sampleRateHz = static_cast<float>(sampleRate > 0.0 ? sampleRate : kDefaultSampleRate);
    activeChannels = juce::jlimit(1, kMaxChannels, maximumChannels);
    delayBufferSamples = juce::jmax(8, static_cast<int>(sampleRateHz * kMaxDelayMs * 0.001f) + juce::jmax(1, maximumBlockSize));
    delayBuffer.setSize(kMaxChannels, delayBufferSamples, false, false, true);

    const auto attackSeconds = kAttackMs * 0.001f;
    const auto releaseSeconds = kReleaseMs * 0.001f;
    attackCoeff = std::exp(-1.0f / (attackSeconds * sampleRateHz));
    releaseCoeff = std::exp(-1.0f / (releaseSeconds * sampleRateHz));

    dirtSmooth.reset(sampleRateHz, kParamSmoothingSeconds);
    crushSmooth.reset(sampleRateHz, kParamSmoothingSeconds);
    wobbleSmooth.reset(sampleRateHz, kParamSmoothingSeconds);
    dirtSmooth.setCurrentAndTargetValue(0.30f);
    crushSmooth.setCurrentAndTargetValue(0.20f);
    wobbleSmooth.setCurrentAndTargetValue(0.10f);

    reset();
}

void KrumpWarpEffect::reset() noexcept
{
    delayBuffer.clear();
    writeIndex = 0;
    envelopeState = 0.0f;
    randomState = 0x5f3759dfu;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        delayTimeState[ch] = sampleRateHz * kBaseDelayMs * 0.001f;
        flangePhase[ch] = ch == 0 ? 0.0f : 0.37f;
        flangeFeedback[ch] = 0.0f;
        flutterNoise[ch] = 0.0f;
        slopNoise[ch] = 0.0f;
        heldSample[ch] = 0.0f;
        crushedState[ch] = 0.0f;
        holdCounter[ch] = 0;
        preLowState[ch] = 0.0f;
        preMidState[ch] = 0.0f;
        postHighPassLowState[ch] = 0.0f;
        postLowPass1[ch] = 0.0f;
        postLowPass2[ch] = 0.0f;
        postLowPass3[ch] = 0.0f;
        postLowPass4[ch] = 0.0f;
        dcX1[ch] = 0.0f;
        dcY1[ch] = 0.0f;
    }
}

float KrumpWarpEffect::sanitize(float value) noexcept
{
    return std::isfinite(value) ? value : 0.0f;
}

float KrumpWarpEffect::fastClip(float value) noexcept
{
    return juce::jlimit(-kOutputCeiling, kOutputCeiling, sanitize(value));
}

float KrumpWarpEffect::nextRandomBipolar(uint32_t& state) noexcept
{
    state = state * 1664525u + 1013904223u;
    const auto mantissa = static_cast<float>((state >> 8) & 0x00ffffffu) / 8388607.5f;
    return mantissa - 1.0f;
}

float KrumpWarpEffect::preEq(float sample, float dirt, int channel) noexcept
{
    const auto lowCoeff = onePoleCoefficient(kPreEqLowHz, sampleRateHz);
    const auto midCoeff = onePoleCoefficient(kPreEqMidHz, sampleRateHz);
    preLowState[channel] += lowCoeff * (sample - preLowState[channel]);
    preMidState[channel] += midCoeff * (sample - preMidState[channel]);
    const auto lowMidBand = preMidState[channel] - preLowState[channel];
    const auto boost = 0.45f + dirt * 1.25f;
    const auto preGain = 1.0f + dirt * 1.8f;
    return fastClip((sample + lowMidBand * boost) * preGain);
}

float KrumpWarpEffect::saturate(float sample, float dirt, float envelope) noexcept
{
    const auto drive = 1.0f + dirt * dirt * 45.0f + envelope * dirt * 15.0f;
    const auto shape = 1.0f + dirt * 8.0f;
    const auto bias = dirt * (0.16f + envelope * 0.42f);
    const auto driven = sample * drive;
    const auto asymmetric = std::tanh((driven + bias) * shape) - std::tanh(bias * shape);
    const auto tape = asymmetric / (1.0f + std::abs(asymmetric) * (0.25f + dirt * 1.75f));
    const auto squared = std::tanh(tape * (1.0f + dirt * 12.0f));
    const auto folded = squared - 0.32f * dirt * std::sin(squared * juce::MathConstants<float>::pi);
    const auto trim = 1.0f / (1.0f + dirt * 1.35f);
    return fastClip(folded * trim);
}

float KrumpWarpEffect::crushSample(float sample, float crush, float envelope, int channel) noexcept
{
    const auto roughness = juce::jlimit(0.0f, 1.0f, crush * (0.88f + envelope * 0.18f));
    const auto shapedRoughness = std::sqrt(roughness);
    const auto preTrim = 1.0f / (1.0f + roughness * 0.45f);
    const auto conditionerDrive = 1.0f + roughness * 0.45f;
    const auto conditioned = std::tanh(sample * preTrim * conditionerDrive) / std::tanh(conditionerDrive);

    const auto bits = juce::jmap(shapedRoughness, 16.0f, 6.25f);
    const auto steps = std::pow(2.0f, bits);
    auto quantized = std::round(conditioned * steps) / steps;

    const auto maxHold = 1 + static_cast<int>(roughness * roughness * 18.0f);
    if (holdCounter[channel] <= 0)
    {
        heldSample[channel] = quantized;
        holdCounter[channel] = juce::jmax(1, maxHold);
    }

    --holdCounter[channel];
    quantized = juce::jmap(roughness, quantized, heldSample[channel]);

    const auto slewCoeff = juce::jmap(roughness, 0.62f, 0.18f);
    crushedState[channel] += slewCoeff * (quantized - crushedState[channel]);
    const auto fizz = nextRandomBipolar(randomState) * roughness * roughness * 0.006f;
    const auto degraded = std::tanh((crushedState[channel] + fizz) * (1.0f + roughness * 0.35f));
    const auto crushMix = juce::jmap(roughness, 0.18f, 0.84f);
    return sanitize(sample * (1.0f - crushMix) + degraded * crushMix);
}

float KrumpWarpEffect::dcBlock(float sample, int channel, float* x1, float* y1) noexcept
{
    const auto y = sample - x1[channel] + kDcBlockPole * y1[channel];
    x1[channel] = sample;
    y1[channel] = y;
    return sanitize(y);
}

float KrumpWarpEffect::onePoleCoefficient(float cutoffHz, float sampleRate) noexcept
{
    const auto clampedCutoff = juce::jlimit(40.0f, sampleRate * 0.45f, cutoffHz);
    return 1.0f - std::exp((-2.0f * juce::MathConstants<float>::pi * clampedCutoff) / sampleRate);
}

float KrumpWarpEffect::readModulatedDelay(int channel, float input, float wobble, float envelope) noexcept
{
    auto* delayData = delayBuffer.getWritePointer(channel);

    const auto phaseRate = juce::jmap(wobble, kFlangeMinHz, kFlangeMaxHz) * (1.0f + envelope * wobble * 0.38f);
    flangePhase[channel] += phaseRate / sampleRateHz;
    if (flangePhase[channel] >= 1.0f)
        flangePhase[channel] -= 1.0f;

    const auto phase = flangePhase[channel] + (channel == 0 ? 0.0f : 0.21f);
    const auto lfo = 0.5f + 0.5f * std::sin(phase * juce::MathConstants<float>::twoPi);
    const auto flutterCoeff = onePoleCoefficient(juce::jmap(wobble, kFlutterMinHz, kFlutterMaxHz), sampleRateHz);
    const auto slopCoeff = onePoleCoefficient(juce::jmap(wobble, 0.8f, 3.5f), sampleRateHz);
    flutterNoise[channel] += flutterCoeff * (nextRandomBipolar(randomState) - flutterNoise[channel]);
    slopNoise[channel] += slopCoeff * (nextRandomBipolar(randomState) - slopNoise[channel]);

    const auto feedbackAmount = wobble * (0.08f + envelope * 0.12f);
    delayData[writeIndex] = fastClip(input + flangeFeedback[channel] * feedbackAmount);

    const auto transientBend = envelope * envelope * wobble;
    const auto polarity = channel == 0 ? 1.0f : -1.0f;
    const auto flangeDepthMs = wobble * (2.2f + wobble * 4.8f) * (0.65f + envelope * 0.75f);
    const auto flutterDepthSamples = sampleRateHz * wobble * 0.0018f;
    const auto jitterMs = juce::jmap(wobble, kJitterMinMs, kJitterMaxMs);
    const auto jitterSamples = sampleRateHz * jitterMs * 0.001f * (0.15f + transientBend);
    const auto targetDelayMs = 1.15f + kBaseDelayMs * (0.28f + wobble * 0.72f)
        + lfo * flangeDepthMs
        - transientBend * 1.35f;
    const auto targetDelay = sampleRateHz * targetDelayMs * 0.001f
        + flutterNoise[channel] * flutterDepthSamples * polarity
        + slopNoise[channel] * jitterSamples;

    delayTimeState[channel] += kDelaySmooth * (targetDelay - delayTimeState[channel]);
    const auto delaySamples = juce::jlimit(2.0f, static_cast<float>(delayBufferSamples - 3), delayTimeState[channel]);
    auto readPosition = static_cast<float>(writeIndex) - delaySamples;
    while (readPosition < 0.0f)
        readPosition += static_cast<float>(delayBufferSamples);

    const auto index0 = static_cast<int>(readPosition) % delayBufferSamples;
    const auto index1 = (index0 + 1) % delayBufferSamples;
    const auto frac = readPosition - static_cast<float>(index0);
    const auto delayed = delayData[index0] + (delayData[index1] - delayData[index0]) * frac;
    flangeFeedback[channel] = sanitize(delayed);

    const auto flangeMix = wobble * (0.34f + envelope * 0.26f);
    const auto combed = input + delayed * flangeMix;
    return sanitize(combed / (1.0f + flangeMix * 0.55f));
}

float KrumpWarpEffect::postBandpass(float sample, float dirt, float crush, int channel) noexcept
{
    const auto hpCutoff = juce::jmap(juce::jlimit(0.0f, 1.0f, dirt * 0.35f + crush * 0.65f),
                                    kPostHighPassMinHz, kPostHighPassMaxHz);
    const auto lpCutoff = juce::jmap(juce::jlimit(0.0f, 1.0f, dirt * 0.45f + crush * 0.85f),
                                    kPostLowPassMaxHz, kPostLowPassMinHz);
    const auto hpCoeff = onePoleCoefficient(hpCutoff, sampleRateHz);
    const auto lpCoeff = onePoleCoefficient(lpCutoff, sampleRateHz);

    postHighPassLowState[channel] += hpCoeff * (sample - postHighPassLowState[channel]);
    auto filtered = sample - postHighPassLowState[channel];
    postLowPass1[channel] += lpCoeff * (filtered - postLowPass1[channel]);
    postLowPass2[channel] += lpCoeff * (postLowPass1[channel] - postLowPass2[channel]);
    postLowPass3[channel] += lpCoeff * (postLowPass2[channel] - postLowPass3[channel]);
    postLowPass4[channel] += lpCoeff * (postLowPass3[channel] - postLowPass4[channel]);
    return sanitize(postLowPass4[channel]);
}

void KrumpWarpEffect::processBlock(juce::AudioBuffer<float>& buffer, float dirt, float crush, float wobble) noexcept
{
    juce::ScopedNoDenormals noDenormals;

    dirtSmooth.setTargetValue(juce::jlimit(0.0f, 1.0f, dirt));
    crushSmooth.setTargetValue(juce::jlimit(0.0f, 1.0f, crush));
    wobbleSmooth.setTargetValue(juce::jlimit(0.0f, 1.0f, wobble));

    const auto numSamples = buffer.getNumSamples();
    const auto channelsToProcess = juce::jmin(buffer.getNumChannels(), activeChannels);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float detector = 0.0f;
        for (int ch = 0; ch < channelsToProcess; ++ch)
            detector = juce::jmax(detector, std::abs(buffer.getReadPointer(ch)[sample]));

        detector = juce::jlimit(0.0f, 1.0f, detector * 1.6f);
        const auto coeff = detector > envelopeState ? attackCoeff : releaseCoeff;
        envelopeState = coeff * envelopeState + (1.0f - coeff) * detector;
        envelopeState = juce::jlimit(0.0f, 1.0f, envelopeState);

        const auto dirtValue = dirtSmooth.getNextValue();
        const auto crushValue = crushSmooth.getNextValue();
        const auto wobbleValue = wobbleSmooth.getNextValue();

        for (int ch = 0; ch < channelsToProcess; ++ch)
        {
            auto* channel = buffer.getWritePointer(ch);
            const auto dry = sanitize(channel[sample]);
            auto wet = preEq(dry, dirtValue, ch);
            wet = saturate(wet, dirtValue, envelopeState);
            wet = crushSample(wet, crushValue, envelopeState, ch);
            wet = readModulatedDelay(ch, wet, wobbleValue, envelopeState);
            wet = postBandpass(wet, dirtValue, crushValue, ch);
            wet = fastClip(wet * (1.0f - crushValue * 0.10f));
            wet = dcBlock(wet, ch, dcX1, dcY1);
            channel[sample] = fastClip(dry * (1.0f - kDryWetMix) + wet * kDryWetMix);
        }

        writeIndex = (writeIndex + 1) % delayBufferSamples;
    }

    for (int ch = channelsToProcess; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);
}
