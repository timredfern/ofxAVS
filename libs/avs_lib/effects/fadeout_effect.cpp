// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "fadeout_effect.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"

namespace avs {

FadeoutEffect::FadeoutEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    make_table();
}

void FadeoutEffect::on_parameter_changed(const std::string& param_name) {
    if (param_name == "fadelen" || param_name == "color") {
        make_table();
    }
}

void FadeoutEffect::make_table() {
    uint32_t color = parameters().get_color("color");
    int fadelen = parameters().get_int("fadelen");

    // Extract target RGB values
    int rseek = color & 0xff;
    int gseek = (color >> 8) & 0xff;
    int bseek = (color >> 16) & 0xff;

    // Build lookup tables - move each channel value toward target by fadelen
    for (int x = 0; x < 256; x++) {
        int r = x;
        int g = x;
        int b = x;

        // Move R toward target
        if (r <= rseek - fadelen) r += fadelen;
        else if (r >= rseek + fadelen) r -= fadelen;
        else r = rseek;

        // Move G toward target
        if (g <= gseek - fadelen) g += fadelen;
        else if (g >= gseek + fadelen) g -= fadelen;
        else g = gseek;

        // Move B toward target
        if (b <= bseek - fadelen) b += fadelen;
        else if (b >= bseek + fadelen) b -= fadelen;
        else b = bseek;

        fadtab_[0][x] = static_cast<unsigned char>(r);
        fadtab_[1][x] = static_cast<unsigned char>(g);
        fadtab_[2][x] = static_cast<unsigned char>(b);
    }
}

int FadeoutEffect::render(AudioData visdata, int isBeat,
                          uint32_t* framebuffer, uint32_t* fbout,
                          int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int fadelen = parameters().get_int("fadelen");
    if (!fadelen) return 0;

    // Process each pixel using lookup tables
    // Original comment: "alphachannel safe 11/21/99"
    // t[0]=R, t[1]=G, t[2]=B, t[3]=A (preserved)
    int count = w * h;
    for (int i = 0; i < count; i++) {
        uint32_t pix = framebuffer[i];

        unsigned char r = fadtab_[0][pix & 0xff];
        unsigned char g = fadtab_[1][(pix >> 8) & 0xff];
        unsigned char b = fadtab_[2][(pix >> 16) & 0xff];

        // Preserve alpha channel
        framebuffer[i] = (pix & 0xFF000000) | (b << 16) | (g << 8) | r;
    }

    return 0;  // Modified framebuffer in-place
}

void FadeoutEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_fadeout.cpp:
    // fadelen (int32)
    // color (int32, BGR format)
    int fadelen = reader.read_u32();
    parameters().set_int("fadelen", fadelen);

    if (reader.remaining() >= 4) {
        uint32_t color = BinaryReader::bgr_to_argb(reader.read_u32());
        parameters().set_color("color", color);
    }

    make_table();
}

// Static member definition - UI layout from res.rc IDD_CFG_FADE
const PluginInfo FadeoutEffect::effect_info {
    .name = "Fadeout",
    .category = "Trans",
    .description = "Gradually fades the screen to a specified color",
    .author = "",
    .version = 1,
    .legacy_index = 3,  // R_FadeOut
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<FadeoutEffect>();
    },
    .ui_layout = {
        {
            // Fade velocity groupbox label
            {
                .id = "velocity_group",
                .text = "Fade velocity",
                .type = ControlType::GROUPBOX,
                .x = 0, .y = 0, .w = 137, .h = 47
            },
            // Fade velocity slider (range 0-92, default 16)
            {
                .id = "fadelen",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 3, .y = 9, .w = 129, .h = 13,
                .range = {0, 92, 1},
                .default_val = 16
            },
            // Labels for slider
            {
                .id = "slow_label",
                .text = "None .. slow",
                .type = ControlType::LABEL,
                .x = 7, .y = 25, .w = 40, .h = 8
            },
            {
                .id = "fast_label",
                .text = "..fast",
                .type = ControlType::LABEL,
                .x = 110, .y = 25, .w = 16, .h = 8
            },
            // Fade to color button
            {
                .id = "color",
                .text = "fade to color",
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 51, .w = 78, .h = 20,
                .default_val = 0  // Black
            }
        }
    }
};

// Register effect at startup
static bool register_fadeout = []() {
    PluginManager::instance().register_effect(FadeoutEffect::effect_info);
    return true;
}();

} // namespace avs
