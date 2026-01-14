// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "unique_tone.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include <algorithm>

namespace avs {

UniqueToneEffect::UniqueToneEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    rebuild_table();
}

void UniqueToneEffect::on_parameter_changed(const std::string& param_name) {
    if (param_name == "color" || param_name == "invert") {
        rebuild_table();
    }
}

void UniqueToneEffect::rebuild_table() {
    uint32_t color = parameters().get_color("color");

    // Extract RGB from color (ABGR format: 0xAABBGGRR)
    float r_target = static_cast<float>(color & 0xff);
    float g_target = static_cast<float>((color >> 8) & 0xff);
    float b_target = static_cast<float>((color >> 16) & 0xff);

    // Build lookup tables - scale each intensity by target color
    for (int i = 0; i < 256; i++) {
        float scale = i / 255.0f;
        tabler_[i] = static_cast<unsigned char>(scale * r_target);
        tableg_[i] = static_cast<unsigned char>(scale * g_target);
        tableb_[i] = static_cast<unsigned char>(scale * b_target);
    }
}

int UniqueToneEffect::render(AudioData visdata, int isBeat,
                              uint32_t* framebuffer, uint32_t* fbout,
                              int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int enabled = parameters().get_int("enabled");
    if (!enabled) return 0;

    int invert = parameters().get_int("invert");
    int blend_mode = parameters().get_int("blend");
    int blendavg = parameters().get_int("blendavg");

    int count = w * h;

    for (int i = 0; i < count; i++) {
        uint32_t pix = framebuffer[i];

        // Calculate depth (max of R, G, B)
        int r = pix & 0xff;
        int g = (pix >> 8) & 0xff;
        int b = (pix >> 16) & 0xff;
        int depth = std::max({r, g, b});

        if (invert) {
            depth = 255 - depth;
        }

        // Look up new color from tables
        uint32_t c = tabler_[depth] | (tableg_[depth] << 8) | (tableb_[depth] << 16);

        // Apply blend mode
        if (blend_mode) {
            framebuffer[i] = BLEND(pix, c);
        } else if (blendavg) {
            framebuffer[i] = BLEND_AVG(pix, c);
        } else {
            framebuffer[i] = (pix & 0xFF000000) | c;
        }
    }

    return 0;
}

void UniqueToneEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_onetone.cpp:
    // enabled, color, blend, blendavg, invert (all int32)
    int enabled = reader.read_u32();
    parameters().set_int("enabled", enabled);

    if (reader.remaining() >= 4) {
        uint32_t color = BinaryReader::bgr_add_alpha(reader.read_u32());
        parameters().set_color("color", color);
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
        int invert = reader.read_u32();
        parameters().set_int("invert", invert);
    }

    rebuild_table();
}

// Static member definition - UI layout from res.rc IDD_CFG_ONETONE
const PluginInfo UniqueToneEffect::effect_info {
    .name = "Unique Tone",
    .category = "Trans",
    .description = "Converts image to single color tone based on brightness",
    .author = "",
    .version = 1,
    .legacy_index = 34,  // R_Onetone from rlib.cpp
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<UniqueToneEffect>();
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
                .id = "invert",
                .text = "Invert",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 15, .w = 78, .h = 10,
                .default_val = 0
            },
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 30, .w = 78, .h = 20,
                .default_val = static_cast<int>(0xFFFFFFFF)
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
static bool register_unique_tone = []() {
    PluginManager::instance().register_effect(UniqueToneEffect::effect_info);
    return true;
}();

} // namespace avs
