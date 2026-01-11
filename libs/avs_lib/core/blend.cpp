// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "blend.h"

namespace avs {

// Global line blend mode - set by Set Render Mode effect
// Format: bit 31 = enabled, bits 0-7 = blend mode, bits 8-15 = alpha, bits 16-23 = line width
int g_line_blend_mode = 0;

} // namespace avs
