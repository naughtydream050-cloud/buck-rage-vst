#include "TimelineScratchEngine.h"
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

TimelineScratchEngine::Transport transport (int bar = 0, double phase = 0.0)
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
}

int main()
{
    TimelineScratchEngine engine;
    engine.prepare (48000.0, 512, 2);
    check (engine.getAllocatedHistorySamples() == 480000, "history-is-exactly-ten-seconds");

    juce::AudioBuffer<float> audio (2, 512);
    auto dry = slots (TimelineScratchEngine::Preset::off, TimelineScratchEngine::Length::oneBar);
    fill (audio, 0.375f);
    engine.process (audio, transport(), dry);
    check (audio.getSample (0, 0) == 0.375f && audio.getSample (1, 511) == 0.375f,
           "off-is-bit-exact-dry");

    // Fill history while playing dry, then start a BACKSPIN on the next BAR.
    for (int block = 0; block < 96; ++block)
    {
        for (int i = 0; i < audio.getNumSamples(); ++i)
            audio.setSample (0, i, std::sin ((float) (block * 512 + i) * 0.01f)),
            audio.setSample (1, i, std::cos ((float) (block * 512 + i) * 0.01f));
        engine.process (audio, transport(), dry);
    }
    auto backspin = slots (TimelineScratchEngine::Preset::backspin, TimelineScratchEngine::Length::sixteenth, 1.0f, 1.0f);
    fill (audio, 0.0f);
    engine.process (audio, transport (1), backspin);
    float energy = 0.0f;
    for (int i = 0; i < audio.getNumSamples(); ++i) energy += std::abs (audio.getSample (0, i));
    check (energy > 0.01f, "backspin-reads-stereo-history");

    // After a 1/16 duration (0.25 quarters), output returns fully dry inside
    // the same bar.  The input is zero, so any tail here would be a failure.
    fill (audio, 0.0f);
    auto afterLength = transport (1, 0.10); // 0.4 quarters, after 1/16 duration
    engine.process (audio, afterLength, backspin);
    engine.process (audio, afterLength, backspin); // complete the click-protection fade
    float tail = 0.0f;
    for (int i = 0; i < audio.getNumSamples(); ++i) tail += std::abs (audio.getSample (0, i));
    check (tail == 0.0f, "length-expiry-is-dry-within-bar");

    engine.resetTransport();
    fill (audio, 0.0f);
    engine.process (audio, transport (2), backspin);
    float resetEnergy = 0.0f;
    for (int i = 0; i < audio.getNumSamples(); ++i) resetEnergy += std::abs (audio.getSample (0, i));
    check (resetEnergy == 0.0f, "transport-reset-never-reads-stale-history");

    engine.release();
    return passed ? 0 : 1;
}
