#pragma once
#include <array>
#include <cstdint>
#include <juce_dsp/juce_dsp.h>

// One audio-thread owner per prepared engine; no shared mutable state or locks.
class XYPadLiveFFT1024 final
{
public:
    static constexpr int size = 1024;
    XYPadLiveFFT1024() noexcept;
    void performRealOnlyForwardTransform (float* data) noexcept;
    // Avoids an intermediate time-domain buffer for the live stereo path.
    // The caller owns a power-of-two interleaved input ring and receives the
    // same packed spectrum as performRealOnlyForwardTransform().
    void performWindowedRingForwardTransform (const float* interleavedInput,
                                              uint64_t firstSerial, int channel,
                                              const float* window, float* destination) noexcept;
    void performRealOnlyInverseTransform (float* data) noexcept;
private:
    void transform (bool inverse) noexcept;
    std::array<uint16_t, size> reversed {};
    std::array<float, size - 1> twiddleReal {}, twiddleImag {};
    std::array<float, size> real {}, imag {};
    juce::dsp::FFT realFft { 10 };
    bool avx2Available = false;
};
