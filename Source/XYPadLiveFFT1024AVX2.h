#pragma once

// Kept in a separately compiled AVX2 translation unit. Callers must first
// confirm xyPadTailAvx2Available(); the ordinary FFT file remains safe on
// CPUs without AVX2.
bool xyPadLiveFftTransformAvx2 (float* real, float* imag,
                                 const float* twiddleReal, const float* twiddleImag,
                                 bool inverse) noexcept;
