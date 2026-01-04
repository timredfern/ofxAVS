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

} // namespace avs
