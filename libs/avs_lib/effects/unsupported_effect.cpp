// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "unsupported_effect.h"

namespace avs {

// Legacy effect index to name mapping
// Based on initfx() order in original rlib.cpp
static const char* legacy_effect_names[] = {
    "Simple (Oscilloscope)",    // 0  - R_SimpleSpectrum
    "Dot Plane",                // 1  - R_DotPlane
    "Oscilloscope Star",        // 2  - R_OscStars
    "Fadeout",                  // 3  - R_FadeOut
    "Convolution Filter",       // 4  - R_BlitterFB
    "OnBeat Clear",             // 5  - R_NFClear
    "Blur",                     // 6  - R_Blur
    "Bass Spin",                // 7  - R_BSpin
    "Moving Particle",          // 8  - R_Parts
    "Roto Blitter",             // 9  - R_RotBlit
    "SVP",                      // 10 - R_SVP
    "Color Fade",               // 11 - R_ColorFade
    "Contrast",                 // 12 - R_ContrastEnhance
    "Rotating Stars",           // 13 - R_RotStar
    "Ring",                     // 14 - R_OscRings
    "Movement",                 // 15 - R_Trans
    "Scatter",                  // 16 - R_Scat
    "Dot Grid",                 // 17 - R_DotGrid
    "Stack",                    // 18 - R_Stack
    "Dot Fountain",             // 19 - R_DotFountain
    "Water",                    // 20 - R_Water
    "Comment",                  // 21 - R_Comment
    "Brightness",               // 22 - R_Brightness
    "Interleave",               // 23 - R_Interleave
    "Grain",                    // 24 - R_Grain
    "Clear",                    // 25 - R_Clear
    "Mirror",                   // 26 - R_Mirror
    "Starfield",                // 27 - R_StarField
    "Text",                     // 28 - R_Text
    "Bump",                     // 29 - R_Bump
    "Mosaic",                   // 30 - R_Mosaic
    "Water Bump",               // 31 - R_WaterBump
    "AVI",                      // 32 - R_AVI
    "BPM",                      // 33 - R_Bpm
    "Picture",                  // 34 - R_Picture
    "Dynamic Distance Modifier",// 35 - R_DDM
    "SuperScope",               // 36 - R_SScope
    "Invert",                   // 37 - R_Invert
    "Unique Tone",              // 38 - R_Onetone
    "Timescope",                // 39 - R_Timescope
    "Set Render Mode",          // 40 - R_LineMode
    "Interferences",            // 41 - R_Interferences
    "Shift",                    // 42 - R_Shift
    "Dynamic Movement",         // 43 - R_DMove
    "Fast Brightness",          // 44 - R_FastBright
    "Color Modifier",           // 45 - R_DColorMod
};

static constexpr int NUM_LEGACY_EFFECTS = sizeof(legacy_effect_names) / sizeof(legacy_effect_names[0]);

const char* get_legacy_effect_name(int legacy_index) {
    if (legacy_index >= 0 && legacy_index < NUM_LEGACY_EFFECTS) {
        return legacy_effect_names[legacy_index];
    }
    return nullptr;
}

} // namespace avs
