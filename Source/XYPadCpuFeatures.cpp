#include "XYPadTailAVX2.h"

#if defined(_WIN32)
 #include <windows.h>
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
