#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class PluginStateModel final
{
public:
    static constexpr int kNumBars = 64, kCountsPerBar = 16, kTotalCountSlots = 1024, kMaxMotionPoints = 256;
    enum class ScratchPreset { off, forwardCut, backspin, chirp, baby, transform, drag, zigzag, tapeBrake, custom };
    enum class NoteLength { sixteenth, eighth, quarter, half, oneBar };
    struct MotionPoint { float x = 0.0f, y = 0.0f; };
    struct CountSlot { ScratchPreset preset = ScratchPreset::off; NoteLength length = NoteLength::sixteenth; float speed = 1.0f, pitch = 0.0f, depth = 0.5f; bool customMotion = false; std::vector<MotionPoint> motion; };
    struct BarPattern { std::array<CountSlot, kCountsPerBar> counts; };
    struct UiState { int selectedTab = 0, selectedBar = 0, selectedCount = 0; bool bypass = false; };

    PluginStateModel();
    const CountSlot& getCount (int bar, int count) const noexcept;
    UiState getUiState() const noexcept { return ui; }
    void selectTab (int); void selectBar (int); void selectCount (int); void setBypass (bool);
    void setCountPreset (int, int, ScratchPreset); void setCountLength (int, int, NoteLength);
    void setCountSpeed (int, int, float); void setCountPitch (int, int, float); void setCountDepth (int, int, float);
    void setCountMotion (int, int, const std::vector<MotionPoint>&); void clearCountMotion (int, int); void resetCountSlot (int, int);
    void setSelectedPreset (ScratchPreset); void setSelectedLength (NoteLength); void setSelectedSpeed (float); void setSelectedPitch (float); void setSelectedDepth (float); void setSelectedMotion (const std::vector<MotionPoint>&); void clearSelectedMotion();
    juce::ValueTree toValueTree() const; bool fromValueTree (const juce::ValueTree&);
    void reset();
    static constexpr float kMinSpeed = 0.25f, kMaxSpeed = 4.0f, kMinPitch = -12.0f, kMaxPitch = 12.0f;
private:
    static int barIndex (int) noexcept; static int countIndex (int) noexcept;
    static float finiteClamp (float, float, float, float) noexcept;
    static std::vector<MotionPoint> sanitiseMotion (const std::vector<MotionPoint>&);
    static std::vector<MotionPoint> presetMotion (ScratchPreset);
    CountSlot& mutableCount (int, int) noexcept;
    std::array<BarPattern, kNumBars> bars {};
    UiState ui {};
    CountSlot fallback {};
};
