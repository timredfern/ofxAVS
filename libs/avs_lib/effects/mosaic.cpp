// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "mosaic.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include <cstdlib>
#include <algorithm>
#include <cstring>

namespace avs {

MosaicEffect::MosaicEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    this_quality_ = 50;
}

int MosaicEffect::render(AudioData visdata, int isBeat,
                          uint32_t* framebuffer, uint32_t* fbout,
                          int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int enabled = parameters().get_int("enabled");
    if (!enabled) return 0;

    int quality = parameters().get_int("quality");
    int quality2 = parameters().get_int("quality2");
    int onbeat = parameters().get_int("onbeat");
    int dur_frames = parameters().get_int("durFrames");
    int blend_mode = parameters().get_int("blend");
    int blendavg = parameters().get_int("blendavg");

    // Handle beat transitions
    if (onbeat && isBeat) {
        this_quality_ = quality2;
        nf_ = dur_frames;
    } else if (!nf_) {
        this_quality_ = quality;
    }

    // Quality 100 means no effect
    if (this_quality_ >= 100) {
        if (nf_) {
            nf_--;
            if (nf_) {
                int a = std::abs(quality - quality2) / dur_frames;
                this_quality_ += a * (quality2 > quality ? -1 : 1);
            }
        }
        return 0;
    }

    // Calculate step sizes using fixed point (16.16)
    int sx_inc = (w * 65536) / this_quality_;
    int sy_inc = (h * 65536) / this_quality_;
    int ypos = (sy_inc >> 17);
    int dypos = 0;

    uint32_t* p = fbout;

    for (int y = 0; y < h; y++) {
        uint32_t* fb_read = framebuffer + ypos * w;
        int dpos = 0;
        int xpos = (sx_inc >> 17);
        uint32_t src = fb_read[xpos];

        if (blend_mode) {
            for (int x = 0; x < w; x++) {
                *p++ = BLEND(framebuffer[y * w + x], src);
                dpos += 1 << 16;
                if (dpos >= sx_inc) {
                    xpos += dpos >> 16;
                    if (xpos >= w) break;
                    src = fb_read[xpos];
                    dpos -= sx_inc;
                }
            }
        } else if (blendavg) {
            for (int x = 0; x < w; x++) {
                *p++ = BLEND_AVG(framebuffer[y * w + x], src);
                dpos += 1 << 16;
                if (dpos >= sx_inc) {
                    xpos += dpos >> 16;
                    if (xpos >= w) break;
                    src = fb_read[xpos];
                    dpos -= sx_inc;
                }
            }
        } else {
            for (int x = 0; x < w; x++) {
                *p++ = src;
                dpos += 1 << 16;
                if (dpos >= sx_inc) {
                    xpos += dpos >> 16;
                    if (xpos >= w) break;
                    src = fb_read[xpos];
                    dpos -= sx_inc;
                }
            }
        }

        dypos += 1 << 16;
        if (dypos >= sy_inc) {
            ypos += (dypos >> 16);
            dypos -= sy_inc;
            if (ypos >= h) break;
        }
    }

    // Handle beat transition interpolation
    if (nf_) {
        nf_--;
        if (nf_) {
            int a = std::abs(quality - quality2) / dur_frames;
            this_quality_ += a * (quality2 > quality ? -1 : 1);
        }
    }

    return 1;  // Output is in fbout
}

void MosaicEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_mosaic.cpp:
    // enabled, quality, quality2, blend, blendavg, onbeat, durFrames
    int enabled = reader.read_u32();
    parameters().set_int("enabled", enabled);

    if (reader.remaining() >= 4) {
        int quality = reader.read_u32();
        parameters().set_int("quality", quality);
        this_quality_ = quality;
    }

    if (reader.remaining() >= 4) {
        int quality2 = reader.read_u32();
        parameters().set_int("quality2", quality2);
    }

    if (reader.remaining() >= 4) {
        int blend = reader.read_u32();
        parameters().set_int("blend", blend);
    }

    if (reader.remaining() >= 4) {
        int blendavg = reader.read_u32();
        parameters().set_int("blendavg", blendavg);
    }

    if (reader.remaining() >= 4) {
        int onbeat = reader.read_u32();
        parameters().set_int("onbeat", onbeat);
    }

    if (reader.remaining() >= 4) {
        int dur_frames = reader.read_u32();
        parameters().set_int("durFrames", dur_frames);
    }
}

// Static member definition - UI layout from res.rc IDD_CFG_MOSAIC
const PluginInfo MosaicEffect::effect_info {
    .name = "Mosaic",
    .category = "Trans",
    .description = "Pixelates the image with adjustable block size",
    .author = "",
    .version = 1,
    .legacy_index = 25,  // R_Mosaic from rlib.cpp
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<MosaicEffect>();
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
                .id = "quality",
                .text = "Size",
                .type = ControlType::SLIDER,
                .x = 0, .y = 15, .w = 150, .h = 20,
                .range = {1, 100, 1},
                .default_val = 50
            },
            {
                .id = "onbeat",
                .text = "On beat",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 40, .w = 78, .h = 10,
                .default_val = 0
            },
            {
                .id = "quality2",
                .text = "Beat Size",
                .type = ControlType::SLIDER,
                .x = 0, .y = 55, .w = 150, .h = 20,
                .range = {1, 100, 1},
                .default_val = 50
            },
            {
                .id = "durFrames",
                .text = "Duration",
                .type = ControlType::SLIDER,
                .x = 0, .y = 80, .w = 150, .h = 20,
                .range = {1, 100, 1},
                .default_val = 15
            },
            {
                .id = "blend",
                .text = "Additive",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 105, .w = 78, .h = 10,
                .default_val = 0
            },
            {
                .id = "blendavg",
                .text = "50/50",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 120, .w = 78, .h = 10,
                .default_val = 0
            }
        }
    }
};

// Register effect at startup
static bool register_mosaic = []() {
    PluginManager::instance().register_effect(MosaicEffect::effect_info);
    return true;
}();

} // namespace avs
