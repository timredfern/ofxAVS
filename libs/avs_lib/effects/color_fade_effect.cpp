// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "color_fade_effect.h"
#include "core/plugin_manager.h"
#include <algorithm>
#include <cstdlib>

namespace avs {

ColorFadeEffect::ColorFadeEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    init_tables();
}

void ColorFadeEffect::init_tables() {
    // Build color category lookup table
    // Maps (g-b, b-r) to one of 4 fade categories
    for (int x = 0; x < 512; x++) {
        for (int y = 0; y < 512; y++) {
            int xp = x - 255;  // g - b
            int yp = y - 255;  // b - r

            if (xp > 0 && xp > -yp) {
                // g > b AND g > r: green dominant
                c_tab_[x][y] = 0;
            } else if (yp < 0 && xp < -yp) {
                // r > b AND r > g: red dominant
                c_tab_[x][y] = 1;
            } else if (xp < 0 && yp > 0) {
                // g < b AND b > r: blue dominant
                c_tab_[x][y] = 2;
            } else {
                // Fallback
                c_tab_[x][y] = 3;
            }
        }
    }

    // Build clipping table with 40-unit padding on each side
    for (int x = 0; x < 256 + 40 + 40; x++) {
        clip_[x] = static_cast<unsigned char>(std::min(std::max(x - 40, 0), 255));
    }
}

int ColorFadeEffect::render(AudioData visdata, int isBeat,
                           uint32_t* framebuffer, uint32_t* fbout,
                           int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;

    int faders[3] = {
        parameters().get_int("fade_red"),
        parameters().get_int("fade_green"),
        parameters().get_int("fade_blue")
    };

    bool onbeat_change = parameters().get_bool("onbeat_change");
    bool onbeat_random = parameters().get_bool("onbeat_random");

    if (!onbeat_change) {
        faderpos_[0] = faders[0];
        faderpos_[1] = faders[1];
        faderpos_[2] = faders[2];
    } else if (isBeat && onbeat_random) {
        faderpos_[0] = (rand() % 32) - 6;
        faderpos_[1] = (rand() % 64) - 32;
        if (faderpos_[1] < 0 && faderpos_[1] > -16) faderpos_[1] = -32;
        if (faderpos_[1] >= 0 && faderpos_[1] < 16) faderpos_[1] = 32;
        faderpos_[2] = (rand() % 32) - 6;
    } else if (isBeat) {
        faderpos_[0] = parameters().get_int("beat_fade_red");
        faderpos_[1] = parameters().get_int("beat_fade_green");
        faderpos_[2] = parameters().get_int("beat_fade_blue");
    } else {
        if (faderpos_[0] < faders[0]) faderpos_[0]++;
        if (faderpos_[1] < faders[2]) faderpos_[1]++;
        if (faderpos_[2] < faders[1]) faderpos_[2]++;
        if (faderpos_[0] > faders[0]) faderpos_[0]--;
        if (faderpos_[1] > faders[2]) faderpos_[1]--;
        if (faderpos_[2] > faders[1]) faderpos_[2]--;
    }

    int fs1 = faderpos_[0];
    int fs2 = faderpos_[1];
    int fs3 = faderpos_[2];

    int ft[4][3];
    ft[0][0] = fs3; ft[0][1] = fs2; ft[0][2] = fs1;
    ft[1][0] = fs2; ft[1][1] = fs1; ft[1][2] = fs3;
    ft[2][0] = fs1; ft[2][1] = fs3; ft[2][2] = fs2;
    ft[3][0] = fs3; ft[3][1] = fs3; ft[3][2] = fs3;

    unsigned char* ctab_ptr = &c_tab_[0][0] + 255 + (255 * 512);
    unsigned char* clip_ptr = clip_ + 40;

    // Process pixels - original uses byte access q[0],q[1],q[2] preserving q[3] (alpha)
    // Our pixel format: 0xAARRGGBB = (A<<24)|(R<<16)|(G<<8)|B
    // In little-endian memory: [B, G, R, A] - so q[0]=B, q[1]=G, q[2]=R
    // Original names them r=q[0], g=q[1], b=q[2] (confusingly B,G,R)
    for (int i = 0; i < w * h; i++) {
        uint32_t pix = framebuffer[i];

        // Extract bytes (matching original's q[0],q[1],q[2] naming)
        int r = pix & 0xFF;           // Actually B
        int g = (pix >> 8) & 0xFF;    // G
        int b = (pix >> 16) & 0xFF;   // Actually R

        // Category lookup: ((g-b)<<9) + b - r
        int idx = ((g - b) << 9) + b - r;
        int cat = ctab_ptr[idx];

        // Apply faders based on category
        int new_r = clip_ptr[r + ft[cat][0]];
        int new_g = clip_ptr[g + ft[cat][1]];
        int new_b = clip_ptr[b + ft[cat][2]];

        // Write back preserving alpha (original doesn't touch q[3])
        framebuffer[i] = (pix & 0xFF000000) | (new_b << 16) | (new_g << 8) | new_r;
    }

    return 0;
}

// Static member definition
const PluginInfo ColorFadeEffect::effect_info {
    .name = "Color Fade",
    .category = "Trans",
    .description = "",
    .author = "",
    .version = 1,
    .legacy_index = 11,  // R_ColorFade
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<ColorFadeEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enable colorfade",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 69, .h = 10,
                .default_val = 1
            },
            {
                .id = "fade_red",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 0, .y = 11, .w = 100, .h = 14,
                .range = {-32, 32, 1},
                .default_val = 8
            },
            {
                .id = "fade_green",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 0, .y = 28, .w = 100, .h = 14,
                .range = {-32, 32, 1},
                .default_val = -8
            },
            {
                .id = "fade_blue",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 0, .y = 46, .w = 100, .h = 14,
                .range = {-32, 32, 1},
                .default_val = -8
            },
            {
                .id = "onbeat_change",
                .text = "OnBeat Change",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 67, .w = 67, .h = 10,
                .default_val = 0
            },
            {
                .id = "onbeat_random",
                .text = "OnBeat Randomize",
                .type = ControlType::CHECKBOX,
                .x = 9, .y = 79, .w = 73, .h = 10,
                .default_val = 0
            },
            {
                .id = "beat_fade_red",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 7, .y = 90, .w = 100, .h = 14,
                .range = {-32, 32, 1},
                .default_val = 8
            },
            {
                .id = "beat_fade_green",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 7, .y = 107, .w = 100, .h = 14,
                .range = {-32, 32, 1},
                .default_val = -8
            },
            {
                .id = "beat_fade_blue",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 7, .y = 125, .w = 100, .h = 14,
                .range = {-32, 32, 1},
                .default_val = -8
            }
        }
    }
};

// Register effect at startup
static bool register_colorfade = []() {
    PluginManager::instance().register_effect(ColorFadeEffect::effect_info);
    return true;
}();

} // namespace avs
