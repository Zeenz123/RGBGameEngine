#ifndef PIXELFORMAT_H
#define PIXELFORMAT_H

#include <stdint.h>

enum class PixelFormat
{
    RGB565,
    RGB666,
    RGB888,
    ARGB8888,
    GRAYSCALE,
    MONO
};

// ============================================================================
// Studio-Grade Hardware-Accelerated / Bitwise Blend & Transparency Modes (RGB565)
// ============================================================================
enum class BlendMode : uint8_t
{
    SOLID = 0,       // 0: Standard opaque overwrite
    ALPHA,           // 1: True 32-level linear alpha blend: (src * a + dst * (32 - a)) >> 5
    ADDITIVE,        // 2: Linear dodge / light-emitting glow: min(max, dst + src * a)
    SUBTRACTIVE,     // 3: Energy absorption / dark void: max(0, dst - src * a)
    MULTIPLY,        // 4: Stained-glass / shadow filter: (src * dst) / max
    SCREEN,          // 5: Optical projector soft bloom: 1 - (1 - src) * (1 - dst)
    OVERLAY,         // 6: High-contrast Multiply/Screen hybrid (darks darker, lights brighter)
    SOFT_LIGHT,      // 7: Diffused studio warm contrast lighting
    HARD_LIGHT,      // 8: Intense specular spotlight contrast
    COLOR_DODGE,     // 9: Extreme highlight blowout: dst / (1 - src)
    COLOR_BURN,      // 10: Deep cinema shadow burn: 1 - (1 - dst) / src
    DIFFERENCE,      // 11: X-Ray / solarized absolute difference: |src - dst|
    EXCLUSION,       // 12: Soft photographic exclusion: src + dst - 2 * src * dst
    COLOR_TINT,      // 13: Bitwise channel mask + tint boost
    LIGHTEN,         // 14: Per-channel max(src, dst)
    DARKEN,          // 15: Per-channel min(src, dst)
    CHECKER_STIPPLE  // 16: Ultra-fast screen-door / Bayer stipple transparency
};

#endif
