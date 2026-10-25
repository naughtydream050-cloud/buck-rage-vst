#pragma once
#include <array>
#include <cstdint>

// One audio-thread owner per prepared engine; no shared mutable state or locks.
class XYPadLiveFFT1024 final
{
public:
    static constexpr int size = 1024;
    XYPadLiveFFT1024() noexcept;
    void performRealOnlyForwardTransform (float* data) noexcept;
    void performRealOnlyInverseTransform (float* data) noexcept;
private:
    void transform (bool inverse) noexcept;
    std::array<uint16_t, size> reversed {};
    std::array<float, size - 1> twiddleReal {}, twiddleImag {};
    std::array<float, size> real {}, imag {};
};
