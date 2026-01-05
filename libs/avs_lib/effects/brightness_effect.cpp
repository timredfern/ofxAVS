// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "brightness_effect.h"
#include "../core/plugin_manager.h"
#include "../core/ui.h"
#include <algorithm>

namespace avs {

BrightnessEffect::BrightnessEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

// Helper: check if color is within distance of reference color
static inline bool inRange(uint32_t color, uint32_t ref, int distance) {
    int db = std::abs((int)(color & 0xFF) - (int)(ref & 0xFF));
    if (db > distance) return false;
    int dg = std::abs((int)((color >> 8) & 0xFF) - (int)((ref >> 8) & 0xFF));
    if (dg > distance) return false;
    int dr = std::abs((int)((color >> 16) & 0xFF) - (int)((ref >> 16) & 0xFF));
    if (dr > distance) return false;
    return true;
}

// Blend macros matching original AVS
static inline uint32_t BLEND(uint32_t a, uint32_t b) {
    // Additive blend with saturation
    uint32_t r = std::min(255u, ((a >> 16) & 0xFF) + ((b >> 16) & 0xFF));
    uint32_t g = std::min(255u, ((a >> 8) & 0xFF) + ((b >> 8) & 0xFF));
    uint32_t bl = std::min(255u, (a & 0xFF) + (b & 0xFF));
    return (r << 16) | (g << 8) | bl;
}

static inline uint32_t BLEND_AVG(uint32_t a, uint32_t b) {
    // 50/50 average blend
    uint32_t r = (((a >> 16) & 0xFF) + ((b >> 16) & 0xFF)) >> 1;
    uint32_t g = (((a >> 8) & 0xFF) + ((b >> 8) & 0xFF)) >> 1;
    uint32_t bl = ((a & 0xFF) + (b & 0xFF)) >> 1;
    return (r << 16) | (g << 8) | bl;
}

int BrightnessEffect::render(AudioData visdata, int isBeat,
                            uint32_t* framebuffer, uint32_t* fbout,
                            int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;

    // Get slider values (0-8192) and convert to internal values (-4096 to 4096)
    int redp = parameters().get_int("red_adjust") - 4096;
    int greenp = parameters().get_int("green_adjust") - 4096;
    int bluep = parameters().get_int("blue_adjust") - 4096;

    // Calculate multipliers exactly as original:
    // rm = (1 + (redp < 0 ? 1 : 16) * (redp/4096)) * 65536
    int rm = (int)((1 + (redp < 0 ? 1 : 16) * ((float)redp / 4096)) * 65536.0);
    int gm = (int)((1 + (greenp < 0 ? 1 : 16) * ((float)greenp / 4096)) * 65536.0);
    int bm = (int)((1 + (bluep < 0 ? 1 : 16) * ((float)bluep / 4096)) * 65536.0);

    // Build lookup tables
    int red_tab[256], green_tab[256], blue_tab[256];
    for (int n = 0; n < 256; n++) {
        red_tab[n] = (n * rm) & 0xffff0000;
        if (red_tab[n] > 0xff0000) red_tab[n] = 0xff0000;
        if (red_tab[n] < 0) red_tab[n] = 0;

        green_tab[n] = ((n * gm) >> 8) & 0xffff00;
        if (green_tab[n] > 0xff00) green_tab[n] = 0xff00;
        if (green_tab[n] < 0) green_tab[n] = 0;

        blue_tab[n] = ((n * bm) >> 16) & 0xffff;
        if (blue_tab[n] > 0xff) blue_tab[n] = 0xff;
        if (blue_tab[n] < 0) blue_tab[n] = 0;
    }

    auto blend_mode = static_cast<BlendMode>(parameters().get_int("blend_mode"));
    bool blend = (blend_mode == BlendMode::ADDITIVE);
    bool blendavg = (blend_mode == BlendMode::BLEND_5050);
    bool exclude = parameters().get_bool("exclude");
    uint32_t exc_color = parameters().get_color("exclude_color");
    int distance = parameters().get_int("distance");

    int pixel_count = w * h;
    uint32_t* p = framebuffer;

    if (blend) {
        // Additive blend mode
        if (!exclude) {
            for (int i = 0; i < pixel_count; i++) {
                uint32_t pix = p[i];
                p[i] = BLEND(pix, red_tab[(pix >> 16) & 0xff] | green_tab[(pix >> 8) & 0xff] | blue_tab[pix & 0xff]);
            }
        } else {
            for (int i = 0; i < pixel_count; i++) {
                uint32_t pix = p[i];
                if (!inRange(pix, exc_color, distance)) {
                    p[i] = BLEND(pix, red_tab[(pix >> 16) & 0xff] | green_tab[(pix >> 8) & 0xff] | blue_tab[pix & 0xff]);
                }
            }
        }
    } else if (blendavg) {
        // 50/50 blend mode
        if (!exclude) {
            for (int i = 0; i < pixel_count; i++) {
                uint32_t pix = p[i];
                p[i] = BLEND_AVG(pix, red_tab[(pix >> 16) & 0xff] | green_tab[(pix >> 8) & 0xff] | blue_tab[pix & 0xff]);
            }
        } else {
            for (int i = 0; i < pixel_count; i++) {
                uint32_t pix = p[i];
                if (!inRange(pix, exc_color, distance)) {
                    p[i] = BLEND_AVG(pix, red_tab[(pix >> 16) & 0xff] | green_tab[(pix >> 8) & 0xff] | blue_tab[pix & 0xff]);
                }
            }
        }
    } else {
        // Replace mode (default)
        if (!exclude) {
            for (int i = 0; i < pixel_count; i++) {
                uint32_t pix = p[i];
                p[i] = (pix & 0xFF000000) | red_tab[(pix >> 16) & 0xff] | green_tab[(pix >> 8) & 0xff] | blue_tab[pix & 0xff];
            }
        } else {
            for (int i = 0; i < pixel_count; i++) {
                uint32_t pix = p[i];
                if (!inRange(pix, exc_color, distance)) {
                    p[i] = (pix & 0xFF000000) | red_tab[(pix >> 16) & 0xff] | green_tab[(pix >> 8) & 0xff] | blue_tab[pix & 0xff];
                }
            }
        }
    }

    return 0; // Modified in place
}

// Static member definition
const PluginInfo BrightnessEffect::effect_info {
    .name = "Brightness",
    .category = "Trans",
    .description = "",
    .author = "",
    .version = 1,
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<BrightnessEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enable Brightness filter",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 87, .h = 10,
                .default_val = 1
            },
            {
                .id = "red_adjust",
                .text = "Red",
                .type = ControlType::SLIDER,
                .x = 25, .y = 13, .w = 97, .h = 13,
                .range = {0, 8192, 256},
                .default_val = 4096
            },
            {
                .id = "red_reset",
                .text = "><",
                .type = ControlType::BUTTON,
                .x = 125, .y = 12, .w = 12, .h = 14
            },
            {
                .id = "green_adjust",
                .text = "Green",
                .type = ControlType::SLIDER,
                .x = 25, .y = 28, .w = 97, .h = 13,
                .range = {0, 8192, 256},
                .default_val = 4096
            },
            {
                .id = "green_reset",
                .text = "><",
                .type = ControlType::BUTTON,
                .x = 125, .y = 28, .w = 12, .h = 14
            },
            {
                .id = "blue_adjust",
                .text = "Blue",
                .type = ControlType::SLIDER,
                .x = 25, .y = 44, .w = 97, .h = 13,
                .range = {0, 8192, 256},
                .default_val = 4096
            },
            {
                .id = "blue_reset",
                .text = "><",
                .type = ControlType::BUTTON,
                .x = 125, .y = 44, .w = 12, .h = 14
            },
            {
                .id = "dissoc",
                .text = "Dissociate RGB values",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 60, .w = 89, .h = 10,
                .default_val = 0
            },
            {
                .id = "blend_mode",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Replace", 0, 71, 43, 10},
                    {"Additive blend", 0, 81, 61, 10},
                    {"50/50 blend", 0, 92, 55, 10}
                },
                .default_val = 0  // Replace
            },
            {
                .id = "exclude",
                .text = "Exclude color range",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 103, .w = 79, .h = 10,
                .default_val = 0
            },
            {
                .id = "exclude_color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 114, .w = 29, .h = 13,
                .default_val = 0x000000
            },
            {
                .id = "distance",
                .text = "Distance",
                .type = ControlType::SLIDER,
                .x = 31, .y = 114, .w = 106, .h = 13,
                .range = {0, 255, 16},
                .default_val = 16
            }
        }
    }
};

// Register effect at startup
static bool register_brightness = []() {
    PluginManager::instance().register_effect(BrightnessEffect::effect_info);
    return true;
}();

} // namespace avs