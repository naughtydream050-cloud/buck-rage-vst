#include "XYPadTailAVX2.h"

#include <cmath>
#include <limits>

#if defined(_MSC_VER)
 #include <immintrin.h>
 #define TOYOTOMI_XY_AVX2_TARGET __declspec(noinline)
#elif defined(__x86_64__) || defined(__i386__)
 #include <immintrin.h>
 #define TOYOTOMI_XY_AVX2_TARGET __attribute__ ((target ("avx2"), noinline))
#endif

#if defined(_MSC_VER) || defined(__x86_64__) || defined(__i386__)
TOYOTOMI_XY_AVX2_TARGET
bool xyPadRenderTailAvx2 (const float* history, const float* previousTail,
                          float* destination, size_t lowerIndex, size_t upperIndex,
                          uint64_t capacity, int direction, float fraction,
                          int startFrame, int frameCount, int previousTailPosition,
                          float blendScale, bool useFma, bool sanitizeOutput) noexcept
{
    if (history == nullptr || destination == nullptr || capacity < 4
        || frameCount < 4 || (frameCount % 4) != 0
        || (direction != 1 && direction != -1))
        return false;
    if ((direction > 0 && (lowerIndex > capacity - static_cast<uint64_t> (frameCount)
                           || upperIndex > capacity - static_cast<uint64_t> (frameCount)))
        || (direction < 0 && (lowerIndex < static_cast<uint64_t> (frameCount - 1)
                              || upperIndex < static_cast<uint64_t> (frameCount - 1))))
        return false;

    const auto fractionVector = _mm256_set1_ps (fraction);
    const auto blendStep = blendScale;
    const auto blendOffsets = _mm256_setr_ps (0.0f, 0.0f, blendStep, blendStep,
                                               2.0f * blendStep, 2.0f * blendStep,
                                               3.0f * blendStep, 3.0f * blendStep);
    const auto reorderReverse = _mm256_setr_epi32 (6, 7, 4, 5, 2, 3, 0, 1);
    const auto maximumFinite = _mm256_set1_ps (std::numeric_limits<float>::max());
    const auto signMask = _mm256_set1_ps (-0.0f);
    auto* output = destination + static_cast<size_t> (startFrame) * 2u;

    for (int offset = 0; offset < frameCount; offset += 4)
    {
        __m256 lower, upper;
        if (direction > 0)
        {
            lower = _mm256_loadu_ps (history + lowerIndex * 2u);
            upper = _mm256_loadu_ps (history + upperIndex * 2u);
            lowerIndex += 4u;
            upperIndex += 4u;
        }
        else
        {
            const auto lowerAscending = _mm256_loadu_ps (history + (lowerIndex - 3u) * 2u);
            const auto upperAscending = _mm256_loadu_ps (history + (upperIndex - 3u) * 2u);
            lower = _mm256_permutevar8x32_ps (lowerAscending, reorderReverse);
            upper = _mm256_permutevar8x32_ps (upperAscending, reorderReverse);
            lowerIndex -= 4u;
            upperIndex -= 4u;
        }

        auto target = useFma
            ? _mm256_fmadd_ps (_mm256_sub_ps (upper, lower), fractionVector, lower)
            : _mm256_add_ps (lower, _mm256_mul_ps (_mm256_sub_ps (upper, lower), fractionVector));
        if (previousTail != nullptr)
        {
            const auto firstTailFrame = previousTailPosition + startFrame + offset;
            const auto qBase = _mm256_set1_ps (static_cast<float> (firstTailFrame) * blendScale);
            const auto q = _mm256_add_ps (qBase, blendOffsets);
            const auto old = _mm256_loadu_ps (previousTail
                + static_cast<size_t> (firstTailFrame) * 2u);
            target = useFma
                ? _mm256_fmadd_ps (_mm256_sub_ps (target, old), q, old)
                : _mm256_add_ps (old, _mm256_mul_ps (_mm256_sub_ps (target, old), q));
        }

        if (sanitizeOutput)
        {
            const auto magnitude = _mm256_andnot_ps (signMask, target);
            const auto finite = _mm256_cmp_ps (magnitude, maximumFinite, _CMP_LE_OQ);
            target = _mm256_and_ps (target, finite);
        }
        _mm256_storeu_ps (output + static_cast<size_t> (offset) * 2u, target);
    }
    return true;
}
#else
bool xyPadRenderTailAvx2 (const float*, const float*, float*, size_t, size_t,
                          uint64_t, int, float, int, int, int, float, bool, bool) noexcept
{
    return false;
}
#endif
