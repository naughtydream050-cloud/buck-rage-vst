#pragma once

#include <array>
#include <cstddef>
#include "TimelineScratchEngine.h"

// Measurement-backed source-song schedules for the project selector.  They are
// separate from the user-editable 64-BAR timeline: selecting a profile changes
// only the audio slot source, never persisted manual slots or XY trajectory.
namespace BravoProjectPresets
{
inline constexpr int kSongScheduleBars = 100;
using Slot = TimelineScratchEngine::Slot;
using Preset = TimelineScratchEngine::Preset;

inline void setEvent (std::array<Slot, kSongScheduleBars>& slots, int oneBasedBar,
                      Preset preset, float depth) noexcept
{
    if (oneBasedBar >= 1 && oneBasedBar <= kSongScheduleBars)
        slots[(size_t) (oneBasedBar - 1)] = { preset, TimelineScratchEngine::Length::oneBar, 1.0f, 0.0f, depth };
}

inline std::array<Slot, kSongScheduleBars> makeSongSchedule (int profileId) noexcept
{
    std::array<Slot, kSongScheduleBars> slots {};
    if (profileId == 1)
    {
        setEvent (slots, 3, Preset::forwardCut, 0.90f);
        for (const auto bar : { 53, 54, 56, 70, 99 }) setEvent (slots, bar, Preset::tapeBrake, 0.90f);
    }
    else if (profileId == 2)
    {
        setEvent (slots, 2, Preset::tapeBrake, 0.85f);
        setEvent (slots, 4, Preset::forwardCut, 0.85f);
        for (const auto bar : { 7, 19, 24, 31, 33 }) setEvent (slots, bar, Preset::transform, 0.85f);
        setEvent (slots, 20, Preset::forwardCut, 0.85f);
    }
    return slots;
}

inline const std::array<Slot, kSongScheduleBars>& songSchedule (int profileId) noexcept
{
    static const auto bravo1 = makeSongSchedule (1);
    static const auto bravo2 = makeSongSchedule (2);
    static const std::array<Slot, kSongScheduleBars> init {};
    return profileId == 1 ? bravo1 : profileId == 2 ? bravo2 : init;
}

inline TimelineScratchEngine::AbsoluteSchedule absoluteSchedule (int profileId) noexcept
{
    if (profileId == 0) return {};
    const auto& slots = songSchedule (profileId);
    return { slots.data(), kSongScheduleBars };
}

inline std::array<Slot, 64> make (int profileId) noexcept
{
    std::array<Slot, 64> slots {};
    const auto& song = songSchedule (profileId);
    for (size_t index = 0; index < slots.size(); ++index) slots[index] = song[index];
    return slots;
}
}
