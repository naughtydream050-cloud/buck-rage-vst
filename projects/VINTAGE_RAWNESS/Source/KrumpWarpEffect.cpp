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
        heldSample[ch] = 0.0f;
        holdCounter[ch] = 0;
        postFilterState[ch] = 0.0f;
        dcX1[ch] = 0.0f;
        dcY1[ch] = 0.0f;
        instability[ch] = 0.0f;
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

float KrumpWarpEffect::saturate(float sample, float dirt, float envelope) noexcept
{
    const auto drive = 1.0f + dirt * dirt * 15.0f + envelope * dirt * 5.0f;
    const auto shape = 1.0f + dirt * 3.5f;
    const auto bias = dirt * (0.08f + envelope * 0.28f);
    const auto driven = sample * drive;
    const auto asymmetric = std::tanh((driven + bias) * shape) - std::tanh(bias * shape);
    const auto folded = asymmetric - 0.18f * dirt * std::sin(asymmetric * juce::MathConstants<float>::pi);
    const auto trim = 1.0f / (1.0f + dirt * 0.85f);
    return fastClip(folded * trim);
}

float KrumpWarpEffect::crushSample(float sample, float crush, float envelope, int channel) noexcept
{
    const auto roughness = juce::jlimit(0.0f, 1.0f, crush + envelope * crush * 0.35f);
    const auto bits = juce::jmap(roughness, 16.0f, 4.5f);
    const auto steps = std::pow(2.0f, bits);
    auto quantized = std::round(sample * steps) / steps;

    const auto maxHold = 1 + static_cast<int>(roughness * roughness * 34.0f);
    if (holdCounter[channel] <= 0)
    {
        heldSample[channel] = quantized;
        holdCounter[channel] = juce::jmax(1, maxHold);
    }

    --holdCounter[channel];
    quantized = juce::jmap(roughness, quantized, heldSample[channel]);
    return fastClip(quantized);
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

float KrumpWarpEffect::readWarpedDelay(int channel, float input, float wobble, float envelope) noexcept
{
    auto* delayData = delayBuffer.getWritePointer(channel);
    delayData[writeIndex] = input;

    instability[channel] += 0.0025f * (nextRandomBipolar(randomState) - instability[channel]);
    const auto transientBend = envelope * envelope;
    const auto wobbleDepthSamples = sampleRateHz * (0.001f + wobble * wobble * 0.030f);
    const auto polarity = channel == 0 ? 1.0f : -1.0f;
    const auto targetDelay = sampleRateHz * kBaseDelayMs * 0.001f
        + wobbleDepthSamples * (0.35f + transientBend * 1.65f)
        + instability[channel] * wobbleDepthSamples * 0.22f * polarity;

    delayTimeState[channel] += kDelaySmooth * (targetDelay - delayTimeState[channel]);
    const auto delaySamples = juce::jlimit(2.0f, static_cast<float>(delayBufferSamples - 3), delayTimeState[channel]);
    auto readPosition = static_cast<float>(writeIndex) - delaySamples;
    while (readPosition < 0.0f)
        readPosition += static_cast<float>(delayBufferSamples);

    const auto index0 = static_cast<int>(readPosition) % delayBufferSamples;
    const auto index1 = (index0 + 1) % delayBufferSamples;
    const auto frac = readPosition - static_cast<float>(index0);
    return delayData[index0] + (delayData[index1] - delayData[index0]) * frac;
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
        const auto cutoff = juce::jmap(juce::jlimit(0.0f, 1.0f, dirtValue * 0.45f + crushValue * 0.75f),
                                       kPostFilterMaxHz, kPostFilterMinHz);
        const auto filterCoeff = onePoleCoefficient(cutoff, sampleRateHz);

        for (int ch = 0; ch < channelsToProcess; ++ch)
        {
            auto* channel = buffer.getWritePointer(ch);
            const auto dry = sanitize(channel[sample]);
            auto wet = readWarpedDelay(ch, dry, wobbleValue, envelopeState);
            wet = saturate(wet, dirtValue, envelopeState);
            wet = crushSample(wet, crushValue, envelopeState, ch);
            postFilterState[ch] += filterCoeff * (wet - postFilterState[ch]);
            wet = juce::jmap(crushValue, wet, postFilterState[ch]);
            wet = fastClip(wet * (1.0f - crushValue * 0.10f));
            wet = dcBlock(wet, ch, dcX1, dcY1);
            channel[sample] = fastClip(dry * (1.0f - kDryWetMix) + wet * kDryWetMix);
        }

        writeIndex = (writeIndex + 1) % delayBufferSamples;
    }

    for (int ch = channelsToProcess; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);
}
