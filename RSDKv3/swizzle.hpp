#ifndef __SWIZZLE_H__
#define __SWIZZLE_H__

#include "RetroEngine.hpp"

// MortonInterleave() and GetMortonOffset() borrowed from Citra:
// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version

static constexpr u32 MortonInterleave(u32 x, u32 y) {
    constexpr u32 xlut[] = {0x00, 0x01, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15};
    constexpr u32 ylut[] = {0x00, 0x02, 0x08, 0x0a, 0x20, 0x22, 0x28, 0x2a};
    return xlut[x % 8] + ylut[y % 8];
}

static inline u32 GetMortonOffset(u32 x, u32 y, u32 bytes_per_pixel) {
    const unsigned int block_height = 8;
    const unsigned int coarse_x = x & ~7;

    u32 i = MortonInterleave(x, y);

    const unsigned int offset = coarse_x * block_height;

    return (i + offset) * bytes_per_pixel;
}

static u32 GetPixelOffset(u32 x, u32 y, u32 width, u32 height, u32 bytes_per_pixel) {
    y = height - 1 - y;
    u32 coarse_y = y & ~7;
    return GetMortonOffset(x, y, 2) + coarse_y * width * 2;
}

static void SwizzleTexBuffer(ushort *in, ushort *out, u32 w, u32 h, u32 bytes_pp) {
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            *((ushort*)((u8*)out + GetPixelOffset(x, y, w, h, bytes_pp))) = in[(y * w) + x];
        }
    }
}

#endif
