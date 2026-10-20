#include "XYPadEngine.h"

#include <chrono>
#include <cmath>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <memory>
#include <new>

namespace AllocationAudit
{
std::atomic<bool> enabled { false };
std::atomic<uint32_t> count { 0 };
}

void* operator new (std::size_t size)
{
    if (AllocationAudit::enabled.load (std::memory_order_relaxed))
        AllocationAudit::count.fetch_add (1, std::memory_order_relaxed);
    if (auto* memory = std::malloc (size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t size)
{
    if (AllocationAudit::enabled.load (std::memory_order_relaxed))
        AllocationAudit::count.fetch_add (1, std::memory_order_relaxed);
    if (auto* memory = std::malloc (size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc();
}
void operator delete (void* memory) noexcept { std::free (memory); }
void operator delete[] (void* memory) noexcept { std::free (memory); }
void operator delete (void* memory, std::size_t) noexcept { std::free (memory); }
void operator delete[] (void* memory, std::size_t) noexcept { std::free (memory); }

class XYPadEngineTestAccess
{
public:
    static bool queueIsBoundedAndFifo (XYPadEngine& engine)
    {
        for (uint32_t i = 0; i < XYPadEngine::kRenderQueueCapacity; ++i)
            if (! engine.enqueueRenderJob ({ i, i + 10u })) return false;
        if (engine.enqueueRenderJob ({ 99u, 109u })) return false;
        for (uint32_t i = 0; i < XYPadEngine::kRenderQueueCapacity; ++i)
        {
            XYPadEngine::RenderJob job;
            if (! engine.dequeueRenderJob (job) || job.bar != i || job.generation != i + 10u)
                return false;
        }
        XYPadEngine::RenderJob empty;
        return ! engine.dequeueRenderJob (empty);
    }

    static bool sampleTimelineAndInterpolationAreBounded()
    {
        XYPadEngine::Transport transport;
        transport.playing = true;
        transport.startBar = 63;
        transport.startPhase = 0.5;
        transport.phasePerSample = 0.125;
        int bar = -1;
        double phase = 0.0;
        XYPadEngine::sampleTimeline (transport, 0, 7, bar, phase);
        if (bar != 63 || std::abs (phase - 0.5) > 1.0e-12) return false;
        XYPadEngine::sampleTimeline (transport, 4, 7, bar, phase);
        if (bar != 0 || std::abs (phase) > 1.0e-12) return false;
        transport.loopBoundarySample = 2;
        transport.loopStartBar = 3;
        transport.loopStartPhase = 0.75;
        XYPadEngine::sampleTimeline (transport, 2, 7, bar, phase);
        if (bar != 3 || std::abs (phase - 0.75) > 1.0e-12) return false;
        XYPadEngine::sampleTimeline (transport, 4, 7, bar, phase);
        if (bar != 4 || std::abs (phase) > 1.0e-12) return false;

        XYPadEngine::Trajectory trajectory;
        trajectory.count = 3;
        trajectory.time[0] = 0.0; trajectory.time[1] = 0.3; trajectory.time[2] = 0.9;
        trajectory.x[0] = 0.0f; trajectory.x[1] = 0.6f; trajectory.x[2] = 1.0f;
        trajectory.y[0] = 1.0f; trajectory.y[1] = 0.5f; trajectory.y[2] = 0.0f;
        float x = 0.0f, y = 0.0f;
        XYPadEngine::interpolateTrajectory (trajectory, 0.5, x, y);
        return std::abs (x - 0.7f) < 1.0e-6f && std::abs (y - 0.375f) < 1.0e-6f;
    }

    static float interpolateSource (const float* source, int frames, double position) noexcept
    {
        return XYPadEngine::interpolate (source, frames, position);
    }

    static void stopWorkerForAllocationProbe (XYPadEngine& engine)
    {
        engine.workerExit.store (true, std::memory_order_release);
        if (engine.worker.joinable()) engine.worker.join();
    }

    static bool fillRenderQueue (XYPadEngine& engine)
    {
        for (uint32_t i = 0; i < XYPadEngine::kRenderQueueCapacity; ++i)
            if (! engine.enqueueRenderJob ({ i % XYPadEngine::kBarCount, i + 1u })) return false;
        return ! engine.enqueueRenderJob ({ 0u, 999u });
    }
};

namespace
{
int failures = 0;
void check (bool condition, const char* name)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL " << name << '\n';
    }
    else
        std::cout << "PASS " << name << '\n';
}

bool renderOneBar (XYPadEngine& engine, bool playback, double phaseStart,
                   double phaseStep, juce::AudioBuffer<float>& source,
                   juce::AudioBuffer<float>& output, juce::AudioBuffer<float>& mask,
                   int& serial, int bar, int frames, double toneHz = 440.0, bool ramp = false,
                   double rightPhaseOffset = 0.0)
{
    constexpr int block = 512;
    juce::AudioBuffer<float> in (2, block), out (2, block), ready (1, block);
    const auto blocks = (frames + block - 1) / block;
    bool sawReady = false;
    for (int b = 0; b < blocks; ++b)
    {
        const auto count = juce::jmin (block, frames - b * block);
        for (int i = 0; i < count; ++i)
        {
            const auto absolute = playback ? i + b * block : serial + i;
            const auto angle = 2.0 * 3.141592653589793 * toneHz * absolute / 48000.0;
            const auto sample = ramp
                ? 2.0 * std::fmod (static_cast<double> (absolute), 4096.0) / 4096.0 - 1.0
                : std::sin (angle);
            const auto rightSample = ramp ? sample * 0.5
                                          : 0.5 * std::sin (angle + rightPhaseOffset);
            in.setSample (0, i, static_cast<float> (sample));
            in.setSample (1, i, static_cast<float> (rightSample));
        }
        XYPadEngine::Transport transport;
        transport.playing = true;
        transport.startBar = bar;
        transport.startPhase = phaseStart + static_cast<double> (b * block) * phaseStep;
        transport.phasePerSample = phaseStep;
        transport.quartersPerBar = 4.0;
        transport.secondsPerQuarter = 60.0 / 90.0;
        transport.timingGeneration = 1;
        engine.process (in, out, ready, transport, count);
        for (int i = 0; i < count; ++i)
        {
            sawReady |= ready.getSample (0, i) > 0.5f;
            for (int channel = 0; channel < 2; ++channel)
                output.setSample (channel, b * block + i, out.getSample (channel, i));
            mask.setSample (0, b * block + i, ready.getSample (0, i));
        }
        serial += count;
    }
    juce::ignoreUnused (source);
    return sawReady;
}

}

int main()
{
    check (sizeof (XYPadEngine) < 64u * 1024u,
           "fixed-64-bar-tables-do-not-consume-windows-thread-stack");
    XYPadEngine queueProbe;
    check (XYPadEngineTestAccess::queueIsBoundedAndFifo (queueProbe),
           "render-job-spsc-queue-bounded-fifo-and-overflow-rejected");
    check (XYPadEngineTestAccess::sampleTimelineAndInterpolationAreBounded(),
           "sample-exact-bar-loop-mapping-and-time-normalized-trajectory-interpolation");
    const float interpolationFixture[] { -1.0f, -0.25f, 0.5f, 1.0f };
    bool scalarInterpolationMatches = true;
    for (const auto x : { 0.0, 0.125, 0.5, 0.875, 1.0 })
    {
        const auto forwardPosition = x * 3.0;
        const auto reversePosition = (1.0 - x) * 3.0;
        const auto forwardExpected = interpolationFixture[static_cast<int> (forwardPosition)]
            + (interpolationFixture[juce::jmin (3, static_cast<int> (forwardPosition) + 1)]
            - interpolationFixture[static_cast<int> (forwardPosition)])
            * static_cast<float> (forwardPosition - std::floor (forwardPosition));
        const auto reverseExpected = interpolationFixture[static_cast<int> (reversePosition)]
            + (interpolationFixture[juce::jmin (3, static_cast<int> (reversePosition) + 1)]
            - interpolationFixture[static_cast<int> (reversePosition)])
            * static_cast<float> (reversePosition - std::floor (reversePosition));
        scalarInterpolationMatches &= std::abs (XYPadEngineTestAccess::interpolateSource (
            interpolationFixture, 4, forwardPosition) - forwardExpected) <= 2.0e-6f;
        scalarInterpolationMatches &= std::abs (XYPadEngineTestAccess::interpolateSource (
            interpolationFixture, 4, reversePosition) - reverseExpected) <= 2.0e-6f;
    }
    scalarInterpolationMatches &= XYPadEngineTestAccess::interpolateSource (interpolationFixture, 4, -3.0) == -1.0f
        && XYPadEngineTestAccess::interpolateSource (interpolationFixture, 4, 8.0) == 1.0f;
    check (scalarInterpolationMatches, "forward-reverse-linear-read-matches-scalar-reference-and-clamps-endpoints");
    XYPadEngine allocationProbe;
    bool allocationPrepared = allocationProbe.prepare (48000.0, 512);
    if (allocationPrepared) XYPadEngineTestAccess::stopWorkerForAllocationProbe (allocationProbe);
    juce::AudioBuffer<float> allocationInput (2, 512), allocationOutput (2, 512), allocationMask (1, 512);
    allocationInput.clear(); allocationOutput.clear(); allocationMask.clear();
    XYPadEngine::Transport allocationTransport;
    allocationTransport.playing = true;
    allocationTransport.startBar = 0;
    allocationTransport.phasePerSample = 1.0 / 128000.0;
    AllocationAudit::count.store (0, std::memory_order_relaxed);
    AllocationAudit::enabled.store (true, std::memory_order_release);
    if (allocationPrepared)
        allocationProbe.process (allocationInput, allocationOutput, allocationMask, allocationTransport, 512);
    AllocationAudit::enabled.store (false, std::memory_order_release);
    check (allocationPrepared && AllocationAudit::count.load (std::memory_order_relaxed) == 0,
           "audio-process-path-performs-no-global-heap-allocation");
    allocationProbe.release();

    XYPadEngine overflowProbe;
    const auto overflowPrepared = overflowProbe.prepare (44100.0, 512);
    if (overflowPrepared)
        XYPadEngineTestAccess::stopWorkerForAllocationProbe (overflowProbe);
    const auto queueFilled = overflowPrepared
        && XYPadEngineTestAccess::fillRenderQueue (overflowProbe);
    const std::vector<PluginStateModel::MotionPoint> overflowTrajectory {{ 0.0f, 0.5f, 0.0 },
                                                                         { 1.0f, 0.5f, 0.8 }};
    overflowProbe.setTrajectory (0, overflowTrajectory, 1);
    juce::AudioBuffer<float> overflowInput (2, 512), overflowOutput (2, 512), overflowMask (1, 512);
    overflowInput.clear();
    double overflowWetSamples = 0.0;
    const auto overflowFrames = static_cast<int> (44100.0 * 4.0 * 60.0 / 300.0);
    for (int captured = 0; overflowPrepared && captured < overflowFrames; captured += 512)
    {
        XYPadEngine::Transport transport;
        transport.playing = true;
        transport.startBar = 0;
        transport.startPhase = static_cast<double> (captured) / overflowFrames;
        transport.phasePerSample = 1.0 / overflowFrames;
        transport.timingGeneration = 1;
        overflowProbe.process (overflowInput, overflowOutput, overflowMask, transport,
                               juce::jmin (512, overflowFrames - captured));
        overflowWetSamples += overflowMask.getMagnitude (0, 0, juce::jmin (512, overflowFrames - captured));
    }
    check (queueFilled && overflowProbe.getState (0) == XYPadEngine::State::empty
           && overflowWetSamples == 0.0,
           "render-queue-overflow-drops-only-xy-and-never-publishes-partial-capture");
    overflowProbe.release();
    XYPadEngine unsupported;
    check (! unsupported.prepare (192000.0, 512, true) && ! unsupported.isEnabled(),
           "unsupported-allocation-configuration-disables-xy-safely");
    const float dryBits = -0.347125f;
    const auto zeroDepth = XYPadEngine::mixLayers (dryBits, 0.0f, 1.0f, 0.0f, true);
    check (std::memcmp (&dryBits, &zeroDepth, sizeof (float)) == 0,
           "depth-zero-bit-exact-dry");
    const auto xyOnly = XYPadEngine::mixLayers (0.25f, 0.25f, -0.5f, 0.4f, false);
    check (std::abs (xyOnly - (-0.05f)) < 1.0e-6f, "xy-only-depth-law");
    const auto presetWet = 0.2f;
    const auto depth = 0.6f;
    const auto presetOutput = 0.4f * 0.1f + depth * presetWet;
    const auto both = XYPadEngine::mixLayers (0.1f, presetOutput, -0.3f, depth, true);
    const auto expectedBoth = 0.1f * 0.4f + depth * 0.7071067811865475f * (presetWet - 0.3f);
    check (std::abs (both - expectedBoth) < 1.0e-6f, "parallel-equal-power-layer-mix");
    const auto correlatedPeak = XYPadEngine::mixLayers (0.0f, 1.0f, 1.0f, 1.0f, true);
    check (correlatedPeak > 1.4f && correlatedPeak < 1.42f, "correlated-layer-peak-measured-no-limiter");

    constexpr double sampleRate = 48000.0;
    constexpr int frames = static_cast<int> (sampleRate * 4.0 * 60.0 / 90.0);
    XYPadEngine engine;
    check (engine.prepare (sampleRate, 512), "prepare-48k-fixed-bank");

    std::vector<PluginStateModel::MotionPoint> forward {
        { 0.0f, 0.5f, 0.0 }, { 1.0f, 0.5f, 8.0 / 3.0 }
    };
    engine.setTrajectory (0, forward, 11);
    std::vector<PluginStateModel::MotionPoint> reverse {
        { 1.0f, 0.0f, 0.0 }, { 0.0f, 0.0f, 8.0 / 3.0 }
    };
    engine.setTrajectory (1, reverse, 21);

    juce::AudioBuffer<float> dummySource (2, frames), dummyOutput (2, frames), dummyMask (1, frames);
    int serial = 0;
    const auto readyDuringCapture = renderOneBar (engine, false, 0.0, 1.0 / frames,
        dummySource, dummyOutput, dummyMask, serial, 0, frames);
    check (! readyDuringCapture, "first-pass-is-xy-dry");
    check (engine.getState (0) == XYPadEngine::State::queued
           || engine.getState (0) == XYPadEngine::State::rendering
           || engine.getState (0) == XYPadEngine::State::ready,
           "capture-queues-after-complete-bar");

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds (12);
    while (engine.getState (0) != XYPadEngine::State::ready
           && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for (std::chrono::milliseconds (5));
    check (engine.getState (0) == XYPadEngine::State::ready, "worker-publishes-rendered-bar");

    serial = 0;
    const auto readyOnReplay = renderOneBar (engine, true, 0.0, 1.0 / frames,
        dummySource, dummyOutput, dummyMask, serial, 0, frames);
    check (readyOnReplay, "ready-bar-replays-on-next-pass");
    bool finite = true;
    for (int i = 0; i < frames; ++i)
        finite = finite && std::isfinite (dummyOutput.getSample (0, i))
                         && std::isfinite (dummyOutput.getSample (1, i));
    check (finite, "ready-render-is-finite-stereo");
    auto estimateFrequency = [sampleRate] (const juce::AudioBuffer<float>& audio, int start, int end)
    {
        int zeroCrossings = 0;
        auto previousSample = audio.getSample (0, start);
        for (int i = start + 1; i < end; ++i)
        {
            const auto sample = audio.getSample (0, i);
            if ((previousSample <= 0.0f && sample > 0.0f)
                || (previousSample >= 0.0f && sample < 0.0f))
                ++zeroCrossings;
            previousSample = sample;
        }
        return zeroCrossings * sampleRate / (2.0 * (end - start));
    };
    check (std::abs (estimateFrequency (dummyOutput, 48000, 72000) - 440.0) / 440.0 < 0.01,
           "pitch-middle-zero-semitones-preserves-frequency");

    juce::File wavFile = juce::File::getCurrentWorkingDirectory().getChildFile ("xy-diagnostic-forward-neutral.wav");
    auto stream = wavFile.createOutputStream();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    if (stream != nullptr)
        writer.reset (wav.createWriterFor (stream.release(), sampleRate, 2, 24, {}, 0));
    const auto wavWritten = writer != nullptr && writer->writeFromAudioSampleBuffer (dummyOutput, 0, frames);
    writer.reset();
    check (wavWritten, "diagnostic-wave-written");

    juce::AudioBuffer<float> transitionInput (2, 8), transitionOutput (2, 8), transitionMask (1, 8);
    transitionInput.clear();
    XYPadEngine::Transport stopped;
    stopped.playing = false;
    engine.process (transitionInput, transitionOutput, transitionMask, stopped, 8);
    check (engine.getState (0) == XYPadEngine::State::ready
           && transitionMask.getMagnitude (0, 0, 8) == 0.0f,
           "stop-keeps-ready-bar-but-does-not-play-stale-xy");

    XYPadEngine::Transport seekAway;
    seekAway.playing = true;
    seekAway.startBar = 1;
    seekAway.phasePerSample = 1.0 / frames;
    seekAway.quartersPerBar = 4.0;
    seekAway.secondsPerQuarter = 60.0 / 90.0;
    seekAway.timingGeneration = 1;
    engine.process (transitionInput, transitionOutput, transitionMask, seekAway, 8);
    XYPadEngine::Transport seekBack = seekAway;
    seekBack.startBar = 0;
    engine.process (transitionInput, transitionOutput, transitionMask, seekBack, 8);
    check (engine.getState (0) == XYPadEngine::State::ready
           && transitionMask.getSample (0, 0) > 0.5f,
           "seek-away-and-return-preserves-completed-bar-capture");

    XYPadEngine::Transport wrapped;
    wrapped.playing = true;
    wrapped.startBar = 3;
    wrapped.startPhase = 0.5;
    wrapped.phasePerSample = 0.125;
    wrapped.loopBoundarySample = 4;
    wrapped.loopStartBar = 0;
    wrapped.loopStartPhase = 0.0;
    wrapped.quartersPerBar = 4.0;
    wrapped.secondsPerQuarter = 60.0 / 90.0;
    wrapped.timingGeneration = 1;
    engine.process (transitionInput, transitionOutput, transitionMask, wrapped, 8);
    check (transitionMask.getSample (0, 4) > 0.5f
           && engine.getState (0) == XYPadEngine::State::ready,
           "confirmed-loop-reuses-ready-bar-at-loop-origin");

    wrapped.startBar = 0;
    wrapped.startPhase = 0.0;
    wrapped.loopBoundarySample = -1;
    wrapped.timingGeneration = 2;
    engine.process (transitionInput, transitionOutput, transitionMask, wrapped, 8);
    check (transitionMask.getSample (0, 0) == 0.0f
           && engine.getState (0) != XYPadEngine::State::ready,
           "timing-generation-invalidates-old-render-before-recapture");

    // A distinct BAR trajectory must remain independent: BAR 1 has no capture
    // until its own full pass, even though BAR 0 is READY.
    serial = 0;
    const auto unrelatedReady = renderOneBar (engine, true, 0.0, 1.0 / frames,
        dummySource, dummyOutput, dummyMask, serial, 1, frames);
    check (! unrelatedReady, "bar-captures-are-independent");
    const auto reverseDeadline = std::chrono::steady_clock::now() + std::chrono::seconds (12);
    while (engine.getState (1) != XYPadEngine::State::ready
           && std::chrono::steady_clock::now() < reverseDeadline)
        std::this_thread::sleep_for (std::chrono::milliseconds (5));
    check (engine.getState (1) == XYPadEngine::State::ready, "reverse-bar-render-ready");
    serial = 0;
    const auto reverseReady = renderOneBar (engine, true, 0.0, 1.0 / frames,
        dummySource, dummyOutput, dummyMask, serial, 1, frames);
    check (reverseReady, "reverse-bar-replays");
    int crossings = 0;
    auto previous = dummyOutput.getSample (0, 48000);
    for (int i = 48001; i < 72000; ++i)
    {
        const auto current = dummyOutput.getSample (0, i);
        if ((previous <= 0.0f && current > 0.0f) || (previous >= 0.0f && current < 0.0f))
            ++crossings;
        previous = current;
    }
    const auto estimatedHz = crossings * sampleRate / (2.0 * (72000 - 48000));
    check (std::abs (estimatedHz - 880.0) / 880.0 < 0.01, "pitch-top-plus-12-semitones");
    auto pitchStream = juce::File::getCurrentWorkingDirectory()
        .getChildFile ("xy-diagnostic-reverse-pitch-up.wav").createOutputStream();
    std::unique_ptr<juce::AudioFormatWriter> pitchWriter;
    if (pitchStream != nullptr)
        pitchWriter.reset (wav.createWriterFor (pitchStream.release(), sampleRate, 2, 24, {}, 0));
    check (pitchWriter != nullptr && pitchWriter->writeFromAudioSampleBuffer (dummyOutput, 0, frames),
           "diagnostic-wave-reverse-pitch-up-written");
    pitchWriter.reset();

    // BAR 3 holds X at the center. The rendered signal must repeat a moving
    // 20ms source window rather than pinning one source sample.
    const int holdFrames = frames;
    std::vector<PluginStateModel::MotionPoint> hold {{ 0.5f, 0.5f, 0.0 }};
    engine.setTrajectory (2, hold, 31);
    serial = 0;
    renderOneBar (engine, false, 0.0, 1.0 / holdFrames, dummySource, dummyOutput,
                  dummyMask, serial, 2, holdFrames, 440.0, true);
    const auto holdDeadline = std::chrono::steady_clock::now() + std::chrono::seconds (12);
    while (engine.getState (2) != XYPadEngine::State::ready
           && std::chrono::steady_clock::now() < holdDeadline)
        std::this_thread::sleep_for (std::chrono::milliseconds (5));
    serial = 0;
    const auto holdReady = renderOneBar (engine, true, 0.0, 1.0 / holdFrames,
        dummySource, dummyOutput, dummyMask, serial, 2, holdFrames, 440.0, true);
    double holdError = 0.0;
    float holdMaxStep = 0.0f;
    int holdPairs = 0;
    for (int i = 8192; i + 960 < holdFrames; ++i)
    {
        holdError += std::abs (dummyOutput.getSample (0, i) - dummyOutput.getSample (0, i + 960));
        holdMaxStep = juce::jmax (holdMaxStep,
            std::abs (dummyOutput.getSample (0, i + 1) - dummyOutput.getSample (0, i)));
        ++holdPairs;
    }
    check (holdReady && holdPairs > 0 && holdError / holdPairs < 0.04,
           "stationary-x-uses-periodic-20ms-micro-loop");
    check (holdMaxStep < 0.02f, "stationary-x-2ms-seam-click-continuity");
    auto holdStream = juce::File::getCurrentWorkingDirectory()
        .getChildFile ("xy-diagnostic-hold.wav").createOutputStream();
    std::unique_ptr<juce::AudioFormatWriter> holdWriter;
    if (holdStream != nullptr)
        holdWriter.reset (wav.createWriterFor (holdStream.release(), sampleRate, 2, 24, {}, 0));
    check (holdWriter != nullptr && holdWriter->writeFromAudioSampleBuffer (dummyOutput, 0, holdFrames),
           "diagnostic-wave-hold-written");
    holdWriter.reset();

    // A fast trajectory with a reversal exercises the bounded jump/fold path.
    std::vector<PluginStateModel::MotionPoint> jump {
        { 0.05f, 0.5f, 0.0 }, { 0.95f, 0.5f, 0.001 }, { 0.1f, 0.5f, 2.0 }
    };
    engine.setTrajectory (3, jump, 41);
    serial = 0;
    renderOneBar (engine, false, 0.0, 1.0 / frames, dummySource, dummyOutput,
                  dummyMask, serial, 3, frames);
    const auto jumpDeadline = std::chrono::steady_clock::now() + std::chrono::seconds (12);
    while (engine.getState (3) != XYPadEngine::State::ready
           && std::chrono::steady_clock::now() < jumpDeadline)
        std::this_thread::sleep_for (std::chrono::milliseconds (5));
    serial = 0;
    const auto jumpReady = renderOneBar (engine, true, 0.0, 1.0 / frames,
        dummySource, dummyOutput, dummyMask, serial, 3, frames);
    bool jumpFinite = jumpReady;
    for (int i = 0; i < frames; ++i)
        jumpFinite = jumpFinite && std::isfinite (dummyOutput.getSample (0, i))
                               && std::isfinite (dummyOutput.getSample (1, i));
    check (jumpFinite, "fast-forward-reverse-jump-remains-finite");
    auto jumpStream = juce::File::getCurrentWorkingDirectory()
        .getChildFile ("xy-diagnostic-fold-jump.wav").createOutputStream();
    std::unique_ptr<juce::AudioFormatWriter> jumpWriter;
    if (jumpStream != nullptr)
        jumpWriter.reset (wav.createWriterFor (jumpStream.release(), sampleRate, 2, 24, {}, 0));
    check (jumpWriter != nullptr && jumpWriter->writeFromAudioSampleBuffer (dummyOutput, 0, frames),
           "diagnostic-wave-fold-jump-written");
    jumpWriter.reset();

    // BAR 4 verifies the opposite pitch extreme independently of the X read direction.
    // One source sample per output sample (forward X rate = +1) isolates the
    // Y=-12 st expectation from any X-derived rate shift.
    std::vector<PluginStateModel::MotionPoint> pitchDown {{ 0.0f, 1.0f, 0.0 }, { 1.0f, 1.0f, 8.0 / 3.0 }};
    engine.setTrajectory (4, pitchDown, 51);
    serial = 0;
    renderOneBar (engine, false, 0.0, 1.0 / frames, dummySource, dummyOutput,
                  dummyMask, serial, 4, frames, 220.0);
    const auto downDeadline = std::chrono::steady_clock::now() + std::chrono::seconds (12);
    while (engine.getState (4) != XYPadEngine::State::ready
           && std::chrono::steady_clock::now() < downDeadline)
        std::this_thread::sleep_for (std::chrono::milliseconds (5));
    serial = 0;
    const auto downReady = renderOneBar (engine, true, 0.0, 1.0 / frames,
        dummySource, dummyOutput, dummyMask, serial, 4, frames, 220.0);
    const auto estimatedDownHz = estimateFrequency (dummyOutput, 48000, 72000);
    if (! downReady || std::abs (estimatedDownHz - 110.0) / 110.0 >= 0.01)
        std::cerr << "PITCH_DOWN_DIAGNOSTIC ready=" << downReady
                  << " estimated_hz=" << estimatedDownHz << '\n';
    check (downReady && std::abs (estimatedDownHz - 110.0) / 110.0 < 0.01,
           "pitch-bottom-minus-12-semitones");
    auto downStream = juce::File::getCurrentWorkingDirectory()
        .getChildFile ("xy-diagnostic-pitch-down.wav").createOutputStream();
    std::unique_ptr<juce::AudioFormatWriter> downWriter;
    if (downStream != nullptr)
        downWriter.reset (wav.createWriterFor (downStream.release(), sampleRate, 2, 24, {}, 0));
    check (downWriter != nullptr && downWriter->writeFromAudioSampleBuffer (dummyOutput, 0, frames),
           "diagnostic-wave-pitch-down-written");
    downWriter.reset();

    // Repeat +12 st with forward X. The earlier reverse-X fixture is the
    // direction-independent counterpart required by the plan.
    std::vector<PluginStateModel::MotionPoint> pitchUpForward {
        { 0.0f, 0.0f, 0.0 }, { 1.0f, 0.0f, 8.0 / 3.0 }
    };
    engine.setTrajectory (5, pitchUpForward, 61);
    serial = 0;
    renderOneBar (engine, false, 0.0, 1.0 / frames, dummySource, dummyOutput,
                  dummyMask, serial, 5, frames, 440.0);
    const auto upForwardDeadline = std::chrono::steady_clock::now() + std::chrono::seconds (12);
    while (engine.getState (5) != XYPadEngine::State::ready
           && std::chrono::steady_clock::now() < upForwardDeadline)
        std::this_thread::sleep_for (std::chrono::milliseconds (5));
    serial = 0;
    const auto upForwardReady = renderOneBar (engine, true, 0.0, 1.0 / frames,
        dummySource, dummyOutput, dummyMask, serial, 5, frames, 440.0);
    const auto upForwardHz = estimateFrequency (dummyOutput, 48000, 72000);
    check (upForwardReady && std::abs (upForwardHz - 880.0) / 880.0 < 0.01,
           "pitch-plus-12-is-independent-of-forward-x-slope");

    // Repeat -12 st with reverse X; this complements the existing forward-X
    // pitch-down fixture without changing the pitch trajectory.
    std::vector<PluginStateModel::MotionPoint> pitchDownReverse {
        { 1.0f, 1.0f, 0.0 }, { 0.0f, 1.0f, 8.0 / 3.0 }
    };
    engine.setTrajectory (6, pitchDownReverse, 62);
    serial = 0;
    renderOneBar (engine, false, 0.0, 1.0 / frames, dummySource, dummyOutput,
                  dummyMask, serial, 6, frames, 220.0);
    const auto downReverseDeadline = std::chrono::steady_clock::now() + std::chrono::seconds (12);
    while (engine.getState (6) != XYPadEngine::State::ready
           && std::chrono::steady_clock::now() < downReverseDeadline)
        std::this_thread::sleep_for (std::chrono::milliseconds (5));
    serial = 0;
    const auto downReverseReady = renderOneBar (engine, true, 0.0, 1.0 / frames,
        dummySource, dummyOutput, dummyMask, serial, 6, frames, 220.0);
    const auto downReverseHz = estimateFrequency (dummyOutput, 48000, 72000);
    check (downReverseReady && std::abs (downReverseHz - 110.0) / 110.0 < 0.01,
           "pitch-minus-12-is-independent-of-reverse-x-slope");

    // A phase-offset stereo sine verifies that both channels follow the same
    // X path through reverse scrubbing and neutral independent pitch.
    const auto expectedStereoPhase = 0.65;
    std::vector<PluginStateModel::MotionPoint> stereoReverse {
        { 1.0f, 0.5f, 0.0 }, { 0.0f, 0.5f, 8.0 / 3.0 }
    };
    engine.setTrajectory (7, stereoReverse, 63);
    serial = 0;
    renderOneBar (engine, false, 0.0, 1.0 / frames, dummySource, dummyOutput,
                  dummyMask, serial, 7, frames, 440.0, false, expectedStereoPhase);
    const auto stereoDeadline = std::chrono::steady_clock::now() + std::chrono::seconds (12);
    while (engine.getState (7) != XYPadEngine::State::ready
           && std::chrono::steady_clock::now() < stereoDeadline)
        std::this_thread::sleep_for (std::chrono::milliseconds (5));
    serial = 0;
    const auto stereoReady = renderOneBar (engine, true, 0.0, 1.0 / frames,
        dummySource, dummyOutput, dummyMask, serial, 7, frames, 440.0, false, expectedStereoPhase);
    double leftSine = 0.0, leftCosine = 0.0, rightSine = 0.0, rightCosine = 0.0;
    constexpr int phaseStart = 48000, phaseEnd = 72000;
    for (int i = phaseStart; i < phaseEnd; ++i)
    {
        const auto phase = 2.0 * 3.141592653589793 * 440.0 * (i - phaseStart) / sampleRate;
        const auto sine = std::sin (phase), cosine = std::cos (phase);
        leftSine += dummyOutput.getSample (0, i) * sine;
        leftCosine += dummyOutput.getSample (0, i) * cosine;
        rightSine += dummyOutput.getSample (1, i) * sine;
        rightCosine += dummyOutput.getSample (1, i) * cosine;
    }
    const auto leftPhase = std::atan2 (leftCosine, leftSine);
    const auto rightPhase = std::atan2 (rightCosine, rightSine);
    const auto measuredStereoPhase = std::abs (std::remainder (rightPhase - leftPhase, 2.0 * 3.141592653589793));
    check (stereoReady && std::abs (measuredStereoPhase - expectedStereoPhase) < 0.12,
           "stereo-phase-offset-preserved-through-linked-reverse-read");

    const auto hadReadyBeforeRateChange = engine.getState (7) == XYPadEngine::State::ready;
    const auto sampleRateReprepared = engine.prepare (44100.0, 512);
    check (hadReadyBeforeRateChange && sampleRateReprepared
           && engine.getState (7) == XYPadEngine::State::empty,
           "sample-rate-change-invalidates-old-rendered-bar");

    engine.release();

    // Exercise every one of the 64 fixed slots with distinct stereo-coded
    // tones. DC-only input is intentionally avoided because a phase vocoder
    // is not a useful BAR identity probe for a pure zero-frequency signal.
    // Replaying a BAR with a different live input must return that BAR's
    // captured PCM, not a neighboring BAR or the current input.
    constexpr double shortRate = 44100.0;
    constexpr double bpm = 300.0;
    constexpr int shortFrames = static_cast<int> (shortRate * 4.0 * 60.0 / bpm);
    XYPadEngine allBars;
    const auto prepared = allBars.prepare (shortRate, 512);
    check (prepared, "prepare-64-bar-isolation-bank");
    bool allIndependent = prepared;
    int barsReady = 0;
    juce::AudioBuffer<float> in (2, 512), out (2, 512), mask (1, 512);
    for (int bar = 0; bar < XYPadEngine::kBarCount && prepared; ++bar)
    {
        const std::vector<PluginStateModel::MotionPoint> hold {{ 0.5f, 0.5f, 0.0 }};
        allBars.setTrajectory (bar, hold, static_cast<uint32_t> (bar + 1));
        const auto level = 0.01f * static_cast<float> (bar + 1);
        const auto markerHz = 220.0 + static_cast<double> (bar) * 2.0;
        int captured = 0;
        while (captured < shortFrames)
        {
            const auto count = juce::jmin (512, shortFrames - captured);
            for (int i = 0; i < count; ++i)
            {
                const auto marker = static_cast<float> (std::sin (2.0 * 3.141592653589793
                    * markerHz * static_cast<double> (captured + i) / shortRate));
                in.setSample (0, i, level * marker);
                in.setSample (1, i, -level * 0.5f * marker);
            }
            XYPadEngine::Transport transport;
            transport.playing = true;
            transport.startBar = bar;
            transport.startPhase = static_cast<double> (captured) / shortFrames;
            transport.phasePerSample = 1.0 / shortFrames;
            transport.quartersPerBar = 4.0;
            transport.secondsPerQuarter = 60.0 / bpm;
            transport.timingGeneration = 1;
            allBars.process (in, out, mask, transport, count);
            captured += count;
        }
        const auto readyDeadline = std::chrono::steady_clock::now() + std::chrono::seconds (3);
        while (allBars.getState (bar) != XYPadEngine::State::ready
               && std::chrono::steady_clock::now() < readyDeadline)
            std::this_thread::sleep_for (std::chrono::milliseconds (2));
        const auto isReady = allBars.getState (bar) == XYPadEngine::State::ready;
        allIndependent = allIndependent && isReady;
        barsReady += isReady ? 1 : 0;
        for (int i = 0; i < 512; ++i)
        {
            in.setSample (0, i, -0.75f);
            in.setSample (1, i, 0.375f);
        }
        XYPadEngine::Transport replay;
        replay.playing = true;
        replay.startBar = bar;
        replay.phasePerSample = 1.0 / shortFrames;
        replay.quartersPerBar = 4.0;
        replay.secondsPerQuarter = 60.0 / bpm;
        replay.timingGeneration = 1;
        allBars.process (in, out, mask, replay, 512);
        const auto maskReady = mask.getSample (0, 128) > 0.5f;
        double leftEnergy = 0.0, rightEnergy = 0.0;
        for (int i = 128; i < 512; ++i)
        {
            leftEnergy += static_cast<double> (out.getSample (0, i)) * out.getSample (0, i);
            rightEnergy += static_cast<double> (out.getSample (1, i)) * out.getSample (1, i);
        }
        const auto leftRms = static_cast<float> (std::sqrt (leftEnergy / 384.0));
        const auto rightRms = static_cast<float> (std::sqrt (rightEnergy / 384.0));
        const auto expectedRms = level / std::sqrt (2.0f);
        const auto barMatches = maskReady && leftRms > 0.65f * expectedRms
            && leftRms < 1.35f * expectedRms
            && std::abs (rightRms - 0.5f * leftRms) < 0.08f * expectedRms;
        if (! barMatches)
            std::cerr << "BAR_ISOLATION_DIAGNOSTIC bar=" << bar
                      << " mask=" << mask.getSample (0, 128)
                      << " left_rms=" << leftRms << " expected_rms=" << expectedRms
                      << " right_rms=" << rightRms
                      << " state=" << static_cast<int> (allBars.getState (bar)) << '\n';
        allIndependent = allIndependent && barMatches;
    }
    check (barsReady == XYPadEngine::kBarCount, "all-64-bar-states-ready");
    check (allIndependent, "all-64-bars-replay-their-own-stereo-capture");
    allBars.release();
    return failures == 0 ? 0 : 1;
}
