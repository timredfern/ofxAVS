// avs_lib - Portable Advanced Visualization Studio library
// Extended blend support - NOT part of original AVS
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Extensions to original AVS blend:
//   - Line style flags for AA, angle correction, rounded endpoints
//   - Point size flag for variable-size points

#pragma once

#include "blend.h"

namespace avs {

// Line style flags (bits 24-27 of g_line_blend_mode)
// These are extensions - not in original AVS
constexpr int LINE_STYLE_AA           = 0x01;  // Anti-aliased (Wu's algorithm)
constexpr int LINE_STYLE_ANGLE_CORRECT = 0x02;  // Angle-corrected thickness
constexpr int LINE_STYLE_ROUNDED      = 0x04;  // Rounded endpoints
constexpr int LINE_STYLE_POINTSIZE    = 0x08;  // Apply line width to points

// Helper to extract line style from g_line_blend_mode
inline int get_line_style() {
    return (g_line_blend_mode >> 24) & 0x0F;
}

} // namespace avs
