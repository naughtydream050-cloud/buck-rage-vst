#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

// Phase 1 timeline state. One BAR MAP cell is one independent timeline slot.
class PluginStateModel final
{
public:
    static constexpr int kNumBars = 64, kMaxMotionPoints = 256;
    enum class ScratchPreset { off, forwardCut, backspin, chirp, baby, transform, drag, zigzag, tapeBrake, custom };
    enum class NoteLength { sixteenth, eighth, quarter, half, oneBar };

    struct MotionPoint { float x = 0.0f, y = 0.0f; };
    struct TimelineSlot
    {
        ScratchPreset preset = ScratchPreset::off;
        NoteLength length = NoteLength::sixteenth;
        float speed = 1.0f, pitch = 0.0f, depth = 0.5f;
        bool customMotion = false;
        std::vector<MotionPoint> motion;
    };
    struct UiState { int selectedTab = 0, selectedBar = 0; bool bypass = false; };

    PluginStateModel();
    const TimelineSlot& getSlot (int bar) const noexcept;
    UiState getUiState() const noexcept { return ui; }

    void selectTab (int); // view page only
    void selectBar (int);
    void setBypass (bool);
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

    juce::ValueTree toValueTree() const;
    bool fromValueTree (const juce::ValueTree&);
    void reset();

    static constexpr float kMinSpeed = 0.25f, kMaxSpeed = 4.0f, kMinPitch = -12.0f, kMaxPitch = 12.0f;

private:
    static int barIndex (int) noexcept;
    static float finiteClamp (float, float, float, float) noexcept;
    static std::vector<MotionPoint> sanitiseMotion (const std::vector<MotionPoint>&);
    static std::vector<MotionPoint> presetMotion (ScratchPreset);
    TimelineSlot& mutableSlot (int) noexcept;

    std::array<TimelineSlot, kNumBars> slots {};
    UiState ui {};
    TimelineSlot fallback {};
};
