#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout RudeHypeProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shout", "SHOUT", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "burn", "BURN", 0.0f, 1.0f, 0.5f));
    return { params.begin(), params.end() };
}

RudeHypeProcessor::RudeHypeProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{}

float RudeHypeProcessor::onePoleCoefficient(float hz, double sampleRate) noexcept
{
    return 1.0f - std::exp((-2.0f * juce::MathConstants<float>::pi * hz) / static_cast<float>(sampleRate));
}

float RudeHypeProcessor::softClip(float x) noexcept
{
    return x / (1.0f + std::abs(x));
}

float RudeHypeProcessor::transistorSat(float x, float drive) noexcept
{
    const auto biased = drive * (x + 0.08f);
    return std::tanh(biased) - std::tanh(0.08f * drive);
}

float RudeHypeProcessor::wavefold(float x, float amount) noexcept
{
    const auto folded = std::sin(x * (1.0f + amount * 5.0f));
    return juce::jmap(amount, x, folded);
}

void RudeHypeProcessor::prepareToPlay(double sr, int)
{
    sampleRate_ = sr;
    lfoPhase = 0.0f;
    tapeEnv = 0.0f;

    for (int ch = 0; ch < kStereoChannels; ++ch)
    {
        shoutLowZ[ch] = 0.0f;
        shoutHighZ[ch] = 0.0f;
        burnLowZ[ch] = 0.0f;
        fizzLowZ[ch] = 0.0f;
        dcX1[ch] = 0.0f;
        dcY1[ch] = 0.0f;
        prevSat[ch] = 0.0f;
    }

    shoutSmooth.reset(sampleRate_, 0.025);
    burnSmooth.reset(sampleRate_, 0.025);
    shoutSmooth.setCurrentAndTargetValue(*apvts.getRawParameterValue("shout"));
    burnSmooth.setCurrentAndTargetValue(*apvts.getRawParameterValue("burn"));
}

void RudeHypeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    shoutSmooth.setTargetValue(*apvts.getRawParameterValue("shout"));
    burnSmooth.setTargetValue(*apvts.getRawParameterValue("burn"));

    const auto channelCount = buffer.getNumChannels();
    const auto sampleCount = buffer.getNumSamples();
    auto* left = buffer.getWritePointer(0);
    auto* right = channelCount > 1 ? buffer.getWritePointer(1) : left;

    const auto shoutLowCoeff = onePoleCoefficient(kShoutFocusLowHz, sampleRate_);
    const auto shoutHighCoeff = onePoleCoefficient(kShoutFocusHighHz, sampleRate_);
    const auto burnLowCoeff = onePoleCoefficient(kBurnLowSplitHz, sampleRate_);
    const auto fizzCoeff = onePoleCoefficient(kBurnFizzLowHz, sampleRate_);
    const auto lfoInc = juce::MathConstants<float>::twoPi * 0.72f / static_cast<float>(sampleRate_);

    for (int i = 0; i < sampleCount; ++i)
    {
        const auto shout = shoutSmooth.getNextValue();
        const auto burn = burnSmooth.getNextValue();
        const auto lfo = std::sin(lfoPhase);
        lfoPhase += lfoInc;
        if (lfoPhase >= juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;

        float in[2] { left[i], right[i] };
        float processed[2] {};

        for (int ch = 0; ch < kStereoChannels; ++ch)
        {
            const auto dry = in[ch];

            const auto shoutDrive = 1.0f + shout * 4.5f;
            auto shoutSat = asymSat(dry, shoutDrive);

            shoutLowZ[ch] += shoutLowCoeff * (shoutSat - shoutLowZ[ch]);
            const auto upperSource = shoutSat - shoutLowZ[ch];
            shoutHighZ[ch] += shoutHighCoeff * (upperSource - shoutHighZ[ch]);
            const auto upperMid = shoutHighZ[ch];
            const auto exciter = softClip(upperMid * (1.5f + shout * 7.0f));
            const auto detunePolarity = ch == 0 ? 1.0f : -1.0f;
            const auto microMotion = (0.012f + shout * 0.035f) * lfo * detunePolarity;
            const auto microChorus = shoutSat + prevSat[ch] * microMotion;
            prevSat[ch] = shoutSat;
            const auto forward = microChorus + exciter * (0.18f + shout * 0.42f);
            const auto flattened = forward - (forward - dry) * (0.04f * shout);
            const auto shouted = softClip(flattened * (1.0f + shout * 0.85f));

            burnLowZ[ch] += burnLowCoeff * (dry - burnLowZ[ch]);
            const auto lowSafe = burnLowZ[ch];
            const auto burnBand = dry - lowSafe;
            auto burned = transistorSat(burnBand, 1.0f + burn * 6.0f);
            burned = wavefold(burned, burn * 0.62f);
            const auto downsampleFlavor = burn > 0.0f ? std::round(burned * (96.0f - burn * 72.0f)) / (96.0f - burn * 72.0f) : burned;

            tapeEnv += 0.0025f * ((std::abs(downsampleFlavor) + std::abs(processed[0])) - tapeEnv);
            const auto tapeGain = 1.0f / (1.0f + tapeEnv * burn * 2.2f);
            fizzLowZ[ch] += fizzCoeff * (downsampleFlavor - fizzLowZ[ch]);
            const auto fizz = softClip((downsampleFlavor - fizzLowZ[ch]) * (0.6f + burn * 2.8f));
            const auto burnedParallel = lowSafe + (downsampleFlavor * tapeGain) + fizz * burn * 0.24f;

            const auto shoutMix = shout * (0.38f + shout * 0.34f);
            const auto burnMix = burn * (0.30f + burn * 0.42f);
            auto out = dry + (shouted - dry) * shoutMix + (burnedParallel - dry) * burnMix;

            const auto dc = out - dcX1[ch] + kDcBlockerPole * dcY1[ch];
            dcX1[ch] = out;
            dcY1[ch] = dc;
            processed[ch] = juce::jlimit(-kOutputCeiling, kOutputCeiling, softClip(dc * (1.0f + shout * 0.16f + burn * 0.10f)));
        }

        const auto mid = (processed[0] + processed[1]) * 0.5f;
        const auto side = (processed[0] - processed[1]) * 0.5f;
        const auto width = 1.0f + shout * 0.36f + burn * 0.18f + lfo * shout * 0.025f;
        left[i] = juce::jlimit(-kOutputCeiling, kOutputCeiling, mid + side * width);
        if (channelCount > 1)
            right[i] = juce::jlimit(-kOutputCeiling, kOutputCeiling, mid - side * width);
    }

    for (int ch = 2; ch < channelCount; ++ch)
        buffer.clear(ch, 0, sampleCount);
}

void RudeHypeProcessor::getStateInformation(juce::MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, d);
}

void RudeHypeProcessor::setStateInformation(const void* d, int s)
{
    if (auto xml = getXmlFromBinary(d, s))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* RudeHypeProcessor::createEditor()
{
    return new RudeHypeEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RudeHypeProcessor();
}
