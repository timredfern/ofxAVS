// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "fast_brightness.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include <algorithm>

namespace avs {

FastBrightnessEffect::FastBrightnessEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int FastBrightnessEffect::render(AudioData visdata, int isBeat,
                                  uint32_t* framebuffer, uint32_t* fbout,
                                  int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int dir = parameters().get_int("dir");

    int count = w * h;

    if (dir == 0) {
        // 2x brightness - double each channel with saturation
        for (int i = 0; i < count; i++) {
            uint32_t pix = framebuffer[i];
            int r = (pix & 0xff) * 2;
            int g = ((pix >> 8) & 0xff) * 2;
            int b = ((pix >> 16) & 0xff) * 2;
            r = std::min(r, 255);
            g = std::min(g, 255);
            b = std::min(b, 255);
            framebuffer[i] = (pix & 0xFF000000) | (b << 16) | (g << 8) | r;
        }
    } else if (dir == 1) {
        // 0.5x brightness - halve each channel
        for (int i = 0; i < count; i++) {
            uint32_t pix = framebuffer[i];
            uint32_t result = ((pix >> 1) & 0x7F7F7F) | (pix & 0xFF000000);
            framebuffer[i] = result;
        }
    }
    // dir == 2: do nothing (leave as is)

    return 0;
}

void FastBrightnessEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);
    int dir = reader.read_u32();
    parameters().set_int("dir", dir);
}

// Static member definition - UI layout from res.rc IDD_CFG_FASTBRIGHT
const PluginInfo FastBrightnessEffect::effect_info {
    .name = "Fast Brightness",
    .category = "Trans",
    .description = "Quickly adjusts brightness by 2x or 0.5x",
    .author = "",
    .version = 1,
    .legacy_index = 27,  // R_FastBright from rlib.cpp
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<FastBrightnessEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "dir",
                .text = "",
                .type = ControlType::RADIO_GROUP,
                .x = 0, .y = 0, .w = 100, .h = 50,
                .radio_options = {
                    {"2 x Brightness", 0, 0, 90, 10},
                    {"0.5 x Brightness", 0, 15, 90, 10},
                    {"Leave as is", 0, 30, 90, 10}
                },
                .default_val = 0
            }
        }
    }
};

// Register effect at startup
static bool register_fast_brightness = []() {
    PluginManager::instance().register_effect(FastBrightnessEffect::effect_info);
    return true;
}();

} // namespace avs
