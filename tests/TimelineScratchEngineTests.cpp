#include "TimelineScratchEngine.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
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

bool renderScratchWav (const char* presetName, TimelineScratchEngine::Preset preset,
                        TimelineScratchEngine::Length length, const char* lengthName,
                        float speed, const char* speedName)
{
    constexpr int sampleRate = 48000, blockSize = 512, channels = 2;
    constexpr double bpm = 140.0, quartersPerBar = 4.0;
    const auto lengthQuarters = length == TimelineScratchEngine::Length::sixteenth ? .25
        : length == TimelineScratchEngine::Length::eighth ? .5
        : length == TimelineScratchEngine::Length::quarter ? 1.0
        : length == TimelineScratchEngine::Length::half ? 2.0 : 4.0;
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
    const auto fileName = juce::String ((preset == TimelineScratchEngine::Preset::baby
        || preset == TimelineScratchEngine::Preset::drag) ? "phase2-render-" : "phase1-render-")
        + presetName + "-" + lengthName
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

void testSampleObserver()
{
    using Engine = TimelineScratchEngine;
    struct Collector
    {
        std::array<Engine::SampleTrace, 512> records {};
        size_t count = 0;
        bool overflow = false;
    };
    const auto collect = [] (const Engine::SampleTrace& trace, void* context) noexcept
    {
        auto& c = *static_cast<Collector*> (context);
        if (c.count < c.records.size()) c.records[c.count++] = trace;
        else c.overflow = true;
    };
    bool sameAudio = true, traceAligned = true, everySlot = true, repeatedWet = true;
    for (const auto preset : { Engine::Preset::off, Engine::Preset::forwardCut,
                               Engine::Preset::backspin, Engine::Preset::chirp,
                               Engine::Preset::baby, Engine::Preset::transform,
                               Engine::Preset::drag, Engine::Preset::zigzag,
                               Engine::Preset::tapeBrake })
    {
        Engine plain, observed;
        Collector collector;
        plain.prepare (48000, 512, 2); observed.prepare (48000, 512, 2);
        observed.setSampleTraceObserver (collect, &collector);
        const auto configuration = slots (preset, Engine::Length::oneBar);
        std::array<std::array<uint64_t, 4>, 3> wetSamples {};
        uint64_t total = 0;
        for (int pass = 0; pass < 3; ++pass)
            for (int first = 0; first < 384000; first += 512)
            {
                const auto frames = juce::jmin (512, 384000 - first);
                juce::AudioBuffer<float> a (2, frames), b (2, frames);
                fillTone (a, first); b.makeCopyOf (a);
                Engine::Transport t;
                t.playing = true; t.startBar = first / 96000;
                t.startBarPhase = (double) (first % 96000) / 96000.0;
                t.barPhasePerSample = 1.0 / 96000.0;
                t.secondsPerQuarter = 0.5;
                t.discontinuity = first == 0;
                t.discontinuityReason = pass == 0 ? Engine::DiscontinuityReason::start
                                                  : Engine::DiscontinuityReason::loopWrap;
                collector.count = 0;
                plain.process (a, t, configuration); observed.process (b, t, configuration);
                for (int channel = 0; channel < 2; ++channel)
                    sameAudio &= std::memcmp (a.getReadPointer (channel), b.getReadPointer (channel),
                                               (size_t) frames * sizeof (float)) == 0;
                traceAligned &= collector.count == (size_t) frames && ! collector.overflow;
                for (size_t i = 0; i < collector.count; ++i)
                {
                    const auto& trace = collector.records[i];
                    const auto expectedBar = (first + (int) i) / 96000;
                    everySlot &= trace.localBar == expectedBar;
                    traceAligned &= trace.sampleIndex == i && trace.writerSerial == total + i
                        && trace.outputLeft == b.getSample (0, (int) i)
                        && trace.outputRight == b.getSample (1, (int) i);
                    if (trace.captured && trace.wetEvaluated && trace.effectiveWet > 0.99f
                        && (! trace.motionGateEnabled || trace.gate > 0.5))
                        ++wetSamples[(size_t) pass][(size_t) expectedBar];
                }
                total += (uint64_t) frames;
            }
        if (preset == Engine::Preset::chirp || preset == Engine::Preset::transform
            || preset == Engine::Preset::zigzag)
            for (int pass = 1; pass < 3; ++pass)
                for (int bar = 0; bar < 4; ++bar)
                    repeatedWet &= wetSamples[(size_t) pass][(size_t) bar] > 20000;
        observed.setSampleTraceObserver (nullptr, nullptr);
    }
    check (sameAudio, "diagnostic-observer-on-off-bit-exact-all-nine-presets");
    check (traceAligned, "diagnostic-observer-every-sample-pcm-serial-aligned");
    check (everySlot, "diagnostic-observer-three-pass-four-bar-every-sample-slot");
    check (repeatedWet, "diagnostic-observer-each-repeat-bar-sustained-gate-open-wet-count");
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

    // BABY should move one fixed recent window forward and backward for the
    // full requested LENGTH, without using a crossfader/gate.
    TimelineScratchEngine babyEngine;
    babyEngine.prepare (48000.0, 512, 2);
    fillHistory (babyEngine, audio);
    const auto baby = slots (TimelineScratchEngine::Preset::baby, TimelineScratchEngine::Length::oneBar);
    double babyPhase = 0.0;
    int64_t babySample = 512000;
    bool babyWetThroughout = true, babyBounded = true, babyPopFree = true;
    bool babyForward = false, babyReverse = false;
    double babyMinRead = 1.0e18, babyMaxRead = -1.0e18;
    float previousBabySample = std::sin ((float) (babySample - 1) * 0.0137f);
    size_t babyPhaseIndex = 0;
    while (babyPhase < 0.96)
    {
        fillTone (audio, babySample);
        babyEngine.process (audio, transport (1, babyPhase), baby);
        const auto diagnostics = babyEngine.getDiagnostics();
        const auto differsFromDry = maximumDifferenceFromTone (audio, babySample) > 0.02f;
        const auto active = diagnostics.preset == TimelineScratchEngine::Preset::baby
            && diagnostics.effectiveWet > 0.99f && differsFromDry;
        babyWetThroughout &= active;
        babyBounded &= diagnostics.captureWindowSamples == 12000.0
            && diagnostics.readPosition > 0.0;
        babyMinRead = juce::jmin (babyMinRead, diagnostics.readPosition);
        babyMaxRead = juce::jmax (babyMaxRead, diagnostics.readPosition);
        babyForward |= diagnostics.readRate > 0.1;
        babyReverse |= diagnostics.readRate < -0.1;
        babyPopFree &= boundedFinite (audio) && noHardJump (audio, previousBabySample);
        previousBabySample = audio.getSample (0, audio.getNumSamples() - 1);
        const auto nextPhase = babyPhase + transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
        while (babyPhaseIndex < phases.size() && nextPhase >= phases[babyPhaseIndex])
        {
            check (active, ("baby-one-bar-wet-at-phase-" + juce::String (phases[babyPhaseIndex], 2)).toRawUTF8());
            ++babyPhaseIndex;
        }
        babyPhase = nextPhase;
        babySample += audio.getNumSamples();
    }
    check (babyPhaseIndex == phases.size() && babyWetThroughout, "baby-wet-throughout-one-bar");
    check (babyForward && babyReverse, "baby-read-motion-alternates-forward-and-reverse");
    check (babyBounded && babyMaxRead - babyMinRead <= 12000.0, "baby-fixed-bounded-capture");
    check (babyPopFree, "baby-no-click-or-nonfinite-output");

    // Constant audio must remain constant while the BABY effect releases at
    // LENGTH end. Returning a zero wet sample during the ramp causes a pop.
    TimelineScratchEngine babyReleaseEngine;
    babyReleaseEngine.prepare (48000.0, 512, 2);
    for (int block = 0; block < 1000; ++block)
    {
        fill (audio, .7f);
        babyReleaseEngine.process (audio, transport (0, 0.0), dry);
    }
    const auto shortBaby = slots (TimelineScratchEngine::Preset::baby,
                                  TimelineScratchEngine::Length::quarter);
    fill (audio, .7f); babyReleaseEngine.process (audio, transport (1, 0.0), shortBaby);
    fill (audio, .7f); babyReleaseEngine.process (audio, transport (1, .249), shortBaby);
    check (noHardJump (audio, .7f) && std::abs (audio.getSample (0, 511) - .7f) < .001f,
           "baby-length-end-releases-without-silence-pop");

    TimelineScratchEngine babyColdEngine;
    babyColdEngine.prepare (48000.0, 512, 2);
    fill (audio, .7f); babyColdEngine.process (audio, transport (1, 0.0), baby);
    check (audio.getSample (0, 511) == .7f && babyColdEngine.getDiagnostics().effectiveWet == 0.0f,
           "baby-waits-dry-for-enough-history");

    std::array<int, 3> babyDirectionChanges {};
    for (size_t speedIndex = 0; speedIndex < babyDirectionChanges.size(); ++speedIndex)
    {
        const auto speed = std::array<float, 3> { .25f, 1.0f, 4.0f }[speedIndex];
        TimelineScratchEngine speedEngine;
        speedEngine.prepare (48000.0, 512, 2);
        fillHistory (speedEngine, audio);
        const auto speedBaby = slots (TimelineScratchEngine::Preset::baby,
                                      TimelineScratchEngine::Length::oneBar, speed);
        double phase = 0.0;
        int previousDirection = 0;
        while (phase < .95)
        {
            fill (audio, .7f);
            speedEngine.process (audio, transport (1, phase), speedBaby);
            const auto rate = speedEngine.getDiagnostics().readRate;
            const auto direction = rate > .05 ? 1 : rate < -.05 ? -1 : 0;
            if (direction != 0 && previousDirection != 0 && direction != previousDirection)
                ++babyDirectionChanges[speedIndex];
            if (direction != 0) previousDirection = direction;
            phase += transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
        }
    }
    check (babyDirectionChanges[0] < babyDirectionChanges[1]
           && babyDirectionChanges[1] < babyDirectionChanges[2],
           "baby-speed-controls-forward-reverse-gesture-rate");

    // DRAG uses one fixed forward snapshot for the whole effect.  Its rate is
    // deliberately below unity and is independent of any live writer head,
    // so the snapshot cannot be recaptured or overtaken during a BAR.
    for (size_t speedIndex = 0; speedIndex < 3; ++speedIndex)
    {
        const auto speed = std::array<float, 3> { .25f, 1.0f, 4.0f }[speedIndex];
        TimelineScratchEngine dragEngine;
        dragEngine.prepare (48000.0, 512, 2);
        fillHistory (dragEngine, audio);
        const auto drag = slots (TimelineScratchEngine::Preset::drag,
                                 TimelineScratchEngine::Length::oneBar, speed, 1.0f);
        double dragPhase = 0.0;
        double firstRead = 0.0, lastRead = 0.0, fixedWindow = 0.0;
        bool dragWet = false, dragForward = true, dragFinite = true;
        for (int block = 0; block < 24; ++block)
        {
            fillTone (audio, 730000 + block * audio.getNumSamples());
            dragEngine.process (audio, transport (1, dragPhase), drag);
            const auto state = dragEngine.getDiagnostics();
            if (block == 0) { firstRead = state.readPosition; fixedWindow = state.captureWindowSamples; }
            lastRead = state.readPosition;
            dragWet |= state.effectiveWet > 0.99f;
            dragForward &= state.readRate > 0.0 && state.readRate < 1.0;
            dragFinite &= boundedFinite (audio);
            dragPhase += transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
        }
        const auto expectedRate = std::array<double, 3> { .25, .45, .85 }[speedIndex];
        check (dragEngine.getDiagnostics().readRate == expectedRate,
               speedIndex == 0 ? "drag-read-rate-0-25" : speedIndex == 1
               ? "drag-read-rate-1-0" : "drag-read-rate-4-0");
        check (dragWet && dragForward && dragFinite,
               speedIndex == 0 ? "drag-forward-fixed-snapshot-0-25" : speedIndex == 1
               ? "drag-forward-fixed-snapshot-1-0" : "drag-forward-fixed-snapshot-4-0");
        check (fixedWindow > 0.0 && lastRead >= firstRead
                   && lastRead <= firstRead + fixedWindow + 1.0,
               speedIndex == 0 ? "drag-bounded-window-0-25" : speedIndex == 1
               ? "drag-bounded-window-1-0" : "drag-bounded-window-4-0");
        dragEngine.release();
    }

    // Every LENGTH remains an effect-duration control, not a read-rate curve.
    for (const auto length : { TimelineScratchEngine::Length::sixteenth,
                               TimelineScratchEngine::Length::eighth,
                               TimelineScratchEngine::Length::quarter,
                               TimelineScratchEngine::Length::half,
                               TimelineScratchEngine::Length::oneBar })
    {
        TimelineScratchEngine dragLengthEngine;
        dragLengthEngine.prepare (48000.0, 512, 2);
        fillHistory (dragLengthEngine, audio);
        auto drag = slots (TimelineScratchEngine::Preset::off, length, 1.0f, 1.0f);
        drag[0] = { TimelineScratchEngine::Preset::drag, length, 1.0f, 0.0f, 1.0f };
        const auto expiryPhase = length == TimelineScratchEngine::Length::oneBar ? 0.999 : 0.95;
        fillTone (audio, 740000); dragLengthEngine.process (audio, transport (1, 0.0), drag);
        fillTone (audio, 740512); dragLengthEngine.process (audio, transport (1, expiryPhase), drag);
        fillTone (audio, 741024);
        if (length == TimelineScratchEngine::Length::oneBar)
            dragLengthEngine.process (audio, transport (2, 0.0), drag);
        else
            dragLengthEngine.process (audio, transport (1, expiryPhase), drag);
        check (dragLengthEngine.getDiagnostics().effectiveWet == 0.0f,
               "drag-length-ends-in-dry");
        dragLengthEngine.release();
    }

    TimelineScratchEngine dragSafetyEngine;
    dragSafetyEngine.prepare (48000.0, 512, 2);
    fillHistory (dragSafetyEngine, audio);
    auto dragDepthZero = slots (TimelineScratchEngine::Preset::drag,
                                TimelineScratchEngine::Length::oneBar, 1.0f, 0.0f);
    fill (audio, 0.375f);
    dragSafetyEngine.process (audio, transport (1, 0.05), dragDepthZero);
    check (audio.getSample (0, 511) == 0.375f && audio.getSample (1, 511) == 0.375f,
           "drag-depth-zero-is-bit-exact-dry");
    auto dragWet = slots (TimelineScratchEngine::Preset::drag,
                          TimelineScratchEngine::Length::oneBar, 1.0f, 1.0f);
    fillTone (audio, 750000);
    dragSafetyEngine.process (audio, transport (1, 0.10), dragWet);
    TimelineScratchEngine::Transport dragStopped;
    dragStopped.playing = false; dragStopped.startBar = -1;
    fill (audio, 0.125f);
    dragSafetyEngine.process (audio, dragStopped, dragWet);
    check (dragSafetyEngine.getDiagnostics().effectiveWet == 0.0f
               && dragSafetyEngine.getDiagnostics().playingBar == -1,
           "drag-stop-invalidates-snapshot-and-returns-dry");
    dragSafetyEngine.release();

    // A loop can fall in the middle of an audio block.  The old BAR must stop
    // at that exact sample; DRAG then anchors to the new BAR's live writer
    // with only the realtime safety margin, not the old BAR's long snapshot.
    TimelineScratchEngine dragLoopEngine;
    dragLoopEngine.prepare (48000.0, 512, 2);
    fillHistory (dragLoopEngine, audio);
    auto dragLoopSlots = slots (TimelineScratchEngine::Preset::drag,
                                TimelineScratchEngine::Length::oneBar, 1.0f, 1.0f);
    fill (audio, 0.85f);
    auto dragLoopTransport = transport (3, 0.99);
    dragLoopTransport.loopBoundarySample = 256;
    dragLoopTransport.loopStartBar = 0;
    dragLoopTransport.loopStartPhase = 0.0;
    for (int sample = 256; sample < audio.getNumSamples(); ++sample)
    {
        audio.setSample (0, sample, 0.125f);
        audio.setSample (1, sample, 0.125f);
    }
    dragLoopEngine.process (audio, dragLoopTransport, dragLoopSlots);
    float postLoopError = 0.0f;
    for (int sample = 384; sample < audio.getNumSamples(); ++sample)
        postLoopError += std::abs (audio.getSample (0, sample) - 0.125f);
    check (dragLoopEngine.getDiagnostics().playingBar == 0
               && postLoopError < 20.0f
               && boundedFinite (audio),
           "drag-mid-block-loop-stops-old-bar-and-restarts-at-bar-one");
    dragLoopEngine.release();

    // FORWARD CUT uses one short forward snapshot, retriggered with a short
    // cut at the end of each cycle.  It must be audible from the BAR start,
    // stay inside the fixed capture, and remain click-safe at retriggers.
    TimelineScratchEngine forwardEngine;
    forwardEngine.prepare (48000.0, 512, 2);
    fillHistory (forwardEngine, audio);
    const auto forward = slots (TimelineScratchEngine::Preset::forwardCut,
                                TimelineScratchEngine::Length::oneBar, 1.0f, 1.0f);
    double forwardPhase = 0.0;
    int64_t forwardSample = 512000;
    bool forwardWetThroughout = true, forwardBounded = true, forwardPopFree = true;
    bool forwardRetriggered = false;
    float previousForwardSample = std::sin ((float) (forwardSample - 1) * 0.0137f);
    size_t forwardPhaseIndex = 0;
    while (forwardPhase < 0.96)
    {
        fillTone (audio, forwardSample);
        forwardEngine.process (audio, transport (1, forwardPhase), forward);
        const auto diagnostics = forwardEngine.getDiagnostics();
        const auto differsFromDry = maximumDifferenceFromTone (audio, forwardSample) > 0.02f;
        const auto active = diagnostics.preset == TimelineScratchEngine::Preset::forwardCut
            && diagnostics.effectiveWet > 0.99f && differsFromDry;
        forwardWetThroughout &= active;
        forwardBounded &= diagnostics.captureWindowSamples == 6000.0
            && diagnostics.readPosition > 0.0;
        forwardRetriggered |= diagnostics.readRate > 0.1 && diagnostics.readPosition > 0.0;
        forwardPopFree &= boundedFinite (audio) && noHardJump (audio, previousForwardSample);
        previousForwardSample = audio.getSample (0, audio.getNumSamples() - 1);
        const auto nextPhase = forwardPhase + transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
        while (forwardPhaseIndex < phases.size() && nextPhase >= phases[forwardPhaseIndex])
        {
            check (active, ("forward-cut-one-bar-effect-at-phase-"
                           + juce::String (phases[forwardPhaseIndex], 2)).toRawUTF8());
            ++forwardPhaseIndex;
        }
        forwardPhase = nextPhase;
        forwardSample += audio.getNumSamples();
    }
    check (forwardPhaseIndex == phases.size() && forwardWetThroughout,
           "forward-cut-wet-throughout-one-bar");
    check (forwardBounded && forwardRetriggered, "forward-cut-fixed-forward-capture");
    check (forwardPopFree, "forward-cut-no-click-or-nonfinite-output");

    // FORWARD CUT's speed controls only the cycle clock.  The read head is
    // always native forward rate, and every cycle contains a real wet-silence
    // interval before a new head-trigger.
    std::array<uint32_t, 3> forwardRetriggers {};
    for (size_t speedIndex = 0; speedIndex < 3; ++speedIndex)
    {
        const auto speed = std::array<float, 3> { .25f, 1.0f, 4.0f }[speedIndex];
        TimelineScratchEngine speedEngine;
        speedEngine.prepare (48000.0, 512, 2);
        fillHistory (speedEngine, audio);
        const auto speedSlots = slots (TimelineScratchEngine::Preset::forwardCut,
                                       TimelineScratchEngine::Length::oneBar, speed, 1.0f);
        double speedPhase = 0.0;
        while (speedPhase < 0.50)
        {
            fillTone (audio, 800000 + (int64_t) (speedPhase * 100000.0));
            speedEngine.process (audio, transport (1, speedPhase), speedSlots);
            const auto state = speedEngine.getDiagnostics();
            check (state.readRate == 1.0, speedIndex == 0 ? "forward-cut-read-rate-0-25"
                : speedIndex == 1 ? "forward-cut-read-rate-1-0" : "forward-cut-read-rate-4-0");
            check (state.forwardCycleSamples > 0.0 && state.forwardReadSamples < state.forwardCycleSamples,
                   speedIndex == 0 ? "forward-cut-cycle-0-25" : speedIndex == 1 ? "forward-cut-cycle-1-0" : "forward-cut-cycle-4-0");
            speedPhase += transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
        }
        forwardRetriggers[speedIndex] = speedEngine.getDiagnostics().forwardRetriggerCount;
        speedEngine.release();
    }
    check (forwardRetriggers[0] < forwardRetriggers[1] && forwardRetriggers[1] < forwardRetriggers[2],
           "forward-cut-speed-increases-retrigger-density");

    // Source silence is measured on the pre-effect input, sample by sample.
    // A short gap is tolerated; a 30 ms hold latches the current BAR dry.
    TimelineScratchEngine silenceEngine;
    silenceEngine.prepare (48000.0, 512, 2);
    fillHistory (silenceEngine, audio);
    const auto silenceSlots = slots (TimelineScratchEngine::Preset::forwardCut,
                                     TimelineScratchEngine::Length::oneBar, 1.0f, 1.0f);
    double silencePhase = 0.0;
    for (int block = 0; block < 12; ++block)
    {
        fillTone (audio, 900000 + block * audio.getNumSamples());
        silenceEngine.process (audio, transport (1, silencePhase), silenceSlots);
        silencePhase += transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
    }
    for (int block = 0; block < 1; ++block)
    {
        fill (audio, 0.0f);
        silenceEngine.process (audio, transport (1, silencePhase), silenceSlots);
        silencePhase += transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
    }
    check (! silenceEngine.getDiagnostics().forwardSourceEnded,
           "forward-cut-short-gap-does-not-end-source");
    const auto retriggersBeforeSourceEnd = silenceEngine.getDiagnostics().forwardRetriggerCount;
    for (int block = 0; block < 3; ++block)
    {
        fill (audio, 0.0f);
        silenceEngine.process (audio, transport (1, silencePhase), silenceSlots);
        silencePhase += transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
    }
    const auto endedState = silenceEngine.getDiagnostics();
    check (endedState.forwardSourceEnded && endedState.forwardRetriggerCount == retriggersBeforeSourceEnd,
           "forward-cut-30ms-source-silence-latches-current-bar");
    check (endedState.effectiveWet < 0.5f, "forward-cut-source-ended-wet-to-dry");
    fillTone (audio, 910000);
    silenceEngine.process (audio, transport (1, silencePhase), silenceSlots);
    check (silenceEngine.getDiagnostics().forwardSourceEnded,
           "forward-cut-source-latch-ignores-input-restart");

    // Transport STOP is independent from source silence and invalidates state.
    fillTone (audio, 920000);
    silenceEngine.process (audio, transport (1, silencePhase), silenceSlots);
    TimelineScratchEngine::Transport stopped;
    stopped.playing = false; stopped.startBar = -1;
    silenceEngine.process (audio, stopped, silenceSlots);
    check (silenceEngine.getDiagnostics().effectiveWet == 0.0f
               && silenceEngine.getDiagnostics().playingBar == -1,
           "forward-cut-transport-stop-returns-dry");
    silenceEngine.release();

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

    // CHIRP, TRANSFORM and ZIGZAG share one fixed capture window but expose
    // distinct motion/gate behavior.  Constant history makes gate silence
    // deterministic instead of depending on an input zero crossing.
    for (const auto& preset : std::array<std::pair<const char*, TimelineScratchEngine::Preset>, 3> {{
             { "chirp", TimelineScratchEngine::Preset::chirp },
             { "transform", TimelineScratchEngine::Preset::transform },
             { "zigzag", TimelineScratchEngine::Preset::zigzag } }})
    {
        TimelineScratchEngine motionEngine;
        motionEngine.prepare (48000.0, 512, 2);
        for (int block = 0; block < 1000; ++block)
        {
            fill (audio, .7f);
            motionEngine.process (audio, transport (0, 0.0), dry);
        }
        const auto motionSlots = slots (preset.second, TimelineScratchEngine::Length::oneBar, 1.0f, 1.0f);
        bool wetSeen = false, finite = true, fixedCapture = true, silenceSeen = false;
        bool positiveRate = false, negativeRate = false;
        double firstWindow = 0.0;
        double phase = 0.0;
        for (int block = 0; block < 90; ++block)
        {
            fill (audio, .7f);
            motionEngine.process (audio, transport (1, phase), motionSlots);
            const auto state = motionEngine.getDiagnostics();
            if (block == 0) firstWindow = state.captureWindowSamples;
            fixedCapture &= state.captureWindowSamples == firstWindow;
            wetSeen |= state.effectiveWet > 0.99f;
            finite &= boundedFinite (audio);
            auto energy = 0.0f;
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                energy += std::abs (audio.getSample (0, sample));
            silenceSeen |= energy < 1.0f;
            positiveRate |= state.readRate > 0.001;
            negativeRate |= state.readRate < -0.001;
            phase += transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
        }
        check (wetSeen && finite && fixedCapture && firstWindow == 6000.0,
               (std::string (preset.first) + "-fixed-capture-and-wet").c_str());
        if (preset.second == TimelineScratchEngine::Preset::zigzag)
            check (positiveRate && negativeRate && ! silenceSeen,
                   "zigzag-forward-reverse-no-gate-silence");
        else
            check (positiveRate && negativeRate && silenceSeen,
                   preset.second == TimelineScratchEngine::Preset::chirp
                       ? "chirp-forward-reverse-gate-silence"
                       : "transform-forward-reverse-gate-silence");
        motionEngine.release();
    }

    TimelineScratchEngine motionSilenceEngine;
    motionSilenceEngine.prepare (48000.0, 512, 2);
    for (int block = 0; block < 1000; ++block)
    {
        fill (audio, .7f);
        motionSilenceEngine.process (audio, transport (0, 0.0), dry);
    }
    const auto chirpSlots = slots (TimelineScratchEngine::Preset::chirp,
                                   TimelineScratchEngine::Length::oneBar, 1.0f, 1.0f);
    double motionSilencePhase = 0.0;
    for (int block = 0; block < 8; ++block)
    {
        fill (audio, .7f);
        motionSilenceEngine.process (audio, transport (1, motionSilencePhase), chirpSlots);
        motionSilencePhase += transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
    }
    for (int block = 0; block < 3; ++block)
    {
        fill (audio, 0.0f);
        motionSilenceEngine.process (audio, transport (1, motionSilencePhase), chirpSlots);
        motionSilencePhase += transport (1, 0.0).barPhasePerSample * audio.getNumSamples();
    }
    const auto motionEnded = motionSilenceEngine.getDiagnostics();
    check (motionEnded.motionSourceEnded,
           "chirp-30ms-source-silence-latches-current-bar");
    fill (audio, 0.0f);
    motionSilenceEngine.process (audio, transport (1, motionSilencePhase), chirpSlots);
    check (motionSilenceEngine.getDiagnostics().effectiveWet < 0.5f,
           "chirp-source-ended-wet-to-dry");
    fill (audio, .7f);
    motionSilenceEngine.process (audio, transport (1, motionSilencePhase), chirpSlots);
    check (motionSilenceEngine.getDiagnostics().motionSourceEnded,
           "chirp-source-latch-ignores-input-restart");
    motionSilenceEngine.release();

    // Cold-start BAR1 keeps the BAR-relative motion phase while the fixed
    // capture window becomes readable. The previous implementation reset it
    // to zero at delayed capture, causing an unstable first phrase.
    for (const auto preset : { TimelineScratchEngine::Preset::chirp,
                               TimelineScratchEngine::Preset::transform,
                               TimelineScratchEngine::Preset::zigzag })
    {
        TimelineScratchEngine coldStart;
        coldStart.prepare (48000.0, 512, 2);
        const auto coldSlots = slots (preset, TimelineScratchEngine::Length::oneBar, 1.0f, 1.0f);
        double phase = 0.0;
        bool captured = false;
        double capturePhase = 0.0;
        for (int block = 0; block < 24; ++block)
        {
            fillTone (audio, (int64_t) block * audio.getNumSamples());
            coldStart.process (audio, transport (0, phase), coldSlots);
            const auto state = coldStart.getDiagnostics();
            if (! captured && state.motionCaptured)
            {
                captured = true;
                capturePhase = state.motionCapturePhase;
            }
            phase += transport (0, 0.0).barPhasePerSample * audio.getNumSamples();
        }
        check (captured && capturePhase > 1.0
                   && capturePhase < coldStart.getDiagnostics().motionCycleSamples,
               "cold-start-motion-capture-preserves-bar-phase");
        coldStart.release();
    }

    // A BAR that starts with more than 30 ms of silence must wait for a real
    // dry signal instead of latching source-ended before capture.  This is
    // the regression that made BAR1/BAR5 silent in four-bar phrases.
    for (const auto preset : { TimelineScratchEngine::Preset::chirp,
                               TimelineScratchEngine::Preset::transform,
                               TimelineScratchEngine::Preset::zigzag })
    {
        TimelineScratchEngine delayedCapture;
        delayedCapture.prepare (48000.0, 512, 2);
        for (int block = 0; block < 1000; ++block)
        {
            fill (audio, .7f);
            delayedCapture.process (audio, transport (63, 0.0), dry);
        }
        const auto motionSlots = slots (preset, TimelineScratchEngine::Length::oneBar, 1.0f, 1.0f);
        double phase = 0.0;
        for (int block = 0; block < 4; ++block)
        {
            fill (audio, 0.0f);
            delayedCapture.process (audio, transport (0, phase), motionSlots);
            phase += transport (0, 0.0).barPhasePerSample * audio.getNumSamples();
        }
        const auto waiting = delayedCapture.getDiagnostics();
        check (! waiting.motionSourceEnded && waiting.effectiveWet < 0.01f,
               "motion-silence-waits-without-latching");
        fillTone (audio, 1900000);
        delayedCapture.process (audio, transport (0, phase), motionSlots);
        const auto started = delayedCapture.getDiagnostics();
        check (! started.motionSourceEnded && started.effectiveWet > 0.5f,
               "motion-captures-after-signal-on-silent-bar");
        delayedCapture.release();
    }

    // Eight consecutive BAR entries, including BAR1 and BAR5, must expose
    // an active effect. DRAG is included to guard against false coupling to
    // the motion source-silence detector. AC-08 keeps the existing OFF,
    // FORWARD CUT, BACKSPIN, TAPE BRAKE and BABY regression checks intact.
    for (const auto preset : { TimelineScratchEngine::Preset::chirp,
                               TimelineScratchEngine::Preset::drag,
                               TimelineScratchEngine::Preset::transform,
                               TimelineScratchEngine::Preset::zigzag })
    {
        TimelineScratchEngine eightBarEngine;
        eightBarEngine.prepare (48000.0, 512, 2);
        for (int block = 0; block < 1000; ++block)
        {
            fillTone (audio, (int64_t) block * audio.getNumSamples());
            eightBarEngine.process (audio, transport (63, 0.0), dry);
        }
        const auto barSlots = slots (preset, TimelineScratchEngine::Length::oneBar, 1.0f, 1.0f);
        bool everyBarWet = true;
        for (int bar = 0; bar < 8; ++bar)
        {
            bool barWet = false;
            for (int block = 0; block < 4; ++block)
            {
                const auto first = (int64_t) (bar * 4 + block) * audio.getNumSamples();
                fillTone (audio, first);
                eightBarEngine.process (audio, transport (bar, 0.05), barSlots);
                barWet |= eightBarEngine.getDiagnostics().effectiveWet > 0.5f
                    && maximumDifferenceFromTone (audio, first) > 0.005f;
            }
            everyBarWet &= barWet;
        }
        check (everyBarWet, "eight-bar-chain-includes-bar1-and-bar5");
        eightBarEngine.release();
    }

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
    for (const auto& preset : std::array<std::pair<const char*, TimelineScratchEngine::Preset>, 9> {{
             { "off", TimelineScratchEngine::Preset::off },
             { "forward-cut", TimelineScratchEngine::Preset::forwardCut },
             { "backspin", TimelineScratchEngine::Preset::backspin },
             { "tape-brake", TimelineScratchEngine::Preset::tapeBrake },
             { "baby", TimelineScratchEngine::Preset::baby },
             { "drag", TimelineScratchEngine::Preset::drag },
             { "chirp", TimelineScratchEngine::Preset::chirp },
             { "transform", TimelineScratchEngine::Preset::transform },
             { "zigzag", TimelineScratchEngine::Preset::zigzag } }})
        for (const auto& length : std::array<std::pair<const char*, TimelineScratchEngine::Length>, 5> {{
                 { "1-16", TimelineScratchEngine::Length::sixteenth },
                 { "1-8", TimelineScratchEngine::Length::eighth },
                 { "1-4", TimelineScratchEngine::Length::quarter },
                 { "1-2", TimelineScratchEngine::Length::half },
                 { "1-bar", TimelineScratchEngine::Length::oneBar } }})
            for (const auto& speed : std::array<std::pair<const char*, float>, 3> {{
                     { "0_25", .25f }, { "1_0", 1.0f }, { "4_0", 4.0f } }})
                wavRenders &= renderScratchWav (preset.first, preset.second, length.second,
                    length.first, speed.second, speed.first);
    check (wavRenders, "scratch-offline-wav-renders-speed-0-25-1-0-4-0");
    check (wavRenders, "forward-cut-all-lengths");

    engine.release(); babyEngine.release(); babyReleaseEngine.release(); babyColdEngine.release(); forwardEngine.release();
    backspinWrapEngine.release(); tapeEngine.release(); transitionEngine.release(); shortLengthEngine.release(); depthEngine.release(); offTransitionEngine.release(); barEngine.release(); continuityEngine.release();
    testSampleObserver();
    return passed ? 0 : 1;
}
