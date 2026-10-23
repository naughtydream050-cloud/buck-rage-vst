#include "XYPadEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
 #include <emmintrin.h>
 #define TOYOTOMI_XY_HAS_SSE2 1
#endif

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

bool XYPadEngine::prepare (double rate, int maximumBlockSize, bool allocateBank, bool liveOnly) noexcept
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
    liveHistory.reset(); liveHistorySerials.reset(); liveHoldSnapshot.reset();
    liveGrainWindow.reset();
    for (auto& tail : liveReentryTailBuffers) tail.reset();
    livePitchFft.reset(); livePitchFftWork.reset(); livePitchFftWork2.reset();
    livePitchInputRing.reset(); livePitchIdentityDelay.reset(); livePitchSemitoneDelay.reset();
    livePitchMaskDelay.reset(); livePitchOlaLeft.reset(); livePitchOlaRight.reset(); livePitchOlaNorm.reset();
    livePitchPreviousPhase.reset(); livePitchSynthesisPhase.reset(); livePitchPhaseInitialized.reset();
    sampleRate = rate;
    maxBlock = maximumBlockSize;
    activeBar = -1;
    liveHistoryCapacityFrames = liveWarmupFrames = liveGrainFrames = liveHopFrames = 0;
    liveTransitionFrames = 0;
    liveReentryTailFrames = liveReentryTailPosition = liveReentryTailBuffer = 0;
    liveReentryTailActive = false;
    liveWriteSerial = liveHistoryValidFrames = 0;
    liveGrains.fill (LiveGrain {});
    liveStationaryFrames = liveHoldPhase = 0;
    liveHoldMix = 0.0f;
    liveHoldReady = false;
    liveActiveBar = -1;
    liveActiveTrajectory = {};
    liveWetGain = 0.0f;
    liveHavePreviousX = liveHaveSignal = livePendingTransition = liveWasPlaying = false;
    liveSourceFadeActive = false;
    activeRevision = 0;
    renderQueueHead.store (0, std::memory_order_relaxed);
    renderQueueTail.store (0, std::memory_order_relaxed);
    activeTrajectory = {};
    std::fill_n (activeTrajectories.get(), kBarCount, Trajectory {});
    liveSamplesToLaunch = 0;
    liveSilenceFramesCount = 0;
    liveActiveRevision = 0;
    livePreviousX = 0.5f;
    liveDirection = 1;
    livePitchSerial = 0;
    liveSmoothedSemitones = livePitchWetMix = 0.0f;
    livePitchSmoothingCoefficient = 0.0f;

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
    maxFramesPerBar = liveOnly ? 0 : static_cast<int> (requestedFrames);
    const auto floatCount = static_cast<size_t> (maxFramesPerBar) * 2u;
    liveHistoryCapacityFrames = juce::jmin (static_cast<int> (kMaxLiveHistoryFrames),
        juce::jmax (1, juce::roundToInt (rate * (kLiveHistorySeconds + kLiveGrainSeconds)) + 2));
    liveWarmupFrames = juce::jmax (1, juce::roundToInt (rate * kLiveWarmupSeconds));
    liveGrainFrames = juce::jmax (1, juce::roundToInt (rate * kLiveGrainSeconds));
    liveHopFrames = juce::jmax (1, juce::roundToInt (rate * kLiveHopSeconds));
    liveTransitionFrames = juce::jmax (2, juce::roundToInt (rate * 0.003));
    try
    {
        if (! liveOnly)
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
        }
        liveHistory.reset (new (std::nothrow) float[static_cast<size_t> (liveHistoryCapacityFrames) * 2u]);
        liveHistorySerials.reset (new (std::nothrow) uint64_t[static_cast<size_t> (liveHistoryCapacityFrames)]);
        liveHoldSnapshot.reset (new (std::nothrow) float[static_cast<size_t> (liveGrainFrames) * 2u]);
        liveGrainWindow.reset (new (std::nothrow) float[static_cast<size_t> (liveGrainFrames)]);
        if (liveGrainWindow)
            for (int frame = 0; frame < liveGrainFrames; ++frame)
            {
                const auto phase = static_cast<double> (frame)
                                 / static_cast<double> (juce::jmax (1, liveGrainFrames - 1));
                liveGrainWindow[static_cast<size_t> (frame)]
                    = static_cast<float> (0.5 - 0.5 * std::cos (2.0 * kPi * phase));
            }
        liveReentryTailFrames = liveTransitionFrames;
        for (auto& tail : liveReentryTailBuffers)
            tail.reset (new (std::nothrow) float[static_cast<size_t> (liveReentryTailFrames) * 2u]);
        livePitchFft = std::make_unique<juce::dsp::FFT> (10);
        livePitchFftWork.reset (new (std::nothrow) float[static_cast<size_t> (kLivePitchFftSize) * 2u]);
        livePitchFftWork2.reset (new (std::nothrow) float[static_cast<size_t> (kLivePitchFftSize) * 2u]);
        livePitchInputRing.reset (new (std::nothrow) float[static_cast<size_t> (kLivePitchFftSize) * 2u]);
        livePitchIdentityDelay.reset (new (std::nothrow) float[static_cast<size_t> (kLivePipelineLatency) * 2u]);
        livePitchSemitoneDelay.reset (new (std::nothrow) float[static_cast<size_t> (kLivePipelineLatency)]);
        livePitchMaskDelay.reset (new (std::nothrow) float[static_cast<size_t> (kLivePipelineLatency)]);
        livePitchOlaLeft.reset (new (std::nothrow) float[static_cast<size_t> (kLivePitchOlaCapacity)]);
        livePitchOlaRight.reset (new (std::nothrow) float[static_cast<size_t> (kLivePitchOlaCapacity)]);
        livePitchOlaNorm.reset (new (std::nothrow) float[static_cast<size_t> (kLivePitchOlaCapacity)]);
        livePitchPreviousPhase.reset (new (std::nothrow) double[static_cast<size_t> (kLivePitchBins)]);
        livePitchSynthesisPhase.reset (new (std::nothrow) double[static_cast<size_t> (kLivePitchBins)]);
        livePitchPhaseInitialized.reset (new (std::nothrow) uint8_t[static_cast<size_t> (kLivePitchBins)]);
        livePitchSmoothingCoefficient = static_cast<float> (1.0 - std::exp (-1.0 / (rate * 0.005)));
        if ((! liveOnly && (! scratch || ! fftWork || ! fftWork2 || ! overlapNorm || ! previousInputPhase
            || ! synthesisPhase || ! phaseInitialized || ! fft)) || ! liveHistory || ! liveHistorySerials
            || ! liveHoldSnapshot || ! liveGrainWindow || ! liveReentryTailBuffers[0]
            || ! liveReentryTailBuffers[1] || ! livePitchFft || ! livePitchFftWork
            || ! livePitchFftWork2 || ! livePitchInputRing || ! livePitchIdentityDelay
            || ! livePitchSemitoneDelay || ! livePitchMaskDelay || ! livePitchOlaLeft
            || ! livePitchOlaRight || ! livePitchOlaNorm || ! livePitchPreviousPhase
            || ! livePitchSynthesisPhase || ! livePitchPhaseInitialized)
        {
            for (int i = 0; i < kBarCount; ++i) slots[static_cast<size_t> (i)].pcm.reset();
            scratch.reset(); fftWork.reset(); fftWork2.reset(); overlapNorm.reset();
            previousInputPhase.reset(); synthesisPhase.reset(); phaseInitialized.reset();
            liveHistory.reset(); liveHistorySerials.reset(); liveHoldSnapshot.reset();
            liveGrainWindow.reset();
            for (auto& tail : liveReentryTailBuffers) tail.reset();
            livePitchFft.reset(); livePitchFftWork.reset(); livePitchFftWork2.reset();
            livePitchInputRing.reset(); livePitchIdentityDelay.reset(); livePitchSemitoneDelay.reset();
            livePitchMaskDelay.reset(); livePitchOlaLeft.reset(); livePitchOlaRight.reset(); livePitchOlaNorm.reset();
            livePitchPreviousPhase.reset(); livePitchSynthesisPhase.reset(); livePitchPhaseInitialized.reset();
            maxFramesPerBar = 0;
            return false;
        }
        std::fill_n (liveHistory.get(), static_cast<size_t> (liveHistoryCapacityFrames) * 2u, 0.0f);
        std::fill_n (liveHistorySerials.get(), static_cast<size_t> (liveHistoryCapacityFrames),
                     std::numeric_limits<uint64_t>::max());
        clearLivePitchState();
        if (! liveOnly)
        {
            workerExit.store (false, std::memory_order_release);
            worker = std::thread ([this] { workerLoop(); });
        }
    }
    catch (...)
    {
        stopWorker();
        for (int i = 0; i < kBarCount; ++i) slots[static_cast<size_t> (i)].pcm.reset();
        scratch.reset(); fftWork.reset(); fftWork2.reset(); overlapNorm.reset();
        previousInputPhase.reset(); synthesisPhase.reset(); phaseInitialized.reset(); fft.reset();
        liveHistory.reset(); liveHistorySerials.reset(); liveHoldSnapshot.reset();
        liveGrainWindow.reset();
        for (auto& tail : liveReentryTailBuffers) tail.reset();
        livePitchFft.reset(); livePitchFftWork.reset(); livePitchFftWork2.reset();
        livePitchInputRing.reset(); livePitchIdentityDelay.reset(); livePitchSemitoneDelay.reset();
        livePitchMaskDelay.reset(); livePitchOlaLeft.reset(); livePitchOlaRight.reset(); livePitchOlaNorm.reset();
        livePitchPreviousPhase.reset(); livePitchSynthesisPhase.reset(); livePitchPhaseInitialized.reset();
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
    liveHistory.reset(); liveHistorySerials.reset(); liveHoldSnapshot.reset();
    liveGrainWindow.reset();
    for (auto& tail : liveReentryTailBuffers) tail.reset();
    livePitchFft.reset(); livePitchFftWork.reset(); livePitchFftWork2.reset();
    livePitchInputRing.reset(); livePitchIdentityDelay.reset(); livePitchSemitoneDelay.reset();
    livePitchMaskDelay.reset(); livePitchOlaLeft.reset(); livePitchOlaRight.reset(); livePitchOlaNorm.reset();
    livePitchPreviousPhase.reset(); livePitchSynthesisPhase.reset(); livePitchPhaseInitialized.reset();
    maxFramesPerBar = maxBlock = 0;
    sampleRate = 0.0;
    activeBar = -1;
    liveHistoryCapacityFrames = liveWarmupFrames = liveGrainFrames = liveHopFrames = 0;
    liveTransitionFrames = 0;
    liveReentryTailFrames = liveReentryTailPosition = liveReentryTailBuffer = 0;
    liveReentryTailActive = false;
    liveWriteSerial = liveHistoryValidFrames = 0;
    liveSamplesToLaunch = 0;
    liveSilenceFramesCount = 0;
    liveGrains.fill (LiveGrain {});
    liveStationaryFrames = liveHoldPhase = 0;
    liveHoldMix = 0.0f;
    liveHoldReady = false;
    liveActiveBar = -1;
    liveActiveRevision = 0;
    liveActiveTrajectory = {};
    livePreviousX = 0.5f;
    liveWetGain = 0.0f;
    liveDirection = 1;
    liveHavePreviousX = liveHaveSignal = livePendingTransition = liveWasPlaying = false;
    liveSourceFadeActive = false;
    livePitchSerial = 0;
    liveSmoothedSemitones = livePitchWetMix = 0.0f;
    livePitchSmoothingCoefficient = 0.0f;
    renderQueueHead.store (0, std::memory_order_relaxed);
    renderQueueTail.store (0, std::memory_order_relaxed);
}

void XYPadEngine::clearLivePitchState() noexcept
{
    if (livePitchInputRing) std::fill_n (livePitchInputRing.get(), static_cast<size_t> (kLivePitchFftSize) * 2u, 0.0f);
    if (livePitchIdentityDelay) std::fill_n (livePitchIdentityDelay.get(), static_cast<size_t> (kLivePipelineLatency) * 2u, 0.0f);
    if (livePitchSemitoneDelay) std::fill_n (livePitchSemitoneDelay.get(), static_cast<size_t> (kLivePipelineLatency), 0.0f);
    if (livePitchMaskDelay) std::fill_n (livePitchMaskDelay.get(), static_cast<size_t> (kLivePipelineLatency), 0.0f);
    if (livePitchOlaLeft) std::fill_n (livePitchOlaLeft.get(), static_cast<size_t> (kLivePitchOlaCapacity), 0.0f);
    if (livePitchOlaRight) std::fill_n (livePitchOlaRight.get(), static_cast<size_t> (kLivePitchOlaCapacity), 0.0f);
    if (livePitchOlaNorm) std::fill_n (livePitchOlaNorm.get(), static_cast<size_t> (kLivePitchOlaCapacity), 0.0f);
    if (livePitchPreviousPhase) std::fill_n (livePitchPreviousPhase.get(), static_cast<size_t> (kLivePitchBins), 0.0);
    if (livePitchSynthesisPhase) std::fill_n (livePitchSynthesisPhase.get(), static_cast<size_t> (kLivePitchBins), 0.0);
    if (livePitchPhaseInitialized) std::fill_n (livePitchPhaseInitialized.get(), static_cast<size_t> (kLivePitchBins), uint8_t (0));
    livePitchSerial = 0;
    liveSmoothedSemitones = livePitchWetMix = 0.0f;
}

void XYPadEngine::processLivePitchSample (float inputLeft, float inputRight, float semitones,
                                          float wetMask, float& outputLeft,
                                          float& outputRight, float& outputMask) noexcept
{
    if (! livePitchFft || ! livePitchFftWork || ! livePitchFftWork2 || ! livePitchInputRing
        || ! livePitchIdentityDelay || ! livePitchSemitoneDelay || ! livePitchMaskDelay
        || ! livePitchOlaLeft || ! livePitchOlaRight || ! livePitchOlaNorm
        || ! livePitchPreviousPhase || ! livePitchSynthesisPhase || ! livePitchPhaseInitialized)
    {
        outputLeft = outputRight = outputMask = 0.0f;
        return;
    }

    const auto serial = livePitchSerial;
    const auto delayIndex = static_cast<size_t> (serial % static_cast<uint64_t> (kLivePipelineLatency));
    const auto delayedLeft = livePitchIdentityDelay[delayIndex * 2u];
    const auto delayedRight = livePitchIdentityDelay[delayIndex * 2u + 1u];
    const auto delayedSemitones = livePitchSemitoneDelay[delayIndex];
    const auto delayedMask = livePitchMaskDelay[delayIndex];
    livePitchIdentityDelay[delayIndex * 2u] = std::isfinite (inputLeft) ? inputLeft : 0.0f;
    livePitchIdentityDelay[delayIndex * 2u + 1u] = std::isfinite (inputRight) ? inputRight : 0.0f;
    livePitchSemitoneDelay[delayIndex] = std::isfinite (semitones)
        ? juce::jlimit (-12.0f, 12.0f, semitones) : 0.0f;
    livePitchMaskDelay[delayIndex] = std::isfinite (wetMask)
        ? juce::jlimit (0.0f, 1.0f, wetMask) : 0.0f;

    const auto inputIndex = static_cast<size_t> (serial % static_cast<uint64_t> (kLivePitchFftSize));
    livePitchInputRing[inputIndex * 2u] = std::isfinite (inputLeft) ? inputLeft : 0.0f;
    livePitchInputRing[inputIndex * 2u + 1u] = std::isfinite (inputRight) ? inputRight : 0.0f;

    if ((serial + 1u) >= static_cast<uint64_t> (kLivePitchFftSize)
        && ((serial + 1u) % static_cast<uint64_t> (kLivePitchHopSize)) == 0u)
    {
        const auto frameStart = serial + 1u - static_cast<uint64_t> (kLivePitchFftSize);
        const auto pitchControlSerial = frameStart + static_cast<uint64_t> (kLivePitchFftSize / 2);
        const auto ratioDelayIndex = static_cast<size_t> (pitchControlSerial
            % static_cast<uint64_t> (kLivePipelineLatency));
        const auto frameSemitones = juce::jlimit (-12.0, 12.0,
            static_cast<double> (livePitchSemitoneDelay[ratioDelayIndex]));
        const auto ratio = std::pow (2.0, frameSemitones / 12.0);
        std::array<double, static_cast<size_t> (kLivePitchBins)> leftPhase {};
        const auto readBin = [] (const float* data, int bin, bool imaginary) noexcept
        {
            if (bin == 0) return imaginary ? 0.0f : data[0];
            if (bin == kLivePitchFftSize / 2) return imaginary ? 0.0f : data[1];
            return data[2 * bin + (imaginary ? 1 : 0)];
        };
        const auto writeBin = [] (float* data, int bin, float real, float imaginary) noexcept
        {
            if (bin == 0) { data[0] = real; return; }
            if (bin == kLivePitchFftSize / 2) { data[1] = real; return; }
            data[2 * bin] = real;
            data[2 * bin + 1] = imaginary;
        };

        for (int channel = 0; channel < 2; ++channel)
        {
            auto* work = livePitchFftWork.get();
            std::fill_n (work, static_cast<size_t> (kLivePitchFftSize) * 2u, 0.0f);
            for (int frame = 0; frame < kLivePitchFftSize; ++frame)
            {
                const auto sourceSerial = frameStart + static_cast<uint64_t> (frame);
                const auto sourceIndex = static_cast<size_t> (sourceSerial
                    % static_cast<uint64_t> (kLivePitchFftSize));
                const auto hann = 0.5 - 0.5 * std::cos (2.0 * kPi * frame / (kLivePitchFftSize - 1));
                work[static_cast<size_t> (frame)] = livePitchInputRing[sourceIndex * 2u
                    + static_cast<size_t> (channel)] * static_cast<float> (hann);
            }
            livePitchFft->performRealOnlyForwardTransform (work);
            auto* transformed = livePitchFftWork2.get();
            std::fill_n (transformed, static_cast<size_t> (kLivePitchFftSize) * 2u, 0.0f);

            if (channel == 0)
            {
                for (int bin = 0; bin < kLivePitchBins; ++bin)
                {
                    const auto sourceBin = static_cast<double> (bin) / ratio;
                    if (sourceBin > kLivePitchFftSize / 2.0) { leftPhase[static_cast<size_t> (bin)] = 0.0; continue; }
                    const auto low = static_cast<int> (sourceBin);
                    const auto high = juce::jmin (kLivePitchFftSize / 2, low + 1);
                    const auto fraction = static_cast<float> (sourceBin - low);
                    const auto real = readBin (work, low, false)
                        + (readBin (work, high, false) - readBin (work, low, false)) * fraction;
                    const auto imag = readBin (work, low, true)
                        + (readBin (work, high, true) - readBin (work, low, true)) * fraction;
                    const auto magnitude = std::hypot (real, imag);
                    const auto phase = std::atan2 (imag, real);
                    leftPhase[static_cast<size_t> (bin)] = phase;
                    const auto index = static_cast<size_t> (bin);
                    auto& previous = livePitchPreviousPhase[index];
                    auto& synthesis = livePitchSynthesisPhase[index];
                    if (bin == 0 || bin == kLivePitchFftSize / 2)
                        synthesis = phase;
                    else if (livePitchPhaseInitialized[index] == 0)
                    {
                        synthesis = phase;
                        livePitchPhaseInitialized[index] = 1;
                    }
                    else
                    {
                        const auto expected = 2.0 * kPi * sourceBin * kLivePitchHopSize / kLivePitchFftSize;
                        const auto residual = std::remainder (phase - previous - expected, 2.0 * kPi);
                        const auto instantaneousBin = sourceBin + residual * kLivePitchFftSize
                            / (2.0 * kPi * kLivePitchHopSize);
                        synthesis += 2.0 * kPi * instantaneousBin * ratio * kLivePitchHopSize
                            / kLivePitchFftSize;
                    }
                    previous = phase;
                    writeBin (transformed, bin,
                        magnitude * static_cast<float> (std::cos (synthesis)),
                        magnitude * static_cast<float> (std::sin (synthesis)));
                }
            }
            else
            {
                for (int bin = 0; bin < kLivePitchBins; ++bin)
                {
                    const auto sourceBin = static_cast<double> (bin) / ratio;
                    if (sourceBin > kLivePitchFftSize / 2.0) continue;
                    const auto low = static_cast<int> (sourceBin);
                    const auto high = juce::jmin (kLivePitchFftSize / 2, low + 1);
                    const auto fraction = static_cast<float> (sourceBin - low);
                    const auto real = readBin (work, low, false)
                        + (readBin (work, high, false) - readBin (work, low, false)) * fraction;
                    const auto imag = readBin (work, low, true)
                        + (readBin (work, high, true) - readBin (work, low, true)) * fraction;
                    const auto phase = std::atan2 (imag, real);
                    const auto phaseOffset = std::remainder (phase - leftPhase[static_cast<size_t> (bin)], 2.0 * kPi);
                    const auto outputPhase = livePitchSynthesisPhase[static_cast<size_t> (bin)] + phaseOffset;
                    const auto magnitude = std::hypot (real, imag);
                    writeBin (transformed, bin,
                        magnitude * static_cast<float> (std::cos (outputPhase)),
                        magnitude * static_cast<float> (std::sin (outputPhase)));
                }
            }
            livePitchFft->performRealOnlyInverseTransform (transformed);
            auto* ola = channel == 0 ? livePitchOlaLeft.get() : livePitchOlaRight.get();
            for (int frame = 0; frame < kLivePitchFftSize; ++frame)
            {
                const auto hann = static_cast<float> (0.5 - 0.5 * std::cos (2.0 * kPi * frame
                    / (kLivePitchFftSize - 1)));
                const auto targetSerial = frameStart + static_cast<uint64_t> (kLivePipelineLatency + frame);
                const auto targetIndex = static_cast<size_t> (targetSerial
                    % static_cast<uint64_t> (kLivePitchOlaCapacity));
                ola[targetIndex] += transformed[static_cast<size_t> (frame)] * hann;
                if (channel == 0) livePitchOlaNorm[targetIndex] += hann * hann;
            }
        }
    }

    const auto olaIndex = static_cast<size_t> (serial % static_cast<uint64_t> (kLivePitchOlaCapacity));
    const auto norm = livePitchOlaNorm[olaIndex];
    const auto pitchedLeft = norm > 1.0e-8f ? livePitchOlaLeft[olaIndex] / norm : delayedLeft;
    const auto pitchedRight = norm > 1.0e-8f ? livePitchOlaRight[olaIndex] / norm : delayedRight;
    livePitchOlaLeft[olaIndex] = livePitchOlaRight[olaIndex] = livePitchOlaNorm[olaIndex] = 0.0f;

    const auto pitchTarget = std::abs (delayedSemitones) >= 0.01f ? 1.0f : 0.0f;
    const auto pitchMixStep = 1.0f / static_cast<float> (juce::jmax (1, juce::roundToInt (sampleRate * 0.005)));
    if (livePitchWetMix < pitchTarget) livePitchWetMix = juce::jmin (pitchTarget, livePitchWetMix + pitchMixStep);
    else if (livePitchWetMix > pitchTarget) livePitchWetMix = juce::jmax (pitchTarget, livePitchWetMix - pitchMixStep);
    outputLeft = delayedLeft + (pitchedLeft - delayedLeft) * livePitchWetMix;
    outputRight = delayedRight + (pitchedRight - delayedRight) * livePitchWetMix;
    outputMask = delayedMask;
    ++livePitchSerial;
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
                                         float& x, float& y, double* xSlopePerPhase) noexcept
{
    if (xSlopePerPhase != nullptr) *xSlopePerPhase = 0.0;
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
    if (xSlopePerPhase != nullptr && t1 > t0 && phase > 0.0 && phase < 1.0)
        *xSlopePerPhase = (static_cast<double> (trajectory.x[static_cast<size_t> (hi)])
                         - static_cast<double> (trajectory.x[static_cast<size_t> (lo)]))
                        / (t1 - t0) * span;
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

bool XYPadEngine::readLiveSample (double serial, int channel, uint64_t oldestSerial,
                                  uint64_t newestSerial, float& value) const noexcept
{
    if (liveHistory == nullptr || liveHistorySerials == nullptr
        || liveHistoryCapacityFrames <= 1 || channel < 0 || channel > 1
        || ! std::isfinite (serial) || serial < static_cast<double> (oldestSerial)
        || serial > static_cast<double> (newestSerial))
        return false;

    const auto lower = static_cast<uint64_t> (std::floor (serial));
    const auto upper = juce::jmin (newestSerial, lower + 1u);
    const auto lowerIndex = static_cast<size_t> (lower % static_cast<uint64_t> (liveHistoryCapacityFrames));
    const auto upperIndex = static_cast<size_t> (upper % static_cast<uint64_t> (liveHistoryCapacityFrames));
    if (liveHistorySerials[lowerIndex] != lower || liveHistorySerials[upperIndex] != upper)
        return false;
    const auto fraction = static_cast<float> (serial - static_cast<double> (lower));
    const auto a = liveHistory[lowerIndex * 2u + static_cast<size_t> (channel)];
    const auto b = liveHistory[upperIndex * 2u + static_cast<size_t> (channel)];
    value = a + (b - a) * fraction;
    return std::isfinite (value);
}

bool XYPadEngine::readLiveStereoSample (double serial, uint64_t oldestSerial,
                                        uint64_t newestSerial, float& left, float& right) const noexcept
{
    if (liveHistory == nullptr || liveHistorySerials == nullptr || liveHistoryCapacityFrames <= 1
        || ! std::isfinite (serial) || serial < static_cast<double> (oldestSerial)
        || serial > static_cast<double> (newestSerial))
        return false;

    const auto lower = static_cast<uint64_t> (std::floor (serial));
    const auto upper = juce::jmin (newestSerial, lower + 1u);
    const auto capacity = static_cast<uint64_t> (liveHistoryCapacityFrames);
    const auto lowerIndex = static_cast<size_t> (lower % capacity);
    const auto upperIndex = static_cast<size_t> (upper % capacity);
    if (liveHistorySerials[lowerIndex] != lower || liveHistorySerials[upperIndex] != upper)
        return false;

    const auto fraction = static_cast<float> (serial - static_cast<double> (lower));
    const auto lowerOffset = lowerIndex * 2u;
    const auto upperOffset = upperIndex * 2u;
    left = liveHistory[lowerOffset] + (liveHistory[upperOffset] - liveHistory[lowerOffset]) * fraction;
    right = liveHistory[lowerOffset + 1u]
          + (liveHistory[upperOffset + 1u] - liveHistory[lowerOffset + 1u]) * fraction;
    return std::isfinite (left) && std::isfinite (right);
}

void XYPadEngine::launchLiveGrain (float x, int direction, bool transition,
                                   uint64_t oldestSerial, uint64_t newestSerial) noexcept
{
    if (liveGrainFrames <= 1 || liveGrainFrames > liveHistoryCapacityFrames
        || newestSerial < oldestSerial)
        return;
    const auto retainedFrames = newestSerial - oldestSerial + 1u;
    const auto selectionFrames = juce::jmin (retainedFrames,
        static_cast<uint64_t> (juce::jmax (1, juce::roundToInt (sampleRate * kLiveHistorySeconds) + 2)));
    if (selectionFrames < static_cast<uint64_t> (liveGrainFrames))
        return;

    const auto selectionOldestSerial = newestSerial + 1u - selectionFrames;
    const auto span = static_cast<double> (selectionFrames - static_cast<uint64_t> (liveGrainFrames));
    const auto offset = static_cast<double> (juce::jlimit (0.0f, 1.0f, x)) * span;
    const auto start = direction >= 0
        ? static_cast<double> (selectionOldestSerial) + offset
        : static_cast<double> (selectionOldestSerial) + offset + static_cast<double> (liveGrainFrames - 1);

    LiveGrain* destination = nullptr;
    for (auto& grain : liveGrains)
        if (! grain.active) { destination = &grain; break; }
    if (destination == nullptr)
    {
        destination = &liveGrains[0];
        for (auto& grain : liveGrains)
            if (grain.age > destination->age) destination = &grain;
    }
    destination->startSerial = start;
    destination->direction = direction >= 0 ? 1 : -1;
    destination->age = 0;
    destination->fadeInFrames = transition ? liveTransitionFrames : 0;
    destination->fadeOutRemaining = 0;
    destination->active = true;
    if (transition)
        for (auto& grain : liveGrains)
            if (grain.active && &grain != destination)
                grain.fadeOutRemaining = liveTransitionFrames;
}

void XYPadEngine::captureLiveContinuationTail() noexcept
{
    if (liveReentryTailFrames < 2 || ! liveReentryTailBuffers[0]
        || ! liveReentryTailBuffers[1] || liveHistory == nullptr || liveHistorySerials == nullptr
        || liveHistoryValidFrames == 0 || liveWriteSerial == 0)
        return;

    const auto destinationIndex = 1 - liveReentryTailBuffer;
    auto* destination = liveReentryTailBuffers[static_cast<size_t> (destinationIndex)].get();
    const auto* previousTail = liveReentryTailActive
        ? liveReentryTailBuffers[static_cast<size_t> (liveReentryTailBuffer)].get() : nullptr;
    const auto newestSerial = liveWriteSerial - 1u;
    const auto oldestSerial = newestSerial + 1u - liveHistoryValidFrames;
    const auto tailBlendScale = 1.0f / static_cast<float> (juce::jmax (1, liveReentryTailFrames - 1));
    int onlyActiveGrain = -1;
    int activeCount = 0;
    for (int index = 0; index < kLiveGrainCapacity; ++index)
        if (liveGrains[static_cast<size_t> (index)].active)
        {
            onlyActiveGrain = index;
            ++activeCount;
        }
    if (activeCount == 1)
    {
        const auto& grain = liveGrains[static_cast<size_t> (onlyActiveGrain)];
        if (grain.fadeInFrames <= 0 && grain.fadeOutRemaining <= 0 && ! liveHoldReady)
        {
            bool referenceValid = true;
            const auto capacity = static_cast<uint64_t> (liveHistoryCapacityFrames);
            const auto firstSerial = grain.startSerial + static_cast<double> (grain.direction * grain.age);
            const auto firstPositionValid = std::isfinite (firstSerial)
                && firstSerial >= static_cast<double> (oldestSerial)
                && firstSerial <= static_cast<double> (newestSerial);
            const auto lowerSerial = std::isfinite (firstSerial) && firstSerial >= 0.0
                ? static_cast<uint64_t> (std::floor (firstSerial)) : uint64_t (0);
            auto lowerIndex = static_cast<size_t> (lowerSerial % capacity);
            const auto fraction = std::isfinite (firstSerial)
                ? static_cast<float> (firstSerial - static_cast<double> (lowerSerial)) : 0.0f;
            const auto firstReadAge = juce::jmax (grain.age, 1);
            const auto lastReadAge = juce::jmin (grain.age + liveReentryTailFrames - 1,
                                                  liveGrainFrames - 2);
            const auto firstReadSerial = grain.startSerial
                + static_cast<double> (grain.direction * firstReadAge);
            const auto lastReadSerial = grain.startSerial
                + static_cast<double> (grain.direction * lastReadAge);
            const auto firstReadLower = std::isfinite (firstReadSerial) && firstReadSerial >= 0.0
                ? static_cast<uint64_t> (std::floor (firstReadSerial)) : uint64_t (0);
            const auto lastReadLower = std::isfinite (lastReadSerial) && lastReadSerial >= 0.0
                ? static_cast<uint64_t> (std::floor (lastReadSerial)) : uint64_t (0);
            const auto minReadLower = juce::jmin (firstReadLower, lastReadLower);
            const auto maxReadLower = juce::jmax (firstReadLower, lastReadLower);
            const auto rangeValid = firstReadAge <= lastReadAge
                && std::isfinite (firstReadSerial) && std::isfinite (lastReadSerial)
                && firstReadSerial >= static_cast<double> (oldestSerial)
                && firstReadSerial <= static_cast<double> (newestSerial)
                && lastReadSerial >= static_cast<double> (oldestSerial)
                && lastReadSerial <= static_cast<double> (newestSerial)
                && minReadLower >= oldestSerial && maxReadLower <= newestSerial
                && (fraction <= 0.0f || maxReadLower < newestSerial);
            const auto ringSerialMatches = [&] (uint64_t serial) noexcept
            {
                const auto index = static_cast<size_t> (serial % capacity);
                return liveHistorySerials[index] == serial;
            };
            const auto endpointsValid = rangeValid
                && ringSerialMatches (minReadLower) && ringSerialMatches (maxReadLower)
                && (fraction <= 0.0f
                    || (ringSerialMatches (minReadLower + 1u)
                        && ringSerialMatches (maxReadLower + 1u)));
            referenceValid = firstPositionValid && endpointsValid;
#if defined(TOYOTOMI_XY_HAS_SSE2)
            const auto tailFrames = static_cast<size_t> (liveReentryTailFrames);
            const auto noRingWrapForTail = grain.direction >= 0
                ? lowerIndex + tailFrames < capacity && upperIndex + tailFrames < capacity
                : lowerIndex >= tailFrames && upperIndex >= tailFrames;
            const auto blendableFrameCount = previousTail == nullptr ? tailFrames
                : static_cast<size_t> (juce::jmax (0, liveReentryTailFrames - liveReentryTailPosition - 1));
#endif
            if (referenceValid)
            {
                auto upperIndex = lowerIndex;
                if (fraction > 0.0f)
                {
                    ++upperIndex;
                    if (upperIndex == capacity) upperIndex = 0;
                }
                for (int frame = 0; frame < liveReentryTailFrames; ++frame)
                {
                const auto age = grain.age + frame;
                const auto validHannFrame = age > 0 && age < liveGrainFrames - 1;
#if defined(TOYOTOMI_XY_HAS_SSE2)
                const auto previousFrame = liveReentryTailPosition + frame;
                if (validHannFrame && age + 1 < liveGrainFrames - 1
                    && static_cast<size_t> (frame + 1) < blendableFrameCount
                    && noRingWrapForTail)
                {
                    const auto* history = liveHistory.get();
                    const auto loadPair = [history, &grain] (size_t index) noexcept
                    {
                        if (grain.direction >= 0)
                            return _mm_loadu_ps (history + index * 2u);
                        const auto reversed = _mm_loadu_ps (history + (index - 1u) * 2u);
                        return _mm_shuffle_ps (reversed, reversed, _MM_SHUFFLE (1, 0, 3, 2));
                    };
                    auto target = loadPair (lowerIndex);
                    const auto upper = loadPair (upperIndex);
                    const auto frac = _mm_set1_ps (fraction);
                    target = _mm_add_ps (target, _mm_mul_ps (_mm_sub_ps (upper, target), frac));
                    if (previousTail != nullptr)
                    {
                        const auto q0 = static_cast<float> (previousFrame) * tailBlendScale;
                        const auto q1 = static_cast<float> (previousFrame + 1) * tailBlendScale;
                        const auto q = _mm_setr_ps (q0, q0, q1, q1);
                        const auto old = _mm_loadu_ps (previousTail
                            + static_cast<size_t> (previousFrame) * 2u);
                        target = _mm_add_ps (_mm_mul_ps (old, _mm_sub_ps (_mm_set1_ps (1.0f), q)),
                                             _mm_mul_ps (target, q));
                    }
                    const auto magnitude = _mm_andnot_ps (_mm_set1_ps (-0.0f), target);
                    const auto finite = _mm_cmple_ps (magnitude,
                        _mm_set1_ps (std::numeric_limits<float>::max()));
                    target = _mm_and_ps (target, finite);
                    _mm_storeu_ps (destination + static_cast<size_t> (frame) * 2u, target);
                    const auto advance = [&] (size_t& index) noexcept
                    {
                        if (grain.direction >= 0)
                        {
                            if (++index == capacity) index = 0;
                        }
                        else
                            --index;
                    };
                    advance (lowerIndex);
                    advance (upperIndex);
                    advance (lowerIndex);
                    advance (upperIndex);
                    ++frame;
                    continue;
                }
#endif
                float left = 0.0f, right = 0.0f;
                if (validHannFrame)
                {
                    const auto lowerOffset = lowerIndex * 2u;
                    const auto upperOffset = upperIndex * 2u;
                    left = liveHistory[lowerOffset]
                         + (liveHistory[upperOffset] - liveHistory[lowerOffset]) * fraction;
                    right = liveHistory[lowerOffset + 1u]
                          + (liveHistory[upperOffset + 1u] - liveHistory[lowerOffset + 1u]) * fraction;
                    referenceValid = std::isfinite (left) && std::isfinite (right);
                    if (! referenceValid) break;
                }
                if (previousTail != nullptr)
                {
                    const auto previousFrame = liveReentryTailPosition + frame;
                    if (previousFrame < liveReentryTailFrames)
                    {
                        const auto q = static_cast<float> (previousFrame) * tailBlendScale;
                        const auto index = static_cast<size_t> (previousFrame) * 2u;
                        left = previousTail[index] * (1.0f - q) + left * q;
                        right = previousTail[index + 1u] * (1.0f - q) + right * q;
                    }
                }
                destination[static_cast<size_t> (frame) * 2u] = std::isfinite (left) ? left : 0.0f;
                destination[static_cast<size_t> (frame) * 2u + 1u] = std::isfinite (right) ? right : 0.0f;
                if (frame + 1 < liveReentryTailFrames && age + 1 > 0 && age + 1 < liveGrainFrames - 1)
                {
                    if (grain.direction >= 0)
                    {
                        if (++lowerIndex == capacity) lowerIndex = 0;
                        if (fraction > 0.0f && ++upperIndex == capacity) upperIndex = 0;
                    }
                    else
                    {
                        lowerIndex = lowerIndex == 0 ? static_cast<size_t> (capacity - 1u) : lowerIndex - 1u;
                        if (fraction > 0.0f)
                            upperIndex = upperIndex == 0 ? static_cast<size_t> (capacity - 1u) : upperIndex - 1u;
                    }
                }
                }
            }
            if (referenceValid)
            {
                liveReentryTailBuffer = destinationIndex;
                liveReentryTailPosition = 0;
                liveReentryTailActive = true;
                return;
            }
        }
    }
    auto simulatedGrains = liveGrains;
    std::array<int, kLiveGrainCapacity> activeGrainIndices {};
    int activeGrainCount = 0;
    for (int index = 0; index < kLiveGrainCapacity; ++index)
        if (simulatedGrains[static_cast<size_t> (index)].active)
            activeGrainIndices[static_cast<size_t> (activeGrainCount++)] = index;
    auto simulatedHoldMix = liveHoldMix;
    auto simulatedHoldPhase = liveHoldPhase;
    auto simulatedHoldReady = liveHoldReady;
    const auto stationaryFramesRequired = juce::jmax (1, static_cast<int> (std::ceil (sampleRate * 0.002)));
    const auto holdTarget = simulatedHoldReady && liveStationaryFrames >= stationaryFramesRequired ? 1.0f : 0.0f;
    const auto holdStep = 1.0f / static_cast<float> (juce::jmax (1, liveTransitionFrames));

    for (int frame = 0; frame < liveReentryTailFrames; ++frame)
    {
        double oldLeft = 0.0, oldRight = 0.0, oldWeight = 0.0;
        double newLeft = 0.0, newRight = 0.0, newWeight = 0.0;
        double stableLeft = 0.0, stableRight = 0.0, stableWeight = 0.0;
        double phase = 0.0;
        bool hasOld = false, hasNew = false;
        for (int active = 0; active < activeGrainCount; ++active)
        {
            auto& grain = simulatedGrains[static_cast<size_t> (activeGrainIndices[static_cast<size_t> (active)])];
            if (! grain.active) continue;
            if (grain.age >= liveGrainFrames)
            {
                grain.active = false;
                continue;
            }
            const auto serial = grain.startSerial + static_cast<double> (grain.direction * grain.age);
            float grainLeft = 0.0f, grainRight = 0.0f;
            if (! readLiveStereoSample (serial, oldestSerial, newestSerial, grainLeft, grainRight))
            {
                grain.active = false;
                continue;
            }
            const auto window = liveGrainWindow[static_cast<size_t> (grain.age)];
            if (grain.fadeOutRemaining > 0 || grain.fadeInFrames > 0)
            {
                phase = grain.fadeOutRemaining > 0
                    ? static_cast<double> (liveTransitionFrames - grain.fadeOutRemaining)
                        / static_cast<double> (liveTransitionFrames - 1)
                    : static_cast<double> (grain.age)
                        / static_cast<double> (juce::jmax (1, grain.fadeInFrames - 1));
            }
            if (grain.fadeInFrames > 0)
            {
                newLeft += static_cast<double> (grainLeft) * window;
                newRight += static_cast<double> (grainRight) * window;
                newWeight += window;
                hasNew = true;
            }
            else if (grain.fadeOutRemaining > 0)
            {
                oldLeft += static_cast<double> (grainLeft) * window;
                oldRight += static_cast<double> (grainRight) * window;
                oldWeight += window;
                hasOld = true;
            }
            else
            {
                stableLeft += static_cast<double> (grainLeft) * window;
                stableRight += static_cast<double> (grainRight) * window;
                stableWeight += window;
            }
            if (grain.fadeOutRemaining > 0 && --grain.fadeOutRemaining == 0)
                grain.active = false;
            if (grain.fadeInFrames > 0 && grain.age + 1 >= grain.fadeInFrames)
                grain.fadeInFrames = 0;
            ++grain.age;
        }

        double wetLeft = stableLeft, wetRight = stableRight, totalWeight = stableWeight;
        if (hasOld || hasNew)
        {
            oldLeft += stableLeft; oldRight += stableRight; oldWeight += stableWeight;
            const auto aLeft = oldWeight > 1.0e-9 ? oldLeft / oldWeight : 0.0;
            const auto aRight = oldWeight > 1.0e-9 ? oldRight / oldWeight : 0.0;
            const auto bLeft = newWeight > 1.0e-9 ? newLeft / newWeight : 0.0;
            const auto bRight = newWeight > 1.0e-9 ? newRight / newWeight : 0.0;
            const auto angle = juce::jlimit (0.0, 1.0, phase) * kPi * 0.5;
            wetLeft = aLeft * std::cos (angle) + bLeft * std::sin (angle);
            wetRight = aRight * std::cos (angle) + bRight * std::sin (angle);
            totalWeight = 1.0;
        }

        if (simulatedHoldMix < holdTarget) simulatedHoldMix = juce::jmin (holdTarget, simulatedHoldMix + holdStep);
        else if (simulatedHoldMix > holdTarget) simulatedHoldMix = juce::jmax (holdTarget, simulatedHoldMix - holdStep);
        if (simulatedHoldReady && liveHoldSnapshot != nullptr && liveGrainFrames > 1)
        {
            const auto seamFrames = juce::jlimit (1, liveGrainFrames / 2,
                                                   juce::roundToInt (sampleRate * kHoldSeamSeconds));
            const auto seamStart = liveGrainFrames - seamFrames;
            const auto hp = juce::jlimit (0, liveGrainFrames - 1, simulatedHoldPhase);
            auto holdLeft = liveHoldSnapshot[static_cast<size_t> (hp) * 2u];
            auto holdRight = liveHoldSnapshot[static_cast<size_t> (hp) * 2u + 1u];
            if (hp >= seamStart)
            {
                const auto blend = static_cast<double> (hp - seamStart)
                    / static_cast<double> (juce::jmax (1, seamFrames - 1));
                const auto head = hp - seamStart;
                holdLeft = static_cast<float> (holdLeft * std::cos (blend * kPi * 0.5)
                    + liveHoldSnapshot[static_cast<size_t> (head) * 2u] * std::sin (blend * kPi * 0.5));
                holdRight = static_cast<float> (holdRight * std::cos (blend * kPi * 0.5)
                    + liveHoldSnapshot[static_cast<size_t> (head) * 2u + 1u] * std::sin (blend * kPi * 0.5));
            }
            const auto normalizedLeft = totalWeight > 1.0e-9 ? wetLeft / totalWeight : 0.0;
            const auto normalizedRight = totalWeight > 1.0e-9 ? wetRight / totalWeight : 0.0;
            wetLeft = normalizedLeft * std::cos (simulatedHoldMix * kPi * 0.5)
                    + static_cast<double> (holdLeft) * std::sin (simulatedHoldMix * kPi * 0.5);
            wetRight = normalizedRight * std::cos (simulatedHoldMix * kPi * 0.5)
                     + static_cast<double> (holdRight) * std::sin (simulatedHoldMix * kPi * 0.5);
            totalWeight = 1.0;
            ++simulatedHoldPhase;
            if (simulatedHoldPhase >= liveGrainFrames) simulatedHoldPhase = seamFrames;
            if (holdTarget == 0.0f && simulatedHoldMix == 0.0f) simulatedHoldReady = false;
        }

        auto left = totalWeight > 1.0e-9 ? static_cast<float> (wetLeft / totalWeight) : 0.0f;
        auto right = totalWeight > 1.0e-9 ? static_cast<float> (wetRight / totalWeight) : 0.0f;
        if (previousTail != nullptr)
        {
            const auto previousFrame = liveReentryTailPosition + frame;
            if (previousFrame < liveReentryTailFrames)
            {
                const auto q = static_cast<float> (previousFrame)
                            / static_cast<float> (liveReentryTailFrames - 1);
                const auto index = static_cast<size_t> (previousFrame) * 2u;
                left = blendLiveReentrySample (previousTail[index], left, q);
                right = blendLiveReentrySample (previousTail[index + 1u], right, q);
            }
        }
        destination[static_cast<size_t> (frame) * 2u] = std::isfinite (left) ? left : 0.0f;
        destination[static_cast<size_t> (frame) * 2u + 1u] = std::isfinite (right) ? right : 0.0f;
    }
    liveReentryTailBuffer = destinationIndex;
    liveReentryTailPosition = 0;
    liveReentryTailActive = true;
}

float XYPadEngine::blendLiveReentrySample (float tail, float target, float q) noexcept
{
    const auto amount = juce::jlimit (0.0f, 1.0f, q);
    return tail * (1.0f - amount) + target * amount;
}

void XYPadEngine::processLive (const juce::AudioBuffer<float>& source,
                               juce::AudioBuffer<float>& rendered,
                               juce::AudioBuffer<float>& readyMask,
                               const Transport& transport, int frameCount) noexcept
{
    const auto frames = juce::jlimit (0, source.getNumSamples(), frameCount);
    const auto outputChannels = juce::jmin (2, rendered.getNumChannels());
    for (int channel = 0; channel < rendered.getNumChannels(); ++channel)
        rendered.clear (channel, 0, juce::jmin (frames, rendered.getNumSamples()));
    if (readyMask.getNumChannels() > 0)
        readyMask.clear (0, 0, juce::jmin (frames, readyMask.getNumSamples()));

    if (! enabled.load (std::memory_order_acquire) || liveHistory == nullptr
        || liveHistorySerials == nullptr
        || livePitchInputRing == nullptr || livePitchIdentityDelay == nullptr
        || ! transport.playing || transport.startBar < 0
        || frames <= 0 || frames > maxBlock || rendered.getNumSamples() < frames
        || readyMask.getNumChannels() < 1 || readyMask.getNumSamples() < frames)
    {
        if (liveWasPlaying) clearLivePitchState();
        liveGrains.fill (LiveGrain {});
        liveHistoryValidFrames = 0;
        liveWriteSerial = 0;
        liveSamplesToLaunch = 0;
        liveSilenceFramesCount = 0;
        liveActiveBar = -1;
        liveActiveRevision = 0;
        liveActiveTrajectory = {};
        livePreviousX = 0.5f;
        liveDirection = 1;
        liveWetGain = 0.0f;
        liveHavePreviousX = false;
        liveHaveSignal = false;
        livePendingTransition = false;
        liveStationaryFrames = liveHoldPhase = 0;
        liveHoldMix = 0.0f;
        liveHoldReady = false;
        liveReentryTailActive = false;
        liveReentryTailPosition = 0;
        liveWasPlaying = false;
        liveSourceFadeActive = false;
        return;
    }

    liveWasPlaying = true;
    if (transport.resetHistory)
    {
        // A seek or fresh transport start invalidates the old audio timeline.
        // Keep the monotonic serial, but require new samples before any grain
        // can reference the ring again. Loop wraps deliberately retain it.
        liveGrains.fill (LiveGrain {});
        liveHistoryValidFrames = 0;
        liveSamplesToLaunch = 0;
        liveSilenceFramesCount = 0;
        liveActiveBar = -1;
        liveActiveRevision = 0;
        liveActiveTrajectory = {};
        livePreviousX = 0.5f;
        liveDirection = 1;
        liveWetGain = 0.0f;
        liveHavePreviousX = false;
        liveHaveSignal = false;
        livePendingTransition = false;
        liveStationaryFrames = liveHoldPhase = 0;
        liveHoldMix = 0.0f;
        liveHoldReady = false;
        liveReentryTailActive = false;
        liveReentryTailPosition = 0;
        liveSourceFadeActive = false;
        clearLivePitchState();
    }
    const auto sourceChannels = source.getNumChannels();
    const auto rampFrames = juce::jmax (1, juce::roundToInt (sampleRate * 0.008));
    const auto silenceFrames = juce::jmax (1, juce::roundToInt (sampleRate * 0.030));
    const auto* sourceLeft = sourceChannels > 0 ? source.getReadPointer (0) : nullptr;
    const auto* sourceRight = sourceChannels > 1 ? source.getReadPointer (1) : sourceLeft;
    auto* outputLeft = outputChannels > 0 ? rendered.getWritePointer (0) : nullptr;
    auto* outputRight = outputChannels > 1 ? rendered.getWritePointer (1) : nullptr;
    auto* mask = readyMask.getWritePointer (0);
    int silentFrames = liveSilenceFramesCount;

    for (int i = 0; i < frames; ++i)
    {
        const auto dryLeft = sourceLeft != nullptr && std::isfinite (sourceLeft[i]) ? sourceLeft[i] : 0.0f;
        const auto dryRight = sourceRight != nullptr && std::isfinite (sourceRight[i]) ? sourceRight[i] : dryLeft;
        const auto writeIndex = static_cast<size_t> (liveWriteSerial
            % static_cast<uint64_t> (liveHistoryCapacityFrames));
        liveHistory[writeIndex * 2u] = dryLeft;
        liveHistory[writeIndex * 2u + 1u] = dryRight;
        liveHistorySerials[writeIndex] = liveWriteSerial;
        ++liveWriteSerial;
        liveHistoryValidFrames = juce::jmin (static_cast<uint64_t> (liveHistoryCapacityFrames),
                                              liveHistoryValidFrames + 1u);
        const auto newestSerial = liveWriteSerial - 1u;
        const auto oldestSerial = newestSerial + 1u - liveHistoryValidFrames;

        const auto isSilent = std::abs (dryLeft) < 1.0e-4f && std::abs (dryRight) < 1.0e-4f;
        silentFrames = isSilent ? juce::jmin (silenceFrames, silentFrames + 1) : 0;
        if (! isSilent)
        {
            if (! liveHaveSignal)
            {
                // A new source segment starts a fresh serial-valid history. Keep
                // the ended segment readable through its fade, then invalidate it
                // here before any stale grain can be relaunched.
                liveHistoryValidFrames = 1;
                liveGrains.fill (LiveGrain {});
                liveSamplesToLaunch = 0;
                liveWetGain = 0.0f;
                liveReentryTailActive = false;
                liveReentryTailPosition = 0;
                liveHoldReady = false;
                liveHoldMix = 0.0f;
                liveStationaryFrames = liveHoldPhase = 0;
                liveSourceFadeActive = false;
            }
            liveHaveSignal = true;
        }
        liveSilenceFramesCount = silentFrames;
        const auto sourceEnded = silentFrames >= silenceFrames;
        if (sourceEnded)
        {
            liveSamplesToLaunch = 0;
            liveHaveSignal = false;
            liveSourceFadeActive = true;
        }

        int bar = -1;
        double phase = 0.0;
        sampleTimeline (transport, i, frames, bar, phase);
        if (bar != liveActiveBar)
        {
            liveActiveBar = bar;
            liveGrains.fill (LiveGrain {});
            liveSamplesToLaunch = 0;
            liveWetGain = 0.0f;
            liveHavePreviousX = false;
            livePendingTransition = false;
            liveStationaryFrames = liveHoldPhase = 0;
            liveHoldMix = 0.0f;
            liveHoldReady = false;
            liveReentryTailActive = false;
            liveReentryTailPosition = 0;
            Trajectory candidate;
            if (publishedTrajectories != nullptr
                && readTrajectory (publishedTrajectories[static_cast<size_t> (bar)], candidate))
                liveActiveTrajectory = candidate;
            else
                liveActiveTrajectory = {};
            liveActiveRevision = liveActiveTrajectory.revision;
        }

        float x = 0.5f, y = 0.5f;
        double xSlopePerPhase = 0.0;
        interpolateTrajectory (liveActiveTrajectory, phase, x, y, &xSlopePerPhase);
        const auto pitchTargetSemitones = liveActiveTrajectory.count > 0
            ? juce::jlimit (-12.0f, 12.0f, 24.0f * (y - 0.5f)) : 0.0f;
        liveSmoothedSemitones += (pitchTargetSemitones - liveSmoothedSemitones)
                               * livePitchSmoothingCoefficient;
        x = juce::jlimit (0.0f, 1.0f, x);
        const auto velocityX = std::isfinite (transport.phasePerSample)
                            && transport.phasePerSample > 0.0
            ? xSlopePerPhase * transport.phasePerSample * sampleRate : 0.0;
        auto direction = liveDirection;
        auto targetJump = false;
        if (liveHavePreviousX)
        {
            const auto delta = x - livePreviousX;
            const auto magnitude = std::abs (velocityX);
            const auto isDiscreteJump = std::abs (delta) > 0.05f;
            if (isDiscreteJump)
            {
                direction = delta > 0.0f ? 1 : -1;
                liveStationaryFrames = 0;
            }
            else if (magnitude >= 0.002)
            {
                direction = velocityX > 0.0 ? 1 : -1;
                liveStationaryFrames = 0;
            }
            else if (magnitude <= 0.001)
            {
                const auto stationaryFramesRequired = juce::jmax (1, static_cast<int> (std::ceil (sampleRate * 0.002)));
                liveStationaryFrames = juce::jmin (stationaryFramesRequired, liveStationaryFrames + 1);
            }
            else if (! liveHoldReady)
                liveStationaryFrames = 0;
            targetJump = direction != liveDirection || isDiscreteJump;
        }
        else
            liveStationaryFrames = 0;
        livePendingTransition = false;
        livePreviousX = x;
        liveHavePreviousX = true;
        liveDirection = direction;

        const auto hasHistory = liveHistoryValidFrames >= static_cast<uint64_t> (liveWarmupFrames)
                             && liveHistoryValidFrames >= static_cast<uint64_t> (liveGrainFrames);
        const auto stationaryFramesRequired = juce::jmax (1, static_cast<int> (std::ceil (sampleRate * 0.002)));
        if (liveHaveSignal && ! liveHoldReady && liveActiveTrajectory.count > 0 && hasHistory
            && liveStationaryFrames >= stationaryFramesRequired && liveHoldSnapshot != nullptr)
        {
            const auto holdStart = newestSerial + 1u - static_cast<uint64_t> (liveGrainFrames);
            bool holdCopyValid = true;
            for (int holdFrame = 0; holdFrame < liveGrainFrames; ++holdFrame)
            {
                float left = 0.0f, right = 0.0f;
                const auto serial = static_cast<double> (holdStart + static_cast<uint64_t> (holdFrame));
                holdCopyValid = holdCopyValid
                    && readLiveStereoSample (serial, oldestSerial, newestSerial, left, right);
                if (! holdCopyValid) break;
                liveHoldSnapshot[static_cast<size_t> (holdFrame) * 2u] = left;
                liveHoldSnapshot[static_cast<size_t> (holdFrame) * 2u + 1u] = right;
            }
            if (holdCopyValid)
            {
                liveHoldReady = true;
                liveHoldPhase = 0;
            }
        }
        bool transitionInProgress = false;
        for (const auto& grain : liveGrains)
            transitionInProgress = transitionInProgress
                || (grain.active && (grain.fadeInFrames > 0 || grain.fadeOutRemaining > 0));
        transitionInProgress = transitionInProgress || liveReentryTailActive;
        const auto reenterNow = liveHaveSignal && targetJump && transitionInProgress && hasHistory
                             && liveActiveTrajectory.count > 0;
        if (reenterNow)
        {
            // Preserve the pre-pitch audible continuation before any grain
            // snapshot slot can be recycled, then launch the newest target on
            // this same sample. Both tail buffers were allocated in prepare().
            captureLiveContinuationTail();
            liveGrains.fill (LiveGrain {});
            launchLiveGrain (x, direction, false, oldestSerial, newestSerial);
            liveSamplesToLaunch = liveHopFrames;
        }
        else if (liveHaveSignal && targetJump && hasHistory && liveActiveTrajectory.count > 0)
        {
            liveSamplesToLaunch = 0;
            launchLiveGrain (x, direction, true, oldestSerial, newestSerial);
            liveSamplesToLaunch = liveHopFrames;
        }
        else if (liveHaveSignal && liveActiveTrajectory.count > 0 && hasHistory && liveSamplesToLaunch <= 0
                 && ! transitionInProgress)
        {
            launchLiveGrain (x, direction, false, oldestSerial, newestSerial);
            liveSamplesToLaunch = liveHopFrames;
        }
        if (liveSamplesToLaunch > 0) --liveSamplesToLaunch;

        double oldLeft = 0.0, oldRight = 0.0, oldWindowTotal = 0.0;
        double newLeft = 0.0, newRight = 0.0, newWindowTotal = 0.0;
        double stableLeft = 0.0, stableRight = 0.0, stableWindowTotal = 0.0;
        double transitionPhase = 0.0;
        bool hasOldBranch = false, hasNewBranch = false;
        bool readFailure = false;
        for (auto& grain : liveGrains)
        {
            if (! grain.active) continue;
            if (grain.age >= liveGrainFrames)
            {
                grain.active = false;
                continue;
            }
            const auto serial = grain.startSerial + static_cast<double> (grain.direction * grain.age);
            float left = 0.0f, right = 0.0f;
            if (! readLiveStereoSample (serial, oldestSerial, newestSerial, left, right))
            {
                readFailure = true;
                grain.active = false;
                continue;
            }
            const auto window = liveGrainWindow[static_cast<size_t> (grain.age)];
            const auto isNewBranch = grain.fadeInFrames > 0;
            const auto isOldBranch = grain.fadeOutRemaining > 0;
            if (isOldBranch || isNewBranch)
            {
                const auto phase = isOldBranch
                    ? static_cast<double> (liveTransitionFrames - grain.fadeOutRemaining)
                        / static_cast<double> (liveTransitionFrames - 1)
                    : static_cast<double> (grain.age)
                        / static_cast<double> (juce::jmax (1, grain.fadeInFrames - 1));
                transitionPhase = juce::jlimit (0.0, 1.0, phase);
            }
            if (isNewBranch)
            {
                newLeft += static_cast<double> (left) * window;
                newRight += static_cast<double> (right) * window;
                newWindowTotal += window;
                hasNewBranch = true;
            }
            else if (isOldBranch)
            {
                oldLeft += static_cast<double> (left) * window;
                oldRight += static_cast<double> (right) * window;
                oldWindowTotal += window;
                hasOldBranch = true;
            }
            else
            {
                stableLeft += static_cast<double> (left) * window;
                stableRight += static_cast<double> (right) * window;
                stableWindowTotal += window;
            }
            if (grain.fadeOutRemaining > 0)
            {
                --grain.fadeOutRemaining;
                if (grain.fadeOutRemaining == 0) grain.active = false;
            }
            if (grain.fadeInFrames > 0 && grain.age + 1 >= grain.fadeInFrames)
                grain.fadeInFrames = 0;
            ++grain.age;
        }

        double wetLeft = stableLeft, wetRight = stableRight, windowTotal = stableWindowTotal;
        if (hasOldBranch || hasNewBranch)
        {
            // Stable overlaps, if any, belong to the outgoing side. Normalize
            // each side independently before equal-power combination; a shared
            // normalization would cancel the intended +3.01 dB midpoint.
            oldLeft += stableLeft;
            oldRight += stableRight;
            oldWindowTotal += stableWindowTotal;
            const auto oldNormLeft = oldWindowTotal > 1.0e-9 ? oldLeft / oldWindowTotal : 0.0;
            const auto oldNormRight = oldWindowTotal > 1.0e-9 ? oldRight / oldWindowTotal : 0.0;
            const auto newNormLeft = newWindowTotal > 1.0e-9 ? newLeft / newWindowTotal : 0.0;
            const auto newNormRight = newWindowTotal > 1.0e-9 ? newRight / newWindowTotal : 0.0;
            const auto angle = transitionPhase * kPi * 0.5;
            wetLeft = oldNormLeft * std::cos (angle) + newNormLeft * std::sin (angle);
            wetRight = oldNormRight * std::cos (angle) + newNormRight * std::sin (angle);
            // The branches are already normalized. A unit denominator keeps
            // the existing hold/readiness path from normalizing them again.
            windowTotal = 1.0;
        }

        const auto holdTarget = liveHoldReady && liveStationaryFrames >= stationaryFramesRequired ? 1.0f : 0.0f;
        const auto holdStep = 1.0f / static_cast<float> (juce::jmax (1, liveTransitionFrames));
        if (liveHoldMix < holdTarget) liveHoldMix = juce::jmin (holdTarget, liveHoldMix + holdStep);
        else if (liveHoldMix > holdTarget) liveHoldMix = juce::jmax (holdTarget, liveHoldMix - holdStep);
        if (liveHoldReady && liveHoldSnapshot != nullptr && liveGrainFrames > 1)
        {
            const auto seamFrames = juce::jlimit (1, liveGrainFrames / 2,
                                                   juce::roundToInt (sampleRate * 0.002));
            const auto seamStart = liveGrainFrames - seamFrames;
            const auto phase = juce::jlimit (0, liveGrainFrames - 1, liveHoldPhase);
            auto loopLeft = liveHoldSnapshot[static_cast<size_t> (phase) * 2u];
            auto loopRight = liveHoldSnapshot[static_cast<size_t> (phase) * 2u + 1u];
            if (phase >= seamStart)
            {
                const auto blend = static_cast<double> (phase - seamStart)
                    / static_cast<double> (juce::jmax (1, seamFrames - 1));
                const auto head = phase - seamStart;
                const auto a = std::cos (blend * kPi * 0.5);
                const auto b = std::sin (blend * kPi * 0.5);
                loopLeft = static_cast<float> (loopLeft * a
                    + liveHoldSnapshot[static_cast<size_t> (head) * 2u] * b);
                loopRight = static_cast<float> (loopRight * a
                    + liveHoldSnapshot[static_cast<size_t> (head) * 2u + 1u] * b);
            }
            const auto normalizedLeft = windowTotal > 1.0e-9 ? wetLeft / windowTotal : 0.0;
            const auto normalizedRight = windowTotal > 1.0e-9 ? wetRight / windowTotal : 0.0;
            wetLeft = normalizedLeft * std::cos (liveHoldMix * kPi * 0.5)
                    + static_cast<double> (loopLeft) * std::sin (liveHoldMix * kPi * 0.5);
            wetRight = normalizedRight * std::cos (liveHoldMix * kPi * 0.5)
                     + static_cast<double> (loopRight) * std::sin (liveHoldMix * kPi * 0.5);
            windowTotal = 1.0;
            ++liveHoldPhase;
            if (liveHoldPhase >= liveGrainFrames) liveHoldPhase = seamFrames;
            if (holdTarget == 0.0f && liveHoldMix == 0.0f)
                liveHoldReady = false;
        }

        const auto hasWet = liveActiveTrajectory.count > 0 && hasHistory
                         && windowTotal > 1.0e-9 && ! readFailure;
        const auto targetGain = hasWet && liveHaveSignal && ! sourceEnded ? 1.0f : 0.0f;
        const auto step = 1.0f / static_cast<float> (rampFrames);
        if (liveWetGain < targetGain) liveWetGain = juce::jmin (targetGain, liveWetGain + step);
        else if (liveWetGain > targetGain) liveWetGain = juce::jmax (targetGain, liveWetGain - step);
        if (liveSourceFadeActive && liveWetGain <= 0.0f)
        {
            liveGrains.fill (LiveGrain {});
            liveReentryTailActive = false;
            liveReentryTailPosition = 0;
            liveHoldReady = false;
            liveHoldMix = 0.0f;
            liveStationaryFrames = liveHoldPhase = 0;
            liveSourceFadeActive = false;
        }

        auto xStageLeft = 0.0f, xStageRight = 0.0f;
        if (hasWet || liveWetGain > 0.0f)
        {
            xStageLeft = windowTotal > 1.0e-9 ? static_cast<float> (wetLeft / windowTotal) : 0.0f;
            xStageRight = windowTotal > 1.0e-9 ? static_cast<float> (wetRight / windowTotal) : 0.0f;
        }
        if (liveReentryTailActive && liveReentryTailBuffers[static_cast<size_t> (liveReentryTailBuffer)])
        {
            const auto q = static_cast<float> (juce::jlimit (0, liveReentryTailFrames - 1,
                                                              liveReentryTailPosition))
                         / static_cast<float> (liveReentryTailFrames - 1);
            const auto tailIndex = static_cast<size_t> (liveReentryTailPosition) * 2u;
            const auto* tail = liveReentryTailBuffers[static_cast<size_t> (liveReentryTailBuffer)].get();
            xStageLeft = blendLiveReentrySample (tail[tailIndex], xStageLeft, q);
            xStageRight = blendLiveReentrySample (tail[tailIndex + 1u], xStageRight, q);
            if (++liveReentryTailPosition >= liveReentryTailFrames)
            {
                liveReentryTailActive = false;
                liveReentryTailPosition = 0;
            }
        }
        if (readFailure)
        {
            liveGrains.fill (LiveGrain {});
            liveReentryTailActive = false;
            liveReentryTailPosition = 0;
            liveWetGain = 0.0f;
            liveSamplesToLaunch = 0;
        }
        float delayedLeft = 0.0f, delayedRight = 0.0f, delayedMask = 0.0f;
        processLivePitchSample (xStageLeft, xStageRight, liveSmoothedSemitones,
                                readFailure ? 0.0f : liveWetGain,
                                delayedLeft, delayedRight, delayedMask);
        if (outputLeft != nullptr) outputLeft[i] = delayedLeft;
        if (outputRight != nullptr) outputRight[i] = delayedRight;
        mask[i] = delayedMask;
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
            // UI/state coordinates use y=1 at the top of the pad.
            const auto semitones = 24.0 * (static_cast<double> (y) - 0.5);
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
                              float depth, bool presetActive, float xyBlend) noexcept
{
    const auto boundedDepth = juce::jlimit (0.0f, 1.0f, depth);
    if (boundedDepth <= 0.0f)
        return dry;
    const auto boundedXyBlend = juce::jlimit (0.0f, 1.0f,
        std::isfinite (xyBlend) ? xyBlend : 0.0f);
    if (! presetActive)
    {
        if (boundedXyBlend <= 0.0f)
            return dry;
        return dry * (1.0f - boundedDepth)
             + boundedDepth * (dry * (1.0f - boundedXyBlend) + xyWet * boundedXyBlend);
    }

    constexpr float equalPower = 0.7071067811865475f;
    const auto presetWet = (presetOutput - dry * (1.0f - boundedDepth)) / boundedDepth;
    const auto layered = dry * (1.0f - boundedDepth)
                       + boundedDepth * (equalPower * presetWet + equalPower * xyWet);
    // Fade from the unchanged preset path into the parallel XY/preset mix. This
    // keeps XY's warmup and source-silence ramps from dipping the preset level.
    return presetOutput + (layered - presetOutput) * boundedXyBlend;
}
