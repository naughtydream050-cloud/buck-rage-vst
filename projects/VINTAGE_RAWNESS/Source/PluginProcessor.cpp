#include "PluginProcessor.h"
#include "PluginEditor.h"

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

void VintageRawnessProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    krumpWarp.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void VintageRawnessProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    krumpWarp.processBlock(buffer,
                           *apvts.getRawParameterValue(dirtParamId),
                           *apvts.getRawParameterValue(crushParamId),
                           *apvts.getRawParameterValue(wobbleParamId));
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
