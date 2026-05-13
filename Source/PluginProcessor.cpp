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

void RudeHypeProcessor::prepareToPlay(double sr, int)
{
    sampleRate_ = sr;
    lfoPhase = 0.0f;
    lpfZ[0] = lpfZ[1] = 0.0f;
}

void RudeHypeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const float shout = *apvts.getRawParameterValue("shout");
    const float burn = *apvts.getRawParameterValue("burn");
    const float drive = 0.5f + shout * 2.5f;
    const float lfoFreq = 0.5f + burn * 8.0f;
    const float lfoDepth = burn * 0.004f;
    const float alpha = 0.05f;
    const float lfoInc = juce::MathConstants<float>::twoPi * lfoFreq / static_cast<float>(sampleRate_);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float phase = lfoPhase;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = data[i];
            x *= (1.0f + lfoDepth * std::sin(phase));
            if (ch == 0)
                phase += lfoInc;
            x = asymSat(x, drive);
            lpfZ[ch] += alpha * (x - lpfZ[ch]);
            data[i] = lpfZ[ch];
        }
    }

    lfoPhase = std::fmod(lfoPhase + lfoInc * static_cast<float>(buffer.getNumSamples()),
                         juce::MathConstants<float>::twoPi);
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
