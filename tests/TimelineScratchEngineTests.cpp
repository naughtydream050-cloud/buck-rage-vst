#include "TimelineScratchEngine.h"
#include <array>
#include <cmath>
#include <iostream>

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
    result.barPhasePerSample = 1.0 / (4.0 * 48000.0);
    result.quartersPerBar = 4.0; result.secondsPerQuarter = 0.5;
    return result;
}

void fill (juce::AudioBuffer<float>& buffer, float value)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            buffer.setSample (channel, sample, value);
}

void fillHistory (TimelineScratchEngine& engine, juce::AudioBuffer<float>& buffer)
{
    const auto dry = slots (TimelineScratchEngine::Preset::off, TimelineScratchEngine::Length::oneBar);
    for (int block = 0; block < 1000; ++block)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto phase = (float) (block * buffer.getNumSamples() + sample) * 0.0137f;
            buffer.setSample (0, sample, std::sin (phase));
            buffer.setSample (1, sample, std::cos (phase));
        }
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
}

int main()
{
    constexpr std::array<double, 5> phases { .05, .25, .50, .75, .95 };
    TimelineScratchEngine engine;
    engine.prepare (48000.0, 512, 2);
    check (engine.getAllocatedHistorySamples() == 480000, "history-is-exactly-ten-seconds");

    juce::AudioBuffer<float> audio (2, 512);
    const auto dry = slots (TimelineScratchEngine::Preset::off, TimelineScratchEngine::Length::oneBar);
    fill (audio, 0.375f);
    engine.process (audio, transport (0, 0.0), dry);
    check (audio.getSample (0, 0) == 0.375f && audio.getSample (1, 511) == 0.375f,
           "off-is-bit-exact-dry");

    fillHistory (engine, audio);
    const auto backspin = slots (TimelineScratchEngine::Preset::backspin, TimelineScratchEngine::Length::oneBar);
    bool backspinAllPhases = true, backspinPopFree = true;
    for (const auto phase : phases)
    {
        fill (audio, 0.0f);
        engine.process (audio, transport (1, phase), backspin);
        const auto detected = tailEnergy (audio) > 0.01f;
        check (detected, ("backspin-one-bar-effect-at-phase-" + juce::String (phase, 2)).toRawUTF8());
        backspinAllPhases &= detected;
        backspinPopFree &= boundedFinite (audio);
    }
    check (backspinAllPhases, "backspin-one-bar-entire-duration-is-wet");

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

    engine.release(); tapeEngine.release(); transitionEngine.release(); shortLengthEngine.release(); depthEngine.release();
    return passed ? 0 : 1;
}
