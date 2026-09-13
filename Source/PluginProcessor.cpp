#include "PluginProcessor.h"
#include "PluginEditorV2.h"
#include <cmath>
namespace
{
constexpr uint64_t kQuant = 65535;
uint64_t quantise (float value, float low, float high) noexcept
{
    return (uint64_t) juce::roundToInt (juce::jlimit (0.0f, 1.0f, (value - low) / (high - low)) * (float) kQuant);
}
float dequantise (uint64_t value, float low, float high) noexcept
{
    return low + (float) value / (float) kQuant * (high - low);
}
}
ToyotomiHideyoshiAudioProcessor::ToyotomiHideyoshiAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true))
{
    for (auto& slot : dspSlots) slot.store (packDspSlot (PluginStateModel::TimelineSlot {}), std::memory_order_relaxed);
    stateModel.setDspSlotChangedCallback ([this] (int bar, const PluginStateModel::TimelineSlot& slot) { publishDspSlot (bar, slot); });
    stateModel.setDspTimingChangedCallback ([this] (const PluginStateModel::UiState& ui)
    {
        hostSyncEnabled.store (ui.hostSync, std::memory_order_relaxed);
        internalBpmForDsp.store (ui.internalBpm, std::memory_order_relaxed);
        internalTimeSigNumeratorForDsp.store (ui.internalTimeSigNumerator, std::memory_order_relaxed);
        internalTimeSigDenominatorForDsp.store (ui.internalTimeSigDenominator, std::memory_order_relaxed);
    });
}
void ToyotomiHideyoshiAudioProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    preparedSampleRate = juce::jmax (1.0, sampleRate);
    scratchEngine.prepare (preparedSampleRate, maximumExpectedSamplesPerBlock, getTotalNumInputChannels());
    internalQuarterPosition = lastHostPpqEnd = 0.0; haveLastHostPpq = internalWasPlaying = false;
}
void ToyotomiHideyoshiAudioProcessor::releaseResources(){ scratchEngine.release(); }
void ToyotomiHideyoshiAudioProcessor::getStateInformation(juce::MemoryBlock& d){if(auto x=stateModel.toValueTree().createXml())copyXmlToBinary(*x,d);} void ToyotomiHideyoshiAudioProcessor::setStateInformation(const void*d,int s){if(auto x=getXmlFromBinary(d,s))if(stateModel.fromValueTree(juce::ValueTree::fromXml(*x)))hostSyncEnabled.store(stateModel.getUiState().hostSync,std::memory_order_relaxed);}
bool ToyotomiHideyoshiAudioProcessor::isBusesLayoutSupported(const BusesLayout& l)const{auto o=l.getMainOutputChannelSet();return(o==juce::AudioChannelSet::mono()||o==juce::AudioChannelSet::stereo())&&o==l.getMainInputChannelSet();}
void ToyotomiHideyoshiAudioProcessor::publishPeak(std::atomic<float>&d,float v)noexcept{auto c=d.load();while(v>c&&!d.compare_exchange_weak(c,v)){}}
uint64_t ToyotomiHideyoshiAudioProcessor::packDspSlot (const PluginStateModel::TimelineSlot& slot) noexcept
{
    return (uint64_t) static_cast<uint8_t> (slot.preset)
         | ((uint64_t) static_cast<uint8_t> (slot.length) << 4)
         | (quantise (slot.speed, PluginStateModel::kMinSpeed, PluginStateModel::kMaxSpeed) << 7)
         | (quantise (slot.pitch, PluginStateModel::kMinPitch, PluginStateModel::kMaxPitch) << 23)
         | (quantise (slot.depth, 0.0f, 1.0f) << 39);
}
TimelineScratchEngine::Slot ToyotomiHideyoshiAudioProcessor::unpackDspSlot (uint64_t packed) noexcept
{
    TimelineScratchEngine::Slot slot;
    slot.preset = static_cast<TimelineScratchEngine::Preset> (packed & 0x0f);
    slot.length = static_cast<TimelineScratchEngine::Length> ((packed >> 4) & 0x07);
    slot.speed = dequantise ((packed >> 7) & kQuant, PluginStateModel::kMinSpeed, PluginStateModel::kMaxSpeed);
    slot.pitch = dequantise ((packed >> 23) & kQuant, PluginStateModel::kMinPitch, PluginStateModel::kMaxPitch);
    slot.depth = dequantise ((packed >> 39) & kQuant, 0.0f, 1.0f);
    return slot;
}
void ToyotomiHideyoshiAudioProcessor::publishDspSlot (int bar, const PluginStateModel::TimelineSlot& slot) noexcept
{
    if (PluginStateModel::hasSelectedBar (bar)) dspSlots[(size_t) bar].store (packDspSlot (slot), std::memory_order_release);
}
void ToyotomiHideyoshiAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
    bool readPosition = false, playing = false;
    int timelineSlot = -1; double hostPpq = 0.0;
    bool hasHostPpq = false, discontinuity = false;
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
            if (playing && hostSyncEnabled.load (std::memory_order_relaxed))
                if (auto ppq = position->getPpqPosition())
                {
                    // PPQ is expressed in quarter notes.  A COUNT BAR is one
                    // host bar, not one sixteenth note: at 4/4, PPQ 0..3.999
                    // is BAR 1 and PPQ 4 starts BAR 2.
                    const auto numerator = timeSignatureNumerator.load (std::memory_order_relaxed);
                    const auto denominator = timeSignatureDenominator.load (std::memory_order_relaxed);
                    const auto quartersPerBar = juce::jmax (0.25, (double) numerator * 4.0 / (double) denominator);
                    const auto hostBar = static_cast<int> (std::floor (*ppq / quartersPerBar));
                    timelineSlot = ((hostBar % PluginStateModel::kNumBars) + PluginStateModel::kNumBars) % PluginStateModel::kNumBars;
                    hostPpq = *ppq; hasHostPpq = true;
                }
        }
    const auto effectiveBpm = juce::jmax (1.0, getEffectiveBpm());
    const auto numerator = getEffectiveTimeSignatureNumerator();
    const auto denominator = getEffectiveTimeSignatureDenominator();
    const auto quartersPerBar = juce::jmax (0.25, (double) numerator * 4.0 / (double) denominator);
    TimelineScratchEngine::Transport transport;
    transport.playing = playing;
    transport.quartersPerBar = quartersPerBar;
    transport.secondsPerQuarter = 60.0 / effectiveBpm;
    transport.barPhasePerSample = effectiveBpm / (60.0 * preparedSampleRate * quartersPerBar);
    if (hostSyncEnabled.load (std::memory_order_relaxed) && hasHostPpq)
    {
        const auto barPosition = hostPpq / quartersPerBar;
        const auto hostBar = (int) std::floor (barPosition);
        transport.startBar = ((hostBar % PluginStateModel::kNumBars) + PluginStateModel::kNumBars) % PluginStateModel::kNumBars;
        transport.startBarPhase = barPosition - std::floor (barPosition);
        if (haveLastHostPpq)
        {
            const auto expected = lastHostPpqEnd;
            discontinuity = std::abs (hostPpq - expected) > juce::jmax (0.01, effectiveBpm * 2.0 / (60.0 * preparedSampleRate));
        }
        lastHostPpqEnd = hostPpq + effectiveBpm * (double) buffer.getNumSamples() / (60.0 * preparedSampleRate);
        haveLastHostPpq = true;
    }
    else
    {
        if (playing && ! internalWasPlaying) { internalQuarterPosition = 0.0; discontinuity = true; }
        const auto barPosition = internalQuarterPosition / quartersPerBar;
        const auto internalBar = (int) std::floor (barPosition);
        transport.startBar = playing ? ((internalBar % PluginStateModel::kNumBars) + PluginStateModel::kNumBars) % PluginStateModel::kNumBars : -1;
        transport.startBarPhase = barPosition - std::floor (barPosition);
        if (playing) internalQuarterPosition += effectiveBpm * (double) buffer.getNumSamples() / (60.0 * preparedSampleRate);
        timelineSlot = transport.startBar;
        haveLastHostPpq = false;
    }
    transport.discontinuity = discontinuity;
    internalWasPlaying = playing;
    std::array<TimelineScratchEngine::Slot, PluginStateModel::kNumBars> snapshot;
    for (int bar = 0; bar < PluginStateModel::kNumBars; ++bar)
        snapshot[(size_t) bar] = unpackDspSlot (dspSlots[(size_t) bar].load (std::memory_order_acquire));
    scratchEngine.process (buffer, transport, snapshot);
    if (buffer.getNumChannels() > 0) publishPeak (outputPeakLeft, buffer.getMagnitude (0, 0, buffer.getNumSamples()));
    if (buffer.getNumChannels() > 1) publishPeak (outputPeakRight, buffer.getMagnitude (1, 0, buffer.getNumSamples()));
    hostPlaying.store (playing, std::memory_order_relaxed);
    currentTimelineSlot.store (timelineSlot, std::memory_order_relaxed);
    hostSyncAvailable.store (readPosition, std::memory_order_relaxed);
}
double ToyotomiHideyoshiAudioProcessor::getEffectiveBpm() const noexcept
{
    return isHostSyncEnabled() ? getHostBpm() : internalBpmForDsp.load (std::memory_order_relaxed);
}
int ToyotomiHideyoshiAudioProcessor::getEffectiveTimeSignatureNumerator() const noexcept
{
    return isHostSyncEnabled() ? getTimeSignatureNumerator() : internalTimeSigNumeratorForDsp.load (std::memory_order_relaxed);
}
int ToyotomiHideyoshiAudioProcessor::getEffectiveTimeSignatureDenominator() const noexcept
{
    return isHostSyncEnabled() ? getTimeSignatureDenominator() : internalTimeSigDenominatorForDsp.load (std::memory_order_relaxed);
}
void ToyotomiHideyoshiAudioProcessor::setHostSyncEnabled (bool enabled) noexcept
{
    stateModel.setHostSync (enabled);
    hostSyncEnabled.store (enabled, std::memory_order_relaxed);
}
float ToyotomiHideyoshiAudioProcessor::consumeOutputPeak(int c)noexcept{return(c==0?outputPeakLeft:outputPeakRight).exchange(0.0f);}
juce::AudioProcessorEditor* ToyotomiHideyoshiAudioProcessor::createEditor(){return new ToyotomiHideyoshiAudioProcessorEditorV2(*this);} juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new ToyotomiHideyoshiAudioProcessor();}
