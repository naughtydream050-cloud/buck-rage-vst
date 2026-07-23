#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetLibrary.h"

#include <cmath>

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
    juce::StringArray presetNames;
    for (const auto& preset : LaoziPresetLibrary::presets) presetNames.add(preset.name);
    presetNames.add("CUSTOM");
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(presetIndexParamId, "PRESET", presetNames, 0));
    return { parameters.begin(), parameters.end() };
}

LaoziBuckRawShitProcessor::LaoziBuckRawShitProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{}

void LaoziBuckRawShitProcessor::resetDspState() noexcept
{
    kickLow.fill(0.0f); auraLow.fill(0.0f); dcInput.fill(0.0f); dcOutput.fill(0.0f);
    glueEnvelope = 0.0f;
    glueGain = 1.0f;
}

void LaoziBuckRawShitProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    resetDspState();
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
    const auto channels = buffer.getNumChannels();
    const auto samples = buffer.getNumSamples();
    if (channels == 0) return;

    const auto pressure = apvts.getRawParameterValue(pressureParamId)->load();
    const auto kick = apvts.getRawParameterValue(kickParamId)->load();
    const auto aura = apvts.getRawParameterValue(auraParamId)->load();
    const auto glue = apvts.getRawParameterValue(glueParamId)->load();
    const auto outputGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue(outputParamId)->load());
    const auto kickCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 125.0f / static_cast<float>(currentSampleRate));
    const auto auraCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 4200.0f / static_cast<float>(currentSampleRate));
    const auto drive = 1.0f + pressure * 7.0f;
    const auto driveNormaliser = 1.0f / std::tanh(drive);
    const auto threshold = juce::Decibels::decibelsToGain(-8.0f - glue * 16.0f);
    const auto ratio = 1.0f + glue * 7.0f;
    const auto attackCoeff = std::exp(-1.0f / (0.004f * static_cast<float>(currentSampleRate)));
    const auto releaseCoeff = std::exp(-1.0f / (0.120f * static_cast<float>(currentSampleRate)));
    const auto gainReleaseCoeff = std::exp(-1.0f / (0.080f * static_cast<float>(currentSampleRate)));
    const auto* const leftRead = buffer.getReadPointer(0);
    auto* const leftWrite = buffer.getWritePointer(0);
    const auto* const rightRead = channels > 1 ? buffer.getReadPointer(1) : nullptr;
    auto* const rightWrite = channels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < samples; ++sample)
    {
        float signal[2] { leftRead[sample], rightRead != nullptr ? rightRead[sample] : leftRead[sample] };
        if (! bypassed)
        {
            for (int channel = 0; channel < 2; ++channel)
            {
                kickLow[static_cast<size_t>(channel)] += kickCoeff * (signal[channel] - kickLow[static_cast<size_t>(channel)]);
                signal[channel] += kickLow[static_cast<size_t>(channel)] * (0.72f * kick);
                auraLow[static_cast<size_t>(channel)] += auraCoeff * (signal[channel] - auraLow[static_cast<size_t>(channel)]);
                signal[channel] += (signal[channel] - auraLow[static_cast<size_t>(channel)]) * (0.42f * aura);
            }

            const auto mid = 0.5f * (signal[0] + signal[1]);
            const auto side = 0.5f * (signal[0] - signal[1]) * (1.0f + aura * 0.45f);
            signal[0] = std::tanh((mid + side) * drive) * driveNormaliser;
            signal[1] = std::tanh((mid - side) * drive) * driveNormaliser;

            const auto detector = juce::jmax(std::abs(signal[0]), std::abs(signal[1]));
            const auto envelopeCoeff = detector > glueEnvelope ? attackCoeff : releaseCoeff;
            glueEnvelope = detector + envelopeCoeff * (glueEnvelope - detector);
            auto targetGain = 1.0f;
            if (glueEnvelope > threshold)
                targetGain = std::pow(glueEnvelope / threshold, -(1.0f - 1.0f / ratio));
            glueGain = targetGain < glueGain ? targetGain : targetGain + gainReleaseCoeff * (glueGain - targetGain);

            for (int channel = 0; channel < 2; ++channel)
            {
                auto value = signal[channel] * glueGain * outputGain;
                const auto blocked = value - dcInput[static_cast<size_t>(channel)] + 0.995f * dcOutput[static_cast<size_t>(channel)];
                dcInput[static_cast<size_t>(channel)] = value;
                dcOutput[static_cast<size_t>(channel)] = blocked;
                signal[channel] = blocked;
            }
        }
        leftWrite[sample] = signal[0];
        if (rightWrite != nullptr) rightWrite[sample] = signal[1];
    }

    float peaks[2] {};
    for (int channel = 0; channel < juce::jmin(channels, 2); ++channel)
        peaks[channel] = buffer.getMagnitude(channel, 0, samples);
    publishPeak(leftPeak, peaks[0]);
    publishPeak(rightPeak, channels > 1 ? peaks[1] : peaks[0]);
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
