#include "XYPadLiveFFT1024AVX2.h"

#if defined(_MSC_VER) || defined(__x86_64__) || defined(__i386__)
 #include <immintrin.h>
#endif

bool xyPadLiveFftTransformAvx2 (float* real, float* imag,
                                 const float* twiddleReal, const float* twiddleImag,
                                 bool inverse) noexcept
{
#if defined(_MSC_VER) || defined(__AVX2__)
    if (real == nullptr || imag == nullptr || twiddleReal == nullptr || twiddleImag == nullptr)
        return false;

    int offset = 0;
    constexpr int size = 1024;
    for (int half = 1; half < size; half *= 2)
    {
        for (int base = 0; base < size; base += 2 * half)
        {
            int k = 0;
            for (; k + 8 <= half; k += 8)
            {
                const auto lo = base + k, hi = lo + half;
                const auto wr = _mm256_loadu_ps (twiddleReal + offset + k);
                auto wi = _mm256_loadu_ps (twiddleImag + offset + k);
                if (inverse) wi = _mm256_sub_ps (_mm256_setzero_ps(), wi);
                const auto hr = _mm256_loadu_ps (real + hi), ii = _mm256_loadu_ps (imag + hi);
                const auto tr = _mm256_sub_ps (_mm256_mul_ps (hr, wr), _mm256_mul_ps (ii, wi));
                const auto ti = _mm256_add_ps (_mm256_mul_ps (hr, wi), _mm256_mul_ps (ii, wr));
                const auto lr = _mm256_loadu_ps (real + lo), li = _mm256_loadu_ps (imag + lo);
                _mm256_storeu_ps (real + lo, _mm256_add_ps (lr, tr));
                _mm256_storeu_ps (imag + lo, _mm256_add_ps (li, ti));
                _mm256_storeu_ps (real + hi, _mm256_sub_ps (lr, tr));
                _mm256_storeu_ps (imag + hi, _mm256_sub_ps (li, ti));
            }
            for (; k < half; ++k)
            {
                const auto lo = base + k, hi = lo + half;
                const auto wr = twiddleReal[offset + k];
                const auto wi = twiddleImag[offset + k] * (inverse ? -1.0f : 1.0f);
                const auto tr = real[hi] * wr - imag[hi] * wi;
                const auto ti = real[hi] * wi + imag[hi] * wr;
                const auto lr = real[lo], li = imag[lo];
                real[lo] = lr + tr; imag[lo] = li + ti;
                real[hi] = lr - tr; imag[hi] = li - ti;
            }
        }
        offset += half;
    }
    return true;
#else
    (void) real; (void) imag; (void) twiddleReal; (void) twiddleImag; (void) inverse;
    return false;
#endif
}
