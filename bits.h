#pragma once

#include "collatypes.h"

uint32 bitsCtz(uint32 v);
uint32 bitsNextPow2(uint32 v);

// == INLINE IMPLEMENTATION ==============================================================

#if COLLA_MSVC
#define BITS_WIN 1
#define BITS_LIN 0
#include <intrin.h>
#elif COLLA_GCC || COLLA_CLANG || COLLA_EMC
#define BITS_WIN 0
#define BITS_LIN 1
#elif COLLA_TCC
#define BITS_WIN 0
#define BITS_LIN 0
#else
#error "bits header not supported on this compiler"
#endif

#include "tracelog.h"

inline uint32 bitsCtz(uint32 v) {
#if BITS_LIN
    return v ? __builtin_ctz(v) : 0;
#elif BITS_WIN
    uint32 trailing = 0;
    return _BitScanForward((unsigned long *)&trailing, v) ? trailing : 32;
#else
    if (v == 0) return 0;
    for (uint32 i = 0; i < 32; ++i) {
        if (v & (1 << i)) return i;
    }
    return 32;
#endif
}

inline uint32 bitsNextPow2(uint32 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

#undef BITS_WIN
#undef BITS_LIN
