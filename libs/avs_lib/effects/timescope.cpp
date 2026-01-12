// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Port of r_timescope.cpp from original AVS

#include "timescope.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include "core/line_draw_ext.h"

namespace avs {

TimescopeEffect::TimescopeEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int TimescopeEffect::render(AudioData visdata, int isBeat,
                            uint32_t* framebuffer, uint32_t* fbout,
                            int w, int h) {
    if (isBeat & 0x80000000) return 0;
    if (!parameters().get_bool("enabled")) return 0;

    int blend_mode = parameters().get_int("blend_mode");
    int which_ch = parameters().get_int("channel");
    int nbands = parameters().get_int("bands");
    uint32_t color = parameters().get_color("color");

    // Build center channel if needed
    char center_channel[576];
    const unsigned char* fa_data;

    if (which_ch >= 2) {
        // Center channel - average left and right
        for (int j = 0; j < 576; j++) {
            center_channel[j] = visdata[1][0][j] / 2 + visdata[1][1][j] / 2;
        }
        fa_data = reinterpret_cast<const unsigned char*>(center_channel);
    } else {
        fa_data = reinterpret_cast<const unsigned char*>(&visdata[1][which_ch][0]);
    }

    // Advance x position and wrap
    x_++;
    x_ %= w;

    // Get color components
    int r = color & 0xff;
    int g = (color >> 8) & 0xff;
    int b = (color >> 16) & 0xff;

    // Draw vertical line at current x position
    uint32_t* fb = framebuffer + x_;
    for (int i = 0; i < h; i++) {
        // Sample spectrum at proportional band index
        int c = visdata[0][0][(i * nbands) / h] & 0xFF;

        // Scale color by spectrum value
        uint32_t pixel = ((r * c) / 256) |
                        (((g * c) / 256) << 8) |
                        (((b * c) / 256) << 16);

        // Apply blend mode
        switch (blend_mode) {
            case 0:  // Replace
                fb[0] = pixel;
                break;
            case 1:  // Additive
                fb[0] = BLEND(fb[0], pixel);
                break;
            case 2:  // 50/50
                fb[0] = BLEND_AVG(fb[0], pixel);
                break;
            case 3:  // Default render blend
                BLEND_LINE(fb, pixel);
                break;
        }
        fb += w;
    }

    return 0;
}

void TimescopeEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Binary format from r_timescope.cpp:
    // enabled (int32)
    // color (int32)
    // blend (int32)
    // blendavg (int32)
    // which_ch (int32)
    // nbands (int32)

    if (reader.remaining() >= 4) {
        parameters().set_bool("enabled", reader.read_i32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_color("color", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        int blend = reader.read_i32();
        if (reader.remaining() >= 4) {
            int blendavg = reader.read_i32();
            // Convert blend/blendavg to unified blend_mode
            // blend=2 -> default render (3)
            // blend=1 -> additive (1)
            // blend=0 && blendavg=1 -> 50/50 (2)
            // blend=0 && blendavg=0 -> replace (0)
            if (blend == 2) {
                parameters().set_int("blend_mode", 3);
            } else if (blend == 1) {
                parameters().set_int("blend_mode", 1);
            } else if (blendavg) {
                parameters().set_int("blend_mode", 2);
            } else {
                parameters().set_int("blend_mode", 0);
            }
        }
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("channel", reader.read_i32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("bands", reader.read_i32());
    }
}

// UI controls from res.rc IDD_CFG_TIMESCOPE
static const std::vector<ControlLayout> ui_controls = {
    {
        .id = "enabled",
        .text = "Enable Timescope",
        .type = ControlType::CHECKBOX,
        .x = 0, .y = 0, .w = 75, .h = 10,
        .default_val = true
    },
    {
        .id = "color",
        .text = "Color",
        .type = ControlType::COLOR_BUTTON,
        .x = 0, .y = 15, .w = 137, .h = 13,
        .default_val = static_cast<int>(0xFFFFFF)
    },
    {
        .id = "blend_mode",
        .text = "",
        .type = ControlType::RADIO_GROUP,
        .x = 0, .y = 30, .w = 80, .h = 40,
        .default_val = 3,  // Default render blend
        .radio_options = {
            {.label = "Replace", .x = 0, .y = 0},
            {.label = "Additive blend", .x = 0, .y = 10},
            {.label = "Blend 50/50", .x = 0, .y = 20},
            {.label = "Default render blend", .x = 0, .y = 29}
        }
    },
    {
        .id = "bands_label",
        .text = "Draw 576 bands",
        .type = ControlType::LABEL,
        .x = 0, .y = 84, .w = 137, .h = 8
    },
    {
        .id = "bands",
        .text = "",
        .type = ControlType::SLIDER,
        .x = 0, .y = 95, .w = 137, .h = 11,
        .range = {16, 576},
        .default_val = 576
    },
    {
        .id = "channel",
        .text = "",
        .type = ControlType::RADIO_GROUP,
        .x = 0, .y = 108, .w = 64, .h = 30,
        .default_val = 2,  // Center channel
        .radio_options = {
            {.label = "Left channel", .x = 0, .y = 0},
            {.label = "Right channel", .x = 0, .y = 10},
            {.label = "Center channel", .x = 0, .y = 20}
        }
    }
};

const PluginInfo TimescopeEffect::effect_info{
    .name = "Timescope",
    .category = "Render",
    .description = "Scrolling spectrogram visualization",
    .author = "",
    .version = 1,
    .legacy_index = 26,  // R_Timescope
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<TimescopeEffect>();
    },
    .ui_layout = {ui_controls},
    .help_text = ""
};

// Register effect at startup
static bool register_timescope = []() {
    PluginManager::instance().register_effect(TimescopeEffect::effect_info);
    return true;
}();

} // namespace avs
