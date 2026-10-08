#pragma once
#include "TimelineScratchEngine.h"
#include <atomic>
#include <memory>
#include <vector>

// Diagnostic-only recorder. All file/thread/storage setup and teardown occurs
// outside processBlock. The producer only copies PODs into a bounded SPSC ring.
class RealHostDiagnostic final : private juce::Thread
{
public:
    struct Block final
    {
        uint64_t ordinal = 0, sampleClock = 0;
        int frames = 0, numerator = 4, denominator = 4, startBar = -1, loopBoundarySample = -1;
        double sampleRate = 0, bpm = 0, ppq = 0, loopStart = 0, loopEnd = 0;
        double originBefore = 0, originAfter = 0, startPhase = 0;
        int64_t hostSamples = 0;
        bool playing = false, looping = false, ppqPresent = false, samplesPresent = false;
        bool loopPointsPresent = false, hostSync = false, resetRequested = false;
        TimelineScratchEngine::DiscontinuityReason resetReason {};
    };

    RealHostDiagnostic() : Thread ("Toyotomi diagnostic writer") {}
    ~RealHostDiagnostic() override { stopAndFlush(); }

    static juce::File diagnosticRoot()
    {
        return juce::File ("D:/Development/RAZOR_FACE_COMPANY/01_PLUGINS/projects/ToyotomiHideyoshi/output/windows/diagnostic");
    }
    void configureFromMarker (double rate)
    {
        stopAndFlush();
        const auto marker = diagnosticRoot().getChildFile ("enable-real-fl-diagnostic.txt");
        if (! marker.existsAsFile() || marker.loadFileAsString().trim() != "ENABLED") return;
        const auto folder = diagnosticRoot().getChildFile ("captures")
            .getChildFile (juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S")
                          + "-" + juce::Uuid().toString());
        startForTest (folder, rate, 180.0, 32768, true);
    }

    // Explicit non-UI test/diagnostic entry; caller must not use this from RT.
    bool startForTest (const juce::File& folder, double rate, double seconds,
                       size_t capacity = 32768, bool startConsumer = true)
    {
        stopAndFlush();
        if (rate <= 0 || seconds <= 0 || capacity < 2 || ! folder.createDirectory()) return false;
        queue.assign (capacity, Packet {});
        readIndex.store (0); writeIndex.store (0); dropped.store (0);
        framesAttempted.store (0); limitReached.store (false); finished.store (false);
        failed = false; sessionStarted = false; blockOrdinal = 0; sampleLimit = (uint64_t) (rate * seconds);
        sampleRate = rate; sessionFolder = folder; writtenFrames = 0;
        blocks = folder.getChildFile ("transport-events.jsonl").createOutputStream();
        samples = folder.getChildFile ("samples.bin").createOutputStream();
        for (size_t i = 0; i < waves.size(); ++i)
        {
            waves[i] = folder.getChildFile (i == 0 ? "dry.wav" : i == 1 ? "wet.wav" : "output.wav").createOutputStream();
            if (waves[i]) writeWaveHeader (*waves[i]);
        }
        if (! blocks || ! samples || ! waves[0] || ! waves[1] || ! waves[2])
        {
            failed = true; finalize(); return false;
        }
        previousEntry = UINT64_MAX; previousReset = UINT32_MAX; previousBar = -2;
        previousCapture = previousEnded = previousOpen = false;
        enabled.store (true, std::memory_order_release);
        if (startConsumer && ! startThread())
        {
            enabled.store (false); failed = true; finalize(); return false;
        }
        return true;
    }
    void stopAndFlush()
    {
        enabled.store (false, std::memory_order_release);
        if (isThreadRunning())
        {
            signalThreadShouldExit();
            stopThread (10000);
        }
        if (! queue.empty() && ! finished.load())
        {
            while (consume()) {}
            finalize();
        }
    }
    bool isEnabled() const noexcept { return enabled.load (std::memory_order_acquire); }
    uint64_t getDropped() const noexcept { return dropped.load (std::memory_order_relaxed); }

    void beginBlock (Block block) noexcept
    {
        if (! isEnabled()) return;
        if (! sessionStarted && ! block.playing) return;
        sessionStarted = true;
        block.ordinal = blockOrdinal++;
        block.sampleClock = framesAttempted.load (std::memory_order_relaxed);
        Packet packet; packet.isBlock = true; packet.block = block;
        push (packet);
    }
    static void observe (const TimelineScratchEngine::SampleTrace& trace, void* context) noexcept
    {
        static_cast<RealHostDiagnostic*> (context)->record (trace);
    }

private:
    struct Packet final
    {
        bool isBlock = false;
        uint64_t sampleClock = 0;
        Block block {};
        TimelineScratchEngine::SampleTrace trace {};
    };
    static_assert (std::atomic<uint64_t>::is_always_lock_free, "Diagnostic atomics must be lock-free");
    void push (const Packet& packet) noexcept
    {
        const auto write = writeIndex.load (std::memory_order_relaxed);
        if (write - readIndex.load (std::memory_order_acquire) >= queue.size())
        {
            dropped.fetch_add (1, std::memory_order_relaxed); return;
        }
        queue[(size_t) (write % queue.size())] = packet;
        writeIndex.store (write + 1, std::memory_order_release);
    }
    void record (const TimelineScratchEngine::SampleTrace& trace) noexcept
    {
        if (! isEnabled() || ! sessionStarted) return;
        const auto clock = framesAttempted.load (std::memory_order_relaxed);
        if (clock >= sampleLimit)
        {
            enabled.store (false, std::memory_order_release);
            limitReached.store (true, std::memory_order_release); return;
        }
        Packet packet; packet.sampleClock = clock; packet.trace = trace;
        push (packet);
        framesAttempted.store (clock + 1, std::memory_order_relaxed);
    }
    void run() override
    {
        for (;;)
        {
            if (consume()) continue;
            if (threadShouldExit() || limitReached.load (std::memory_order_acquire)) break;
            wait (5);
        }
        finalize();
    }
    static void add (juce::DynamicObject& object, const char* key, const juce::var& value)
    {
        object.setProperty (key, value);
    }
    void writeEvent (juce::DynamicObject* object)
    {
        const juce::var owned (object);
        failed |= ! blocks->writeText (juce::JSON::toString (owned, true) + "\n", false, false, nullptr);
    }
    void writeBlock (const Block& b)
    {
        auto* o = new juce::DynamicObject;
        add (*o, "type", "block"); add (*o, "block", (int64_t) b.ordinal);
        add (*o, "sampleClock", (int64_t) b.sampleClock); add (*o, "frames", b.frames);
        add (*o, "sampleRate", b.sampleRate); add (*o, "bpm", b.bpm);
        add (*o, "numerator", b.numerator); add (*o, "denominator", b.denominator);
        add (*o, "ppqPresent", b.ppqPresent); add (*o, "ppq", b.ppq);
        add (*o, "samplesPresent", b.samplesPresent); add (*o, "hostSamples", b.hostSamples);
        add (*o, "playing", b.playing); add (*o, "looping", b.looping);
        add (*o, "loopPointsPresent", b.loopPointsPresent);
        add (*o, "loopStart", b.loopStart); add (*o, "loopEnd", b.loopEnd);
        add (*o, "originBefore", b.originBefore); add (*o, "originAfter", b.originAfter);
        add (*o, "hostSync", b.hostSync); add (*o, "startBar", b.startBar);
        add (*o, "startPhase", b.startPhase); add (*o, "loopBoundarySample", b.loopBoundarySample);
        add (*o, "resetRequested", b.resetRequested); add (*o, "resetReason", (int) b.resetReason);
        writeEvent (o);
    }
    void writeTrace (uint64_t clock, const TimelineScratchEngine::SampleTrace& t)
    {
        // Explicit little-endian schema; no struct padding or pointer bytes.
        for (auto v : { clock, t.sampleIndex, t.writerSerial, t.barEntrySerial }) failed |= ! samples->writeInt64 ((int64_t) v);
        for (auto v : { t.localBar, t.activeBar, (int) t.preset, (int) t.transportResetCount, (int) t.resetReason }) failed |= ! samples->writeInt (v);
        const auto flags = (t.captured ? 1 : 0) | (t.motionGateEnabled ? 2 : 0)
            | (t.wetEvaluated ? 4 : 0) | (t.sourceEnded ? 8 : 0) | (t.waitingForDry ? 16 : 0);
        failed |= ! samples->writeInt (flags);
        for (auto v : { t.barPhase, t.effectPhase, t.historyValidSamples, t.captureStartSerial,
                        t.captureEndSerial, t.motionPhase, t.motionCycle, t.gate, t.readPosition, t.readRate }) failed |= ! samples->writeDouble (v);
        for (auto v : { t.dryLeft, t.dryRight, t.wetLeft, t.wetRight,
                        t.outputLeft, t.outputRight, t.effectiveWet }) failed |= ! samples->writeFloat (v);
        failed |= ! waves[0]->writeFloat (t.dryLeft); failed |= ! waves[0]->writeFloat (t.dryRight);
        failed |= ! waves[1]->writeFloat (t.wetLeft); failed |= ! waves[1]->writeFloat (t.wetRight);
        failed |= ! waves[2]->writeFloat (t.outputLeft); failed |= ! waves[2]->writeFloat (t.outputRight);
        ++writtenFrames;
        const auto open = t.wetEvaluated && (! t.motionGateEnabled || t.gate > 0.001);
        if (t.barEntrySerial != previousEntry || t.transportResetCount != previousReset
            || t.activeBar != previousBar || t.captured != previousCapture
            || t.sourceEnded != previousEnded || open != previousOpen)
        {
            auto* o = new juce::DynamicObject;
            add (*o, "type", "sample-event"); add (*o, "sampleClock", (int64_t) clock);
            add (*o, "writerSerial", (int64_t) t.writerSerial); add (*o, "localBar", t.localBar);
            add (*o, "activeBar", t.activeBar); add (*o, "entry", (int64_t) t.barEntrySerial);
            add (*o, "captured", t.captured); add (*o, "captureStart", t.captureStartSerial);
            add (*o, "captureEnd", t.captureEndSerial); add (*o, "historyValid", t.historyValidSamples);
            add (*o, "motionPhase", t.motionPhase); add (*o, "motionCycle", t.motionCycle);
            add (*o, "gate", t.gate); add (*o, "wet", t.effectiveWet);
            add (*o, "sourceEnded", t.sourceEnded); add (*o, "resetCount", (int) t.transportResetCount);
            add (*o, "resetReason", (int) t.resetReason); writeEvent (o);
        }
        previousEntry = t.barEntrySerial; previousReset = t.transportResetCount;
        previousBar = t.activeBar; previousCapture = t.captured;
        previousEnded = t.sourceEnded; previousOpen = open;
    }
    bool consume()
    {
        const auto read = readIndex.load (std::memory_order_relaxed);
        if (read == writeIndex.load (std::memory_order_acquire)) return false;
        const auto packet = queue[(size_t) (read % queue.size())];
        readIndex.store (read + 1, std::memory_order_release);
        if (packet.isBlock) writeBlock (packet.block);
        else writeTrace (packet.sampleClock, packet.trace);
        return true;
    }
    void writeWaveHeader (juce::FileOutputStream& out)
    {
        out.write ("RIFF", 4); out.writeInt (36); out.write ("WAVEfmt ", 8); out.writeInt (16);
        out.writeShort (3); out.writeShort (2); out.writeInt ((int) sampleRate);
        out.writeInt ((int) sampleRate * 8); out.writeShort (8); out.writeShort (32);
        out.write ("data", 4); out.writeInt (0);
    }
    void finalize()
    {
        if (finished.exchange (true)) return;
        if (samples) { samples->flush(); failed |= samples->getStatus().failed(); samples.reset(); }
        if (blocks) { blocks->flush(); failed |= blocks->getStatus().failed(); blocks.reset(); }
        for (auto& wave : waves)
            if (wave)
            {
                failed |= ! wave->setPosition (4); failed |= ! wave->writeInt ((int) (36 + writtenFrames * 8));
                failed |= ! wave->setPosition (40); failed |= ! wave->writeInt ((int) (writtenFrames * 8));
                wave->flush(); failed |= wave->getStatus().failed(); wave.reset();
            }
        auto* object = new juce::DynamicObject;
        add (*object, "schemaVersion", 1); add (*object, "sampleRate", sampleRate);
        add (*object, "framesAttempted", (int64_t) framesAttempted.load());
        add (*object, "framesWritten", (int64_t) writtenFrames); add (*object, "droppedRecords", (int64_t) dropped.load());
        add (*object, "valid", ! failed && dropped.load() == 0 && writtenFrames > 0 && writtenFrames == framesAttempted.load());
        add (*object, "limitReached", limitReached.load());
        add (*object, "sampleRecordBytes", 164);
        add (*object, "sampleSchema", "LE: u64(clock,index,writer,entry); i32(localBar,activeBar,preset,resetCount,resetReason,flags); f64(barPhase,effectPhase,historyValid,captureStart,captureEnd,motionPhase,motionCycle,gate,readPosition,readRate); f32(dryL,dryR,wetL,wetR,outL,outR,effectiveWet). flags: captured=1,motion=2,wetEvaluated=4,ended=8,waiting=16");
        const juce::var owned (object);
        sessionFolder.getChildFile ("session.json").replaceWithText (juce::JSON::toString (owned));
    }
    std::vector<Packet> queue;
    std::atomic<uint64_t> readIndex { 0 }, writeIndex { 0 }, dropped { 0 }, framesAttempted { 0 };
    std::atomic<bool> enabled { false }, limitReached { false }, finished { true };
    std::unique_ptr<juce::FileOutputStream> blocks, samples;
    std::array<std::unique_ptr<juce::FileOutputStream>, 3> waves;
    juce::File sessionFolder;
    double sampleRate = 0;
    uint64_t sampleLimit = 0, blockOrdinal = 0, writtenFrames = 0, previousEntry = UINT64_MAX;
    uint32_t previousReset = UINT32_MAX;
    int previousBar = -2;
    bool sessionStarted = false, failed = false, previousCapture = false, previousEnded = false, previousOpen = false;
};
