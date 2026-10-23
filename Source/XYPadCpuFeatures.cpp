#include "XYPadTailAVX2.h"

#if defined(_WIN32)
 #include <windows.h>
 #include <intrin.h>
#elif defined(__x86_64__) || defined(__i386__)
 #include <cpuid.h>
#endif

bool xyPadTailAvx2Available() noexcept
{
#if defined(_WIN32)
    // PF_AVX2_INSTRUCTIONS_AVAILABLE (40) includes OS AVX-state support.
    return IsProcessorFeaturePresent (40) != FALSE;
#elif defined(__x86_64__) || defined(__i386__)
    return __builtin_cpu_supports ("avx2");
#else
    return false;
#endif
}

bool xyPadTailFmaAvailable() noexcept
{
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    int registers[4] {};
    __cpuid (registers, 1);
    return (registers[2] & (1 << 12)) != 0;
#elif defined(__x86_64__) || defined(__i386__)
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    return __get_cpuid (1, &eax, &ebx, &ecx, &edx) != 0 && (ecx & bit_FMA) != 0;
#else
    return false;
#endif
}
