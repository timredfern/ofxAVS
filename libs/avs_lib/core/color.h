// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include <cstdint>

namespace avs {

/**
 * Color format utilities for AVS
 *
 * Internal format: ABGR (0xAABBGGRR)
 * - Alpha in bits 24-31
 * - Blue in bits 16-23
 * - Green in bits 8-15
 * - Red in bits 0-7
 *
 * This matches Windows COLORREF (0x00BBGGRR) with alpha added.
 * It also matches OF_PIXELS_BGRA on little-endian systems.
 *
 * JSON format: ARGB (0xAARRGGBB)
 * - Standard hex color notation for human readability
 * - Orange is #FFFF8000, not #FF0080FF
 */
namespace color {

// Standard colors in internal ABGR format
constexpr uint32_t RED   = 0xFF0000FF;  // A=FF, B=00, G=00, R=FF
constexpr uint32_t GREEN = 0xFF00FF00;  // A=FF, B=00, G=FF, R=00
constexpr uint32_t BLUE  = 0xFFFF0000;  // A=FF, B=FF, G=00, R=00
constexpr uint32_t WHITE = 0xFFFFFFFF;
constexpr uint32_t BLACK = 0xFF000000;

// Extract components from internal ABGR format
inline uint8_t red(uint32_t color)   { return color & 0xFF; }
inline uint8_t green(uint32_t color) { return (color >> 8) & 0xFF; }
inline uint8_t blue(uint32_t color)  { return (color >> 16) & 0xFF; }
inline uint8_t alpha(uint32_t color) { return (color >> 24) & 0xFF; }

// Build internal ABGR color from RGB(A) components
inline uint32_t make(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return static_cast<uint32_t>(r) |
           (static_cast<uint32_t>(g) << 8) |
           (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(a) << 24);
}

// ============================================================================
// Format conversions
// ============================================================================

/**
 * Add alpha to Windows BGR color for internal storage.
 * Use when loading colors from binary AVS presets.
 *
 * Input:  Windows COLORREF (0x00BBGGRR)
 * Output: Internal ABGR (0xAABBGGRR) with A=FF
 */
inline uint32_t bgr_add_alpha(uint32_t bgr) {
    return 0xFF000000 | bgr;
}

/**
 * Convert internal ABGR to standard ARGB.
 * Use when outputting colors to JSON for human readability.
 *
 * Input:  Internal ABGR (0xAABBGGRR)
 * Output: Standard ARGB (0xAARRGGBB)
 */
inline uint32_t abgr_to_argb(uint32_t abgr) {
    uint32_t a = (abgr >> 24) & 0xFF;
    uint32_t b = (abgr >> 16) & 0xFF;
    uint32_t g = (abgr >> 8) & 0xFF;
    uint32_t r = abgr & 0xFF;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

/**
 * Convert standard ARGB to internal ABGR.
 * Use when loading colors from JSON.
 *
 * Input:  Standard ARGB (0xAARRGGBB)
 * Output: Internal ABGR (0xAABBGGRR)
 */
inline uint32_t argb_to_abgr(uint32_t argb) {
    uint32_t a = (argb >> 24) & 0xFF;
    uint32_t r = (argb >> 16) & 0xFF;
    uint32_t g = (argb >> 8) & 0xFF;
    uint32_t b = argb & 0xFF;
    return (a << 24) | (b << 16) | (g << 8) | r;
}

} // namespace color
} // namespace avs
