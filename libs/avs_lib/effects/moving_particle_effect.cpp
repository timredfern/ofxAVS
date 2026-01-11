// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "moving_particle_effect.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace avs {

MovingParticleEffect::MovingParticleEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    s_pos_ = parameters().get_int("size");
}

int MovingParticleEffect::render(AudioData visdata, int isBeat,
                                  uint32_t* framebuffer, uint32_t* fbout,
                                  int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;

    uint32_t color = parameters().get_color("color");
    int maxdist = parameters().get_int("maxdist");
    int size = parameters().get_int("size");
    int size2 = parameters().get_int("size2");
    int blend = parameters().get_int("blend_mode");
    bool onbeat_size = parameters().get_bool("onbeat_size");

    int ss = std::min(h / 2, (w * 3) / 8);

    // On beat: set random target position
    if (isBeat) {
        c_[0] = ((rand() % 33) - 16) / 48.0;
        c_[1] = ((rand() % 33) - 16) / 48.0;
    }

    // Physics simulation: accelerate toward target, apply damping
    v_[0] -= 0.004 * (p_[0] - c_[0]);
    v_[1] -= 0.004 * (p_[1] - c_[1]);

    p_[0] += v_[0];
    p_[1] += v_[1];

    v_[0] *= 0.991;
    v_[1] *= 0.991;

    // Calculate screen position
    int xp = (int)(p_[0] * ss * (maxdist / 32.0)) + w / 2;
    int yp = (int)(p_[1] * ss * (maxdist / 32.0)) + h / 2;

    // On beat size change
    if (isBeat && onbeat_size) {
        s_pos_ = size2;
    }

    int sz = s_pos_;
    s_pos_ = (s_pos_ + size) / 2;  // Interpolate toward target size

    // Single pixel case
    if (sz <= 1) {
        if (xp >= 0 && yp >= 0 && xp < w && yp < h) {
            uint32_t* pixel = &framebuffer[xp + yp * w];
            if (blend == 0) {
                *pixel = color;
            } else if (blend == 2) {
                *pixel = BLEND_AVG(*pixel, color);
            } else {
                // blend == 1 or 3: additive
                *pixel = BLEND(*pixel, color);
            }
        }
        return 0;
    }

    // Clamp size
    if (sz > 128) sz = 128;

    // Draw filled circle
    double md = sz * sz * 0.25;
    int start_y = yp - sz / 2;

    for (int y = 0; y < sz; y++) {
        int screen_y = start_y + y;
        if (screen_y < 0 || screen_y >= h) continue;

        double yd = y - sz * 0.5;
        double l = sqrt(md - yd * yd);
        int xs = (int)(l + 0.99);
        if (xs < 1) xs = 1;

        int xe = xp + xs;
        if (xe > w) xe = w;
        int xst = xp - xs;
        if (xst < 0) xst = 0;

        uint32_t* f = &framebuffer[xst + screen_y * w];

        if (blend == 0) {
            // Replace
            for (int x = xst; x < xe; x++) {
                *f++ = color;
            }
        } else if (blend == 2) {
            // 50/50
            for (int x = xst; x < xe; x++) {
                *f = BLEND_AVG(*f, color);
                f++;
            }
        } else {
            // Additive (blend == 1 or 3)
            for (int x = xst; x < xe; x++) {
                *f = BLEND(*f, color);
                f++;
            }
        }
    }

    return 0;
}

void MovingParticleEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_parts.cpp:
    // enabled (int32) - bit 0: enabled, bit 1: on-beat size change
    // colors (int32) - BGR color
    // maxdist (int32) - 1-32
    // size (int32) - 1-128
    // size2 (int32) - 1-128
    // blend (int32) - 0=replace, 1=additive, 2=50/50, 3=default

    if (reader.remaining() >= 4) {
        int enabled = reader.read_u32();
        parameters().set_bool("enabled", (enabled & 1) != 0);
        parameters().set_bool("onbeat_size", (enabled & 2) != 0);
    }
    if (reader.remaining() >= 4) {
        // Color is stored as BGR in binary
        uint32_t bgr = reader.read_u32();
        uint32_t r = bgr & 0xFF;
        uint32_t g = (bgr >> 8) & 0xFF;
        uint32_t b = (bgr >> 16) & 0xFF;
        parameters().set_color("color", (r << 16) | (g << 8) | b);
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("maxdist", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("size", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("size2", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("blend_mode", reader.read_u32());
    }

    s_pos_ = parameters().get_int("size");
}

// Static member definition
const PluginInfo MovingParticleEffect::effect_info {
    .name = "Moving Particle",
    .category = "Render",
    .description = "Physics-based bouncing particle",
    .author = "",
    .version = 1,
    .legacy_index = 8,  // R_Parts
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<MovingParticleEffect>();
    },
    .ui_layout = {
        {
            // Original UI from r_parts.cpp g_DlgProc:
            // IDC_LEFT: enabled checkbox
            // IDC_LC: color button
            // IDC_SLIDER1: maxdist, range 1-32
            // IDC_SLIDER3: size, range 1-128
            // IDC_CHECK1: onbeat size change
            // IDC_SLIDER4: size2, range 1-128
            // IDC_RADIO1-4: blend modes
            {
                .id = "enabled",
                .text = "Enabled",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 41, .h = 10,
                .default_val = 1
            },
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 43, .y = 0, .w = 40, .h = 10,
                .default_val = (uint32_t)0xFFFFFF
            },
            {
                .id = "maxdist",
                .text = "Distance from center",
                .type = ControlType::SLIDER,
                .x = 5, .y = 21, .w = 130, .h = 16,
                .range = {1, 32, 1},
                .default_val = 16
            },
            {
                .id = "size",
                .text = "Particle size",
                .type = ControlType::SLIDER,
                .x = 5, .y = 47, .w = 130, .h = 16,
                .range = {1, 128, 8},
                .default_val = 8
            },
            {
                .id = "onbeat_size",
                .text = "Onbeat sizechange",
                .type = ControlType::CHECKBOX,
                .x = 9, .y = 75, .w = 107, .h = 10,
                .default_val = 0
            },
            {
                .id = "size2",
                .text = "Particle size (onbeat)",
                .type = ControlType::SLIDER,
                .x = 5, .y = 84, .w = 130, .h = 16,
                .range = {1, 128, 8},
                .default_val = 8
            },
            {
                .id = "blend_mode",
                .text = "Blend mode",
                .type = ControlType::RADIO_GROUP,
                .x = 5, .y = 114, .w = 130, .h = 40,
                .default_val = 1,
                .radio_options = {
                    {"Replace", 5, 114, 43, 10},
                    {"Additive", 51, 114, 41, 10},
                    {"50/50", 93, 114, 35, 10},
                    {"Default", 5, 124, 99, 9}
                }
            }
        }
    }
};

// Register effect at startup
static bool register_moving_particle = []() {
    PluginManager::instance().register_effect(MovingParticleEffect::effect_info);
    return true;
}();

} // namespace avs
