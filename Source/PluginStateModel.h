#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

// Phase 1 timeline state. One BAR MAP cell is one independent timeline slot.
class PluginStateModel final
{
public:
    static constexpr int kNumBars = 64, kMaxMotionPoints = 256;
    static constexpr int kNoSelectedBar = -1;
    enum class ScratchPreset { off, forwardCut, backspin, chirp, baby, transform, drag, zigzag, tapeBrake, custom };
    enum class NoteLength { sixteenth, eighth, quarter, half, oneBar };

    struct MotionPoint
    {
        float x = 0.0f, y = 0.0f; double timeSeconds = 0.0;
        bool operator== (const MotionPoint& other) const noexcept
        {
            return x == other.x && y == other.y && timeSeconds == other.timeSeconds;
        }
    };
    struct TimelineSlot
    {
        ScratchPreset preset = ScratchPreset::off;
        NoteLength length = NoteLength::sixteenth;
        float speed = 1.0f, pitch = 0.0f, depth = 0.5f;
        bool customMotion = false;
        std::vector<MotionPoint> motion;
        // XY PAD data is independent from the existing preset/custom motion.
        float currentX = 0.5f, currentY = 0.5f;
        bool xyMotionExists = false;
        double xyMotionDurationSeconds = 0.0;
        std::vector<MotionPoint> xyMotion;
    };
    struct UiState
    {
        // selectedTab is the visible 16-BAR page.  It is deliberately not a
        // visual selection: a fresh instance must show page 1-16 without a
        // gold TAB rim.  tabHighlight is set only by an explicit TAB click.
        int selectedTab = 0, tabHighlight = -1, selectedBar = kNoSelectedBar;
        bool bypass = false, hostSync = true;
        double internalBpm = 120.0;
        int internalTimeSigNumerator = 4, internalTimeSigDenominator = 4;
        int projectPresetId = 0;
    };

    PluginStateModel();
    const TimelineSlot& getSlot (int bar) const noexcept;
    UiState getUiState() const noexcept { return ui; }
    static bool hasSelectedBar (int bar) noexcept { return bar >= 0 && bar < kNumBars; }

    void selectTab (int); // view page only
    void selectBar (int);
    void setBypass (bool);
    void setHostSync (bool);
    void setInternalBpm (double);
    void setInternalTimeSignature (int numerator, int denominator);
    void setProjectPresetId (int);
    void setSlotPreset (int, ScratchPreset);
    void setSlotLength (int, NoteLength);
    void setSlotSpeed (int, float);
    void setSlotPitch (int, float);
    void setSlotDepth (int, float);
    void setSlotMotion (int, const std::vector<MotionPoint>&);
    void clearSlotMotion (int);
    void resetSlot (int);

    void setSelectedPreset (ScratchPreset);
    void setSelectedLength (NoteLength);
    void setSelectedSpeed (float);
    void setSelectedPitch (float);
    void setSelectedDepth (float);
    void setSelectedMotion (const std::vector<MotionPoint>&);
    void clearSelectedMotion();
    void resetSelectedSlot();

    void setSlotXYPosition (int, float x, float y);
    void beginSlotXYMotion (int, float x, float y);
    void appendSlotXYMotion (int, float x, float y, double elapsedSeconds);
    void clearSlotXYMotion (int);
    void resetSlotXYPosition (int);
    void setSelectedXYPosition (float x, float y);
    void beginSelectedXYMotion (float x, float y);
    void appendSelectedXYMotion (float x, float y, double elapsedSeconds);
    void clearSelectedBarXYMotion();
    void resetSelectedBarXYPosition();

    juce::ValueTree toValueTree() const;
    bool fromValueTree (const juce::ValueTree&);
    void reset();

    static constexpr float kMinSpeed = 0.25f, kMaxSpeed = 4.0f, kMinPitch = -12.0f, kMaxPitch = 12.0f;
    static constexpr double kMinInternalBpm = 20.0, kMaxInternalBpm = 300.0;

private:
    static int barIndex (int) noexcept;
    static float finiteClamp (float, float, float, float) noexcept;
    static std::vector<MotionPoint> sanitiseMotion (const std::vector<MotionPoint>&);
    static std::vector<MotionPoint> sanitiseXYMotion (const std::vector<MotionPoint>&);
    static std::vector<MotionPoint> presetMotion (ScratchPreset);
    TimelineSlot& mutableSlot (int) noexcept;

    std::array<TimelineSlot, kNumBars> slots {};
    UiState ui {};
    TimelineSlot fallback {};
};
