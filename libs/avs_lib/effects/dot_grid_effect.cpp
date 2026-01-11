// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "dot_grid_effect.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"

namespace avs {

DotGridEffect::DotGridEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int DotGridEffect::render(AudioData visdata, int isBeat,
                          uint32_t* framebuffer, uint32_t* fbout,
                          int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int num_colors = parameters().get_int("num_colors");
    if (num_colors < 1) return 0;
    if (num_colors > 16) num_colors = 16;

    // Color cycling
    color_pos_++;
    if (color_pos_ >= num_colors * 64) color_pos_ = 0;

    uint32_t current_color;
    if (num_colors == 1) {
        current_color = parameters().get_color("color_0") | 0xFF000000;
    } else {
        int p = color_pos_ / 64;
        int r = color_pos_ & 63;

        std::string c1_param = "color_" + std::to_string(p);
        std::string c2_param = "color_" + std::to_string((p + 1 < num_colors) ? p + 1 : 0);
        uint32_t c1 = parameters().get_color(c1_param);
        uint32_t c2 = parameters().get_color(c2_param);

        // Interpolate RGB channels
        int r1 = (((c1 & 0xff) * (63 - r)) + ((c2 & 0xff) * r)) / 64;
        int r2 = ((((c1 >> 8) & 0xff) * (63 - r)) + (((c2 >> 8) & 0xff) * r)) / 64;
        int r3 = ((((c1 >> 16) & 0xff) * (63 - r)) + (((c2 >> 16) & 0xff) * r)) / 64;
        current_color = r1 | (r2 << 8) | (r3 << 16) | 0xFF000000;  // Set alpha to opaque
    }

    int spacing = parameters().get_int("spacing");
    if (spacing < 2) spacing = 2;

    int blend_mode = parameters().get_int("blend_mode");

    // Wrap position
    while (yp_ < 0) yp_ += spacing * 256;
    while (xp_ < 0) xp_ += spacing * 256;

    int sy = (yp_ >> 8) % spacing;
    int sx = (xp_ >> 8) % spacing;

    uint32_t* p = framebuffer + sy * w;
    for (int y = sy; y < h; y += spacing) {
        if (blend_mode == 1) {
            for (int x = sx; x < w; x += spacing)
                p[x] = BLEND(p[x], current_color);
        } else if (blend_mode == 2) {
            for (int x = sx; x < w; x += spacing)
                p[x] = BLEND_AVG(p[x], current_color);
        } else if (blend_mode == 3) {
            // Use global line blend mode (set by Set Render Mode effect)
            for (int x = sx; x < w; x += spacing)
                BLEND_LINE(&p[x], current_color);
        } else {
            for (int x = sx; x < w; x += spacing)
                p[x] = current_color;
        }
        p += w * spacing;
    }

    xp_ += parameters().get_int("x_move");
    yp_ += parameters().get_int("y_move");

    return 0;
}

void DotGridEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_dotgrid.cpp:
    // num_colors (int32)
    // colors[num_colors] (int32 each, BGR format)
    // spacing (int32)
    // x_move (int32)
    // y_move (int32)
    // blend (int32)
    int num_colors = reader.read_u32();
    if (num_colors > 16) num_colors = 16;
    if (num_colors < 1) num_colors = 1;
    parameters().set_int("num_colors", num_colors);

    for (int i = 0; i < num_colors && reader.remaining() >= 4; i++) {
        uint32_t color = BinaryReader::bgr_add_alpha(reader.read_u32());
        parameters().set_color("color_" + std::to_string(i), color);
    }

    if (reader.remaining() >= 4) {
        int spacing = reader.read_u32();
        parameters().set_int("spacing", spacing);
    }
    if (reader.remaining() >= 4) {
        int x_move = static_cast<int32_t>(reader.read_u32());
        parameters().set_int("x_move", x_move);
    }
    if (reader.remaining() >= 4) {
        int y_move = static_cast<int32_t>(reader.read_u32());
        parameters().set_int("y_move", y_move);
    }
    if (reader.remaining() >= 4) {
        int blend = reader.read_u32();
        parameters().set_int("blend_mode", blend);
    }
}

const PluginInfo DotGridEffect::effect_info {
    .name = "Dot Grid",
    .category = "Render",
    .description = "",
    .author = "",
    .version = 1,
    .legacy_index = 17,  // R_DotGrid
    .factory = []() -> std::unique_ptr<EffectBase> {
        return std::make_unique<DotGridEffect>();
    },
    .ui_layout = {
        {
            // Match original res.rc IDD_CFG_DOTGRID
            {
                .id = "colors_group",
                .text = "Colors",
                .type = ControlType::GROUPBOX,
                .x = 0, .y = 0, .w = 137, .h = 36
            },
            {
                .id = "cycle_label",
                .text = "Cycle through ",
                .type = ControlType::LABEL,
                .x = 7, .y = 8, .w = 46, .h = 8
            },
            {
                .id = "num_colors",
                .text = "",
                .type = ControlType::TEXT_INPUT,
                .x = 53, .y = 6, .w = 19, .h = 12,
                .range = {1, 16},
                .default_val = 1
            },
            {
                .id = "colors_max_label",
                .text = "colors (max 16)",
                .type = ControlType::LABEL,
                .x = 77, .y = 8, .w = 48, .h = 8
            },
            {
                .id = "colors",
                .text = "",
                .type = ControlType::COLOR_ARRAY,
                .x = 6, .y = 21, .w = 127, .h = 11,
                .default_val = 0xFFFFFF,
                .max_items = 16
            },
            {
                .id = "x_move",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 16, .y = 47, .w = 85, .h = 13,
                .range = {-512, 512, 32},
                .default_val = 128
            },
            {
                .id = "x_reset",
                .text = "zero",
                .type = ControlType::BUTTON,
                .x = 103, .y = 50, .w = 28, .h = 10
            },
            {
                .id = "y_move",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 16, .y = 65, .w = 85, .h = 13,
                .range = {-512, 512, 32},
                .default_val = 128
            },
            {
                .id = "y_reset",
                .text = "zero",
                .type = ControlType::BUTTON,
                .x = 103, .y = 68, .w = 28, .h = 10
            },
            {
                .id = "blend_mode",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Replace", 5, 99, 43, 10},
                    {"Additive", 51, 99, 41, 10},
                    {"50/50", 93, 99, 35, 10},
                    {"Default render blend mode", 5, 109, 99, 10}
                },
                .default_val = 3
            },
            {
                .id = "spacing_label",
                .text = "Dot spacing: ",
                .type = ControlType::LABEL,
                .x = 0, .y = 127, .w = 43, .h = 8
            },
            {
                .id = "spacing",
                .text = "",
                .type = ControlType::TEXT_INPUT,
                .x = 44, .y = 125, .w = 20, .h = 12,
                .range = {2, 100},
                .default_val = 8
            }
        }
    }
};

static bool register_dot_grid = []() {
    PluginManager::instance().register_effect(DotGridEffect::effect_info);
    return true;
}();

} // namespace avs
