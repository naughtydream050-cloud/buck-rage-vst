#include "XYPadEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>

namespace
{
constexpr double kPi = 3.1415926535897932384626433832795;
constexpr double kMaxSampleRate = 96000.0;
constexpr double kMaxBarSeconds = 4.0 * 60.0 / 90.0;
constexpr double kTransitionSeconds = 0.003;
constexpr double kHoldLoopSeconds = 0.020;
constexpr double kHoldSeamSeconds = 0.002;

uint32_t floatBits (float value) noexcept
{
    uint32_t bits = 0;
    std::memcpy (&bits, &value, sizeof (bits));
    return bits;
}

static_assert (sizeof (float) == sizeof (uint32_t), "XY float snapshots require IEEE-width float");
static_assert (sizeof (double) == sizeof (uint64_t), "XY timestamp snapshots require 64-bit double");

float bitsFloat (uint32_t bits) noexcept
{
    float value = 0.0f;
    std::memcpy (&value, &bits, sizeof (value));
    return value;
}

uint64_t doubleBits (double value) noexcept
{
    uint64_t bits = 0;
    std::memcpy (&bits, &value, sizeof (bits));
    return bits;
}

double bitsDouble (uint64_t bits) noexcept
{
    double value = 0.0;
    std::memcpy (&value, &bits, sizeof (value));
    return value;
}

int wrapBar (int bar) noexcept
{
    const auto wrapped = bar % XYPadEngine::kBarCount;
    return wrapped < 0 ? wrapped + XYPadEngine::kBarCount : wrapped;
}
}

XYPadEngine::~XYPadEngine()
{
    release();
}

XYPadEngine::XYPadEngine() noexcept
    : slots (new (std::nothrow) Slot[static_cast<size_t> (kBarCount)]),
      publishedTrajectories (new (std::nothrow) AtomicTrajectory[static_cast<size_t> (kBarCount)]),
      activeTrajectories (new (std::nothrow) Trajectory[static_cast<size_t> (kBarCount)])
{
}

bool XYPadEngine::prepare (double rate, int maximumBlockSize, bool allocateBank) noexcept
{
    const std::lock_guard<std::mutex> lifecycleLock (nonRealtimeLifecycleMutex);
    stopWorker();
    enabled.store (false, std::memory_order_release);
    if (! slots || ! publishedTrajectories || ! activeTrajectories)
        return false;
    for (int i = 0; i < kBarCount; ++i)
    {
        auto& slot = slots[static_cast<size_t> (i)];
        slot.pcm.reset();
        slot.state.store (State::empty, std::memory_order_relaxed);
        slot.generation.fetch_add (1, std::memory_order_relaxed);
        slot.frames.store (0, std::memory_order_relaxed);
        slot.captureFrames = slot.expectedFrames = 0;
        slot.captureTrajectory = {};
        slot.activeTimingGeneration = 0;
    }
    scratch.reset(); fftWork.reset(); fftWork2.reset(); overlapNorm.reset();
    previousInputPhase.reset(); synthesisPhase.reset(); phaseInitialized.reset(); fft.reset();
    sampleRate = rate;
    maxBlock = maximumBlockSize;
    activeBar = -1;
    activeRevision = 0;
    renderQueueHead.store (0, std::memory_order_relaxed);
    renderQueueTail.store (0, std::memory_order_relaxed);
    activeTrajectory = {};
    std::fill_n (activeTrajectories.get(), kBarCount, Trajectory {});

    if (! std::isfinite (rate) || rate <= 0.0 || rate > kMaxSampleRate
        || maximumBlockSize <= 0)
        return false;

    if (! allocateBank)
    {
        maxFramesPerBar = 0;
        return true;
    }

    const auto requestedFrames = std::ceil (rate * kMaxBarSeconds);
    if (requestedFrames > static_cast<double> (std::numeric_limits<int>::max() / 2))
        return false;
    maxFramesPerBar = static_cast<int> (requestedFrames);
    const auto floatCount = static_cast<size_t> (maxFramesPerBar) * 2u;
    try
    {
        for (int i = 0; i < kBarCount; ++i)
        {
            auto& slot = slots[static_cast<size_t> (i)];
            slot.pcm.reset (new (std::nothrow) float[floatCount]);
            if (! slot.pcm)
            {
                for (int j = 0; j < kBarCount; ++j) slots[static_cast<size_t> (j)].pcm.reset();
                maxFramesPerBar = 0;
                return false;
            }
            std::fill_n (slot.pcm.get(), floatCount, 0.0f);
        }
        scratch.reset (new (std::nothrow) float[floatCount]);
        fftWork.reset (new (std::nothrow) float[static_cast<size_t> (fftSize) * 2u]);
        fftWork2.reset (new (std::nothrow) float[static_cast<size_t> (fftSize) * 2u]);
        overlapNorm.reset (new (std::nothrow) float[static_cast<size_t> (maxFramesPerBar)]);
        const auto phaseCount = static_cast<size_t> (2 * (fftSize / 2 + 1));
        previousInputPhase.reset (new (std::nothrow) double[phaseCount]);
        synthesisPhase.reset (new (std::nothrow) double[phaseCount]);
        phaseInitialized.reset (new (std::nothrow) uint8_t[phaseCount]);
        fft = std::make_unique<juce::dsp::FFT> (11);
        if (! scratch || ! fftWork || ! fftWork2 || ! overlapNorm || ! previousInputPhase
            || ! synthesisPhase || ! phaseInitialized || ! fft)
        {
            for (int i = 0; i < kBarCount; ++i) slots[static_cast<size_t> (i)].pcm.reset();
            scratch.reset(); fftWork.reset(); fftWork2.reset(); overlapNorm.reset();
            previousInputPhase.reset(); synthesisPhase.reset(); phaseInitialized.reset(); fft.reset();
            maxFramesPerBar = 0;
            return false;
        }
        workerExit.store (false, std::memory_order_release);
        worker = std::thread ([this] { workerLoop(); });
    }
    catch (...)
    {
        stopWorker();
        for (int i = 0; i < kBarCount; ++i) slots[static_cast<size_t> (i)].pcm.reset();
        scratch.reset(); fftWork.reset(); fftWork2.reset(); overlapNorm.reset();
        previousInputPhase.reset(); synthesisPhase.reset(); phaseInitialized.reset(); fft.reset();
        maxFramesPerBar = 0;
        return false;
    }
    enabled.store (true, std::memory_order_release);
    return true;
}

void XYPadEngine::release() noexcept
{
    const std::lock_guard<std::mutex> lifecycleLock (nonRealtimeLifecycleMutex);
    enabled.store (false, std::memory_order_release);
    stopWorker();
    for (int i = 0; slots && i < kBarCount; ++i)
    {
        auto& slot = slots[static_cast<size_t> (i)];
        slot.state.store (State::empty, std::memory_order_relaxed);
        slot.pcm.reset();
        slot.frames.store (0, std::memory_order_relaxed);
    }
    scratch.reset(); fftWork.reset(); fftWork2.reset(); overlapNorm.reset();
    previousInputPhase.reset(); synthesisPhase.reset(); phaseInitialized.reset(); fft.reset();
    maxFramesPerBar = maxBlock = 0;
    sampleRate = 0.0;
    activeBar = -1;
    renderQueueHead.store (0, std::memory_order_relaxed);
    renderQueueTail.store (0, std::memory_order_relaxed);
}

void XYPadEngine::stopWorker() noexcept
{
    workerExit.store (true, std::memory_order_release);
    if (worker.joinable()) worker.join();
}

void XYPadEngine::setTrajectory (int bar,
                                 const std::vector<PluginStateModel::MotionPoint>& points,
                                 uint32_t revision) noexcept
{
    if (bar < 0 || bar >= kBarCount)
        return;
    if (! publishedTrajectories)
        return;
    auto& target = publishedTrajectories[static_cast<size_t> (bar)];
    auto seq = target.sequence.load (std::memory_order_relaxed);
    if ((seq & 1u) != 0u) ++seq;
    target.sequence.store (seq + 1u, std::memory_order_release);
    const auto count = juce::jmin (static_cast<int> (points.size()), kMaxPoints);
    int written = 0;
    double previousTime = -1.0;
    for (int i = 0; i < count; ++i)
    {
        const auto& point = points[static_cast<size_t> (i)];
        if (! std::isfinite (point.x) || ! std::isfinite (point.y)
            || ! std::isfinite (point.timeSeconds) || point.timeSeconds < previousTime)
            continue;
        target.x[static_cast<size_t> (written)].store (floatBits (juce::jlimit (0.0f, 1.0f, point.x)), std::memory_order_relaxed);
        target.y[static_cast<size_t> (written)].store (floatBits (juce::jlimit (0.0f, 1.0f, point.y)), std::memory_order_relaxed);
        const auto time = doubleBits (point.timeSeconds);
        target.timeLow[static_cast<size_t> (written)].store (static_cast<uint32_t> (time), std::memory_order_relaxed);
        target.timeHigh[static_cast<size_t> (written)].store (static_cast<uint32_t> (time >> 32u), std::memory_order_relaxed);
        previousTime = point.timeSeconds;
        ++written;
    }
    target.revision.store (revision, std::memory_order_relaxed);
    target.count.store (written, std::memory_order_relaxed);
    target.sequence.store (seq + 2u, std::memory_order_release);
}

bool XYPadEngine::readTrajectory (const AtomicTrajectory& source, Trajectory& result) noexcept
{
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        const auto before = source.sequence.load (std::memory_order_acquire);
        if ((before & 1u) != 0u)
            continue;
        const auto count = juce::jlimit (0, kMaxPoints, static_cast<int> (source.count.load (std::memory_order_relaxed)));
        result.revision = source.revision.load (std::memory_order_relaxed);
        result.count = count;
        for (int i = 0; i < count; ++i)
        {
            result.x[static_cast<size_t> (i)] = bitsFloat (source.x[static_cast<size_t> (i)].load (std::memory_order_relaxed));
            result.y[static_cast<size_t> (i)] = bitsFloat (source.y[static_cast<size_t> (i)].load (std::memory_order_relaxed));
        {
            const auto low = source.timeLow[static_cast<size_t> (i)].load (std::memory_order_relaxed);
            const auto high = source.timeHigh[static_cast<size_t> (i)].load (std::memory_order_relaxed);
            result.time[static_cast<size_t> (i)] = bitsDouble (static_cast<uint64_t> (low)
                                                               | (static_cast<uint64_t> (high) << 32u));
        }
        }
        const auto after = source.sequence.load (std::memory_order_acquire);
        if (before == after && (after & 1u) == 0u)
            return true;
    }
    return false;
}

void XYPadEngine::sampleTimeline (const Transport& transport, int sample, int totalSamples,
                                  int& bar, double& phase) noexcept
{
    if (transport.loopBoundarySample >= 0 && sample >= transport.loopBoundarySample)
    {
        const auto offset = sample - transport.loopBoundarySample;
        const auto absolute = transport.loopStartPhase + static_cast<double> (offset) * transport.phasePerSample;
        const auto crossed = static_cast<int> (std::floor (absolute));
        bar = wrapBar (transport.loopStartBar + crossed);
        phase = absolute - static_cast<double> (crossed);
        return;
    }
    const auto absolute = transport.startPhase + static_cast<double> (sample) * transport.phasePerSample;
    const auto crossed = static_cast<int> (std::floor (absolute));
    bar = wrapBar (transport.startBar + crossed);
    phase = absolute - static_cast<double> (crossed);
    juce::ignoreUnused (totalSamples);
}

void XYPadEngine::interpolateTrajectory (const Trajectory& trajectory, double phase,
                                         float& x, float& y) noexcept
{
    if (trajectory.count <= 0)
    {
        x = y = 0.5f;
        return;
    }
    if (trajectory.count == 1 || phase <= 0.0)
    {
        x = trajectory.x[0]; y = trajectory.y[0];
        return;
    }
    double first = trajectory.time[0];
    double last = trajectory.time[static_cast<size_t> (trajectory.count - 1)];
    const auto span = juce::jmax (1.0e-9, last - first);
    const auto query = first + juce::jlimit (0.0, 1.0, phase) * span;
    int hi = 1;
    while (hi < trajectory.count - 1 && trajectory.time[static_cast<size_t> (hi)] < query)
        ++hi;
    const auto lo = hi - 1;
    const auto t0 = trajectory.time[static_cast<size_t> (lo)];
    const auto t1 = trajectory.time[static_cast<size_t> (hi)];
    const auto mix = t1 > t0 ? static_cast<float> ((query - t0) / (t1 - t0)) : 1.0f;
    x = trajectory.x[static_cast<size_t> (lo)]
        + (trajectory.x[static_cast<size_t> (hi)] - trajectory.x[static_cast<size_t> (lo)]) * mix;
    y = trajectory.y[static_cast<size_t> (lo)]
        + (trajectory.y[static_cast<size_t> (hi)] - trajectory.y[static_cast<size_t> (lo)]) * mix;
}

float XYPadEngine::interpolate (const float* source, int frames, double position, int stride) noexcept
{
    if (source == nullptr || frames <= 0 || ! std::isfinite (position))
        return 0.0f;
    const auto bounded = juce::jlimit (0.0, static_cast<double> (frames - 1), position);
    const auto i0 = static_cast<int> (bounded);
    const auto i1 = juce::jmin (frames - 1, i0 + 1);
    const auto t = static_cast<float> (bounded - static_cast<double> (i0));
    return source[static_cast<size_t> (i0) * static_cast<size_t> (stride)]
         + (source[static_cast<size_t> (i1) * static_cast<size_t> (stride)]
          - source[static_cast<size_t> (i0) * static_cast<size_t> (stride)]) * t;
}

void XYPadEngine::process (const juce::AudioBuffer<float>& source,
                           juce::AudioBuffer<float>& rendered,
                           juce::AudioBuffer<float>& readyMask,
                           const Transport& transport, int frameCount) noexcept
{
    const auto framesInBlock = juce::jlimit (0, source.getNumSamples(), frameCount);
    for (int channel = 0; channel < rendered.getNumChannels(); ++channel)
        rendered.clear (channel, 0, juce::jmin (framesInBlock, rendered.getNumSamples()));
    if (readyMask.getNumChannels() > 0)
        readyMask.clear (0, 0, juce::jmin (framesInBlock, readyMask.getNumSamples()));
    if (! enabled.load (std::memory_order_acquire) || ! transport.playing
        || transport.startBar < 0 || framesInBlock <= 0 || framesInBlock > maxBlock
        || rendered.getNumSamples() < framesInBlock || readyMask.getNumChannels() < 1
        || readyMask.getNumSamples() < framesInBlock)
    {
        for (int slotIndex = 0; slots && slotIndex < kBarCount; ++slotIndex)
        {
            auto& slot = slots[static_cast<size_t> (slotIndex)];
            if (slot.state.load (std::memory_order_acquire) == State::capturing)
                slot.state.store (State::empty, std::memory_order_release);
        }
        activeBar = -1;
        return;
    }

    const auto sourceChannels = source.getNumChannels();
    const auto outputChannels = juce::jmin (2, rendered.getNumChannels());
    auto* mask = readyMask.getWritePointer (0);
    const auto framesPerBar = transport.secondsPerQuarter * transport.quartersPerBar * sampleRate;
    if (! std::isfinite (framesPerBar) || framesPerBar < 1.0
        || framesPerBar > static_cast<double> (maxFramesPerBar))
    {
        for (int slotIndex = 0; slots && slotIndex < kBarCount; ++slotIndex)
        {
            auto& slot = slots[static_cast<size_t> (slotIndex)];
            if (slot.state.load (std::memory_order_acquire) == State::capturing)
                slot.state.store (State::empty, std::memory_order_release);
        }
        activeBar = -1;
        return;
    }
    const auto expected = juce::jlimit (1, maxFramesPerBar, juce::roundToInt (framesPerBar));

    for (int i = 0; i < framesInBlock; ++i)
    {
        int bar = -1;
        double phase = 0.0;
        sampleTimeline (transport, i, framesInBlock, bar, phase);
        if (bar != activeBar)
        {
            if (activeBar >= 0)
            {
                auto& old = slots[static_cast<size_t> (activeBar)];
                if (old.state.load (std::memory_order_acquire) == State::capturing)
                    old.state.store (State::empty, std::memory_order_release);
            }
            activeBar = bar;
            activeRevision = 0;
            Trajectory candidate;
            auto& slot = slots[static_cast<size_t> (bar)];
            if (readTrajectory (publishedTrajectories[static_cast<size_t> (bar)], candidate))
                activeTrajectories[static_cast<size_t> (bar)] = candidate;
            activeTrajectory = activeTrajectories[static_cast<size_t> (bar)];
            activeRevision = activeTrajectory.revision;
            if (activeRevision != 0)
            {
                const auto oldRevision = slot.trajectoryRevision.load (std::memory_order_acquire);
                const auto oldTiming = slot.activeTimingGeneration;
                if ((oldRevision != 0 && oldRevision != activeRevision)
                    || (slot.frames.load (std::memory_order_acquire) > 0
                        && oldTiming != transport.timingGeneration))
                {
                    auto ready = State::ready;
                    if (slot.state.compare_exchange_strong (ready, State::empty, std::memory_order_acq_rel))
                    {
                        slot.frames.store (0, std::memory_order_release);
                        slot.generation.fetch_add (1, std::memory_order_acq_rel);
                    }
                }
            }
        }

        auto& slot = slots[static_cast<size_t> (bar)];
        const auto slotState = slot.state.load (std::memory_order_acquire);
        if (slotState == State::ready
            && (slot.trajectoryRevision.load (std::memory_order_acquire) != activeRevision
                || slot.activeTimingGeneration != transport.timingGeneration))
        {
            auto ready = State::ready;
            if (slot.state.compare_exchange_strong (ready, State::empty, std::memory_order_acq_rel))
                slot.frames.store (0, std::memory_order_release);
        }
        else if (slotState == State::capturing
                 && (slot.trajectoryRevision.load (std::memory_order_acquire) != activeRevision
                     || slot.activeTimingGeneration != transport.timingGeneration))
        {
            slot.state.store (State::empty, std::memory_order_release);
            slot.captureFrames = 0;
        }
        const auto state = slot.state.load (std::memory_order_acquire);
        if (activeTrajectory.count > 0 && state == State::empty
            && phase <= juce::jmax (transport.phasePerSample * 1.5, 1.0e-8))
        {
            auto empty = State::empty;
            if (slot.state.compare_exchange_strong (empty, State::capturing, std::memory_order_acq_rel))
            {
                slot.captureFrames = 0;
                slot.expectedFrames = expected;
                slot.captureTrajectory = activeTrajectory;
                slot.trajectoryRevision.store (activeRevision, std::memory_order_release);
                slot.activeTimingGeneration = transport.timingGeneration;
            }
        }

        if (slot.state.load (std::memory_order_acquire) == State::capturing)
        {
            const auto phaseExpected = static_cast<double> (slot.captureFrames)
                                    / static_cast<double> (juce::jmax (1, slot.expectedFrames));
            const auto tolerance = juce::jmax (transport.phasePerSample * 2.0, 2.0 / slot.expectedFrames);
            if (std::abs (phase - phaseExpected) > tolerance)
            {
                slot.state.store (State::empty, std::memory_order_release);
                slot.captureFrames = 0;
            }
            else
            {
                const auto frame = slot.captureFrames;
                const auto left = sourceChannels > 0 ? source.getReadPointer (0)[i] : 0.0f;
                const auto right = sourceChannels > 1 ? source.getReadPointer (1)[i] : left;
                slot.pcm[static_cast<size_t> (frame) * 2u] = left;
                slot.pcm[static_cast<size_t> (frame) * 2u + 1u] = right;
                ++slot.captureFrames;
                if (slot.captureFrames >= slot.expectedFrames)
                {
                    slot.frames.store (slot.expectedFrames, std::memory_order_release);
                    const auto generation = slot.generation.fetch_add (1, std::memory_order_acq_rel) + 1u;
                    slot.state.store (State::queued, std::memory_order_release);
                    if (! enqueueRenderJob ({ static_cast<uint32_t> (bar), generation }))
                    {
                        auto queued = State::queued;
                        if (slot.state.compare_exchange_strong (queued, State::empty, std::memory_order_acq_rel))
                            slot.frames.store (0, std::memory_order_release);
                    }
                }
            }
        }

        if (activeTrajectory.count > 0 && slot.trajectoryRevision.load (std::memory_order_acquire) == activeRevision
            && slot.activeTimingGeneration == transport.timingGeneration
            && slot.state.load (std::memory_order_acquire) == State::ready)
        {
            const auto frames = slot.frames.load (std::memory_order_acquire);
            if (frames > 0 && frames <= maxFramesPerBar)
            {
                const auto frame = juce::jlimit (0, frames - 1,
                    juce::roundToInt (juce::jlimit (0.0, 1.0, phase) * (frames - 1)));
                if (outputChannels > 0) rendered.setSample (0, i, slot.pcm[static_cast<size_t> (frame) * 2u]);
                if (outputChannels > 1) rendered.setSample (1, i, slot.pcm[static_cast<size_t> (frame) * 2u + 1u]);
                mask[i] = 1.0f;
            }
        }
    }
}

bool XYPadEngine::renderSlot (Slot& slot) noexcept
{
    const auto frames = slot.frames.load (std::memory_order_acquire);
    if (frames < fftSize || frames > maxFramesPerBar || slot.pcm == nullptr || scratch == nullptr)
        return false;
    return renderScrubAndPitch (slot, frames);
}

bool XYPadEngine::renderScrubAndPitch (Slot& slot, int frames, int* longestTransitionFrames,
                                      double* transitionEndTargetErrorSamples) noexcept
{
    auto* input = slot.pcm.get();
    auto* out = scratch.get();
    const auto& trajectory = slot.captureTrajectory;
    if (trajectory.count <= 0)
        return false;

    float previousX = trajectory.x[0];
    double transitionFrom = 0.0;
    int transitionRemaining = 0;
    float previousDelta = 0.0f;
    bool wasHolding = false;
    bool haveLastRead = false;
    double lastReadPosition = 0.0;
    int holdPhase = 0;
    int transitionRunFrames = 0;
    if (longestTransitionFrames != nullptr)
        *longestTransitionFrames = 0;
    if (transitionEndTargetErrorSamples != nullptr)
        *transitionEndTargetErrorSamples = std::numeric_limits<double>::infinity();
    const auto transitionFrames = juce::jmax (1, juce::roundToInt (sampleRate * kTransitionSeconds));
    const auto holdFrames = juce::jmax (8, juce::roundToInt (sampleRate * kHoldLoopSeconds));
    const auto seamFrames = juce::jmax (2, juce::roundToInt (sampleRate * kHoldSeamSeconds));

    for (int n = 0; n < frames; ++n)
    {
        const auto phase = static_cast<double> (n) / static_cast<double> (juce::jmax (1, frames - 1));
        float x = 0.5f, y = 0.5f;
        interpolateTrajectory (trajectory, phase, x, y);
        x = juce::jlimit (0.0f, 1.0f, x);
        y = juce::jlimit (0.0f, 1.0f, y);
        const auto target = static_cast<double> (x) * static_cast<double> (frames - 1);
        const auto normalizedDelta = x - previousX;
        const auto delta = static_cast<double> (normalizedDelta) * static_cast<double> (frames - 1);
        const auto holding = std::abs (delta) < 1.0e-5;
        const auto reverse = n > 1 && normalizedDelta * previousDelta < 0.0f;
        const auto previousRate = static_cast<double> (previousDelta)
                                * static_cast<double> (frames - 1);
        const auto jump = std::abs (delta) > 4.0 && std::abs (delta - previousRate) > 4.0;
        if (n > 0 && transitionRemaining == 0 && (reverse || jump || holding != wasHolding))
        {
            transitionFrom = haveLastRead ? lastReadPosition
                                          : static_cast<double> (previousX) * static_cast<double> (frames - 1);
            transitionRemaining = transitionFrames;
        }

        double readPosition = target;
        bool holdingSeam = false;
        double holdHeadPosition = 0.0;
        double holdSeamMix = 0.0;
        if (holding)
        {
            const auto loop = juce::jmin (holdFrames + seamFrames, frames);
            const auto half = 0.5 * static_cast<double> (loop - 1);
            const auto start = juce::jlimit (0.0, static_cast<double> (frames - loop), target - half);
            if (! wasHolding)
                holdPhase = juce::jlimit (0, loop - 1, juce::roundToInt (target - start));
            const auto loopPhase = holdPhase;
            readPosition = start + static_cast<double> (loopPhase);
            if (loopPhase >= loop - seamFrames)
            {
                holdingSeam = true;
                holdHeadPosition = start + static_cast<double> (loopPhase - (loop - seamFrames));
                holdSeamMix = juce::jlimit (0.0, 1.0,
                    static_cast<double> (loopPhase - (loop - seamFrames)) / juce::jmax (1, seamFrames - 1));
            }
            ++holdPhase;
            if (holdPhase >= loop)
                holdPhase = juce::jmin (seamFrames, loop - 1);
        }

        for (int channel = 0; channel < 2; ++channel)
        {
            auto value = interpolate (input + channel, frames, readPosition, 2);
            if (holdingSeam)
            {
                const auto head = interpolate (input + channel, frames, holdHeadPosition, 2);
                value = static_cast<float> (value * std::cos (holdSeamMix * kPi * 0.5)
                                          + head * std::sin (holdSeamMix * kPi * 0.5));
            }
            if (transitionRemaining > 0)
            {
                const auto progress = 1.0 - static_cast<double> (transitionRemaining - 1)
                                      / static_cast<double> (juce::jmax (1, transitionFrames - 1));
                const auto oldValue = interpolate (input + channel, frames, transitionFrom, 2);
                value = static_cast<float> (oldValue * std::cos (progress * kPi * 0.5)
                                          + value * std::sin (progress * kPi * 0.5));
            }
            out[static_cast<size_t> (n) * 2u + static_cast<size_t> (channel)] = value;
        }
        if (transitionRemaining > 0)
        {
            if (transitionRemaining == 1 && transitionEndTargetErrorSamples != nullptr)
                *transitionEndTargetErrorSamples = std::abs (readPosition - target);
            ++transitionRunFrames;
            if (longestTransitionFrames != nullptr)
                *longestTransitionFrames = juce::jmax (*longestTransitionFrames, transitionRunFrames);
            --transitionRemaining;
            transitionFrom = juce::jlimit (0.0, static_cast<double> (frames - 1), transitionFrom + (delta < 0.0 ? -1.0 : 1.0));
        }
        else
        {
            transitionRunFrames = 0;
        }
        lastReadPosition = readPosition;
        haveLastRead = true;
        wasHolding = holding;
        previousDelta = normalizedDelta;
        previousX = x;
    }

    // Offline, centered, Hann-windowed spectral remap. The input BAR is already
    // complete, so FFT windows add no host latency. Frame pitch follows the BAR
    // trajectory and does not alter the previously rendered X scrub path.
    std::fill_n (overlapNorm.get(), static_cast<size_t> (frames), 0.0f);
    for (int channel = 0; channel < 2; ++channel)
    {
        const auto phaseBase = static_cast<size_t> (channel) * static_cast<size_t> (fftSize / 2 + 1);
        std::fill_n (phaseInitialized.get() + phaseBase, static_cast<size_t> (fftSize / 2 + 1), uint8_t (0));
        for (int i = 0; i < frames; ++i)
            input[static_cast<size_t> (i) * 2u + static_cast<size_t> (channel)] = 0.0f;

        for (int frameStart = 0; frameStart < frames; frameStart += hopSize)
        {
            const auto centre = frameStart;
            const auto phase = static_cast<double> (centre) / static_cast<double> (juce::jmax (1, frames - 1));
            float x = 0.5f, y = 0.5f;
            interpolateTrajectory (trajectory, phase, x, y);
            const auto semitones = 24.0 * (0.5 - static_cast<double> (y));
            const auto ratio = std::pow (2.0, semitones / 12.0);
            const auto offset = -fftSize / 2;
            std::fill_n (fftWork.get(), static_cast<size_t> (fftSize) * 2u, 0.0f);
            for (int i = 0; i < fftSize; ++i)
            {
                const auto src = frameStart + offset + i;
                if (src >= 0 && src < frames)
                {
                    const auto hann = 0.5 - 0.5 * std::cos (2.0 * kPi * i / (fftSize - 1));
                    fftWork[static_cast<size_t> (i)] = out[static_cast<size_t> (src) * 2u + static_cast<size_t> (channel)]
                                                       * static_cast<float> (hann);
                }
            }
            fft->performRealOnlyForwardTransform (fftWork.get());
            std::fill_n (fftWork2.get(), static_cast<size_t> (fftSize) * 2u, 0.0f);
            const auto readBin = [this] (const float* data, int bin, bool imaginary) noexcept
            {
                if (bin == 0) return imaginary ? 0.0f : data[0];
                if (bin == fftSize / 2) return imaginary ? 0.0f : data[1];
                return data[2 * bin + (imaginary ? 1 : 0)];
            };
            const auto writeBin = [this] (float* data, int bin, float real, float imaginary) noexcept
            {
                if (bin == 0) { data[0] = real; return; }
                if (bin == fftSize / 2) { data[1] = real; return; }
                data[2 * bin] = real;
                data[2 * bin + 1] = imaginary;
            };
            for (int bin = 0; bin <= fftSize / 2; ++bin)
            {
                const auto sourceBin = static_cast<double> (bin) / ratio;
                if (sourceBin > fftSize / 2.0)
                    continue;
                const auto low = static_cast<int> (sourceBin);
                const auto high = juce::jmin (fftSize / 2, low + 1);
                const auto mix = static_cast<float> (sourceBin - low);
                const auto real = readBin (fftWork.get(), low, false)
                                + (readBin (fftWork.get(), high, false) - readBin (fftWork.get(), low, false)) * mix;
                const auto imag = readBin (fftWork.get(), low, true)
                                + (readBin (fftWork.get(), high, true) - readBin (fftWork.get(), low, true)) * mix;
                const auto magnitude = std::hypot (real, imag);
                const auto inputPhase = std::atan2 (imag, real);
                const auto phaseIndex = phaseBase + static_cast<size_t> (bin);
                auto& lastInput = previousInputPhase[phaseIndex];
                auto& outputPhase = synthesisPhase[phaseIndex];
                if (bin == 0 || bin == fftSize / 2)
                {
                    outputPhase = inputPhase;
                }
                else if (phaseInitialized[phaseIndex] == 0)
                {
                    outputPhase = inputPhase;
                    phaseInitialized[phaseIndex] = 1;
                }
                else
                {
                    const auto expectedAdvance = 2.0 * kPi * sourceBin * hopSize / fftSize;
                    const auto residual = std::remainder (inputPhase - lastInput - expectedAdvance, 2.0 * kPi);
                    const auto instantaneousBin = sourceBin + residual * fftSize / (2.0 * kPi * hopSize);
                    outputPhase += 2.0 * kPi * instantaneousBin * ratio * hopSize / fftSize;
                }
                lastInput = inputPhase;
                writeBin (fftWork2.get(), bin,
                          magnitude * static_cast<float> (std::cos (outputPhase)),
                          magnitude * static_cast<float> (std::sin (outputPhase)));
            }
            fft->performRealOnlyInverseTransform (fftWork2.get());
            for (int i = 0; i < fftSize; ++i)
            {
                const auto dest = frameStart + offset + i;
                if (dest < 0 || dest >= frames) continue;
                const auto hann = static_cast<float> (0.5 - 0.5 * std::cos (2.0 * kPi * i / (fftSize - 1)));
                // JUCE 7.0.12 normalizes performRealOnlyInverseTransform() by
                // 1/N internally. Applying a second 1/N here attenuates XY by 66 dB.
                const auto value = fftWork2[static_cast<size_t> (i)] * hann;
                input[static_cast<size_t> (dest) * 2u + static_cast<size_t> (channel)] += value;
                if (channel == 0) overlapNorm[static_cast<size_t> (dest)] += hann * hann;
            }
        }
    }
    for (int i = 0; i < frames; ++i)
    {
        const auto norm = juce::jmax (1.0e-6f, overlapNorm[static_cast<size_t> (i)]);
        input[static_cast<size_t> (i) * 2u] /= norm;
        input[static_cast<size_t> (i) * 2u + 1u] /= norm;
        if (! std::isfinite (input[static_cast<size_t> (i) * 2u])
            || ! std::isfinite (input[static_cast<size_t> (i) * 2u + 1u]))
            return false;
    }
    return true;
}

void XYPadEngine::workerLoop() noexcept
{
    while (! workerExit.load (std::memory_order_acquire))
    {
        RenderJob job;
        if (dequeueRenderJob (job))
        {
            if (! slots || job.bar >= static_cast<uint32_t> (kBarCount))
                continue;
            auto& slot = slots[job.bar];
            auto expected = State::queued;
            if (! slot.state.compare_exchange_strong (expected, State::rendering,
                                                       std::memory_order_acq_rel))
                continue;
            const auto generation = slot.generation.load (std::memory_order_acquire);
            if (generation != job.generation)
            {
                auto rendering = State::rendering;
                slot.state.compare_exchange_strong (rendering, State::empty, std::memory_order_acq_rel);
                continue;
            }
            const auto success = renderSlot (slot);
            if (success && generation == slot.generation.load (std::memory_order_acquire))
                slot.state.store (State::ready, std::memory_order_release);
            else
                slot.state.store (State::empty, std::memory_order_release);
            continue;
        }
        std::this_thread::sleep_for (std::chrono::milliseconds (1));
    }
}

bool XYPadEngine::enqueueRenderJob (RenderJob job) noexcept
{
    const auto head = renderQueueHead.load (std::memory_order_relaxed);
    const auto tail = renderQueueTail.load (std::memory_order_acquire);
    if (head - tail >= kRenderQueueCapacity)
        return false;
    renderQueue[head % kRenderQueueCapacity] = job;
    renderQueueHead.store (head + 1u, std::memory_order_release);
    return true;
}

bool XYPadEngine::dequeueRenderJob (RenderJob& job) noexcept
{
    const auto tail = renderQueueTail.load (std::memory_order_relaxed);
    const auto head = renderQueueHead.load (std::memory_order_acquire);
    if (tail == head)
        return false;
    job = renderQueue[tail % kRenderQueueCapacity];
    renderQueueTail.store (tail + 1u, std::memory_order_release);
    return true;
}

XYPadEngine::State XYPadEngine::getState (int bar) const noexcept
{
    if (bar < 0 || bar >= kBarCount)
        return State::empty;
    return slots ? slots[static_cast<size_t> (bar)].state.load (std::memory_order_acquire)
                 : State::empty;
}

float XYPadEngine::mixLayers (float dry, float presetOutput, float xyWet,
                              float depth, bool presetActive) noexcept
{
    const auto boundedDepth = juce::jlimit (0.0f, 1.0f, depth);
    if (boundedDepth <= 0.0f)
        return dry;
    if (! presetActive)
        return dry * (1.0f - boundedDepth) + xyWet * boundedDepth;

    constexpr float equalPower = 0.7071067811865475f;
    const auto presetWet = (presetOutput - dry * (1.0f - boundedDepth)) / boundedDepth;
    return dry * (1.0f - boundedDepth)
         + boundedDepth * (equalPower * presetWet + equalPower * xyWet);
}
