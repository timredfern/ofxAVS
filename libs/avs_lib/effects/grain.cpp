// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "grain.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include <cstdlib>
#include <algorithm>

namespace avs {

GrainEffect::GrainEffect() {
    init_parameters_from_layout(effect_info.ui_layout);

    // Initialize random table
    for (size_t i = 0; i < sizeof(randtab_); i++) {
        randtab_[i] = std::rand() & 255;
    }
    randtab_pos_ = std::rand() % sizeof(randtab_);
}

unsigned char GrainEffect::fast_rand_byte() {
    unsigned char r = randtab_[randtab_pos_];
    randtab_pos_++;
    if (!(randtab_pos_ & 15)) {
        randtab_pos_ += std::rand() % 73;
    }
    if (randtab_pos_ >= 491) randtab_pos_ -= 491;
    return r;
}

void GrainEffect::reinit_depth_buffer(int w, int h) {
    depth_buffer_.resize(w * h * 2);
    for (int i = 0; i < w * h; i++) {
        depth_buffer_[i * 2] = std::rand() & 255;      // intensity
        depth_buffer_[i * 2 + 1] = std::rand() % 100;  // probability
    }
    old_w_ = w;
    old_h_ = h;
}

int GrainEffect::render(AudioData visdata, int isBeat,
                         uint32_t* framebuffer, uint32_t* fbout,
                         int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int enabled = parameters().get_int("enabled");
    if (!enabled) return 0;

    int smax = parameters().get_int("smax");
    int blend_mode = parameters().get_int("blend");
    int blendavg = parameters().get_int("blendavg");
    int staticgrain = parameters().get_int("staticgrain");

    int smax_sc = (smax * 255) / 100;

    // Reinit depth buffer if size changed
    if (w != old_w_ || h != old_h_) {
        reinit_depth_buffer(w, h);
    }

    // Randomize position each frame
    randtab_pos_ += std::rand() % 300;
    if (randtab_pos_ >= 491) randtab_pos_ -= 491;

    int count = w * h;

    if (staticgrain) {
        // Use pre-generated depth buffer for static grain
        for (int i = 0; i < count; i++) {
            uint32_t pix = framebuffer[i];
            if ((pix & 0x00FFFFFF) == 0) continue;  // Skip black pixels

            if (depth_buffer_[i * 2 + 1] < smax_sc) {
                int s = depth_buffer_[i * 2];

                // Scale each channel by random intensity
                int r = (((pix & 0xff) * s) >> 8);
                int g = ((((pix >> 8) & 0xff) * s) >> 8);
                int b = ((((pix >> 16) & 0xff) * s) >> 8);

                r = std::min(r, 255);
                g = std::min(g, 255);
                b = std::min(b, 255);

                uint32_t c = r | (g << 8) | (b << 16);

                if (blend_mode) {
                    framebuffer[i] = BLEND(pix, c);
                } else if (blendavg) {
                    framebuffer[i] = BLEND_AVG(pix, c);
                } else {
                    framebuffer[i] = (pix & 0xFF000000) | c;
                }
            }
        }
    } else {
        // Dynamic random grain
        for (int i = 0; i < count; i++) {
            uint32_t pix = framebuffer[i];
            if ((pix & 0x00FFFFFF) == 0) continue;  // Skip black pixels

            if (fast_rand_byte() < smax_sc) {
                int s = fast_rand_byte();

                // Scale each channel by random intensity
                int r = (((pix & 0xff) * s) >> 8);
                int g = ((((pix >> 8) & 0xff) * s) >> 8);
                int b = ((((pix >> 16) & 0xff) * s) >> 8);

                r = std::min(r, 255);
                g = std::min(g, 255);
                b = std::min(b, 255);

                uint32_t c = r | (g << 8) | (b << 16);

                if (blend_mode) {
                    framebuffer[i] = BLEND(pix, c);
                } else if (blendavg) {
                    framebuffer[i] = BLEND_AVG(pix, c);
                } else {
                    framebuffer[i] = (pix & 0xFF000000) | c;
                }
            }
        }
    }

    return 0;
}

void GrainEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_grain.cpp:
    // enabled, blend, blendavg, smax, staticgrain (all int32)
    int enabled = reader.read_u32();
    parameters().set_int("enabled", enabled);

    if (reader.remaining() >= 4) {
        int blend = reader.read_u32();
        parameters().set_int("blend", blend);
    }

    if (reader.remaining() >= 4) {
        int blendavg = reader.read_u32();
        parameters().set_int("blendavg", blendavg);
    }

    if (reader.remaining() >= 4) {
        int smax = reader.read_u32();
        parameters().set_int("smax", smax);
    }

    if (reader.remaining() >= 4) {
        int staticgrain = reader.read_u32();
        parameters().set_int("staticgrain", staticgrain);
    }
}

// Static member definition - UI layout from res.rc IDD_CFG_GRAIN
const PluginInfo GrainEffect::effect_info {
    .name = "Grain",
    .category = "Trans",
    .description = "Adds random noise/grain to the image",
    .author = "",
    .version = 1,
    .legacy_index = 26,  // R_Grain from rlib.cpp
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<GrainEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enable",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 78, .h = 10,
                .default_val = 1
            },
            {
                .id = "smax",
                .text = "Amount",
                .type = ControlType::SLIDER,
                .x = 0, .y = 15, .w = 150, .h = 20,
                .range = {0, 100, 1},
                .default_val = 100
            },
            {
                .id = "staticgrain",
                .text = "Static",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 40, .w = 78, .h = 10,
                .default_val = 0
            },
            {
                .id = "blend",
                .text = "Additive",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 55, .w = 78, .h = 10,
                .default_val = 0
            },
            {
                .id = "blendavg",
                .text = "50/50",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 70, .w = 78, .h = 10,
                .default_val = 0
            }
        }
    }
};

// Register effect at startup
static bool register_grain = []() {
    PluginManager::instance().register_effect(GrainEffect::effect_info);
    return true;
}();

} // namespace avs
