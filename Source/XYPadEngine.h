#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginStateModel.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// Phase 2 XY scratch storage. All dynamic allocation and FFT setup happen in
// prepare(); process() only copies into preallocated storage and uses atomics.
class XYPadEngine final
{
public:
    static constexpr int kBarCount = 64;
    static constexpr int kMaxPoints = 256;
    enum class State : uint8_t { empty, capturing, queued, rendering, ready };

    struct Transport
    {
        bool playing = false;
        int startBar = -1;
        double startPhase = 0.0;
        double phasePerSample = 0.0;
        int loopBoundarySample = -1;
        int loopStartBar = 0;
        double loopStartPhase = 0.0;
        double quartersPerBar = 4.0;
        double secondsPerQuarter = 0.5;
        uint32_t timingGeneration = 0;
    };

    XYPadEngine() noexcept;
    ~XYPadEngine();
    XYPadEngine (const XYPadEngine&) = delete;
    XYPadEngine& operator= (const XYPadEngine&) = delete;

    bool prepare (double sampleRate, int maximumBlockSize, bool allocateBank = true) noexcept;
    void release() noexcept;

    // Non-audio-thread publisher. The saved PluginStateModel schema is not
    // changed; the bounded copy is adopted by the audio thread at BAR entry.
    void setTrajectory (int bar, const std::vector<PluginStateModel::MotionPoint>& points,
                        uint32_t revision) noexcept;

    // source is a pre-effect copy. rendered and readyMask are preallocated by
    // the processor. A zero mask means XY contributes nothing for that sample.
    void process (const juce::AudioBuffer<float>& source,
                  juce::AudioBuffer<float>& rendered,
                  juce::AudioBuffer<float>& readyMask,
                  const Transport& transport, int frameCount) noexcept;

    State getState (int bar) const noexcept;
    static float mixLayers (float dry, float presetOutput, float xyWet,
                            float depth, bool presetActive) noexcept;
    int getMaxFramesPerBar() const noexcept { return maxFramesPerBar; }
    bool isEnabled() const noexcept { return enabled.load (std::memory_order_acquire); }

private:
    friend class XYPadEngineTestAccess;
    struct AtomicTrajectory
    {
        std::atomic<uint32_t> sequence { 0 };
        std::atomic<uint32_t> revision { 0 };
        std::atomic<uint32_t> count { 0 };
        std::array<std::atomic<uint32_t>, kMaxPoints> x {}, y {}, timeLow {}, timeHigh {};
    };
    struct Trajectory
    {
        uint32_t revision = 0;
        int count = 0;
        std::array<float, kMaxPoints> x {}, y {};
        std::array<double, kMaxPoints> time {};
    };
    struct Slot
    {
        std::unique_ptr<float[]> pcm;
        std::atomic<State> state { State::empty };
        std::atomic<uint32_t> generation { 1 };
        std::atomic<uint32_t> trajectoryRevision { 0 };
        std::atomic<int> frames { 0 };
        Trajectory captureTrajectory {};
        int captureFrames = 0;
        int expectedFrames = 0;
        uint32_t activeTimingGeneration = 0;
    };
    struct RenderJob
    {
        uint32_t bar = 0;
        uint32_t generation = 0;
    };

    static_assert (std::atomic<uint32_t>::is_always_lock_free,
                   "XY trajectory publication requires lock-free 32-bit atomics");
    static_assert (std::atomic<State>::is_always_lock_free,
                   "XY slot state publication requires lock-free atomics");
    static float interpolate (const float* source, int frames, double position, int stride = 1) noexcept;
    static void sampleTimeline (const Transport&, int sample, int totalSamples,
                                int& bar, double& phase) noexcept;
    static bool readTrajectory (const AtomicTrajectory&, Trajectory&) noexcept;
    static void interpolateTrajectory (const Trajectory&, double phase,
                                       float& x, float& y) noexcept;
    void workerLoop() noexcept;
    bool enqueueRenderJob (RenderJob) noexcept;
    bool dequeueRenderJob (RenderJob&) noexcept;
    bool renderSlot (Slot&) noexcept;
    bool renderScrubAndPitch (Slot&, int frames, int* longestTransitionFrames = nullptr,
                              double* transitionEndTargetErrorSamples = nullptr) noexcept;
    void stopWorker() noexcept;

    double sampleRate = 0.0;
    int maxFramesPerBar = 0, maxBlock = 0;
    // These fixed-size BAR tables exceed the default Windows thread stack when
    // embedded in a JUCE processor/test fixture. Allocate them off the audio
    // thread while preserving fixed capacity and the same indexed access.
    std::unique_ptr<Slot[]> slots;
    static constexpr uint32_t kRenderQueueCapacity = 64;
    std::array<RenderJob, kRenderQueueCapacity> renderQueue {};
    std::atomic<uint32_t> renderQueueHead { 0 }, renderQueueTail { 0 };
    std::unique_ptr<AtomicTrajectory[]> publishedTrajectories;
    Trajectory activeTrajectory {};
    std::unique_ptr<Trajectory[]> activeTrajectories;
    int activeBar = -1;
    uint32_t activeRevision = 0;
    std::unique_ptr<float[]> scratch;
    std::unique_ptr<float[]> fftWork, fftWork2;
    std::unique_ptr<float[]> overlapNorm;
    std::unique_ptr<double[]> previousInputPhase, synthesisPhase;
    std::unique_ptr<uint8_t[]> phaseInitialized;
    std::unique_ptr<juce::dsp::FFT> fft;
    int fftSize = 2048, hopSize = 512;
    std::atomic<bool> enabled { false }, workerExit { false };
    std::thread worker;
    std::mutex nonRealtimeLifecycleMutex;
};
