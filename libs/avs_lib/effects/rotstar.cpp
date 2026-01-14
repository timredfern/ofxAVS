// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Port of r_rotstar.cpp from original AVS

#include "rotstar.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/line_draw.h"
#include <cmath>

namespace avs {

static constexpr double PI = 3.14159265358979323846;

RotStarEffect::RotStarEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int RotStarEffect::render(AudioData visdata, int isBeat,
                          uint32_t* framebuffer, uint32_t* fbout,
                          int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int num_colors = parameters().get_int("num_colors");
    if (num_colors <= 0) return 0;

    // Update color position and calculate current color
    color_pos_++;
    if (color_pos_ >= num_colors * 64) color_pos_ = 0;

    uint32_t current_color;
    {
        int p = color_pos_ / 64;
        int r = color_pos_ & 63;

        uint32_t c1 = parameters().get_color("color_" + std::to_string(p));
        uint32_t c2;
        if (p + 1 < num_colors) {
            c2 = parameters().get_color("color_" + std::to_string(p + 1));
        } else {
            c2 = parameters().get_color("color_0");
        }

        // Interpolate colors
        int r1 = (((c1 & 255) * (63 - r)) + ((c2 & 255) * r)) / 64;
        int r2 = ((((c1 >> 8) & 255) * (63 - r)) + (((c2 >> 8) & 255) * r)) / 64;
        int r3 = ((((c1 >> 16) & 255) * (63 - r)) + (((c2 >> 16) & 255) * r)) / 64;

        current_color = r1 | (r2 << 8) | (r3 << 16);
    }

    // Calculate star center offset based on rotation
    int x = (int)(cos(r1_) * w / 4.0);
    int y = (int)(sin(r1_) * h / 4.0);

    // Draw two stars (one on each side)
    for (int c = 0; c < 2; c++) {
        double r2 = -r1_;
        int s = 0;

        // Find peak in waveform data for this channel
        for (int l = 3; l < 14; l++) {
            if (visdata[0][c][l] > s &&
                visdata[0][c][l] > visdata[0][c][l + 1] + 4 &&
                visdata[0][c][l] > visdata[0][c][l - 1] + 4) {
                s = visdata[0][c][l];
            }
        }

        int a = x;
        int b = y;
        if (c == 1) {
            a = -a;
            b = -b;
        }

        // Calculate star size based on audio peak
        double vw = w / 8.0 * (s + 9) / 88.0;
        double vh = h / 8.0 * (s + 9) / 88.0;

        // Calculate first point of star
        int lx = w / 2 + a + (int)(cos(r2) * vw);
        int ly = h / 2 + b + (int)(sin(r2) * vh);

        r2 += PI * 4.0 / 5.0;

        // Draw 5-pointed star (pentagram)
        for (int t = 0; t < 5; t++) {
            int nx = (int)(cos(r2) * vw + w / 2 + a);
            int ny = (int)(sin(r2) * vh + h / 2 + b);
            r2 += PI * 4.0 / 5.0;

            draw_line(framebuffer, lx, ly, nx, ny, w, h, current_color);

            lx = nx;
            ly = ny;
        }
    }

    // Update rotation
    r1_ += 0.1;

    return 0;
}

void RotStarEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Binary format from r_rotstar.cpp:
    // num_colors (int32)
    // colors[num_colors] (int32 each)

    if (reader.remaining() >= 4) {
        int num_colors = reader.read_i32();
        if (num_colors > 16) num_colors = 16;
        if (num_colors < 0) num_colors = 0;
        parameters().set_int("num_colors", num_colors);

        for (int i = 0; i < num_colors && reader.remaining() >= 4; i++) {
            uint32_t color = reader.read_u32();
            parameters().set_color("color_" + std::to_string(i), color);
        }
    }
}

// UI controls from res.rc IDD_CFG_ROTSTAR
static const std::vector<ControlLayout> ui_controls = {
    {
        .id = "colors_group",
        .text = "Colors",
        .type = ControlType::GROUPBOX,
        .x = 0, .y = 0, .w = 137, .h = 36
    },
    {
        .id = "cycle_label",
        .text = "Cycle through",
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
        .id = "colors_label",
        .text = "colors (max 16)",
        .type = ControlType::LABEL,
        .x = 77, .y = 8, .w = 48, .h = 8
    },
    {
        .id = "colors",
        .text = "",
        .type = ControlType::COLOR_ARRAY,
        .x = 6, .y = 21, .w = 127, .h = 11,
        .default_val = 0xFFFFFF,  // White
        .max_items = 16
    }
};

const PluginInfo RotStarEffect::effect_info{
    .name = "Rotating Stars",
    .category = "Render",
    .description = "Two rotating 5-pointed stars driven by waveform peaks",
    .author = "",
    .version = 1,
    .legacy_index = 15,  // R_RotStar
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<RotStarEffect>();
    },
    .ui_layout = {ui_controls},
    .help_text = ""
};

// Register effect at startup
static bool register_rotstar = []() {
    PluginManager::instance().register_effect(RotStarEffect::effect_info);
    return true;
}();

} // namespace avs
