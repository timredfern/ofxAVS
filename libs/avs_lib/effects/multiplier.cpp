// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "multiplier.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include <algorithm>

namespace avs {

MultiplierEffect::MultiplierEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int MultiplierEffect::render(AudioData visdata, int isBeat,
                              uint32_t* framebuffer, uint32_t* fbout,
                              int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int mode = parameters().get_int("mode");
    int count = w * h;

    switch (mode) {
        case MD_XI:  // Infinite root - non-black becomes white
            for (int i = 0; i < count; i++) {
                uint32_t pix = framebuffer[i];
                if ((pix & 0x00FFFFFF) != 0) {
                    framebuffer[i] = (pix & 0xFF000000) | 0x00FFFFFF;
                }
            }
            break;

        case MD_XS:  // Square - only pure white stays white
            for (int i = 0; i < count; i++) {
                uint32_t pix = framebuffer[i];
                if ((pix & 0x00FFFFFF) != 0x00FFFFFF) {
                    framebuffer[i] = (pix & 0xFF000000);
                }
            }
            break;

        case MD_X8:  // 8x brightness
            for (int i = 0; i < count; i++) {
                uint32_t pix = framebuffer[i];
                int r = std::min(static_cast<int>((pix & 0xff) << 3), 255);
                int g = std::min(static_cast<int>(((pix >> 8) & 0xff) << 3), 255);
                int b = std::min(static_cast<int>(((pix >> 16) & 0xff) << 3), 255);
                framebuffer[i] = (pix & 0xFF000000) | (b << 16) | (g << 8) | r;
            }
            break;

        case MD_X4:  // 4x brightness
            for (int i = 0; i < count; i++) {
                uint32_t pix = framebuffer[i];
                int r = std::min(static_cast<int>((pix & 0xff) << 2), 255);
                int g = std::min(static_cast<int>(((pix >> 8) & 0xff) << 2), 255);
                int b = std::min(static_cast<int>(((pix >> 16) & 0xff) << 2), 255);
                framebuffer[i] = (pix & 0xFF000000) | (b << 16) | (g << 8) | r;
            }
            break;

        case MD_X2:  // 2x brightness
            for (int i = 0; i < count; i++) {
                uint32_t pix = framebuffer[i];
                int r = std::min(static_cast<int>((pix & 0xff) << 1), 255);
                int g = std::min(static_cast<int>(((pix >> 8) & 0xff) << 1), 255);
                int b = std::min(static_cast<int>(((pix >> 16) & 0xff) << 1), 255);
                framebuffer[i] = (pix & 0xFF000000) | (b << 16) | (g << 8) | r;
            }
            break;

        case MD_X05:  // 0.5x brightness
            for (int i = 0; i < count; i++) {
                uint32_t pix = framebuffer[i];
                framebuffer[i] = (pix & 0xFF000000) | ((pix >> 1) & 0x7F7F7F);
            }
            break;

        case MD_X025:  // 0.25x brightness
            for (int i = 0; i < count; i++) {
                uint32_t pix = framebuffer[i];
                framebuffer[i] = (pix & 0xFF000000) | ((pix >> 2) & 0x3F3F3F);
            }
            break;

        case MD_X0125:  // 0.125x brightness
            for (int i = 0; i < count; i++) {
                uint32_t pix = framebuffer[i];
                framebuffer[i] = (pix & 0xFF000000) | ((pix >> 3) & 0x1F1F1F);
            }
            break;
    }

    return 0;
}

void MultiplierEffect::load_parameters(const std::vector<uint8_t>& data) {
    // Original uses apeconfig struct with just int ml
    if (data.size() < 4) return;

    BinaryReader reader(data);
    int mode = reader.read_u32();
    parameters().set_int("mode", mode);
}

// Static member definition - UI layout from res.rc IDD_CFG_MULT
const PluginInfo MultiplierEffect::effect_info {
    .name = "Multiplier",
    .category = "Trans",
    .description = "Multiplies pixel brightness by various factors",
    .author = "",
    .version = 1,
    .legacy_index = 36,  // R_Multiplier from rlib.cpp
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<MultiplierEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "mode",
                .text = "",
                .type = ControlType::RADIO_GROUP,
                .x = 0, .y = 0, .w = 120, .h = 120,
                .radio_options = {
                    {"Infinite Root", 0, 0, 80, 10},
                    {"x 8", 0, 14, 80, 10},
                    {"x 4", 0, 28, 80, 10},
                    {"x 2", 0, 42, 80, 10},
                    {"x 0.5", 0, 56, 80, 10},
                    {"x 0.25", 0, 70, 80, 10},
                    {"x 0.125", 0, 84, 80, 10},
                    {"Square", 0, 98, 80, 10}
                },
                .default_val = 3  // Default to x2
            }
        }
    }
};

// Register effect at startup
static bool register_multiplier = []() {
    PluginManager::instance().register_effect(MultiplierEffect::effect_info);
    return true;
}();

} // namespace avs
