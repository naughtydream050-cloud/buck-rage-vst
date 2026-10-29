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

// Phase 2 XY scratch storage. All dynamic allocation and setup happen in
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
        bool resetHistory = false;
    };

    XYPadEngine() noexcept;
    ~XYPadEngine();
    XYPadEngine (const XYPadEngine&) = delete;
    XYPadEngine& operator= (const XYPadEngine&) = delete;

    bool prepare (double sampleRate, int maximumBlockSize, bool allocateBank = true,
                  bool liveOnly = false) noexcept;
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

    // Low-latency live path. Unlike process(), this never waits for a full BAR
    // capture or the background renderer; it reads only from bounded recent input.
    void processLive (const juce::AudioBuffer<float>& source,
                      juce::AudioBuffer<float>& rendered,
                      juce::AudioBuffer<float>& readyMask,
                      const Transport& transport, int frameCount) noexcept;

    State getState (int bar) const noexcept;
    static float mixLayers (float dry, float presetOutput, float xyWet,
                            float depth, bool presetActive, float xyBlend = 1.0f) noexcept;
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
    struct LiveGrain
    {
        double startSerial = 0.0;
        double readIncrement = 1.0;
        int direction = 1;
        int age = 0;
        int fadeInFrames = 0;
        int fadeOutRemaining = 0;
        bool active = false;
    };

    static_assert (std::atomic<uint32_t>::is_always_lock_free,
                   "XY trajectory publication requires lock-free 32-bit atomics");
    static_assert (std::atomic<State>::is_always_lock_free,
                   "XY slot state publication requires lock-free atomics");
    static float interpolate (const float* source, int frames, double position, int stride = 1) noexcept;
    bool readLiveSample (double serial, int channel, uint64_t oldestSerial,
                         uint64_t newestSerial, float& value) const noexcept;
    bool readLiveStereoSample (double serial, uint64_t oldestSerial, uint64_t newestSerial,
                               float& left, float& right) const noexcept;
    bool copyLiveHoldSnapshot (uint64_t startSerial, uint64_t newestSerial) noexcept;
    void launchLiveGrain (float x, double requestedIncrement, bool transition,
                          uint64_t oldestSerial, uint64_t newestSerial,
                          bool preserveExactTarget = false) noexcept;
    void captureLiveContinuationTail() noexcept;
    static float blendLiveReentrySample (float tail, float target, float q) noexcept;
    void processLiveDelaySample (float inputLeft, float inputRight, float wetMask,
                                 float& outputLeft, float& outputRight, float& outputMask) noexcept;
    void clearLiveDelayState() noexcept;
    static void liveSinCos (double phase, double& sine, double& cosine) noexcept;
    static void sampleTimeline (const Transport&, int sample, int totalSamples,
                                int& bar, double& phase) noexcept;
    static bool readTrajectory (const AtomicTrajectory&, Trajectory&) noexcept;
    static void interpolateTrajectory (const Trajectory&, double phase,
                                       float& x, float& y,
                                       double* xSlopePerPhase = nullptr) noexcept;
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
    static constexpr double kLiveHistorySeconds = 0.125;
    static constexpr double kLiveWarmupSeconds = 0.020;
    static constexpr double kLiveGrainSeconds = 0.020;
    static constexpr double kLiveHopSeconds = 0.010;
    static constexpr uint64_t kMaxLiveHistoryFrames = 13922;
    static constexpr int kLiveGrainCapacity = 8;
    std::unique_ptr<float[]> liveHistory;
    std::unique_ptr<float[]> liveHoldSnapshot;
    std::unique_ptr<float[]> liveGrainWindow;
    std::array<std::unique_ptr<float[]>, 2> liveReentryTailBuffers;
    std::unique_ptr<uint64_t[]> liveHistorySerials;
    static constexpr int kLivePipelineLatency = 1024;
    std::unique_ptr<float[]> liveOutputDelay;
    std::unique_ptr<float[]> liveOutputMaskDelay;
    uint64_t liveOutputDelaySerial = 0;
    std::array<LiveGrain, kLiveGrainCapacity> liveGrains {};
    uint64_t liveWriteSerial = 0;
    uint64_t liveHistoryValidFrames = 0;
    int liveHistoryCapacityFrames = 0;
    int liveHistoryRetentionFrames = 0;
    uint64_t liveHistoryIndexMask = 0;
    int liveWarmupFrames = 0;
    int liveGrainFrames = 0;
    int liveHopFrames = 0;
    int liveTransitionFrames = 0;
    int liveReentryTailFrames = 0, liveReentryTailPosition = 0, liveReentryTailBuffer = 0;
    int liveReentryTailStart = 0, liveReentryTailAvailable = 0;
    int liveSamplesToLaunch = 0;
    int liveSilenceFramesCount = 0;
    int liveStationaryFrames = 0, liveHoldPhase = 0;
    int liveActiveBar = -1;
    uint32_t liveActiveRevision = 0;
    Trajectory liveActiveTrajectory {};
    float livePreviousX = 0.5f;
    float liveWetGain = 0.0f;
    float liveHoldMix = 0.0f;
    int liveDirection = 1;
    bool liveHavePreviousX = false, liveHaveSignal = false, livePendingTransition = false;
    bool liveHoldReady = false;
    bool liveReentryTailActive = false;
    bool liveTailAvx2Available = false;
    bool liveTailFmaAvailable = false;
    bool liveHistoryHasUnsafeMagnitude = false;
    bool liveWasPlaying = false;
    bool liveSourceFadeActive = false;
    std::atomic<bool> enabled { false }, workerExit { false };
    std::thread worker;
    std::mutex nonRealtimeLifecycleMutex;

};
