#include "XYPadLiveFFT1024.h"
#include "XYPadLiveFFT1024AVX2.h"
#include "XYPadTailAVX2.h"
#include <cmath>
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
 #include <emmintrin.h>
 #define XY_LIVE_FFT_SSE2 1
#endif

XYPadLiveFFT1024::XYPadLiveFFT1024() noexcept
{
    for (int i = 0; i < size; ++i)
    {
        unsigned value = static_cast<unsigned> (i), result = 0;
        for (int bit = 0; bit < 10; ++bit) { result = (result << 1u) | (value & 1u); value >>= 1u; }
        reversed[static_cast<size_t> (i)] = static_cast<uint16_t> (result);
    }
    int offset = 0;
    for (int half = 1; half < size; half *= 2)
    {
        for (int k = 0; k < half; ++k)
        {
            const auto angle = -3.1415926535897932384626433832795 * k / half;
            twiddleReal[static_cast<size_t> (offset + k)] = static_cast<float> (std::cos (angle));
            twiddleImag[static_cast<size_t> (offset + k)] = static_cast<float> (std::sin (angle));
        }
        offset += half;
    }
}

void XYPadLiveFFT1024::transform (bool inverse) noexcept
{
    if (xyPadTailAvx2Available()
        && xyPadLiveFftTransformAvx2 (real.data(), imag.data(), twiddleReal.data(),
                                       twiddleImag.data(), inverse))
        return;

    int offset = 0;
    for (int half = 1; half < size; half *= 2)
    {
        for (int base = 0; base < size; base += 2 * half)
        {
            int k = 0;
#if defined(XY_LIVE_FFT_SSE2)
            for (; k + 4 <= half; k += 4)
            {
                const auto lo = static_cast<size_t> (base + k), hi = lo + static_cast<size_t> (half);
                const auto wr = _mm_loadu_ps (twiddleReal.data() + offset + k);
                auto wi = _mm_loadu_ps (twiddleImag.data() + offset + k);
                if (inverse) wi = _mm_sub_ps (_mm_setzero_ps(), wi);
                const auto hr = _mm_loadu_ps (real.data() + hi), ii = _mm_loadu_ps (imag.data() + hi);
                const auto tr = _mm_sub_ps (_mm_mul_ps (hr, wr), _mm_mul_ps (ii, wi));
                const auto ti = _mm_add_ps (_mm_mul_ps (hr, wi), _mm_mul_ps (ii, wr));
                const auto lr = _mm_loadu_ps (real.data() + lo), li = _mm_loadu_ps (imag.data() + lo);
                _mm_storeu_ps (real.data() + lo, _mm_add_ps (lr, tr));
                _mm_storeu_ps (imag.data() + lo, _mm_add_ps (li, ti));
                _mm_storeu_ps (real.data() + hi, _mm_sub_ps (lr, tr));
                _mm_storeu_ps (imag.data() + hi, _mm_sub_ps (li, ti));
            }
#endif
            for (; k < half; ++k)
            {
                const auto lo = static_cast<size_t> (base + k), hi = lo + static_cast<size_t> (half);
                const auto wr = twiddleReal[static_cast<size_t> (offset + k)];
                const auto wi = twiddleImag[static_cast<size_t> (offset + k)] * (inverse ? -1.0f : 1.0f);
                const auto tr = real[hi] * wr - imag[hi] * wi;
                const auto ti = real[hi] * wi + imag[hi] * wr;
                const auto lr = real[lo], li = imag[lo];
                real[lo] = lr + tr; imag[lo] = li + ti;
                real[hi] = lr - tr; imag[hi] = li - ti;
            }
        }
        offset += half;
    }
}

void XYPadLiveFFT1024::performRealOnlyForwardTransform (float* data) noexcept
{
    if (data == nullptr) return;
    for (int i = 0; i < size; ++i)
    {
        const auto at = reversed[static_cast<size_t> (i)];
        real[at] = data[i]; imag[at] = 0.0f;
    }
    transform (false);
    for (int i = 0; i < size; ++i) { data[2 * i] = real[static_cast<size_t> (i)]; data[2 * i + 1] = imag[static_cast<size_t> (i)]; }
}

void XYPadLiveFFT1024::performRealOnlyInverseTransform (float* data) noexcept
{
    if (data == nullptr) return;
    for (int i = 0; i < size; ++i)
    {
        const auto source = i >= size / 2 ? size - i : i;
        const auto at = reversed[static_cast<size_t> (i)];
        real[at] = data[2 * source];
        imag[at] = data[2 * source + 1] * (i >= size / 2 ? -1.0f : 1.0f);
    }
    transform (true);
    constexpr auto scale = 1.0f / static_cast<float> (size);
    for (int i = 0; i < size; ++i) { data[i] = real[static_cast<size_t> (i)] * scale; data[i + size] = imag[static_cast<size_t> (i)] * scale; }
}
