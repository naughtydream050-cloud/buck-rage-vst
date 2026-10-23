#pragma once

#include <cstddef>
#include <cstdint>

bool xyPadTailAvx2Available() noexcept;
bool xyPadTailFmaAvailable() noexcept;

bool xyPadRenderTailAvx2 (const float* history, const float* previousTail,
                          float* destination, size_t lowerIndex, size_t upperIndex,
                          uint64_t capacity, int direction, float fraction,
                          int startFrame, int frameCount, int previousTailPosition,
                          float blendScale, bool useFma) noexcept;
