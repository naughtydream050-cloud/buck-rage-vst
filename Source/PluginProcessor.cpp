#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout BuckRageProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "buck", "Distortion/Drive", 0.f, 1.f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "rage", "Pitch Mod/Expression", 0.f, 1.f, 0.0f));
    return { params.begin(), params.end() };
}

BuckRageProcessor::BuckRageProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{}

void BuckRageProcessor::prepareToPlay(double sr, int)
{
    sampleRate_ = sr;
    lfoPhase = 0.f;
    lpfZ[0] = lpfZ[1] = 0.f;
}

void BuckRageProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const float buck = *apvts.getRawParameterValue("buck");
    const float rage = *apvts.getRawParameterValue("rage");
    const float drive = 0.5f + buck * 2.5f;
    const float lfoFreq = 0.5f + rage * 8.f;
    const float lfoDepth = rage * 0.004f;
    const float alpha = 0.05f;
    const float lfoInc = juce::MathConstants<float>::twoPi * lfoFreq / (float)sampleRate_;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float phase = lfoPhase;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = data[i];
            x *= (1.f + lfoDepth * std::sin(phase));
            if (ch == 0) phase += lfoInc;
            x = asymSat(x, drive);
            lpfZ[ch] += alpha * (x - lpfZ[ch]);
            data[i] = lpfZ[ch];
        }
    }
    lfoPhase = std::fmod(lfoPhase + lfoInc * buffer.getNumSamples(), juce::MathConstants<float>::twoPi);
}

void BuckRageProcessor::getStateInformation(juce::MemoryBlock& d)
{ if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, d); }

void BuckRageProcessor::setStateInformation(const void* d, int s)
{ if (auto xml = getXmlFromBinary(d, s)) apvts.replaceState(juce::ValueTree::fromXml(*xml)); }

juce::AudioProcessorEditor* BuckRageProcessor::createEditor() { return new BuckRageEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BuckRageProcessor(); }
