#pragma once

#include <array>
#include <cstddef>
#include "TimelineScratchEngine.h"

// Fixed generic four-BAR patterns for the project selector.  This is separate
// from the user-editable 64-BAR timeline: selecting a profile changes only the
// audio slot source, never the persisted manual slots or XY trajectory.
namespace BravoProjectPresets
{
inline std::array<TimelineScratchEngine::Slot, 64> make (int profileId) noexcept
{
    std::array<TimelineScratchEngine::Slot, 64> slots {};
    const auto setEveryFourBars = [&slots] (TimelineScratchEngine::Preset first,
                                            TimelineScratchEngine::Preset second,
                                            float depth)
    {
        for (int bar = 0; bar < 64; bar += 4)
        {
            slots[(size_t) (bar + 2)] = { first, TimelineScratchEngine::Length::oneBar, 1.0f, 0.0f, depth };
            slots[(size_t) (bar + 3)] = { second, TimelineScratchEngine::Length::oneBar, 1.0f, 0.0f, depth };
        }
    };

    if (profileId == 1)
        setEveryFourBars (TimelineScratchEngine::Preset::transform,
                          TimelineScratchEngine::Preset::forwardCut, 0.90f);
    else if (profileId == 2)
        setEveryFourBars (TimelineScratchEngine::Preset::zigzag,
                          TimelineScratchEngine::Preset::tapeBrake, 0.85f);
    return slots;
}
}
