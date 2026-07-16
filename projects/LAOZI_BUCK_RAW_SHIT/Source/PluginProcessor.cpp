#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout LaoziBuckRawShitProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(pressureParamId, "PRESSURE", 0.0f, 1.0f, 0.50f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(kickParamId, "KICK", 0.0f, 1.0f, 0.55f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(auraParamId, "AURA", 0.0f, 1.0f, 0.45f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(glueParamId, "GLUE", 0.0f, 1.0f, 0.50f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(outputParamId, "OUTPUT", juce::NormalisableRange<float>(-12.0f, 6.0f, 0.1f), 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(bypassParamId, "BYPASS", false));
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(oversampleParamId, "OVERSAMPLE", juce::StringArray { "Off", "2x", "4x" }, 0));
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(presetIndexParamId, "PRESET", juce::StringArray { "BUCK MASTER", "MORF FORCE", "GRAND KICK", "DARK CEREMONY", "RAW SHIT" }, 0));
    return { parameters.begin(), parameters.end() };
}

LaoziBuckRawShitProcessor::LaoziBuckRawShitProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{}

void LaoziBuckRawShitProcessor::prepareToPlay(double, int)
{
    leftPeak.store(0.0f, std::memory_order_relaxed);
    rightPeak.store(0.0f, std::memory_order_relaxed);
}

void LaoziBuckRawShitProcessor::publishPeak(std::atomic<float>& destination, float value) noexcept
{
    auto current = destination.load(std::memory_order_relaxed);
    while (value > current && ! destination.compare_exchange_weak(current, value, std::memory_order_relaxed)) {}
}

void LaoziBuckRawShitProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto bypassed = apvts.getRawParameterValue(bypassParamId)->load() > 0.5f;
    const auto gain = bypassed ? 1.0f : juce::Decibels::decibelsToGain(apvts.getRawParameterValue(outputParamId)->load());
    const auto channels = buffer.getNumChannels();
    const auto samples = buffer.getNumSamples();
    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        float peak = 0.0f;
        for (int sample = 0; sample < samples; ++sample)
        {
            data[sample] *= gain;
            peak = juce::jmax(peak, std::abs(data[sample]));
        }
        if (channel == 0) publishPeak(leftPeak, peak);
        else if (channel == 1) publishPeak(rightPeak, peak);
    }
}

void LaoziBuckRawShitProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, destination);
}

void LaoziBuckRawShitProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size)) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* LaoziBuckRawShitProcessor::createEditor() { return new LaoziBuckRawShitEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LaoziBuckRawShitProcessor(); }
