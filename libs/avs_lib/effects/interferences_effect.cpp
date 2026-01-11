// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "interferences_effect.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include <cmath>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace avs {

static constexpr int MAX_POINTS = 8;

InterferencesEffect::InterferencesEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    int rotation = parameters().get_int("rotation");
    a_ = (float)rotation / 255.0f * (float)M_PI * 2.0f;
    status_ = (float)M_PI;
}

void InterferencesEffect::update_blend_table(int alpha) {
    if (alpha == last_alpha_) return;
    last_alpha_ = alpha;
    for (int i = 0; i < 256; i++) {
        blend_table_[i] = (uint8_t)((i * alpha) >> 8);
    }
}

int InterferencesEffect::render(AudioData visdata, int isBeat,
                                 uint32_t* framebuffer, uint32_t* fbout,
                                 int w, int h) {
    if (isBeat & 0x80000000) return 0;
    if (!is_enabled()) return 0;

    int nPoints = parameters().get_int("npoints");
    if (nPoints == 0) return 0;

    int rotation = parameters().get_int("rotation");
    int distance = parameters().get_int("distance");
    int alpha = parameters().get_int("alpha");
    int rotationinc = parameters().get_int("rotationinc");
    int distance2 = parameters().get_int("distance2");
    int alpha2 = parameters().get_int("alpha2");
    int rotationinc2 = parameters().get_int("rotationinc2");
    bool rgb = parameters().get_bool("rgb");
    bool onbeat = parameters().get_bool("onbeat");
    float speed = parameters().get_int("speed") / 100.0f;
    int blend_mode = parameters().get_int("blend_mode");

    // On beat: reset transition
    if (onbeat && isBeat) {
        if (status_ >= (float)M_PI) {
            status_ = 0.0f;
        }
    }

    // Calculate interpolated values based on on-beat transition
    float s = (float)sin(status_);
    _rotationinc_ = rotationinc + (int)((float)(rotationinc2 - rotationinc) * s);
    _alpha_ = alpha + (int)((float)(alpha2 - alpha) * s);
    _distance_ = distance + (int)((float)(distance2 - distance) * s);

    // Update blend table for current alpha
    update_blend_table(_alpha_);

    // Calculate angle from rotation parameter
    a_ = (float)rotation / 255.0f * (float)M_PI * 2.0f;

    float angle = (float)(2.0 * M_PI) / nPoints;

    // Calculate point offsets
    int xpoints[MAX_POINTS], ypoints[MAX_POINTS];
    int minx = 0, maxx = 0, miny = 0, maxy = 0;

    for (int i = 0; i < nPoints; i++) {
        xpoints[i] = (int)(cos(a_) * _distance_);
        ypoints[i] = (int)(sin(a_) * _distance_);
        if (ypoints[i] > miny) miny = ypoints[i];
        if (-ypoints[i] > maxy) maxy = -ypoints[i];
        if (xpoints[i] > minx) minx = xpoints[i];
        if (-xpoints[i] > maxx) maxx = -xpoints[i];
        a_ += angle;
    }

    uint8_t* bt = blend_table_;
    uint32_t* outp = fbout;

    for (int y = 0; y < h; y++) {
        // Pre-compute y offsets for each point
        int yoffs[MAX_POINTS];
        for (int i = 0; i < nPoints; i++) {
            if (y >= ypoints[i] && y - ypoints[i] < h) {
                yoffs[i] = (y - ypoints[i]) * w;
            } else {
                yoffs[i] = -1;
            }
        }

        // RGB separation mode (only for 3 or 6 points)
        if (rgb && (nPoints == 3 || nPoints == 6)) {
            if (nPoints == 3) {
                for (int x = 0; x < w; x++) {
                    int r = 0, g = 0, b = 0;
                    int xp;

                    xp = x - xpoints[0];
                    if (xp >= 0 && xp < w && yoffs[0] != -1) {
                        uint32_t pix = framebuffer[xp + yoffs[0]];
                        r = bt[pix & 0xff];
                    }
                    xp = x - xpoints[1];
                    if (xp >= 0 && xp < w && yoffs[1] != -1) {
                        uint32_t pix = framebuffer[xp + yoffs[1]];
                        g = bt[(pix >> 8) & 0xff];
                    }
                    xp = x - xpoints[2];
                    if (xp >= 0 && xp < w && yoffs[2] != -1) {
                        uint32_t pix = framebuffer[xp + yoffs[2]];
                        b = bt[(pix >> 16) & 0xff];
                    }
                    *outp++ = r | (g << 8) | (b << 16);
                }
            } else {
                // 6 points
                for (int x = 0; x < w; x++) {
                    int r = 0, g = 0, b = 0;
                    int xp;

                    xp = x - xpoints[0];
                    if (xp >= 0 && xp < w && yoffs[0] != -1) {
                        uint32_t pix = framebuffer[xp + yoffs[0]];
                        r = bt[pix & 0xff];
                    }
                    xp = x - xpoints[1];
                    if (xp >= 0 && xp < w && yoffs[1] != -1) {
                        uint32_t pix = framebuffer[xp + yoffs[1]];
                        g = bt[(pix >> 8) & 0xff];
                    }
                    xp = x - xpoints[2];
                    if (xp >= 0 && xp < w && yoffs[2] != -1) {
                        uint32_t pix = framebuffer[xp + yoffs[2]];
                        b = bt[(pix >> 16) & 0xff];
                    }
                    xp = x - xpoints[3];
                    if (xp >= 0 && xp < w && yoffs[3] != -1) {
                        uint32_t pix = framebuffer[xp + yoffs[3]];
                        r += bt[pix & 0xff];
                    }
                    xp = x - xpoints[4];
                    if (xp >= 0 && xp < w && yoffs[4] != -1) {
                        uint32_t pix = framebuffer[xp + yoffs[4]];
                        g += bt[(pix >> 8) & 0xff];
                    }
                    xp = x - xpoints[5];
                    if (xp >= 0 && xp < w && yoffs[5] != -1) {
                        uint32_t pix = framebuffer[xp + yoffs[5]];
                        b += bt[(pix >> 16) & 0xff];
                    }
                    if (r > 255) r = 255;
                    if (g > 255) g = 255;
                    if (b > 255) b = 255;
                    *outp++ = r | (g << 8) | (b << 16);
                }
            }
        } else {
            // Standard mode: blend all channels from all points
            for (int x = 0; x < w; x++) {
                int r = 0, g = 0, b = 0;
                for (int i = 0; i < nPoints; i++) {
                    int xp = x - xpoints[i];
                    if (xp >= 0 && xp < w && yoffs[i] != -1) {
                        uint32_t pix = framebuffer[xp + yoffs[i]];
                        r += bt[pix & 0xff];
                        g += bt[(pix >> 8) & 0xff];
                        b += bt[(pix >> 16) & 0xff];
                    }
                }
                if (r > 255) r = 255;
                if (g > 255) g = 255;
                if (b > 255) b = 255;
                *outp++ = r | (g << 8) | (b << 16);
            }
        }
    }

    // Update rotation
    int new_rotation = rotation + _rotationinc_;
    if (new_rotation > 255) new_rotation -= 255;
    if (new_rotation < -255) new_rotation += 255;
    parameters().set_int("rotation", new_rotation);

    // Update on-beat status
    status_ += speed;
    status_ = std::min(status_, (float)M_PI);
    if (status_ < -(float)M_PI) status_ = (float)M_PI;

    // Handle output blending
    if (blend_mode == 0) {
        // Replace - output is in fbout
        return 1;
    } else if (blend_mode == 2) {
        // 50/50 blend back to framebuffer
        uint32_t* p = framebuffer;
        uint32_t* d = fbout;
        int count = w * h;
        for (int i = 0; i < count; i++) {
            p[i] = BLEND_AVG(p[i], d[i]);
        }
        return 0;
    } else {
        // Additive blend back to framebuffer
        uint32_t* p = framebuffer;
        uint32_t* d = fbout;
        int count = w * h;
        for (int i = 0; i < count; i++) {
            p[i] = BLEND(p[i], d[i]);
        }
        return 0;
    }
}

void InterferencesEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_interf.cpp:
    // enabled (int32)
    // nPoints (int32) - 0-8
    // rotation (int32) - 0-255
    // distance (int32) - 1-64
    // alpha (int32) - 1-255
    // rotationinc (int32) - rotation speed
    // blend (int32) - additive flag
    // blendavg (int32) - 50/50 flag
    // distance2 (int32) - on-beat distance
    // alpha2 (int32) - on-beat alpha
    // rotationinc2 (int32) - on-beat rotation speed
    // rgb (int32) - RGB separation mode
    // onbeat (int32)
    // speed (float) - on-beat transition speed

    if (reader.remaining() >= 4) {
        parameters().set_bool("enabled", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("npoints", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("rotation", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("distance", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("alpha", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("rotationinc", reader.read_i32());
    }
    if (reader.remaining() >= 4) {
        int blend = reader.read_u32();
        if (reader.remaining() >= 4) {
            int blendavg = reader.read_u32();
            // Convert to blend_mode: 0=replace, 1=additive, 2=50/50
            if (blendavg) {
                parameters().set_int("blend_mode", 2);
            } else if (blend) {
                parameters().set_int("blend_mode", 1);
            } else {
                parameters().set_int("blend_mode", 0);
            }
        }
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("distance2", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("alpha2", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("rotationinc2", reader.read_i32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("rgb", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("onbeat", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        // Read float as raw bits (same as original AVS GET_FLOAT)
        uint32_t bits = reader.read_u32();
        float speed;
        memcpy(&speed, &bits, sizeof(float));
        parameters().set_int("speed", (int)(speed * 100));
    }

    // Initialize state
    int rotation = parameters().get_int("rotation");
    a_ = (float)rotation / 255.0f * (float)M_PI * 2.0f;
    status_ = (float)M_PI;
}

// Static member definition
const PluginInfo InterferencesEffect::effect_info {
    .name = "Interferences",
    .category = "Trans",
    .description = "Wave interference patterns",
    .author = "",
    .version = 1,
    .legacy_index = 41,  // R_Interferences
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<InterferencesEffect>();
    },
    .ui_layout = {
        {
            // Original UI from r_interf.cpp g_DlgProc
            {
                .id = "enabled",
                .text = "Enable Interferences",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 81, .h = 10,
                .default_val = 1
            },
            {
                .id = "npoints",
                .text = "Wave count",
                .type = ControlType::SLIDER,
                .x = 0, .y = 13, .w = 67, .h = 11,
                .range = {0, 8, 1},
                .default_val = 2
            },
            {
                .id = "alpha",
                .text = "Alpha",
                .type = ControlType::SLIDER,
                .x = 70, .y = 12, .w = 67, .h = 11,
                .range = {1, 255, 16},
                .default_val = 128
            },
            {
                .id = "rotationinc",
                .text = "Rotation",
                .type = ControlType::SLIDER,
                .x = 0, .y = 33, .w = 67, .h = 11,
                .range = {-32, 32, 4},
                .default_val = 0
            },
            {
                .id = "distance",
                .text = "Distance",
                .type = ControlType::SLIDER,
                .x = 70, .y = 33, .w = 67, .h = 11,
                .range = {1, 64, 8},
                .default_val = 10
            },
            {
                .id = "rgb",
                .text = "Separate RGB",
                .type = ControlType::CHECKBOX,
                .x = 2, .y = 51, .w = 62, .h = 10,
                .default_val = 1
            },
            {
                .id = "onbeat",
                .text = "OnBeat",
                .type = ControlType::CHECKBOX,
                .x = 2, .y = 60, .w = 40, .h = 10,
                .default_val = 1
            },
            {
                .id = "rotation",
                .text = "Init Rotation",
                .type = ControlType::SLIDER,
                .x = 70, .y = 52, .w = 67, .h = 11,
                .range = {0, 255, 16},
                .default_val = 0
            },
            {
                .id = "speed",
                .text = "OnBeat Speed",
                .type = ControlType::SLIDER,
                .x = 4, .y = 78, .w = 63, .h = 11,
                .range = {1, 128, 10},
                .default_val = 20
            },
            {
                .id = "alpha2",
                .text = "OnBeat Alpha",
                .type = ControlType::SLIDER,
                .x = 70, .y = 78, .w = 63, .h = 11,
                .range = {1, 255, 16},
                .default_val = 192
            },
            {
                .id = "rotationinc2",
                .text = "OnBeat Rotation",
                .type = ControlType::SLIDER,
                .x = 4, .y = 99, .w = 63, .h = 11,
                .range = {-32, 32, 4},
                .default_val = 25
            },
            {
                .id = "distance2",
                .text = "OnBeat Distance",
                .type = ControlType::SLIDER,
                .x = 70, .y = 99, .w = 63, .h = 11,
                .range = {1, 64, 8},
                .default_val = 32
            },
            {
                .id = "blend_mode",
                .text = "Blend mode",
                .type = ControlType::RADIO_GROUP,
                .x = 2, .y = 123, .w = 130, .h = 12,
                .default_val = 0,
                .radio_options = {
                    {"Replace", 2, 123, 43, 10},
                    {"Additive", 51, 123, 41, 10},
                    {"50/50", 100, 123, 35, 10}
                }
            }
        }
    }
};

// Register effect at startup
static bool register_interferences = []() {
    PluginManager::instance().register_effect(InterferencesEffect::effect_info);
    return true;
}();

} // namespace avs
