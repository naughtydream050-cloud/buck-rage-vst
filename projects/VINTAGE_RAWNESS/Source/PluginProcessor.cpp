#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
float nextRandomBipolar(uint32_t& state) noexcept
{
    state = state * 1664525u + 1013904223u;
    const auto mantissa = static_cast<float>((state >> 8) & 0x00ffffffu) / 8388607.5f;
    return mantissa - 1.0f;
}
}

juce::AudioProcessorValueTreeState::ParameterLayout VintageRawnessProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(dirtParamId, "DIRT", 0.0f, 1.0f, 0.30f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(crushParamId, "CRUSH", 0.0f, 1.0f, 0.20f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(wobbleParamId, "WOBBLE", 0.0f, 1.0f, 0.10f));
    return { params.begin(), params.end() };
}

VintageRawnessProcessor::VintageRawnessProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{}

void VintageRawnessProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    randomState = 0x6d2b79f5u;

    for (int ch = 0; ch < kMaxTrackedChannels; ++ch)
    {
        sampleHold[ch] = 0.0f;
        holdCounter[ch] = 0;
        wobbleState[ch] = 0.0f;
        previousSample[ch] = 0.0f;
        dcX1[ch] = 0.0f;
        dcY1[ch] = 0.0f;
    }

    dirtSmooth.reset(currentSampleRate, kSmoothingSeconds);
    crushSmooth.reset(currentSampleRate, kSmoothingSeconds);
    wobbleSmooth.reset(currentSampleRate, kSmoothingSeconds);
    dirtSmooth.setCurrentAndTargetValue(*apvts.getRawParameterValue(dirtParamId));
    crushSmooth.setCurrentAndTargetValue(*apvts.getRawParameterValue(crushParamId));
    wobbleSmooth.setCurrentAndTargetValue(*apvts.getRawParameterValue(wobbleParamId));
}

float VintageRawnessProcessor::saturate(float sample, float drive) noexcept
{
    return std::tanh(sample * drive);
}

float VintageRawnessProcessor::bitReduce(float sample, float amount) noexcept
{
    const auto steps = juce::jmap(amount, 65536.0f, 128.0f);
    return std::round(sample * steps) / steps;
}

float VintageRawnessProcessor::dcBlock(float sample, int channel, float* x1, float* y1) noexcept
{
    const auto y = sample - x1[channel] + kDcBlockerPole * y1[channel];
    x1[channel] = sample;
    y1[channel] = y;
    return y;
}

void VintageRawnessProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    dirtSmooth.setTargetValue(*apvts.getRawParameterValue(dirtParamId));
    crushSmooth.setTargetValue(*apvts.getRawParameterValue(crushParamId));
    wobbleSmooth.setTargetValue(*apvts.getRawParameterValue(wobbleParamId));

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto dirt = dirtSmooth.getNextValue();
        const auto crush = crushSmooth.getNextValue();
        const auto wobble = wobbleSmooth.getNextValue();
        const auto holdRate = juce::jmap(crush, kCrushMaxHoldHz, kCrushMinHoldHz);
        const auto holdSamples = juce::jmax(1, static_cast<int>(currentSampleRate / holdRate));
        const auto wobbleRate = juce::jmap(wobble, kWobbleMinRateHz, kWobbleMaxRateHz);
        const auto wobbleCoeff = 1.0f - std::exp((-2.0f * juce::MathConstants<float>::pi * wobbleRate) / static_cast<float>(currentSampleRate));
        const auto wobbleBlend = wobble * kWobbleMaxBlend;

        for (int ch = 0; ch < juce::jmin(numChannels, kMaxTrackedChannels); ++ch)
        {
            auto* channel = buffer.getWritePointer(ch);
            const auto dry = channel[sample];
            auto processed = saturate(dry, 1.0f + dirt * 8.0f);
            processed = bitReduce(processed, crush);

            if (holdCounter[ch] <= 0)
            {
                sampleHold[ch] = processed;
                holdCounter[ch] = holdSamples;
            }
            --holdCounter[ch];
            processed = juce::jmap(crush, processed, sampleHold[ch]);

            wobbleState[ch] += wobbleCoeff * (nextRandomBipolar(randomState) - wobbleState[ch]);
            const auto polarity = ch == 0 ? 1.0f : -1.0f;
            const auto bent = processed + (previousSample[ch] - processed) * (wobbleBlend + wobbleState[ch] * wobbleBlend * 0.25f * polarity);
            previousSample[ch] = processed;

            const auto blocked = dcBlock(bent, ch, dcX1, dcY1);
            channel[sample] = juce::jlimit(-kOutputCeiling, kOutputCeiling, blocked * (1.0f - crush * 0.12f));
        }
    }

    for (int ch = kMaxTrackedChannels; ch < numChannels; ++ch)
        buffer.clear(ch, 0, numSamples);
}

void VintageRawnessProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void VintageRawnessProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* VintageRawnessProcessor::createEditor()
{
    return new VintageRawnessEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VintageRawnessProcessor();
}
