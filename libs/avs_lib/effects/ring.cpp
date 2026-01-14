// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Port of r_oscring.cpp from original AVS

#include "ring.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/line_draw.h"
#include <cmath>
#include <algorithm>

namespace avs {

static constexpr double PI = 3.14159265358979323846;

RingEffect::RingEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int RingEffect::render(AudioData visdata, int isBeat,
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

    // Get audio data - source: 0=waveform (visdata[1]), 1=spectrum (visdata[0])
    int which_ch = parameters().get_int("channel");
    int source = parameters().get_int("source");
    char center_channel[576];
    unsigned char* fa_data;

    if (which_ch >= 2) {
        // Center channel - average left and right
        for (int x = 0; x < 576; x++) {
            center_channel[x] = visdata[source ? 0 : 1][0][x] / 2 + visdata[source ? 0 : 1][1][x] / 2;
        }
        fa_data = (unsigned char*)center_channel;
    } else {
        fa_data = (unsigned char*)&visdata[source ? 0 : 1][which_ch][0];
    }

    // Get position and size settings
    int x_pos = parameters().get_int("position");
    int size = parameters().get_int("size");

    double s = size / 32.0;
    double is = std::min((double)(h * s), (double)(w * s));

    int c_x;
    int c_y = h / 2;
    if (x_pos == 2) c_x = w / 2;        // Center
    else if (x_pos == 0) c_x = w / 4;   // Left
    else c_x = w / 2 + w / 4;           // Right

    // Draw ring - 80 line segments around a circle
    int q = 0;
    double a = 0.0;
    double sca;

    // Calculate initial scale from audio
    if (!source) {
        sca = 0.1 + ((fa_data[q] ^ 128) / 255.0) * 0.9;
    } else {
        sca = 0.1 + ((fa_data[q * 2] / 2 + fa_data[q * 2 + 1] / 2) / 255.0) * 0.9;
    }

    int lx = c_x + (int)(cos(a) * is * sca);
    int ly = c_y + (int)(sin(a) * is * sca);

    for (q = 1; q <= 80; q++) {
        int tx, ty;

        a -= PI * 2.0 / 80.0;

        // Mirror audio data for symmetric ring: use q or (80-q) depending on position
        int audio_idx = q > 40 ? 80 - q : q;

        if (!source) {
            sca = 0.1 + ((fa_data[audio_idx] ^ 128) / 255.0) * 0.90;
        } else {
            sca = 0.1 + ((fa_data[audio_idx * 2] / 2 + fa_data[audio_idx * 2 + 1] / 2) / 255.0) * 0.9;
        }

        tx = c_x + (int)(cos(a) * is * sca);
        ty = c_y + (int)(sin(a) * is * sca);

        if ((tx >= 0 && tx < w && ty >= 0 && ty < h) ||
            (lx >= 0 && lx < w && ly >= 0 && ly < h)) {
            draw_line(framebuffer, tx, ty, lx, ly, w, h, current_color);
        }

        lx = tx;
        ly = ty;
    }

    return 0;
}

void RingEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Binary format from r_oscring.cpp:
    // effect (int32) - bits 2-3: channel, bits 4-5: position
    // num_colors (int32)
    // colors[num_colors] (int32 each)
    // size (int32)
    // source (int32) - 0=waveform, 1=spectrum

    if (reader.remaining() >= 4) {
        int effect = reader.read_i32();
        int which_ch = (effect >> 2) & 3;
        int y_pos = (effect >> 4) & 3;
        parameters().set_int("channel", which_ch);
        parameters().set_int("position", y_pos);
    }

    if (reader.remaining() >= 4) {
        int num_colors = reader.read_i32();
        if (num_colors > 16) num_colors = 16;
        if (num_colors < 1) num_colors = 1;
        parameters().set_int("num_colors", num_colors);

        for (int i = 0; i < num_colors && reader.remaining() >= 4; i++) {
            uint32_t color = reader.read_u32();
            parameters().set_color("color_" + std::to_string(i), color);
        }
    }

    if (reader.remaining() >= 4) {
        parameters().set_int("size", reader.read_i32());
    }

    if (reader.remaining() >= 4) {
        parameters().set_int("source", reader.read_i32());
    }
}

// UI controls from res.rc IDD_CFG_OSCRING
static const std::vector<ControlLayout> ui_controls = {
    {
        .id = "size_group",
        .text = "Ring size",
        .type = ControlType::GROUPBOX,
        .x = 0, .y = 0, .w = 137, .h = 42
    },
    {
        .id = "small_label",
        .text = "Small",
        .type = ControlType::LABEL,
        .x = 8, .y = 29, .w = 18, .h = 8
    },
    {
        .id = "large_label",
        .text = "Large",
        .type = ControlType::LABEL,
        .x = 106, .y = 30, .w = 19, .h = 8
    },
    {
        .id = "size",
        .text = "",
        .type = ControlType::SLIDER,
        .x = 4, .y = 10, .w = 128, .h = 15,
        .range = {1, 64},
        .default_val = 8
    },
    {
        .id = "source_group",
        .text = "Ring source and position",
        .type = ControlType::GROUPBOX,
        .x = 0, .y = 44, .w = 137, .h = 57
    },
    {
        .id = "source",
        .text = "",
        .type = ControlType::RADIO_GROUP,
        .x = 8, .y = 56, .w = 110, .h = 12,
        .default_val = 0,
        .radio_options = {
            {.label = "Oscilloscope", .x = 0, .y = 0},
            {.label = "Spectrum", .x = 57, .y = 0}
        }
    },
    {
        .id = "channel",
        .text = "",
        .type = ControlType::RADIO_GROUP,
        .x = 8, .y = 68, .w = 70, .h = 33,
        .default_val = 0,
        .radio_options = {
            {.label = "Left Channel", .x = 0, .y = 0},
            {.label = "Right Channel", .x = 0, .y = 10},
            {.label = "Center Channel", .x = 0, .y = 20}
        }
    },
    {
        .id = "position",
        .text = "",
        .type = ControlType::RADIO_GROUP,
        .x = 86, .y = 68, .w = 45, .h = 33,
        .default_val = 2,  // Center
        .radio_options = {
            {.label = "Left", .x = 0, .y = 0},
            {.label = "Right", .x = 0, .y = 10},
            {.label = "Center", .x = 0, .y = 20}
        }
    },
    {
        .id = "colors_group",
        .text = "Colors",
        .type = ControlType::GROUPBOX,
        .x = 0, .y = 101, .w = 137, .h = 36
    },
    {
        .id = "cycle_label",
        .text = "Cycle through",
        .type = ControlType::LABEL,
        .x = 7, .y = 109, .w = 46, .h = 8
    },
    {
        .id = "num_colors",
        .text = "",
        .type = ControlType::TEXT_INPUT,
        .x = 53, .y = 107, .w = 19, .h = 12,
        .range = {1, 16},
        .default_val = 1
    },
    {
        .id = "colors_label",
        .text = "colors (max 16)",
        .type = ControlType::LABEL,
        .x = 77, .y = 109, .w = 48, .h = 8
    },
    {
        .id = "colors",
        .text = "",
        .type = ControlType::COLOR_ARRAY,
        .x = 6, .y = 122, .w = 127, .h = 11,
        .default_val = 0xFFFFFF,  // White
        .max_items = 16
    }
};

const PluginInfo RingEffect::effect_info{
    .name = "Ring",
    .category = "Render",
    .description = "Audio-reactive ring visualization",
    .author = "",
    .version = 1,
    .legacy_index = 13,  // R_OscRings
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<RingEffect>();
    },
    .ui_layout = {ui_controls},
    .help_text = ""
};

// Register effect at startup
static bool register_ring = []() {
    PluginManager::instance().register_effect(RingEffect::effect_info);
    return true;
}();

} // namespace avs
