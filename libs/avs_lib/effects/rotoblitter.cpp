// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "rotoblitter.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace avs {

// Original parameter ranges from r_rotblit.cpp
// zoom_scale: 0-256, default 31 (31 = no zoom, zoom = 1.0)
// rot_dir: 0-64, default 31 (32 = no rotation)
// beatch_speed: 0-8

RotoBlitterEffect::RotoBlitterEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    current_zoom_ = parameters().get_int("zoom_scale");
}

int RotoBlitterEffect::render(AudioData visdata, int isBeat,
                              uint32_t* framebuffer, uint32_t* fbout,
                              int w, int h) {
    // Rebuild width multiplier table if size changed
    if (last_w_ != w || last_h_ != h || w_mul_.empty()) {
        last_w_ = w;
        last_h_ = h;
        w_mul_.resize(h);
        for (int y = 0; y < h; y++) {
            w_mul_[y] = y * w;
        }
    }

    if (isBeat & 0x80000000) return 0;

    auto* src = framebuffer;
    auto* dest = fbout;
    auto* bdest = framebuffer;  // For 50/50 blend source

    int beatch = parameters().get_bool("beatch") ? 1 : 0;
    int beatch_speed = parameters().get_int("beatch_speed");
    int beatch_scale = parameters().get_bool("beatch_scale") ? 1 : 0;
    int zoom_scale = parameters().get_int("zoom_scale");
    int zoom_scale2 = parameters().get_int("zoom_scale2");
    int rot_dir = parameters().get_int("rot_dir");
    int blend = parameters().get_bool("blend") ? 1 : 0;
    int subpixel = parameters().get_bool("subpixel") ? 1 : 0;

    // Handle beat-triggered direction reversal (original: rot_rev)
    if (isBeat && beatch) {
        direction_ = -direction_;
    }
    if (!beatch) {
        direction_ = 1;
    }

    // Smooth transition to target direction (original: rot_rev_pos)
    current_rotation_ += (1.0 / (1 + beatch_speed * 4)) * (direction_ - current_rotation_);
    if (current_rotation_ > direction_ && direction_ > 0) {
        current_rotation_ = direction_;
    }
    if (current_rotation_ < direction_ && direction_ < 0) {
        current_rotation_ = direction_;
    }

    // Handle beat-triggered zoom (original: scale_fpos)
    if (isBeat && beatch_scale) {
        current_zoom_ = zoom_scale2;
    }

    // Interpolate zoom towards target (original: f_val calculation)
    int f_val;
    if (zoom_scale < zoom_scale2) {
        f_val = std::max((int)current_zoom_, zoom_scale);
        if (current_zoom_ > zoom_scale) {
            current_zoom_ -= 3;
        }
    } else {
        f_val = std::min((int)current_zoom_, zoom_scale);
        if (current_zoom_ < zoom_scale) {
            current_zoom_ += 3;
        }
    }

    // Original zoom formula: zoom = 1.0 + (f_val-31)/31.0
    double zoom = 1.0 + (f_val - 31) / 31.0;

    // Original rotation formula: theta = (rot_dir-32) * rot_rev_pos
    double theta = (rot_dir - 32) * current_rotation_;

    // Calculate rotation transform (fixed-point 16.16)
    double temp = cos(theta * M_PI / 180.0) * zoom;
    int ds_dx = (int)(temp * 65536.0);
    int dt_dy = (int)(temp * 65536.0);
    temp = sin(theta * M_PI / 180.0) * zoom;
    int ds_dy = -(int)(temp * 65536.0);
    int dt_dx = (int)(temp * 65536.0);

    // Starting position (center of output maps to center of input)
    int sstart = -(((w - 1) / 2) * ds_dx + ((h - 1) / 2) * ds_dy)
                 + (w - 1) * (32768 + (1 << 20));
    int tstart = -(((w - 1) / 2) * dt_dx + ((h - 1) / 2) * dt_dy)
                 + (h - 1) * (32768 + (1 << 20));

    int s = sstart;
    int t = tstart;
    int ds = (w - 1) << 16;
    int dt = (h - 1) << 16;

    // Only render if transform stays in bounds (original check)
    if (ds_dx <= -ds || ds_dx >= ds || dt_dx <= -dt || dt_dx >= dt) {
        // Out of bounds - skip rendering
    } else {
        for (int y = h; y > 0; y--) {
            // Wrap source coordinates
            if (ds) s %= ds;
            if (dt) t %= dt;
            if (s < 0) s += ds;
            if (t < 0) t += dt;

            for (int x = w; x > 0; x--) {
                // Wrap coordinates per original DO_LOOPS macro
                if (ds_dx <= 0 && dt_dx <= 0) {
                    if (t < 0) t += dt;
                    if (s < 0) s += ds;
                } else if (ds_dx <= 0) {
                    if (t >= dt) t -= dt;
                    if (s < 0) s += ds;
                } else if (dt_dx <= 0) {
                    if (t < 0) t += dt;
                    if (s >= ds) s -= ds;
                } else {
                    if (t >= dt) t -= dt;
                    if (s >= ds) s -= ds;
                }

                int src_idx = (s >> 16) + w_mul_[t >> 16];

                // Original render modes:
                // subpixel && blend: BLEND_AVG with bilinear
                // subpixel: bilinear only
                // blend: BLEND_AVG with nearest
                // neither: nearest neighbor
                if (subpixel && blend) {
                    uint32_t bilinear = blend_bilinear_2x2(
                        &src[src_idx], w, (s >> 8) & 0xff, (t >> 8) & 0xff);
                    *dest++ = BLEND_AVG(*bdest++, bilinear);
                } else if (subpixel) {
                    *dest++ = blend_bilinear_2x2(
                        &src[src_idx], w, (s >> 8) & 0xff, (t >> 8) & 0xff);
                } else if (!blend) {
                    *dest++ = src[src_idx];
                } else {
                    *dest++ = BLEND_AVG(*bdest++, src[src_idx]);
                }

                s += ds_dx;
                t += dt_dx;
            }

            s = (sstart += ds_dy);
            t = (tstart += dt_dy);
        }
    }

    return 1;  // Output in fbout
}

void RotoBlitterEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_rotblit.cpp:
    // zoom_scale (int32) - 0-256, default 31
    // rot_dir (int32) - 0-64, default 31
    // blend (int32) - 0/1
    // beatch (int32) - 0/1 on beat reverse
    // beatch_speed (int32) - 0-8
    // zoom_scale2 (int32) - 0-256, on beat zoom target
    // beatch_scale (int32) - 0/1 on beat zoom enable
    // subpixel (int32) - 0/1 bilinear filtering

    if (reader.remaining() >= 4) {
        parameters().set_int("zoom_scale", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("rot_dir", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("blend", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("beatch", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("beatch_speed", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("zoom_scale2", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("beatch_scale", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("subpixel", reader.read_u32() != 0);
    }

    current_zoom_ = parameters().get_int("zoom_scale");
}

// Static member definition
const PluginInfo RotoBlitterEffect::effect_info {
    .name = "Roto Blitter",
    .category = "Trans",
    .description = "Rotating bitmap blitter with zoom",
    .author = "",
    .version = 1,
    .legacy_index = 9,  // R_RotBlit
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<RotoBlitterEffect>();
    },
    .ui_layout = {
        {
            // Original UI from r_rotblit.cpp g_DlgProc:
            // IDC_SLIDER1: zoom_scale, range 0-256
            // IDC_SLIDER2: rot_dir, range 0-64
            // IDC_SLIDER5: beatch_speed, range 0-8
            // IDC_SLIDER6: zoom_scale2, range 0-256
            // IDC_BLEND: blend checkbox
            // IDC_CHECK1: beatch (on beat reverse)
            // IDC_CHECK2: subpixel
            // IDC_CHECK6: beatch_scale (on beat zoom)
            {
                .id = "zoom_scale",
                .text = "Zoom",
                .type = ControlType::SLIDER,
                .x = 48, .y = 4, .w = 140, .h = 14,
                .range = {0, 256, 16},
                .default_val = 31
            },
            {
                .id = "rot_dir",
                .text = "Rotate",
                .type = ControlType::SLIDER,
                .x = 48, .y = 20, .w = 140, .h = 14,
                .range = {0, 64, 4},
                .default_val = 31
            },
            {
                .id = "blend",
                .text = "50/50 blend",
                .type = ControlType::CHECKBOX,
                .x = 4, .y = 40, .w = 60, .h = 10,
                .default_val = 0
            },
            {
                .id = "subpixel",
                .text = "Subpixel",
                .type = ControlType::CHECKBOX,
                .x = 4, .y = 52, .w = 60, .h = 10,
                .default_val = 1
            },
            {
                .id = "beatch",
                .text = "On beat reverse",
                .type = ControlType::CHECKBOX,
                .x = 4, .y = 68, .w = 80, .h = 10,
                .default_val = 0
            },
            {
                .id = "beatch_speed",
                .text = "Reverse speed",
                .type = ControlType::SLIDER,
                .x = 48, .y = 82, .w = 140, .h = 14,
                .range = {0, 8, 1},
                .default_val = 0
            },
            {
                .id = "beatch_scale",
                .text = "On beat zoom",
                .type = ControlType::CHECKBOX,
                .x = 4, .y = 100, .w = 80, .h = 10,
                .default_val = 0
            },
            {
                .id = "zoom_scale2",
                .text = "On beat zoom",
                .type = ControlType::SLIDER,
                .x = 48, .y = 114, .w = 140, .h = 14,
                .range = {0, 256, 16},
                .default_val = 31
            }
        }
    }
};

// Register effect at startup
static bool register_rotoblitter = []() {
    PluginManager::instance().register_effect(RotoBlitterEffect::effect_info);
    return true;
}();

} // namespace avs
