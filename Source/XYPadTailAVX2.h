#pragma once

#include <cstddef>
#include <cstdint>

bool xyPadTailAvx2Available() noexcept;
bool xyPadTailFmaAvailable() noexcept;

// Executes the vector-sized prefix of one radix-2 FFT stage. Kept in the
// existing AVX2 translation unit so generic builds remain CPU-safe.
int xyPadLiveFftTransformStageAvx2 (float* real, float* imag,
                                    const float* twiddleReal, const float* twiddleImag,
                                    int size, int half, bool inverse) noexcept;

bool xyPadRenderTailAvx2 (const float* history, const float* previousTail,
                          float* destination, size_t lowerIndex, size_t upperIndex,
                          uint64_t capacity, int direction, float fraction,
                          int startFrame, int frameCount, int previousTailPosition,
                          float blendScale, bool useFma, bool sanitizeOutput) noexcept;
