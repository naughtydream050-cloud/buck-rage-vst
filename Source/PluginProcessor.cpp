#include "PluginProcessor.h"
#include "PluginEditorV2.h"
#include <cmath>
ToyotomiHideyoshiAudioProcessor::ToyotomiHideyoshiAudioProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)){}
void ToyotomiHideyoshiAudioProcessor::prepareToPlay(double,int){} void ToyotomiHideyoshiAudioProcessor::releaseResources(){}
void ToyotomiHideyoshiAudioProcessor::getStateInformation(juce::MemoryBlock& d){if(auto x=stateModel.toValueTree().createXml())copyXmlToBinary(*x,d);} void ToyotomiHideyoshiAudioProcessor::setStateInformation(const void*d,int s){if(auto x=getXmlFromBinary(d,s))stateModel.fromValueTree(juce::ValueTree::fromXml(*x));}
bool ToyotomiHideyoshiAudioProcessor::isBusesLayoutSupported(const BusesLayout& l)const{auto o=l.getMainOutputChannelSet();return(o==juce::AudioChannelSet::mono()||o==juce::AudioChannelSet::stereo())&&o==l.getMainInputChannelSet();}
void ToyotomiHideyoshiAudioProcessor::publishPeak(std::atomic<float>&d,float v)noexcept{auto c=d.load();while(v>c&&!d.compare_exchange_weak(c,v)){}}
void ToyotomiHideyoshiAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
    if (buffer.getNumChannels() > 0) publishPeak (outputPeakLeft, buffer.getMagnitude (0, 0, buffer.getNumSamples()));
    if (buffer.getNumChannels() > 1) publishPeak (outputPeakRight, buffer.getMagnitude (1, 0, buffer.getNumSamples()));

    bool readPosition = false, playing = false;
    int timelineSlot = -1;
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
        {
            readPosition = true;
            playing = position->getIsPlaying();
            if (auto bpm = position->getBpm()) hostBpm.store (*bpm, std::memory_order_relaxed);
            if (auto time = position->getTimeSignature())
            {
                timeSignatureNumerator.store (time->numerator, std::memory_order_relaxed);
                timeSignatureDenominator.store (time->denominator, std::memory_order_relaxed);
            }
            if (playing)
                if (auto ppq = position->getPpqPosition())
                {
                    const auto sixteenth = static_cast<int> (std::floor (*ppq * 4.0));
                    timelineSlot = ((sixteenth % PluginStateModel::kNumBars) + PluginStateModel::kNumBars) % PluginStateModel::kNumBars;
                }
        }
    hostPlaying.store (playing, std::memory_order_relaxed);
    currentTimelineSlot.store (timelineSlot, std::memory_order_relaxed);
    hostSyncAvailable.store (readPosition, std::memory_order_relaxed);
}
float ToyotomiHideyoshiAudioProcessor::consumeOutputPeak(int c)noexcept{return(c==0?outputPeakLeft:outputPeakRight).exchange(0.0f);}
juce::AudioProcessorEditor* ToyotomiHideyoshiAudioProcessor::createEditor(){return new ToyotomiHideyoshiAudioProcessorEditorV2(*this);} juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new ToyotomiHideyoshiAudioProcessor();}
