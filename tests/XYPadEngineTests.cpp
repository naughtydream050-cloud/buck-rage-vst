#include "XYPadEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <thread>
#include <memory>
#include <new>
#include <vector>

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
    static bool liveGrainWindowLutMatchesAnalytic()
    {
        XYPadEngine engine;
        if (! engine.prepare (96000.0, 64, true, true)) return false;
        double maxError = 0.0;
        for (int frame = 0; frame < engine.liveGrainFrames; ++frame)
        {
            const auto phase = static_cast<double> (frame)
                             / static_cast<double> (juce::jmax (1, engine.liveGrainFrames - 1));
            const auto reference = static_cast<float> (0.5 - 0.5 * std::cos (2.0 * 3.14159265358979323846 * phase));
            maxError = std::max (maxError, static_cast<double> (std::abs (
                engine.liveGrainWindow[static_cast<size_t> (frame)] - reference)));
        }
        engine.release();
        std::cout << "METRIC live_grain_window_lut max_error=" << maxError << " tolerance=1e-7\n";
        return maxError <= 1.0e-7;
    }

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

    struct TransitionMeasurement
    {
        int transitionFrames = 0;
        int longestTransitionFrames = 0;
        double transitionEndTargetErrorSamples = std::numeric_limits<double>::infinity();
        bool allReadsInRange = true;
        bool allFinite = true;
    };

    static TransitionMeasurement measureTransition (XYPadEngine& engine,
                                                     const std::vector<PluginStateModel::MotionPoint>& points,
                                                     int frames)
    {
        TransitionMeasurement result;
        if (! engine.slots || ! engine.slots[0].pcm || ! engine.scratch
            || frames < engine.fftSize || frames > engine.maxFramesPerBar)
            return result;

        auto& slot = engine.slots[0];
        slot.captureTrajectory = {};
        slot.captureTrajectory.count = static_cast<int> (std::min<size_t> (
            points.size(), static_cast<size_t> (XYPadEngine::kMaxPoints)));
        for (int i = 0; i < slot.captureTrajectory.count; ++i)
        {
            const auto& point = points[static_cast<size_t> (i)];
            slot.captureTrajectory.x[static_cast<size_t> (i)] = point.x;
            slot.captureTrajectory.y[static_cast<size_t> (i)] = point.y;
            slot.captureTrajectory.time[static_cast<size_t> (i)] = point.timeSeconds;
        }
        for (int i = 0; i < frames; ++i)
        {
            const auto encodedPosition = static_cast<float> (i) / static_cast<float> (frames - 1);
            slot.pcm[static_cast<size_t> (i) * 2u] = encodedPosition;
            slot.pcm[static_cast<size_t> (i) * 2u + 1u] = -encodedPosition;
        }
        for (int i = frames; i < engine.maxFramesPerBar; ++i)
        {
            slot.pcm[static_cast<size_t> (i) * 2u] = std::numeric_limits<float>::quiet_NaN();
            slot.pcm[static_cast<size_t> (i) * 2u + 1u] = std::numeric_limits<float>::quiet_NaN();
        }
        result.transitionFrames = juce::jmax (1, juce::roundToInt (engine.sampleRate * 0.003));
        result.allReadsInRange = true;
        result.allFinite = engine.renderScrubAndPitch (slot, frames, &result.longestTransitionFrames,
                                                       &result.transitionEndTargetErrorSamples);

        for (int i = 0; i < frames; ++i)
        {
            const auto rendered = static_cast<double> (engine.scratch[static_cast<size_t> (i) * 2u]);
            const auto renderedRight = static_cast<double> (engine.scratch[static_cast<size_t> (i) * 2u + 1u]);
            result.allFinite = result.allFinite && std::isfinite (rendered) && std::isfinite (renderedRight);
        }
        result.allReadsInRange = result.allFinite
            && std::isnan (slot.pcm[static_cast<size_t> (frames) * 2u])
            && std::isnan (slot.pcm[static_cast<size_t> (frames) * 2u + 1u]);
        return result;
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

    static bool liveGrainDirectionIsCorrect()
    {
        XYPadEngine engine;
        if (! engine.prepare (48000.0, 64)) return false;
        const auto capacity = static_cast<size_t> (engine.liveHistoryCapacityFrames);
        if (! engine.liveHistory || ! engine.liveHistorySerials || capacity < 64u)
        {
            engine.release();
            return false;
        }
        for (uint64_t serial = 0; serial < 64; ++serial)
        {
            const auto index = static_cast<size_t> (serial % capacity);
            engine.liveHistory[index * 2u] = static_cast<float> (serial);
            engine.liveHistory[index * 2u + 1u] = -static_cast<float> (serial);
            engine.liveHistorySerials[index] = serial;
        }
        engine.liveGrainFrames = 20;
        engine.liveTransitionFrames = 3;
        engine.liveHistoryValidFrames = 64;
        const auto oldest = uint64_t { 0 };
        const auto newest = uint64_t { 63 };
        engine.launchLiveGrain (0.5f, 1, false, oldest, newest);
        auto forward = engine.liveGrains[0];
        float first = 0.0f, second = 0.0f;
        const auto* forwardSnapshot = engine.liveGrainSnapshots.get()
            + static_cast<size_t> (forward.snapshotIndex) * static_cast<size_t> (engine.liveGrainFrames) * 2u;
        const auto frozenFirst = forwardSnapshot[0];
        const auto frozenSecond = forwardSnapshot[2];
        double maximumReferenceError = 0.0;
        for (int age = 0; age < engine.liveGrainFrames; ++age)
        {
            const auto expected = forward.startSerial + age;
            maximumReferenceError = std::max (maximumReferenceError,
                std::abs (static_cast<double> (forwardSnapshot[static_cast<size_t> (age) * 2u]) - expected));
            maximumReferenceError = std::max (maximumReferenceError,
                std::abs (static_cast<double> (forwardSnapshot[static_cast<size_t> (age) * 2u + 1u]) + expected));
        }
        const auto sourceReadable = engine.readLiveSample (forward.startSerial, 0, oldest, newest, first)
            && engine.readLiveSample (forward.startSerial + 1.0, 0, oldest, newest, second);
        for (uint64_t serial = 0; serial < 64; ++serial)
            engine.liveHistory[static_cast<size_t> (serial % capacity) * 2u] = -999.0f;
        auto valid = forward.active && forward.direction == 1
            && sourceReadable && second > first && frozenSecond > frozenFirst
            && forwardSnapshot[0] == frozenFirst
            && forwardSnapshot[0] != -999.0f;
        for (uint64_t serial = 0; serial < 64; ++serial)
            engine.liveHistory[static_cast<size_t> (serial % capacity) * 2u] = static_cast<float> (serial);
        engine.liveGrains.fill (XYPadEngine::LiveGrain {});
        engine.launchLiveGrain (0.5f, -1, true, oldest, newest);
        const auto reverse = engine.liveGrains[0];
        const auto* reverseSnapshot = engine.liveGrainSnapshots.get()
            + static_cast<size_t> (reverse.snapshotIndex) * static_cast<size_t> (engine.liveGrainFrames) * 2u;
        for (int age = 0; age < engine.liveGrainFrames; ++age)
        {
            const auto expected = reverse.startSerial - age;
            maximumReferenceError = std::max (maximumReferenceError,
                std::abs (static_cast<double> (reverseSnapshot[static_cast<size_t> (age) * 2u]) - expected));
            maximumReferenceError = std::max (maximumReferenceError,
                std::abs (static_cast<double> (reverseSnapshot[static_cast<size_t> (age) * 2u + 1u]) + expected));
        }
        valid = valid && reverse.active && reverse.direction == -1
            && engine.readLiveSample (reverse.startSerial, 0, oldest, newest, first)
            && engine.readLiveSample (reverse.startSerial - 1.0, 0, oldest, newest, second)
            && second < first && reverse.fadeInFrames == 3
            && reverse.startSerial >= static_cast<double> (oldest)
            && reverse.startSerial <= static_cast<double> (newest)
            && reverse.startSerial + reverse.direction * 19.0 >= static_cast<double> (oldest)
            && reverse.startSerial + reverse.direction * 19.0 <= static_cast<double> (newest);
        std::cout << "METRIC live_linear_reference max_error=" << maximumReferenceError << " tolerance=2e-6\n";
        engine.release();
        return valid && maximumReferenceError <= 2.0e-6;
    }

    static bool stationaryLiveTrajectoryFreezesAndLoops()
    {
        XYPadEngine engine;
        if (! engine.prepare (48000.0, 256)) return false;
        const std::vector<PluginStateModel::MotionPoint> stationary {
            { 0.5f, 0.5f, 0.0 }, { 0.5f, 0.5f, 1.0 }
        };
        engine.setTrajectory (0, stationary, 91);
        juce::AudioBuffer<float> input (2, 256), output (2, 256), mask (1, 256);
        uint64_t processed = 0;
        const auto processSignal = [&] (float amplitude)
        {
            for (int i = 0; i < 256; ++i)
            {
                const auto sample = amplitude * std::sin (2.0 * 3.141592653589793 * 440.0
                                                         * static_cast<double> (processed + i) / 48000.0);
                input.setSample (0, i, static_cast<float> (sample));
                input.setSample (1, i, static_cast<float> (sample * 0.5));
            }
            XYPadEngine::Transport transport;
            transport.playing = true;
            transport.startBar = 0;
            transport.startPhase = static_cast<double> (processed) / 128000.0;
            transport.phasePerSample = 1.0 / 128000.0;
            engine.processLive (input, output, mask, transport, 256);
            processed += 256;
        };
        for (int block = 0; block < 7; ++block) processSignal (0.4f);
        const auto frozen = engine.liveHoldReady && engine.liveHoldMix >= 0.999f;
        const auto heldSample = engine.liveHoldSnapshot != nullptr ? engine.liveHoldSnapshot[64] : 0.0f;
        processSignal (0.1f);
        const auto remainsFrozen = engine.liveHoldReady && engine.liveHoldMix >= 0.999f
            && engine.liveHoldSnapshot[64] == heldSample;
        engine.release();
        return frozen && remainsFrozen;
    }

    static bool primeLiveTransitionProbe (XYPadEngine& engine, float x)
    {
        if (! engine.prepare (48000.0, 1)) return false;
        const auto capacity = static_cast<uint64_t> (engine.liveHistoryCapacityFrames);
        for (uint64_t serial = 0; serial < capacity; ++serial)
        {
            engine.liveHistory[static_cast<size_t> (serial) * 2u] = 0.25f;
            engine.liveHistory[static_cast<size_t> (serial) * 2u + 1u] = -0.25f;
            engine.liveHistorySerials[static_cast<size_t> (serial)] = serial;
        }
        engine.liveWriteSerial = capacity;
        engine.liveHistoryValidFrames = capacity;
        engine.liveActiveBar = 0;
        engine.liveActiveTrajectory.count = 1;
        engine.liveActiveTrajectory.x[0] = x;
        engine.liveActiveTrajectory.y[0] = 0.5f;
        engine.liveActiveTrajectory.time[0] = 0.0;
        engine.liveHaveSignal = engine.liveHavePreviousX = true;
        engine.livePreviousX = x;
        return true;
    }

    static bool liveJumpStartsWithoutWaitingForNextHop()
    {
        XYPadEngine engine;
        if (! primeLiveTransitionProbe (engine, 0.25f)) return false;
        const auto newest = engine.liveWriteSerial - 1u;
        engine.launchLiveGrain (0.75f, 1, false, 0, newest);
        engine.liveSamplesToLaunch = engine.liveHopFrames - 1;
        engine.livePreviousX = 0.75f;
        engine.liveDirection = 1;
        juce::AudioBuffer<float> input (2, 1), output (2, 1), mask (1, 1);
        input.setSample (0, 0, 0.25f); input.setSample (1, 0, -0.25f);
        XYPadEngine::Transport transport; transport.playing = true; transport.startBar = 0;
        engine.processLive (input, output, mask, transport, 1);
        int newGrains = 0;
        for (const auto& grain : engine.liveGrains)
            if (grain.active && grain.direction == -1 && grain.fadeInFrames == engine.liveTransitionFrames)
                ++newGrains;
        std::cout << "METRIC live_transition immediate_reverse_grains=" << newGrains << "\n";
        return newGrains == 1;
    }

    static bool interruptedTargetsRestartAtEveryOffset()
    {
        XYPadEngine engine;
        if (! engine.prepare (48000.0, 1, true, true)) return false;
        const auto capacity = static_cast<uint64_t> (engine.liveHistoryCapacityFrames);
        if (capacity < static_cast<uint64_t> (engine.liveGrainFrames))
        {
            engine.release();
            return false;
        }

        const auto resetProbe = [&]()
        {
            engine.liveGrains.fill (XYPadEngine::LiveGrain {});
            for (uint64_t serial = 0; serial < capacity; ++serial)
            {
                const auto index = static_cast<size_t> (serial);
                engine.liveHistory[index * 2u] = 0.25f;
                engine.liveHistory[index * 2u + 1u] = -0.25f;
                engine.liveHistorySerials[index] = serial;
            }
            engine.liveWriteSerial = capacity;
            engine.liveHistoryValidFrames = capacity;
            engine.liveActiveBar = 0;
            engine.liveActiveTrajectory = {};
            engine.liveActiveTrajectory.count = 3;
            engine.liveActiveTrajectory.time[0] = 0.0;
            engine.liveActiveTrajectory.time[1] = 0.5;
            engine.liveActiveTrajectory.time[2] = 1.0;
            engine.liveActiveTrajectory.x[0] = 0.1f;
            engine.liveActiveTrajectory.x[1] = 0.9f;
            engine.liveActiveTrajectory.x[2] = 0.1f;
            engine.liveActiveTrajectory.y[0] = 0.5f;
            engine.liveActiveTrajectory.y[1] = 0.5f;
            engine.liveActiveTrajectory.y[2] = 0.5f;
            engine.liveHaveSignal = engine.liveHavePreviousX = true;
            engine.livePreviousX = 0.1f;
            engine.liveDirection = 1;
            engine.livePendingTransition = false;
            engine.liveReentryTailActive = false;
            engine.liveReentryTailPosition = 0;
            engine.liveSamplesToLaunch = 100000;
            engine.liveWetGain = 1.0f;
            engine.liveWasPlaying = true;
            engine.liveHoldReady = false;
            engine.liveHoldMix = 0.0f;
            engine.liveSourceFadeActive = false;
            const auto newest = capacity - 1u;
            engine.launchLiveGrain (0.1f, 1, false, 0, newest);
            engine.launchLiveGrain (0.1f, 1, true, 0, newest);
            engine.liveSamplesToLaunch = 100000;
        };

        const auto transitionFrames = engine.liveTransitionFrames;
        juce::AudioBuffer<float> input (2, 1), output (2, 1), mask (1, 1);
        input.setSample (0, 0, 0.25f);
        input.setSample (1, 0, -0.25f);
        bool allOffsetsReenteredImmediately = transitionFrames >= 2;
        double maximumTailReferenceError = 0.0;
        double maximumTargetPositionError = 0.0;
        for (int eventOffset = 1; eventOffset < transitionFrames; ++eventOffset)
        {
            resetProbe();
            for (int prior = 0; prior < eventOffset - 1; ++prior)
            {
                XYPadEngine::Transport beforeEvent;
                beforeEvent.playing = true;
                beforeEvent.startBar = 0;
                beforeEvent.startPhase = static_cast<double> (prior) * 1.0e-6;
                beforeEvent.phasePerSample = 1.0e-6;
                engine.processLive (input, output, mask, beforeEvent, 1);
            }

            XYPadEngine::Transport event;
            event.playing = true;
            event.startBar = 0;
            event.startPhase = 0.5 + static_cast<double> (eventOffset - 1) * 1.0e-6;
            event.phasePerSample = 1.0e-6;
            engine.processLive (input, output, mask, event, 1);

            bool launchedLatestTargetAtEvent = false;
            double targetPositionError = std::numeric_limits<double>::infinity();
            for (const auto& grain : engine.liveGrains)
            {
                launchedLatestTargetAtEvent = launchedLatestTargetAtEvent
                    || (grain.active && grain.direction == 1 && grain.age == 1);
                if (grain.active && grain.direction == 1 && grain.age == 1)
                {
                    const auto available = engine.liveHistoryValidFrames;
                    const auto span = static_cast<double> (available - static_cast<uint64_t> (engine.liveGrainFrames));
                    const auto eventPhase = 0.5 + static_cast<double> (eventOffset - 1) * 1.0e-6;
                    const auto targetX = 0.9 - (eventPhase - 0.5) * 1.6;
                    const auto expected = static_cast<double> (engine.liveWriteSerial - available) + targetX * span;
                    targetPositionError = std::abs (grain.startSerial - expected);
                }
            }
            int activeAtEvent = 0;
            for (const auto& grain : engine.liveGrains) activeAtEvent += grain.active ? 1 : 0;
            const auto immediate = launchedLatestTargetAtEvent && ! engine.livePendingTransition
                && engine.liveReentryTailActive && engine.liveReentryTailPosition == 1
                && targetPositionError <= 1.0e-3
                && reentryRecordPasses (eventOffset, launchedLatestTargetAtEvent ? eventOffset : eventOffset + 1,
                                        maximumTailReferenceError, activeAtEvent, true, 0.0, 2.0);
            allOffsetsReenteredImmediately = allOffsetsReenteredImmediately && immediate;
            maximumTargetPositionError = std::max (maximumTargetPositionError, targetPositionError);
            if (immediate)
            {
                const auto* tail = engine.liveReentryTailBuffers[static_cast<size_t> (engine.liveReentryTailBuffer)].get();
                const auto prior = eventOffset - 1;
                for (int frame = 0; frame < transitionFrames; ++frame)
                {
                    const auto transitionSample = prior + frame;
                    const auto expected = transitionSample == 0 ? 0.0f
                        : transitionSample < transitionFrames
                            ? static_cast<float> (0.25 * std::cos (transitionSample * 1.5707963267948966
                                                                   / (transitionFrames - 1))
                                                  + 0.25 * std::sin (transitionSample * 1.5707963267948966
                                                                   / (transitionFrames - 1)))
                            : 0.25f;
                    const auto index = static_cast<size_t> (frame) * 2u;
                    maximumTailReferenceError = juce::jmax (maximumTailReferenceError,
                        static_cast<double> (std::abs (tail[index] - expected)));
                    maximumTailReferenceError = juce::jmax (maximumTailReferenceError,
                        static_cast<double> (std::abs (tail[index + 1u] + expected)));
                }
            }
            if (! immediate)
                std::cerr << "RED reentrant_transition offset=" << eventOffset
                          << " pending=" << engine.livePendingTransition
                          << " launched=" << launchedLatestTargetAtEvent
                          << " active=" << activeAtEvent
                          << " tail_error=" << maximumTailReferenceError
                          << " target_position_error=" << targetPositionError << '\n';
        }
        std::cout << "METRIC reentrant_tail_reference max_error=" << maximumTailReferenceError
                  << " tolerance=2e-6 max_target_position_error_samples=" << maximumTargetPositionError
                  << " target_position_tolerance_samples=0.001\n";
        resetProbe();
        engine.liveHoldReady = true;
        engine.liveHoldMix = 1.0f;
        engine.liveHoldPhase = engine.liveGrainFrames - 1;
        XYPadEngine::Transport holdFold;
        holdFold.playing = true;
        holdFold.startBar = 0;
        holdFold.startPhase = 0.5;
        holdFold.phasePerSample = 0.0;
        engine.processLive (input, output, mask, holdFold, 1);
        int activeAfterHoldFold = 0;
        for (const auto& grain : engine.liveGrains) activeAfterHoldFold += grain.active ? 1 : 0;
        bool finiteTail = engine.liveReentryTailActive && activeAfterHoldFold == 1;
        const auto* holdTail = engine.liveReentryTailBuffers[static_cast<size_t> (engine.liveReentryTailBuffer)].get();
        for (int i = 0; i < transitionFrames * 2; ++i)
            finiteTail = finiteTail && std::isfinite (holdTail[i]);
        std::cout << "METRIC hold_fold_reentry active_grains=" << activeAfterHoldFold
                  << " tail_finite=" << finiteTail << "\n";
        engine.release();
        return allOffsetsReenteredImmediately && maximumTailReferenceError <= 2.0e-6 && finiteTail;
    }

    static bool reentryRecordPasses (int eventSample, int targetStartSample, double tailError,
                                     int activeGrains, bool finite, double outputPeak,
                                     double peakBound) noexcept
    {
        return targetStartSample == eventSample && tailError <= 2.0e-6
            && activeGrains == 1 && finite && outputPeak <= peakBound + 2.0e-6;
    }

    static bool repeatedReentryBlendIsFiniteAndPeakBounded()
    {
        XYPadEngine engine;
        if (! engine.prepare (48000.0, 64, true, true)) return false;
        const auto total = 10 * engine.liveTransitionFrames;
        double maxError = 0.0, peak = 0.0, energy = 0.0;
        bool finite = true;
        for (int fixture = 0; fixture < 5; ++fixture)
            for (int i = 0; i < total; ++i)
            {
                float tailL = 0.0f, tailR = 0.0f, targetL = 0.0f, targetR = 0.0f;
                const auto t = static_cast<double> (i + fixture * total) / 37.0;
                if (fixture == 0) { tailL = targetL = 0.75f; tailR = targetR = -0.75f; }
                else if (fixture == 1) { tailL = 0.7f; tailR = -0.7f; targetL = -0.6f; targetR = 0.6f; }
                else if (fixture == 2) { tailL = i == 0 ? 1.0f : 0.0f; targetR = i == 1 ? 1.0f : 0.0f; }
                else if (fixture == 3) { tailL = static_cast<float> (0.8 * std::sin (t)); tailR = static_cast<float> (0.7 * std::sin (t + 0.6)); targetL = static_cast<float> (0.8 * std::sin (t + 0.4)); targetR = static_cast<float> (0.7 * std::sin (t - 0.2)); }
                else { tailL = static_cast<float> (0.8 * std::sin (t)); tailR = static_cast<float> (0.6 * std::cos (t)); targetL = static_cast<float> (0.7 * std::sin (t + 1.1)); targetR = static_cast<float> (0.9 * std::cos (t - 0.3)); }
                const auto q = static_cast<float> (i % engine.liveTransitionFrames)
                             / static_cast<float> (engine.liveTransitionFrames - 1);
                const auto outL = XYPadEngine::blendLiveReentrySample (tailL, targetL, q);
                const auto outR = XYPadEngine::blendLiveReentrySample (tailR, targetR, q);
                finite = finite && std::isfinite (outL) && std::isfinite (outR);
                maxError = std::max (maxError, static_cast<double> (std::abs (outL
                    - (tailL * (1.0f - q) + targetL * q))));
                maxError = std::max (maxError, static_cast<double> (std::abs (outR
                    - (tailR * (1.0f - q) + targetR * q))));
                const auto envelopeL = std::max (std::abs (tailL), std::abs (targetL)) + 2.0e-6f;
                const auto envelopeR = std::max (std::abs (tailR), std::abs (targetR)) + 2.0e-6f;
                finite = finite && std::abs (outL) <= envelopeL && std::abs (outR) <= envelopeR;
                peak = std::max (peak, static_cast<double> (std::max (std::abs (outL), std::abs (outR))));
                energy += static_cast<double> (outL) * outL + static_cast<double> (outR) * outR;
            }
        const auto capacity = static_cast<uint64_t> (engine.liveHistoryCapacityFrames);
        for (uint64_t serial = 0; serial < capacity; ++serial)
        {
            const auto index = static_cast<size_t> (serial);
            engine.liveHistory[index * 2u] = 0.75f;
            engine.liveHistory[index * 2u + 1u] = -0.75f;
            engine.liveHistorySerials[index] = serial;
        }
        engine.liveWriteSerial = capacity;
        engine.liveHistoryValidFrames = capacity;
        engine.liveActiveBar = 0;
        engine.liveActiveTrajectory.count = 2;
        engine.liveActiveTrajectory.time[0] = 0.0;
        engine.liveActiveTrajectory.time[1] = 1.0;
        engine.liveActiveTrajectory.x[0] = 0.1f;
        engine.liveActiveTrajectory.x[1] = 0.9f;
        engine.liveActiveTrajectory.y[0] = engine.liveActiveTrajectory.y[1] = 0.5f;
        engine.liveHaveSignal = engine.liveHavePreviousX = true;
        engine.livePreviousX = 0.7f;
        engine.liveDirection = 1;
        engine.liveSamplesToLaunch = std::numeric_limits<int>::max() / 2;
        engine.liveWetGain = 1.0f;
        engine.liveWasPlaying = true;
        juce::AudioBuffer<float> input (2, 1), output (2, 1), mask (1, 1);
        input.setSample (0, 0, 0.75f); input.setSample (1, 0, -0.75f);
        bool processFinite = true;
        int reentries = 0;
        double processPeak = 0.0;
        for (int i = 0; i < total; ++i)
        {
            XYPadEngine::Transport transport;
            transport.playing = true;
            transport.startBar = 0;
            transport.startPhase = (i & 1) == 0 ? 0.25 : 0.75;
            engine.processLive (input, output, mask, transport, 1);
            processFinite = processFinite && std::isfinite (output.getSample (0, 0))
                                          && std::isfinite (output.getSample (1, 0));
            processPeak = std::max (processPeak, static_cast<double> (std::max (
                std::abs (output.getSample (0, 0)), std::abs (output.getSample (1, 0)))));
            if (engine.liveReentryTailActive && engine.liveReentryTailPosition == 1) ++reentries;
        }
        std::cout << "METRIC reentry_blend frames=" << total * 5 << " peak=" << peak
                  << " rms=" << std::sqrt (energy / (total * 10.0))
                  << " scalar_error=" << maxError << "\n"
                  << "METRIC repeated_reentry events=" << reentries << " frames=" << total
                  << " process_peak=" << processPeak << " finite=" << processFinite << "\n";
        engine.release();
        return finite && maxError <= 2.0e-6 && processFinite && processPeak < 1.1
            && reentries >= total - 1;
    }

    static bool reentryMutationGuardsRejectUnsafeChanges()
    {
        const auto delayedLaunchRejected = ! reentryRecordPasses (100, 101, 0.0, 1, true, 0.5, 1.0);
        const auto overwrittenTailRejected = ! reentryRecordPasses (100, 100, 0.5, 1, true, 0.5, 1.0);
        const auto correlatedEqualPower = 0.75f * std::cos (0.25f * 3.14159265358979323846f)
                                        + 0.75f * std::sin (0.25f * 3.14159265358979323846f);
        const auto wrongBlendRejected = ! reentryRecordPasses (100, 100, 0.0, 1, true,
                                                                std::abs (correlatedEqualPower), 0.75);
        const auto staleGrainRejected = ! reentryRecordPasses (100, 100, 0.0, 2, true, 0.5, 1.0);
        const auto peakViolationRejected = ! reentryRecordPasses (100, 100, 0.0, 1, true, 1.0001, 1.0);
        std::cout << "METRIC reentry_mutation_guards delayed=" << delayedLaunchRejected
                  << " overwritten_tail=" << overwrittenTailRejected
                  << " wrong_blend=" << wrongBlendRejected
                  << " stale_grain=" << staleGrainRejected
                  << " peak_violation=" << peakViolationRejected << "\n";
        return delayedLaunchRejected && overwrittenTailRejected && wrongBlendRejected
            && staleGrainRejected && peakViolationRejected;
    }

    static bool runControlledHostBenchmark()
    {
        const double rates[] { 44100.0, 48000.0, 96000.0 };
        const int fixedBlocks[] { 64, 128, 256, 512, 1024 };
        bool allWithinDeadline = true;
        int conditionCount = 0;
        for (const auto rate : rates)
        {
            for (int mode = 0; mode < 6; ++mode)
            {
                XYPadEngine engine;
                if (! engine.prepare (rate, 1024, true, true)) return false;
                const auto points = std::vector<PluginStateModel::MotionPoint> {
                    { 0.1f, 0.5f, 0.0 }, { 0.9f, 0.5f, 1.0 }
                };
                engine.setTrajectory (0, points, 701);
                juce::AudioBuffer<float> input (2, 1), output (2, 1), mask (1, 1);
                const auto sample = static_cast<float> (0.5 * std::sin (2.0 * 3.141592653589793 * 440.0 / rate));
                input.setSample (0, 0, sample);
                input.setSample (1, 0, static_cast<float> (0.5 * std::sin (
                    2.0 * 3.141592653589793 * 440.0 / rate + 0.37)));
                const auto blockAt = [&] (uint64_t index)
                {
                    if (mode < 5) return fixedBlocks[mode];
                    const int pattern[] { 64, 127, 256, 511, 1024 };
                    return pattern[index % 5u];
                };
                uint64_t sequence = 0;
                uint64_t warmSamples = 0;
                while (warmSamples < static_cast<uint64_t> (rate * 2.0))
                {
                    const auto block = blockAt (sequence);
                    for (int frame = 0; frame < block; ++frame)
                    {
                        XYPadEngine::Transport eventTransport;
                        eventTransport.playing = true;
                        eventTransport.startBar = 0;
                        eventTransport.startPhase = (warmSamples + static_cast<uint64_t> (frame)) % 2u == 0u
                            ? 0.25 : 0.75;
                        engine.processLive (input, output, mask, eventTransport, 1);
                    }
                    warmSamples += static_cast<uint64_t> (block);
                    ++sequence;
                }
                std::vector<double> deadlineRatios;
                deadlineRatios.reserve (static_cast<size_t> (rate / 64.0 * 30.0 + 64.0));
                uint64_t measuredSamples = 0, events = 0;
                double peakRatio = 0.0;
                while (measuredSamples < static_cast<uint64_t> (rate * 30.0))
                {
                    const auto block = blockAt (sequence);
                    const auto start = std::chrono::steady_clock::now();
                    // Stress the actual per-sample re-entry path at the maximum
                    // requested event rate while measuring one nominal host block.
                    for (int frame = 0; frame < block; ++frame)
                    {
                        XYPadEngine::Transport eventTransport;
                        eventTransport.playing = true;
                        eventTransport.startBar = 0;
                        eventTransport.startPhase = (measuredSamples + static_cast<uint64_t> (frame)) % 2u == 0u
                            ? 0.25 : 0.75;
                        engine.processLive (input, output, mask, eventTransport, 1);
                    }
                    const auto stop = std::chrono::steady_clock::now();
                    const auto elapsed = std::chrono::duration<double, std::micro> (stop - start).count();
                    const auto ratio = elapsed / (1.0e6 * block / rate);
                    deadlineRatios.push_back (ratio);
                    peakRatio = std::max (peakRatio, ratio);
                    measuredSamples += static_cast<uint64_t> (block);
                    events += static_cast<uint64_t> (block);
                    ++sequence;
                }
                std::sort (deadlineRatios.begin(), deadlineRatios.end());
                const auto p99Ratio = deadlineRatios[static_cast<size_t> (std::ceil (0.99 * deadlineRatios.size())) - 1u];
                const auto within = p99Ratio <= 0.50 && peakRatio <= 0.80;
                allWithinDeadline = allWithinDeadline && within;
                ++conditionCount;
                std::cout << "METRIC reentrant_benchmark rate=" << rate << " mode="
                          << (mode < 5 ? fixedBlocks[mode] : -1) << " p99_deadline_ratio=" << p99Ratio
                          << " max_deadline_ratio=" << peakRatio << " events=" << events
                          << " samples=" << measuredSamples << " event_rate=1_per_sample status="
                          << (within ? "PASS" : "FAIL") << "\n";
                engine.release();
            }
        }
        std::cout << "METRIC reentrant_benchmark_complete conditions=" << conditionCount
                  << " status=" << (allWithinDeadline ? "PASS" : "FAIL") << "\n";
        return conditionCount == 18 && allWithinDeadline;
    }

    static bool liveFadedGrainNeverReturnsToFullGain()
    {
        XYPadEngine engine;
        if (! primeLiveTransitionProbe (engine, 1.0f)) return false;
        const auto newest = engine.liveWriteSerial - 1u;
        engine.launchLiveGrain (0.75f, 1, false, 0, newest);
        engine.launchLiveGrain (0.25f, -1, true, 0, newest);
        engine.liveSamplesToLaunch = 100000;
        engine.liveDirection = -1;
        engine.liveActiveTrajectory.count = 2;
        engine.liveActiveTrajectory.x[1] = 0.0f;
        engine.liveActiveTrajectory.y[1] = 0.5f;
        engine.liveActiveTrajectory.time[1] = 1.0;
        juce::AudioBuffer<float> input (2, 1), output (2, 1), mask (1, 1);
        input.setSample (0, 0, 0.25f); input.setSample (1, 0, -0.25f);
        for (int sample = 0; sample <= engine.liveTransitionFrames; ++sample)
        {
            XYPadEngine::Transport transport; transport.playing = true; transport.startBar = 0;
            transport.startPhase = sample / 48000.0; transport.phasePerSample = 1.0 / 48000.0;
            engine.processLive (input, output, mask, transport, 1);
        }
        std::cout << "METRIC live_transition old_grain_active_after_fade=" << engine.liveGrains[0].active << "\n";
        return ! engine.liveGrains[0].active;
    }

    static bool liveHoldSeamContinuesAfterCrossfadedHead()
    {
        XYPadEngine engine;
        if (! primeLiveTransitionProbe (engine, 0.5f)) return false;
        engine.liveSamplesToLaunch = 100000;
        engine.liveStationaryFrames = 96;
        engine.liveHoldReady = true; engine.liveHoldMix = 1.0f;
        engine.liveHoldPhase = engine.liveGrainFrames - 1;
        for (int sample = 0; sample < engine.liveGrainFrames; ++sample)
        {
            engine.liveHoldSnapshot[static_cast<size_t> (sample) * 2u] = sample / 960.0f;
            engine.liveHoldSnapshot[static_cast<size_t> (sample) * 2u + 1u] = -sample / 960.0f;
        }
        juce::AudioBuffer<float> input (2, 1), output (2, 1), mask (1, 1);
        input.setSample (0, 0, 0.25f); input.setSample (1, 0, -0.25f);
        XYPadEngine::Transport transport; transport.playing = true; transport.startBar = 0;
        engine.processLive (input, output, mask, transport, 1);
        std::cout << "METRIC live_transition post_seam_phase=" << engine.liveHoldPhase << " expected=96\n";
        return engine.liveHoldPhase == 96;
    }

    static bool liveSourceEndedFadesAudioWithoutHardCut()
    {
        XYPadEngine engine;
        if (! engine.prepare (48000.0, 1)) return false;
        const std::vector<PluginStateModel::MotionPoint> hold {{ 0.5f, 0.5f, 0.0 }};
        engine.setTrajectory (0, hold, 101);
        juce::AudioBuffer<float> input (2, 1), output (2, 1), mask (1, 1);
        float previous = 0.0f, maximumStep = 0.0f;
        bool finite = true;
        for (int sample = 0; sample < 8192; ++sample)
        {
            const float value = sample < 4096 ? 0.25f : 0.0f;
            input.setSample (0, 0, value); input.setSample (1, 0, value * 0.5f);
            XYPadEngine::Transport transport; transport.playing = true; transport.startBar = 0;
            transport.startPhase = sample / 96000.0; transport.phasePerSample = 1.0 / 96000.0;
            engine.processLive (input, output, mask, transport, 1);
            const float contribution = output.getSample (0, 0) * mask.getSample (0, 0);
            finite &= std::isfinite (contribution);
            if (sample >= 4096 + 1024) maximumStep = juce::jmax (maximumStep, std::abs (contribution - previous));
            previous = contribution;
        }
        std::cout << "METRIC live_source_ended max_contribution_step=" << maximumStep
                  << " limit=0.02 final_mask=" << mask.getSample (0, 0) << "\n";
        return finite && maximumStep < 0.02f && mask.getSample (0, 0) == 0.0f;
    }

    static bool livePipelineHasExact1024SampleIdentityDelay()
    {
        XYPadEngine engine;
        if (! engine.prepare (48000.0, 256, true)) return false;
        bool exact = true;
        float outL = 0.0f, outR = 0.0f, mask = 0.0f;
        for (int sample = 0; sample <= XYPadEngine::kLivePipelineLatency; ++sample)
        {
            const auto inL = sample == 0 ? 0.75f : 0.0f;
            const auto inR = sample == 0 ? -0.25f : 0.0f;
            engine.processLivePitchSample (inL, inR, 0.0f, sample == 0 ? 1.0f : 0.0f,
                                           outL, outR, mask);
            if (sample < XYPadEngine::kLivePipelineLatency)
                exact = exact && outL == 0.0f && outR == 0.0f && mask == 0.0f;
            else
                exact = exact && outL == 0.75f && outR == -0.25f && mask == 1.0f;
        }
        engine.release();
        return exact;
    }

    static double renderLivePitchFrequency (double rate, float y,
                                            bool& finite, bool& noAllocation)
    {
        XYPadEngine engine;
        if (! engine.prepare (rate, 1024, true)) return 0.0;
        std::vector<float> rendered (static_cast<size_t> (rate));
        finite = true;
        AllocationAudit::count.store (0, std::memory_order_relaxed);
        AllocationAudit::enabled.store (true, std::memory_order_release);
        const auto semitones = 24.0f * (y - 0.5f);
        for (size_t sampleIndex = 0; sampleIndex < rendered.size(); ++sampleIndex)
        {
            const auto phase = 2.0 * 3.141592653589793 * 440.0
                * static_cast<double> (sampleIndex) / rate;
            const auto input = static_cast<float> (0.5 * std::sin (phase));
            float left = 0.0f, right = 0.0f, mask = 0.0f;
            engine.processLivePitchSample (input, input * 0.5f, semitones, 1.0f,
                                           left, right, mask);
            rendered[sampleIndex] = left;
            finite = finite && std::isfinite (left) && std::isfinite (right)
                && std::isfinite (mask);
        }
        AllocationAudit::enabled.store (false, std::memory_order_release);
        noAllocation = AllocationAudit::count.load (std::memory_order_relaxed) == 0;
        const auto start = static_cast<size_t> (rate * 0.30);
        const auto end = static_cast<size_t> (rate * 0.80);
        int crossings = 0;
        for (size_t i = start + 1; i < end; ++i)
            if ((rendered[i - 1] <= 0.0f && rendered[i] > 0.0f)
                || (rendered[i - 1] >= 0.0f && rendered[i] < 0.0f)) ++crossings;
        const auto frequency = crossings * rate / (2.0 * static_cast<double> (end - start));
        engine.release();
        return frequency;
    }

    static bool livePitchDoesNotChangeXDirectionOrSampleClock()
    {
        struct Result { int direction = 0; uint64_t writeSerial = 0, pitchSerial = 0; };
        const auto render = [] (float y, bool reverse)
        {
            Result result;
            XYPadEngine engine;
            constexpr int blockSize = 256;
            if (! engine.prepare (48000.0, blockSize, true)) return result;
            const std::vector<PluginStateModel::MotionPoint> trajectory = reverse
                ? std::vector<PluginStateModel::MotionPoint> {{ 0.8f, y, 0.0 }, { 0.2f, y, 2.0 }}
                : std::vector<PluginStateModel::MotionPoint> {{ 0.2f, y, 0.0 }, { 0.8f, y, 2.0 }};
            engine.setTrajectory (0, trajectory, 1);
            juce::AudioBuffer<float> input (2, blockSize), output (2, blockSize), mask (1, blockSize);
            uint64_t processed = 0;
            for (int block = 0; block < 24; ++block)
            {
                for (int i = 0; i < blockSize; ++i)
                {
                    const auto angle = 2.0 * 3.141592653589793 * 440.0
                        * static_cast<double> (processed + static_cast<uint64_t> (i)) / 48000.0;
                    const auto sample = static_cast<float> (0.5 * std::sin (angle));
                    input.setSample (0, i, sample);
                    input.setSample (1, i, sample * 0.5f);
                }
                XYPadEngine::Transport transport;
                transport.playing = true;
                transport.startBar = 0;
                transport.startPhase = static_cast<double> (processed) / 128000.0;
                transport.phasePerSample = 1.0 / 128000.0;
                engine.processLive (input, output, mask, transport, blockSize);
                processed += blockSize;
            }
            result = { engine.liveDirection, engine.liveWriteSerial, engine.livePitchSerial };
            engine.release();
            return result;
        };

        const auto forwardTop = render (1.0f, false);
        const auto forwardBottom = render (0.0f, false);
        const auto reverseTop = render (1.0f, true);
        const auto reverseBottom = render (0.0f, true);
        return forwardTop.direction == 1 && forwardBottom.direction == 1
            && reverseTop.direction == -1 && reverseBottom.direction == -1
            && forwardTop.writeSerial == forwardBottom.writeSerial
            && reverseTop.writeSerial == reverseBottom.writeSerial
            && forwardTop.pitchSerial == forwardBottom.pitchSerial
            && reverseTop.pitchSerial == reverseBottom.pitchSerial;
    }

    struct LiveVelocityObservation
    {
        int direction = 0;
        int stationaryFrames = 0;
        int requiredHoldFrames = 0;
        bool holdReady = false;
    };

    static LiveVelocityObservation observeLiveVelocity (double rate, double velocity)
    {
        LiveVelocityObservation result;
        XYPadEngine engine;
        constexpr int blockSize = 256;
        if (! engine.prepare (rate, blockSize, true, true)) return result;

        const auto startX = velocity >= 0.0 ? 0.25f : 0.75f;
        const auto endX = static_cast<float> (static_cast<double> (startX) + velocity);
        const std::vector<PluginStateModel::MotionPoint> points {
            { startX, 0.5f, 0.0 }, { endX, 0.5f, 1.0 }
        };
        engine.setTrajectory (0, points, 1);

        juce::AudioBuffer<float> input (2, blockSize), output (2, blockSize), mask (1, blockSize);
        input.clear();
        for (int i = 0; i < blockSize; ++i)
        {
            input.setSample (0, i, 0.25f);
            input.setSample (1, i, -0.125f);
        }

        const auto targetSamples = static_cast<int> (rate * 0.05);
        int processed = 0;
        while (processed < targetSamples)
        {
            const auto count = juce::jmin (blockSize, targetSamples - processed);
            XYPadEngine::Transport transport;
            transport.playing = true;
            transport.startBar = 0;
            transport.startPhase = static_cast<double> (processed) / rate;
            transport.phasePerSample = 1.0 / rate;
            engine.processLive (input, output, mask, transport, count);
            processed += count;
        }

        result.direction = engine.liveDirection;
        result.stationaryFrames = engine.liveStationaryFrames;
        result.requiredHoldFrames = juce::jmax (1, static_cast<int> (std::ceil (rate * 0.002)));
        result.holdReady = engine.liveHoldReady;
        engine.release();
        return result;
    }

    static bool liveEqualPowerTransitionAudio (double rate, double& measuredRatio,
                                               bool& mutationOracleRejects)
    {
        XYPadEngine engine;
        constexpr int blockSize = 1;
        if (! engine.prepare (rate, blockSize, true, true)) return false;
        const auto transitionFrames = engine.liveTransitionFrames;
        const auto pipelineDelay = XYPadEngine::kLivePipelineLatency;
        const auto targetSampleCount = static_cast<int> (rate * 0.10);
        std::vector<PluginStateModel::MotionPoint> points {
            { 0.2f, 0.5f, 0.0 }, { 0.21f, 0.5f, 0.05 },
            { 0.8f, 0.5f, 0.05001 }, { 0.8f, 0.5f, 1.0 }
        };
        engine.setTrajectory (0, points, 1);

        juce::AudioBuffer<float> input (2, 1), output (2, 1), mask (1, 1);
        input.setSample (0, 0, 0.25f);
        input.setSample (1, 0, 0.25f);
        std::vector<float> rendered (static_cast<size_t> (targetSampleCount), 0.0f);
        std::vector<float> renderedMask (static_cast<size_t> (targetSampleCount), 0.0f);
        int transitionStart = -1;
        bool holdWasInactiveAtTransition = true;

        for (int sample = 0; sample < targetSampleCount; ++sample)
        {
            XYPadEngine::Transport transport;
            transport.playing = true;
            transport.startBar = 0;
            transport.startPhase = static_cast<double> (sample) / rate;
            transport.phasePerSample = 1.0 / rate;
            engine.processLive (input, output, mask, transport, 1);
            rendered[static_cast<size_t> (sample)] = output.getSample (0, 0);
            renderedMask[static_cast<size_t> (sample)] = mask.getSample (0, 0);

            if (transitionStart < 0)
                for (const auto& grain : engine.liveGrains)
                    if (grain.active && grain.fadeOutRemaining == transitionFrames - 1)
                    {
                        transitionStart = sample;
                        holdWasInactiveAtTransition = ! engine.liveHoldReady
                            && engine.liveHoldMix <= 1.0e-6f;
                        break;
                    }
        }

        bool valid = transitionFrames >= 2 && transitionStart >= 0;
        const auto midpointOffset = juce::jmax (0, (transitionFrames - 1) / 2);
        const auto midpointIndex = transitionStart + pipelineDelay + midpointOffset;
        const auto baselineEnd = transitionStart + pipelineDelay - 16;
        const auto baselineStart = baselineEnd - 64;
        double baseline = 0.0;
        int baselineCount = 0;
        if (baselineStart < 0 || midpointIndex >= targetSampleCount)
            valid = false;
        else
            for (int i = baselineStart; i < baselineEnd; ++i)
                if (renderedMask[static_cast<size_t> (i)] > 0.5f)
                {
                    baseline += rendered[static_cast<size_t> (i)];
                    ++baselineCount;
                }
        valid = valid && baselineCount > 0;
        if (baselineCount > 0)
        {
            baseline /= baselineCount;
            measuredRatio = baseline != 0.0
                ? rendered[static_cast<size_t> (midpointIndex)] / baseline : 0.0;
        }

        constexpr double equalPowerMidpoint = 1.4142135623730951;
        constexpr double tolerance = 0.08;
        const auto accepts = [tolerance] (double actual, double expected)
        {
            return std::abs (actual - expected) <= tolerance;
        };
        mutationOracleRejects = ! accepts (equalPowerMidpoint, 1.0);
        const auto audioMatchesEqualPower = accepts (measuredRatio, equalPowerMidpoint);
        std::cout << "METRIC xy_live_r2 live_transition rate=" << rate
                  << " start=" << transitionStart << " frames=" << transitionFrames
                  << " baseline=" << baseline << " measured_ratio=" << measuredRatio
                  << " expected_ratio=" << equalPowerMidpoint
                  << " expected_peak_db=" << 20.0 * std::log10 (equalPowerMidpoint)
                  << " mutation_rejected=" << mutationOracleRejects
                  << " hold_inactive=" << holdWasInactiveAtTransition << '\n';
        engine.release();
        return valid && holdWasInactiveAtTransition && mutationOracleRejects
            && audioMatchesEqualPower;
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

int main (int argc, char** argv)
{
    if (argc == 2 && std::strcmp (argv[1], "--benchmark-reentrant") == 0)
        return XYPadEngineTestAccess::runControlledHostBenchmark() ? 0 : 1;
    check (sizeof (XYPadEngine) < 64u * 1024u,
           "fixed-64-bar-tables-do-not-consume-windows-thread-stack");
    XYPadEngine queueProbe;
    check (XYPadEngineTestAccess::queueIsBoundedAndFifo (queueProbe),
           "render-job-spsc-queue-bounded-fifo-and-overflow-rejected");
    check (XYPadEngineTestAccess::sampleTimelineAndInterpolationAreBounded(),
           "sample-exact-bar-loop-mapping-and-time-normalized-trajectory-interpolation");
    for (const auto rate : { 44100.0, 48000.0, 96000.0 })
        for (const auto velocity : { 0.01, 0.1, 0.5, -0.01, -0.1, -0.5 })
        {
            const auto observation = XYPadEngineTestAccess::observeLiveVelocity (rate, velocity);
            const auto requiredDirection = velocity > 0.0 ? 1 : -1;
            const auto pass = observation.direction == requiredDirection
                && observation.stationaryFrames < observation.requiredHoldFrames
                && ! observation.holdReady;
            std::cout << "METRIC xy_live_r2 velocity rate=" << rate << " v=" << velocity
                      << " direction=" << observation.direction
                      << " stationary=" << observation.stationaryFrames
                      << " required=" << observation.requiredHoldFrames
                      << " hold_ready=" << observation.holdReady << '\n';
            check (pass, "xy-live-r2-normalized-velocity-remains-moving-across-sample-rates");
        }
    for (const auto rate : { 44100.0, 48000.0, 96000.0 })
    {
        double liveTransitionRatio = 0.0;
        bool transitionMutationRejected = false;
        check (XYPadEngineTestAccess::liveEqualPowerTransitionAudio (
                   rate, liveTransitionRatio, transitionMutationRejected),
               "xy-live-r2-rendered-transition-matches-equal-power-reference");
        check (transitionMutationRejected,
               "xy-live-r2-equal-power-reference-mutation-is-rejected");
    }
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
    const auto xyBlendZero = XYPadEngine::mixLayers (0.1f, 0.37f, -0.3f, 0.6f, true, 0.0f);
    check (xyBlendZero == 0.37f, "xy-warmup-does-not-change-existing-preset-output");
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
    XYPadEngine transitionProbe;
    const auto transitionPrepared = transitionProbe.prepare (sampleRate, 512);
    if (transitionPrepared)
        XYPadEngineTestAccess::stopWorkerForAllocationProbe (transitionProbe);
    constexpr int transitionFixtureFrames = 2048;
    const std::vector<PluginStateModel::MotionPoint> reversalTransition {
        { 0.1f, 0.5f, 0.0 }, { 0.9f, 0.5f, 0.5 }, { 0.2f, 0.5f, 1.0 }
    };
    const std::vector<PluginStateModel::MotionPoint> rateJumpTransition {
        { 0.1f, 0.5f, 0.0 }, { 0.9f, 0.5f, 0.01 }, { 0.95f, 0.5f, 0.02 }, { 1.0f, 0.5f, 1.0 }
    };
    const auto reversalMeasurement = transitionPrepared
        ? XYPadEngineTestAccess::measureTransition (transitionProbe, reversalTransition, transitionFixtureFrames)
        : XYPadEngineTestAccess::TransitionMeasurement {};
    const auto jumpMeasurement = transitionPrepared
        ? XYPadEngineTestAccess::measureTransition (transitionProbe, rateJumpTransition, transitionFixtureFrames)
        : XYPadEngineTestAccess::TransitionMeasurement {};
    const auto expectedTransitionFrames = juce::roundToInt (sampleRate * 0.003);
    std::cout << "METRIC ac07 transition_expected_samples=" << expectedTransitionFrames
              << " reversal_samples=" << reversalMeasurement.longestTransitionFrames
              << " jump_samples=" << jumpMeasurement.longestTransitionFrames
              << " reversal_target_error_samples=" << reversalMeasurement.transitionEndTargetErrorSamples
              << " jump_target_error_samples=" << jumpMeasurement.transitionEndTargetErrorSamples
              << " reversal_finite=" << reversalMeasurement.allFinite
              << " jump_finite=" << jumpMeasurement.allFinite
              << " range_guard=" << (reversalMeasurement.allReadsInRange && jumpMeasurement.allReadsInRange)
              << "\n";
    check (transitionPrepared && reversalMeasurement.longestTransitionFrames == expectedTransitionFrames,
           "ac07-reversal-transition-duration-measured-at-three-ms-bound");
    check (transitionPrepared && jumpMeasurement.longestTransitionFrames == expectedTransitionFrames,
           "ac07-over-four-x-rate-jump-transition-duration-measured-at-three-ms-bound");
    check (reversalMeasurement.allReadsInRange && jumpMeasurement.allReadsInRange,
           "ac07-position-coded-source-reads-remain-within-capture-range");
    check (reversalMeasurement.allFinite && jumpMeasurement.allFinite
           && reversalMeasurement.transitionEndTargetErrorSamples <= 1.0
           && jumpMeasurement.transitionEndTargetErrorSamples <= 1.0,
           "ac07-finite-render-and-target-convergence-within-one-source-sample");
    transitionProbe.release();

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

    check (XYPadEngineTestAccess::liveGrainDirectionIsCorrect(),
           "live-grains-read-forward-and-reverse-within-valid-serial-range-and-snapshot-source");
    check (XYPadEngineTestAccess::stationaryLiveTrajectoryFreezesAndLoops(),
           "live-stationary-xy-freezes-a-bounded-grain-and-holds-it-across-blocks");
    check (XYPadEngineTestAccess::liveJumpStartsWithoutWaitingForNextHop(),
           "live-fold-jump-starts-transition-on-the-next-sample-without-hop-delay");
    check (XYPadEngineTestAccess::interruptedTargetsRestartAtEveryOffset(),
           "reentrant-target-restarts-on-event-sample-at-every-active-transition-offset");
    check (XYPadEngineTestAccess::liveGrainWindowLutMatchesAnalytic(),
           "live-grain-window-lut-matches-analytic-hann");
    check (XYPadEngineTestAccess::repeatedReentryBlendIsFiniteAndPeakBounded(),
           "reentrant-linear-blend-is-finite-and-per-channel-peak-bounded-for-ten-transition-lengths");
    check (XYPadEngineTestAccess::reentryMutationGuardsRejectUnsafeChanges(),
           "reentrant-mutation-guards-reject-delay-tail-overwrite-wrong-blend-stale-grain-and-peak-overflow");
    check (XYPadEngineTestAccess::liveFadedGrainNeverReturnsToFullGain(),
           "live-old-grain-is-retired-at-three-ms-fade-end-and-cannot-reappear");
    check (XYPadEngineTestAccess::liveHoldSeamContinuesAfterCrossfadedHead(),
           "live-hold-seam-continues-at-the-next-head-sample-without-two-ms-back-jump");
    check (XYPadEngineTestAccess::liveSourceEndedFadesAudioWithoutHardCut(),
           "live-source-ended-fades-actual-xy-contribution-without-a-hard-audio-cut");
    XYPadEngine liveProbe;
    const auto livePrepared = liveProbe.prepare (48000.0, 512);
    std::vector<PluginStateModel::MotionPoint> liveBarFour {{ 0.0f, 0.5f, 0.0 },
                                                            { 1.0f, 0.5f, 1.0 }};
    liveProbe.setTrajectory (3, liveBarFour, 44);
    juce::AudioBuffer<float> liveInput (2, 512), liveOutput (2, 512), liveMask (1, 512);
    int liveProcessed = 0;
    int firstFullWet = -1;
    bool liveFinite = livePrepared;
    constexpr int liveTestFrames = 4200;
    AllocationAudit::count.store (0, std::memory_order_relaxed);
    AllocationAudit::enabled.store (true, std::memory_order_release);
    while (livePrepared && liveProcessed < liveTestFrames)
    {
        const auto blockFrames = juce::jmin (512, liveTestFrames - liveProcessed);
        for (int i = 0; i < blockFrames; ++i)
        {
            liveInput.setSample (0, i, 0.5f);
            liveInput.setSample (1, i, -0.25f);
        }
        XYPadEngine::Transport liveTransport;
        liveTransport.playing = true;
        liveTransport.startBar = 3;
        liveTransport.startPhase = static_cast<double> (liveProcessed) / 128000.0;
        liveTransport.phasePerSample = 1.0 / 128000.0;
        liveProbe.processLive (liveInput, liveOutput, liveMask, liveTransport, blockFrames);
        for (int i = 0; i < blockFrames; ++i)
        {
            const auto absolute = liveProcessed + i;
            const auto maskValue = liveMask.getSample (0, i);
            liveFinite = liveFinite && std::isfinite (maskValue)
                && std::isfinite (liveOutput.getSample (0, i))
                && std::isfinite (liveOutput.getSample (1, i));
            if (absolute < 960)
                liveFinite = liveFinite && maskValue == 0.0f;
            if (firstFullWet < 0 && maskValue >= 0.99f)
                firstFullWet = absolute;
        }
        liveProcessed += blockFrames;
    }
    int silentRemaining = 4200;
    float silenceFinalMask = 1.0f;
    while (livePrepared && silentRemaining > 0)
    {
        const auto blockFrames = juce::jmin (512, silentRemaining);
        liveInput.clear();
        XYPadEngine::Transport liveTransport;
        liveTransport.playing = true;
        liveTransport.startBar = 3;
        liveTransport.startPhase = static_cast<double> (liveProcessed) / 128000.0;
        liveTransport.phasePerSample = 1.0 / 128000.0;
        liveProbe.processLive (liveInput, liveOutput, liveMask, liveTransport, blockFrames);
        silenceFinalMask = liveMask.getSample (0, blockFrames - 1);
        liveProcessed += blockFrames;
        silentRemaining -= blockFrames;
    }
    float resumedMaximumMask = 0.0f;
    int resumeRemaining = 4200;
    while (livePrepared && resumeRemaining > 0)
    {
        const auto blockFrames = juce::jmin (512, resumeRemaining);
        for (int i = 0; i < blockFrames; ++i)
        {
            liveInput.setSample (0, i, 0.5f);
            liveInput.setSample (1, i, -0.25f);
        }
        XYPadEngine::Transport liveTransport;
        liveTransport.playing = true;
        liveTransport.startBar = 3;
        liveTransport.startPhase = static_cast<double> (liveProcessed) / 128000.0;
        liveTransport.phasePerSample = 1.0 / 128000.0;
        liveProbe.processLive (liveInput, liveOutput, liveMask, liveTransport, blockFrames);
        resumedMaximumMask = juce::jmax (resumedMaximumMask, liveMask.getMagnitude (0, 0, blockFrames));
        liveProcessed += blockFrames;
        resumeRemaining -= blockFrames;
    }
    AllocationAudit::enabled.store (false, std::memory_order_release);
    std::cout << "METRIC xy_live first_full_wet_sample=" << firstFullWet
              << " expected_min=" << 2300 << " expected_max=" << 2800
              << " silence_final_mask=" << silenceFinalMask
              << " resumed_max_mask=" << resumedMaximumMask
              << " allocations=" << AllocationAudit::count.load (std::memory_order_relaxed)
              << " finite=" << liveFinite << "\n";
    check (livePrepared && firstFullWet >= 2300 && firstFullWet <= 2800 && liveFinite,
           "first-visit-bar4-live-wet-after-20ms-history-plus-1024-sample-pipeline-and-8ms-ramp");
    check (livePrepared && silenceFinalMask == 0.0f && resumedMaximumMask >= 0.99f,
           "live-xy-source-silence-returns-dry-and-reacquires-within-same-bar");
    check (AllocationAudit::count.load (std::memory_order_relaxed) == 0,
           "live-xy-processing-performs-no-heap-allocation");
    liveProbe.release();

    check (XYPadEngineTestAccess::livePipelineHasExact1024SampleIdentityDelay(),
           "live-pitch-neutral-identity-has-exact-1024-sample-delay");
    bool pitchMatrixFinite = true, pitchMatrixNoAllocation = true;
    bool pitchRanges = true;
    for (const auto rate : { 44100.0, 48000.0, 96000.0 })
    {
        for (const auto y : { 1.0f, 0.5f, 0.0f })
        {
            bool finiteAtRate = true, noAllocationAtRate = true;
            const auto frequency = XYPadEngineTestAccess::renderLivePitchFrequency (
                rate, y, finiteAtRate, noAllocationAtRate);
            const auto expected = y > 0.75f ? 880.0 : (y < 0.25f ? 220.0 : 440.0);
            std::cout << "METRIC live_pitch sample_rate=" << rate << " saved_y=" << y
                      << " estimated_hz=" << frequency << " expected_hz=" << expected
                      << " finite=" << finiteAtRate << " no_allocation=" << noAllocationAtRate << "\n";
            pitchRanges &= std::abs (frequency - expected) / expected < 0.01;
            pitchMatrixFinite &= finiteAtRate;
            pitchMatrixNoAllocation &= noAllocationAtRate;
        }
        bool reverseFinite = true, reverseNoAllocation = true;
        const auto reverseFrequency = XYPadEngineTestAccess::renderLivePitchFrequency (
            rate, 1.0f, reverseFinite, reverseNoAllocation);
        pitchMatrixFinite &= reverseFinite && std::abs (reverseFrequency - 880.0) / 880.0 < 0.01;
        pitchMatrixNoAllocation &= reverseNoAllocation;
    }
    check (pitchRanges, "live-pitch-top-center-bottom-map-to-plus12-zero-minus12-st");
    check (pitchMatrixFinite && pitchMatrixNoAllocation,
           "live-pitch-is-finite-and-allocation-free-at-44k1-48k-96k-forward-and-reverse");
    check (XYPadEngineTestAccess::livePitchDoesNotChangeXDirectionOrSampleClock(),
           "live-pitch-y-does-not-change-x-direction-or-sample-clock");

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
    check (std::abs (estimatedHz - 220.0) / 220.0 < 0.01, "pitch-bottom-minus-12-semitones");
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

    // BAR 4 verifies the top-edge pitch independently of the X read direction.
    // The saved Y coordinate is 1 at the visual top, which maps to +12 st.
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
    if (! downReady || std::abs (estimatedDownHz - 440.0) / 440.0 >= 0.01)
        std::cerr << "PITCH_DOWN_DIAGNOSTIC ready=" << downReady
                  << " estimated_hz=" << estimatedDownHz << '\n';
    check (downReady && std::abs (estimatedDownHz - 440.0) / 440.0 < 0.01,
           "pitch-top-plus-12-semitones");
    auto downStream = juce::File::getCurrentWorkingDirectory()
        .getChildFile ("xy-diagnostic-pitch-down.wav").createOutputStream();
    std::unique_ptr<juce::AudioFormatWriter> downWriter;
    if (downStream != nullptr)
        downWriter.reset (wav.createWriterFor (downStream.release(), sampleRate, 2, 24, {}, 0));
    check (downWriter != nullptr && downWriter->writeFromAudioSampleBuffer (dummyOutput, 0, frames),
           "diagnostic-wave-pitch-down-written");
    downWriter.reset();

    // Repeat +12 st with forward X. Saved Y=1 is the visual top.
    std::vector<PluginStateModel::MotionPoint> pitchUpForward {
        { 0.0f, 1.0f, 0.0 }, { 1.0f, 1.0f, 8.0 / 3.0 }
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

    // Repeat -12 st with reverse X; saved Y=0 is the visual bottom.
    std::vector<PluginStateModel::MotionPoint> pitchDownReverse {
        { 1.0f, 0.0f, 0.0 }, { 0.0f, 0.0f, 8.0 / 3.0 }
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
