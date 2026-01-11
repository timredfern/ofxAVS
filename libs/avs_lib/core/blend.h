// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root

// Blend operations ported from r_defs.h

#pragma once

#include <cstdint>
#include <algorithm>

namespace avs {

// Additive blend with saturation (clamps to 255 per channel)
static inline uint32_t BLEND(uint32_t a, uint32_t b) {
    uint32_t r, t;
    r = (a & 0xff) + (b & 0xff);
    t = std::min(r, 0xffu);
    r = (a & 0xff00) + (b & 0xff00);
    t |= std::min(r, 0xff00u);
    r = (a & 0xff0000) + (b & 0xff0000);
    t |= std::min(r, 0xff0000u);
    r = (a & 0xff000000) + (b & 0xff000000);
    return t | std::min(r, 0xff000000u);
}

// 50/50 average blend - uses original AVS bit manipulation trick
static inline uint32_t BLEND_AVG(uint32_t a, uint32_t b) {
    return ((a >> 1) & ~((1 << 7) | (1 << 15) | (1 << 23))) +
           ((b >> 1) & ~((1 << 7) | (1 << 15) | (1 << 23)));
}

// Maximum of each channel
static inline uint32_t BLEND_MAX(uint32_t a, uint32_t b) {
    uint32_t t;
    t = std::max(a & 0xff, b & 0xff);
    t |= std::max(a & 0xff00, b & 0xff00);
    t |= std::max(a & 0xff0000, b & 0xff0000);
    return t;
}

// Minimum of each channel
static inline uint32_t BLEND_MIN(uint32_t a, uint32_t b) {
    uint32_t t;
    t = std::min(a & 0xff, b & 0xff);
    t |= std::min(a & 0xff00, b & 0xff00);
    t |= std::min(a & 0xff0000, b & 0xff0000);
    return t;
}

// Subtractive blend with clamping to 0
static inline uint32_t BLEND_SUB(uint32_t a, uint32_t b) {
    int r, t;
    r = (a & 0xff) - (b & 0xff);
    t = std::max(r, 0);
    r = (a & 0xff00) - (b & 0xff00);
    t |= std::max(r, 0);
    r = (a & 0xff0000) - (b & 0xff0000);
    t |= std::max(r, 0);
    return t;
}

// Adjustable alpha blend: blends a over b with alpha (0-255)
// alpha=0 returns b, alpha=255 returns a
static inline uint32_t BLEND_ADJ(uint32_t a, uint32_t b, int alpha) {
    int inv_alpha = 256 - alpha;
    uint32_t r = (((a & 0xff) * alpha) + ((b & 0xff) * inv_alpha)) >> 8;
    uint32_t g = ((((a >> 8) & 0xff) * alpha) + (((b >> 8) & 0xff) * inv_alpha)) >> 8;
    uint32_t bl = ((((a >> 16) & 0xff) * alpha) + (((b >> 16) & 0xff) * inv_alpha)) >> 8;
    return (r & 0xff) | ((g & 0xff) << 8) | ((bl & 0xff) << 16);
}

// Multiply blend - per-channel multiply (result = (a * b) / 256)
static inline uint32_t BLEND_MUL(uint32_t a, uint32_t b) {
    uint32_t r = ((a & 0xff) * (b & 0xff)) >> 8;
    uint32_t g = (((a >> 8) & 0xff) * ((b >> 8) & 0xff)) >> 8;
    uint32_t bl = (((a >> 16) & 0xff) * ((b >> 16) & 0xff)) >> 8;
    return (r & 0xff) | ((g & 0xff) << 8) | ((bl & 0xff) << 16);
}

// Global line blend mode - set by Set Render Mode effect
// Format: bit 31 = enabled, bits 0-7 = blend mode, bits 8-15 = alpha, bits 16-23 = line width
extern int g_line_blend_mode;

// BLEND_LINE - writes a pixel using the current global line blend mode
// Blend modes:
//   0 = Replace
//   1 = Additive
//   2 = Maximum
//   3 = 50/50
//   4 = Subtractive 1 (fb - color)
//   5 = Subtractive 2 (color - fb)
//   6 = Multiply
//   7 = Adjustable (uses alpha from bits 8-15)
//   8 = XOR
//   9 = Minimum
static inline void BLEND_LINE(uint32_t* fb, uint32_t color) {
    if (!(g_line_blend_mode & 0x80000000)) {
        // Mode not enabled - use replace
        *fb = color;
        return;
    }

    switch (g_line_blend_mode & 0xff) {
        case 1: *fb = BLEND(*fb, color); break;           // Additive
        case 2: *fb = BLEND_MAX(*fb, color); break;       // Maximum
        case 3: *fb = BLEND_AVG(*fb, color); break;       // 50/50
        case 4: *fb = BLEND_SUB(*fb, color); break;       // Sub 1 (fb - color)
        case 5: *fb = BLEND_SUB(color, *fb); break;       // Sub 2 (color - fb)
        case 6: *fb = BLEND_MUL(*fb, color); break;       // Multiply
        case 7: *fb = BLEND_ADJ(*fb, color, (g_line_blend_mode >> 8) & 0xff); break;  // Adjustable
        case 8: *fb = *fb ^ color; break;                 // XOR
        case 9: *fb = BLEND_MIN(*fb, color); break;       // Minimum
        default: *fb = color; break;                      // Replace (case 0)
    }
}

// Bilinear interpolation of a 2x2 pixel block
// src points to top-left pixel, w is stride
// lerp_x and lerp_y are 0-255 interpolation factors (fractional position * 256)
static inline uint32_t blend_bilinear_2x2(const uint32_t* src, int w,
                                          uint8_t lerp_x, uint8_t lerp_y) {
    // Get the 4 source pixels
    uint32_t p00 = src[0];       // top-left
    uint32_t p10 = src[1];       // top-right
    uint32_t p01 = src[w];       // bottom-left
    uint32_t p11 = src[w + 1];   // bottom-right

    // Inverse lerp factors
    uint32_t inv_x = 256 - lerp_x;
    uint32_t inv_y = 256 - lerp_y;

    // Interpolate each channel separately
    // Red (bits 0-7)
    uint32_t r = ((p00 & 0xff) * inv_x * inv_y +
                  (p10 & 0xff) * lerp_x * inv_y +
                  (p01 & 0xff) * inv_x * lerp_y +
                  (p11 & 0xff) * lerp_x * lerp_y) >> 16;

    // Green (bits 8-15)
    uint32_t g = (((p00 >> 8) & 0xff) * inv_x * inv_y +
                  ((p10 >> 8) & 0xff) * lerp_x * inv_y +
                  ((p01 >> 8) & 0xff) * inv_x * lerp_y +
                  ((p11 >> 8) & 0xff) * lerp_x * lerp_y) >> 16;

    // Blue (bits 16-23)
    uint32_t b = (((p00 >> 16) & 0xff) * inv_x * inv_y +
                  ((p10 >> 16) & 0xff) * lerp_x * inv_y +
                  ((p01 >> 16) & 0xff) * inv_x * lerp_y +
                  ((p11 >> 16) & 0xff) * lerp_x * lerp_y) >> 16;

    return (r & 0xff) | ((g & 0xff) << 8) | ((b & 0xff) << 16);
}

} // namespace avs
