#include "TimelineScratchEngine.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

namespace
{
bool passed = true;
void check (bool value, const char* label)
{
    std::cout << (value ? "PASS " : "FAIL ") << label << '\n';
    passed &= value;
}

std::array<TimelineScratchEngine::Slot, 64> slots (TimelineScratchEngine::Preset preset,
                                                    TimelineScratchEngine::Length length,
                                                    float speed = 1.0f, float depth = 1.0f)
{
    std::array<TimelineScratchEngine::Slot, 64> result {};
    for (auto& slot : result) slot = { preset, length, speed, 0.0f, depth };
    return result;
}

TimelineScratchEngine::Transport transport (int bar, double phase)
{
    TimelineScratchEngine::Transport result;
    result.playing = true; result.startBar = bar; result.startBarPhase = phase;
    result.barPhasePerSample = 140.0 / (60.0 * 48000.0 * 4.0);
    result.quartersPerBar = 4.0; result.secondsPerQuarter = 60.0 / 140.0;
    return result;
}

void fill (juce::AudioBuffer<float>& buffer, float value)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            buffer.setSample (channel, sample, value);
}

void fillTone (juce::AudioBuffer<float>& buffer, int64_t firstSample)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto phase = (float) (firstSample + sample) * 0.0137f;
        buffer.setSample (0, sample, std::sin (phase));
        buffer.setSample (1, sample, std::cos (phase));
    }
}

void fillHistory (TimelineScratchEngine& engine, juce::AudioBuffer<float>& buffer)
{
    const auto dry = slots (TimelineScratchEngine::Preset::off, TimelineScratchEngine::Length::oneBar);
    for (int block = 0; block < 1000; ++block)
    {
        fillTone (buffer, (int64_t) block * buffer.getNumSamples());
        engine.process (buffer, transport (0, 0.0), dry);
    }
}

float tailEnergy (const juce::AudioBuffer<float>& buffer)
{
    float result = 0.0f;
    for (int sample = buffer.getNumSamples() / 2; sample < buffer.getNumSamples(); ++sample)
        result += std::abs (buffer.getSample (0, sample));
    return result;
}

bool boundedFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (! std::isfinite (buffer.getSample (channel, sample)) || std::abs (buffer.getSample (channel, sample)) > 1.01f)
                return false;
    return true;
}

bool noHardJump (const juce::AudioBuffer<float>& buffer, float previous = 0.0f)
{
    auto last = previous;
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto current = buffer.getSample (0, sample);
        if (std::abs (current - last) > 0.15f) return false;
        last = current;
    }
    return true;
}

float maximumDifferenceFromTone (const juce::AudioBuffer<float>& buffer, int64_t firstSample)
{
    auto maximum = 0.0f;
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto dry = std::sin ((float) (firstSample + sample) * 0.0137f);
        maximum = juce::jmax (maximum, std::abs (buffer.getSample (0, sample) - dry));
    }
    return maximum;
}

void writeLittleEndian16 (std::ofstream& stream, uint16_t value)
{
    stream.put ((char) (value & 0xff)); stream.put ((char) ((value >> 8) & 0xff));
}

void writeLittleEndian32 (std::ofstream& stream, uint32_t value)
{
    writeLittleEndian16 (stream, (uint16_t) (value & 0xffff));
    writeLittleEndian16 (stream, (uint16_t) ((value >> 16) & 0xffff));
}

bool renderPhaseOneWav (const char* presetName, TimelineScratchEngine::Preset preset,
                        TimelineScratchEngine::Length length, const char* lengthName,
                        float speed, const char* speedName)
{
    constexpr int sampleRate = 48000, blockSize = 512, channels = 2;
    constexpr double bpm = 140.0, quartersPerBar = 4.0;
    const auto lengthQuarters = length == TimelineScratchEngine::Length::sixteenth ? .25
        : length == TimelineScratchEngine::Length::oneBar ? 4.0 : 1.0;
    const auto totalSamples = juce::roundToInt ((lengthQuarters * 60.0 / bpm + .03) * sampleRate);
    TimelineScratchEngine engine;
    engine.prepare (sampleRate, blockSize, channels);
    juce::AudioBuffer<float> block (channels, blockSize);
    fillHistory (engine, block);
    std::vector<int16_t> rendered;
    rendered.reserve ((size_t) totalSamples * channels);
    auto slotSet = slots (preset, length, speed, 1.0f);
    auto sourceSample = (int64_t) 1000000;
    for (int offset = 0; offset < totalSamples; offset += blockSize)
    {
        const auto samples = juce::jmin (blockSize, totalSamples - offset);
        fillTone (block, sourceSample);
        if (samples < blockSize) block.clear (0, samples, blockSize - samples);
        auto t = transport (1, (double) offset / (sampleRate * quartersPerBar * 60.0 / bpm));
        engine.process (block, t, slotSet);
        for (int sample = 0; sample < samples; ++sample)
            for (int channel = 0; channel < channels; ++channel)
                rendered.push_back ((int16_t) juce::roundToInt (juce::jlimit (-1.0f, 1.0f,
                    block.getSample (channel, sample)) * 32767.0f));
        sourceSample += samples;
    }
    const auto fileName = juce::String ("phase1-render-") + presetName + "-" + lengthName
        + "-speed-" + speedName + ".wav";
    std::ofstream file (fileName.toStdString(), std::ios::binary);
    if (! file.good()) return false;
    const auto payloadBytes = (uint32_t) rendered.size() * sizeof (int16_t);
    file.write ("RIFF", 4); writeLittleEndian32 (file, 36u + payloadBytes); file.write ("WAVEfmt ", 8);
    writeLittleEndian32 (file, 16); writeLittleEndian16 (file, 1); writeLittleEndian16 (file, channels);
    writeLittleEndian32 (file, sampleRate); writeLittleEndian32 (file, sampleRate * channels * 2);
    writeLittleEndian16 (file, channels * 2); writeLittleEndian16 (file, 16);
    file.write ("data", 4); writeLittleEndian32 (file, payloadBytes);
    file.write (reinterpret_cast<const char*> (rendered.data()), (std::streamsize) payloadBytes);
    return file.good() && payloadBytes > 0;
}
}

int main()
{
    constexpr std::array<double, 7> phases { .05, .10, .25, .50, .75, .90, .95 };
    TimelineScratchEngine engine;
    engine.prepare (48000.0, 512, 2);
    check (engine.getAllocatedHistorySamples() == 480000, "history-is-exactly-ten-seconds");

    juce::AudioBuffer<float> audio (2, 512);
    const auto dry = slots (TimelineScratchEngine::Preset::off, TimelineScratchEngine::Length::oneBar);
    fill (audio, 0.375f);
    engine.process (audio, transport (0, 0.0), dry);
    check (audio.getSample (0, 0) == 0.375f && audio.getSample (1, 511) == 0.375f,
           "off-is-bit-exact-dry");

    const auto backspin = slots (TimelineScratchEngine::Preset::backspin, TimelineScratchEngine::Length::oneBar);
    TimelineScratchEngine backspinWrapEngine;
    backspinWrapEngine.prepare (48000.0, 512, 2);
    fillHistory (backspinWrapEngine, audio);
    double backspinPhase = 0.0;
    int64_t backspinSample = 512000;
    float previousBackspinSample = std::sin ((float) (backspinSample - 1) * 0.0137f);
    bool backspinPopFree = true, continuousBackspinIsBounded = true, backspinIsAudiblyWet = true;
    size_t phaseIndex = 0;
    while (backspinPhase < 0.96)
    {
        fillTone (audio, backspinSample);
        backspinWrapEngine.process (audio, transport (1, backspinPhase), backspin);
        const auto state = backspinWrapEngine.getBackspinReadState();
        const auto stateIsBoundedWetReverse = state.active && state.reverseReadRate == -1.0
                                           && state.wetRamp > 0.99f
                                           && state.windowSamples == 12000.0
                                           && state.windowEndSerial - state.windowStartSerial == state.windowSamples
                                           && state.primaryReadSerial >= state.windowStartSerial
                                           && state.primaryReadSerial <= state.windowEndSerial
                                           && state.secondaryReadSerial >= state.windowStartSerial
                                           && state.secondaryReadSerial <= state.windowEndSerial
                                           && state.historyValidSamples >= state.windowSamples;
        continuousBackspinIsBounded &= stateIsBoundedWetReverse;
        const auto differsFromDry = maximumDifferenceFromTone (audio, backspinSample) > 0.02f;
        backspinIsAudiblyWet &= differsFromDry;
        backspinPopFree &= noHardJump (audio, previousBackspinSample) && boundedFinite (audio);
        previousBackspinSample = audio.getSample (0, audio.getNumSamples() - 1);
        const auto nextBackspinPhase = backspinPhase + transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
        while (phaseIndex < phases.size() && nextBackspinPhase >= phases[phaseIndex])
        {
            check (stateIsBoundedWetReverse && differsFromDry,
                   ("backspin-one-bar-bounded-effect-at-phase-" + juce::String (phases[phaseIndex], 2)).toRawUTF8());
            ++phaseIndex;
        }
        backspinPhase = nextBackspinPhase;
        backspinSample += audio.getNumSamples();
    }
    const auto finalBackspinState = backspinWrapEngine.getBackspinReadState();
    check (phaseIndex == phases.size() && continuousBackspinIsBounded && backspinIsAudiblyWet
           && finalBackspinState.completedWraps > 0,
           "backspin-window-stays-bounded-through-one-bar");

    TimelineScratchEngine tapeEngine;
    tapeEngine.prepare (48000.0, 512, 2);
    fillHistory (tapeEngine, audio);
    const auto tapeBrake = slots (TimelineScratchEngine::Preset::tapeBrake, TimelineScratchEngine::Length::oneBar);
    bool tapeAllPhases = true, ratesAreBraking = true, tapePopFree = true;
    auto previousRate = 1.0;
    for (const auto phase : phases)
    {
        const auto rate = TimelineScratchEngine::tapeBrakePlaybackRate (phase, 1.0f);
        fill (audio, 0.0f);
        tapeEngine.process (audio, transport (1, phase), tapeBrake);
        const auto detected = tailEnergy (audio) > 0.01f;
        check (detected, ("tape-brake-one-bar-effect-at-phase-" + juce::String (phase, 2)).toRawUTF8());
        tapeAllPhases &= detected;
        ratesAreBraking &= rate < 1.0 && rate < previousRate;
        previousRate = rate;
        tapePopFree &= boundedFinite (audio);
    }
    check (tapeAllPhases, "tape-brake-one-bar-entire-duration-is-wet");
    check (ratesAreBraking, "tape-brake-rate-is-below-unity-and-continuously-decreases");
    const auto tapeSlow = TimelineScratchEngine::tapeBrakePlaybackRate (.50, .25f);
    const auto tapeNormal = TimelineScratchEngine::tapeBrakePlaybackRate (.50, 1.0f);
    const auto tapeFast = TimelineScratchEngine::tapeBrakePlaybackRate (.50, 4.0f);
    check (tapeSlow > tapeNormal && tapeNormal > tapeFast
           && TimelineScratchEngine::tapeBrakePlaybackRate (1.0, .25f) == 0.0
           && TimelineScratchEngine::tapeBrakePlaybackRate (1.0, 1.0f) == 0.0
           && TimelineScratchEngine::tapeBrakePlaybackRate (1.0, 4.0f) == 0.0,
           "tape-brake-speed-shapes-curve-and-always-stops-at-length-end");
    // A real BAR transition is contiguous. Do not mistake sparse phase probes
    // for audio-adjacent blocks when asserting the click/pop guard.
    TimelineScratchEngine transitionEngine;
    transitionEngine.prepare (48000.0, 512, 2);
    fillHistory (transitionEngine, audio);
    fill (audio, 0.0f);
    transitionEngine.process (audio, transport (1, .995), tapeBrake);
    const auto priorWetSample = audio.getSample (0, audio.getNumSamples() - 1);
    fill (audio, 0.0f);
    transitionEngine.process (audio, transport (2, 0.0), dry);
    tapePopFree &= noHardJump (audio, priorWetSample);
    check (backspinPopFree && tapePopFree, "wet-transition-is-finite-and-bounded");

    TimelineScratchEngine shortLengthEngine;
    shortLengthEngine.prepare (48000.0, 512, 2);
    fillHistory (shortLengthEngine, audio);
    const auto quarterBackspin = slots (TimelineScratchEngine::Preset::backspin, TimelineScratchEngine::Length::quarter);
    fill (audio, 0.0f); shortLengthEngine.process (audio, transport (1, .05), quarterBackspin);
    const auto beforeExpiry = tailEnergy (audio);
    fill (audio, 0.0f); shortLengthEngine.process (audio, transport (1, .40), quarterBackspin);
    shortLengthEngine.process (audio, transport (1, .40), quarterBackspin);
    check (beforeExpiry > 0.01f && tailEnergy (audio) == 0.0f, "length-remains-duration-not-curve-strength");

    TimelineScratchEngine depthEngine;
    depthEngine.prepare (48000.0, 512, 2);
    fillHistory (depthEngine, audio);
    auto depthZero = slots (TimelineScratchEngine::Preset::backspin, TimelineScratchEngine::Length::oneBar, 1.0f, 0.0f);
    fill (audio, .25f); depthEngine.process (audio, transport (1, .05), depthZero);
    check (audio.getSample (0, 511) == .25f && audio.getSample (1, 511) == .25f, "depth-zero-is-bit-exact-dry");

    // OFF permits a short release from a previous wet BAR, then must become
    // exact passthrough rather than merely approximately dry.
    TimelineScratchEngine offTransitionEngine;
    offTransitionEngine.prepare (48000.0, 512, 2);
    fillHistory (offTransitionEngine, audio);
    fillTone (audio, 777000); offTransitionEngine.process (audio, transport (1, .05), backspin);
    fill (audio, .375f); offTransitionEngine.process (audio, transport (2, .0), dry);
    bool offSteadyStateExact = false;
    for (int block = 0; block < 4; ++block)
    {
        fill (audio, .375f); offTransitionEngine.process (audio, transport (2, .01 + block * .01), dry);
        offSteadyStateExact = audio.getSample (0, 511) == .375f && audio.getSample (1, 511) == .375f;
    }
    check (offSteadyStateExact, "off-steady-state-is-bit-exact-after-transition");

    // A BAR's DSP choice is absolute: an effect stored on BAR 4 must not
    // contaminate neighbouring slots, and every BAR remains independently addressable.
    TimelineScratchEngine barEngine;
    barEngine.prepare (48000.0, 512, 2);
    fillHistory (barEngine, audio);
    auto perBar = slots (TimelineScratchEngine::Preset::off, TimelineScratchEngine::Length::oneBar);
    perBar[3] = { TimelineScratchEngine::Preset::backspin, TimelineScratchEngine::Length::oneBar, 1.0f, 0.0f, 1.0f };
    fillTone (audio, 880000); barEngine.process (audio, transport (2, .05), perBar);
    const auto barThreeDry = maximumDifferenceFromTone (audio, 880000) == 0.0f;
    fillTone (audio, 880512); barEngine.process (audio, transport (3, .05), perBar);
    const auto barFourWet = maximumDifferenceFromTone (audio, 880512) > 0.02f;
    fillTone (audio, 881024); barEngine.process (audio, transport (4, .05), perBar);
    fillTone (audio, 881536); barEngine.process (audio, transport (4, .06), perBar);
    const auto barFiveDry = maximumDifferenceFromTone (audio, 881536) == 0.0f;
    check (barThreeDry && barFourWet && barFiveDry, "absolute-bar-slot-selection-is-isolated");

    TimelineScratchEngine continuityEngine;
    continuityEngine.prepare (48000.0, 512, 2);
    bool allBarsAddressed = true;
    for (int bar = 0; bar < 64; ++bar)
    {
        fill (audio, .125f);
        continuityEngine.process (audio, transport (bar, 0.0), dry);
        const auto diagnostics = continuityEngine.getDiagnostics();
        allBarsAddressed &= diagnostics.playingBar == bar && diagnostics.transportResetCount == 0;
    }
    check (allBarsAddressed, "continuous-absolute-bars-1-through-64-do-not-reset");

    bool wavRenders = true;
    for (const auto& preset : std::array<std::pair<const char*, TimelineScratchEngine::Preset>, 3> {{
             { "off", TimelineScratchEngine::Preset::off },
             { "backspin", TimelineScratchEngine::Preset::backspin },
             { "tape-brake", TimelineScratchEngine::Preset::tapeBrake } }})
        for (const auto& length : std::array<std::pair<const char*, TimelineScratchEngine::Length>, 2> {{
                 { "short", TimelineScratchEngine::Length::sixteenth },
                 { "one-bar", TimelineScratchEngine::Length::oneBar } }})
            for (const auto& speed : std::array<std::pair<const char*, float>, 3> {{
                     { "0_25", .25f }, { "1_0", 1.0f }, { "4_0", 4.0f } }})
                wavRenders &= renderPhaseOneWav (preset.first, preset.second, length.second,
                    length.first, speed.second, speed.first);
    check (wavRenders, "phase-one-offline-wav-renders-speed-0-25-1-0-4-0");

    engine.release(); backspinWrapEngine.release(); tapeEngine.release(); transitionEngine.release(); shortLengthEngine.release(); depthEngine.release(); offTransitionEngine.release(); barEngine.release(); continuityEngine.release();
    return passed ? 0 : 1;
}
