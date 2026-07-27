#include "PluginProcessor.h"
#include "PluginEditor.h"

ToyotomiHideyoshiAudioProcessor::ToyotomiHideyoshiAudioProcessor()
    : AudioProcessor (BusesProperties()
                         .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                         .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void ToyotomiHideyoshiAudioProcessor::prepareToPlay (double, int) {}
void ToyotomiHideyoshiAudioProcessor::releaseResources() {}

bool ToyotomiHideyoshiAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && output == layouts.getMainInputChannelSet();
}

void ToyotomiHideyoshiAudioProcessor::publishPeak (std::atomic<float>& destination, float value) noexcept
{
    auto current = destination.load (std::memory_order_relaxed);
    while (value > current
           && ! destination.compare_exchange_weak (current, value, std::memory_order_relaxed))
    {
    }
}

void ToyotomiHideyoshiAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    // Phase 1 deliberately leaves the input samples unchanged: complete pass-through.
    if (buffer.getNumChannels() > 0)
        publishPeak (outputPeakLeft, buffer.getMagnitude (0, 0, buffer.getNumSamples()));

    if (buffer.getNumChannels() > 1)
        publishPeak (outputPeakRight, buffer.getMagnitude (1, 0, buffer.getNumSamples()));

    auto syncWasRead = false;
    if (auto* audioPlayHead = getPlayHead())
    {
        if (const auto position = audioPlayHead->getPosition())
        {
            syncWasRead = true;
            hostPlaying.store (position->getIsPlaying(), std::memory_order_relaxed);

            if (const auto bpm = position->getBpm())
                hostBpm.store (*bpm, std::memory_order_relaxed);

            if (const auto signature = position->getTimeSignature())
            {
                timeSignatureNumerator.store (signature->numerator, std::memory_order_relaxed);
                timeSignatureDenominator.store (signature->denominator, std::memory_order_relaxed);
            }
        }
    }

    hostSyncAvailable.store (syncWasRead, std::memory_order_relaxed);
}

float ToyotomiHideyoshiAudioProcessor::consumeOutputPeak (int channel) noexcept
{
    auto& source = channel == 0 ? outputPeakLeft : outputPeakRight;
    return source.exchange (0.0f, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ToyotomiHideyoshiAudioProcessor::createEditor()
{
    return new ToyotomiHideyoshiAudioProcessorEditor (*this);
}

void ToyotomiHideyoshiAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    destinationData.reset(); // Persistent state is the next implementation stage.
}

void ToyotomiHideyoshiAudioProcessor::setStateInformation (const void*, int)
{
    // Persistent state is intentionally not implemented at this checkpoint.
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ToyotomiHideyoshiAudioProcessor();
}
