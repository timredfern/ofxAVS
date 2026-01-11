// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "set_render_mode_effect.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"

namespace avs {

SetRenderModeEffect::SetRenderModeEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int SetRenderModeEffect::render(AudioData visdata, int isBeat,
                                uint32_t* framebuffer, uint32_t* fbout,
                                int w, int h) {
    if (isBeat & 0x80000000) return 0;

    if (parameters().get_bool("enabled")) {
        // Build newmode value matching original format
        int blend_mode = parameters().get_int("blend_mode");
        int alpha = parameters().get_int("alpha");
        int line_width = parameters().get_int("line_width");

        // Line style flags (bits 24-26) - new extension
        int line_style = 0;
        if (parameters().get_bool("anti_aliased")) line_style |= LINE_STYLE_AA;
        if (parameters().get_bool("angle_corrected")) line_style |= LINE_STYLE_ANGLE_CORRECT;
        if (parameters().get_bool("rounded_ends")) line_style |= LINE_STYLE_ROUNDED;

        // Format: 0x80000000 | (line_style << 24) | (line_width << 16) | (alpha << 8) | blend_mode
        g_line_blend_mode = 0x80000000 | (line_style << 24) |
                           ((line_width & 0xFF) << 16) |
                           ((alpha & 0xFF) << 8) | (blend_mode & 0xFF);
    }

    return 0;  // No framebuffer modification
}

void SetRenderModeEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Binary format from r_linemode.cpp:
    // newmode (int32) - packed value:
    //   bit 31: enabled
    //   bits 0-7: blend mode
    //   bits 8-15: alpha (for adjustable blend)
    //   bits 16-23: line width

    if (reader.remaining() >= 4) {
        int newmode = reader.read_u32();

        parameters().set_bool("enabled", (newmode & 0x80000000) != 0);
        parameters().set_int("blend_mode", newmode & 0xFF);
        parameters().set_int("alpha", (newmode >> 8) & 0xFF);
        parameters().set_int("line_width", (newmode >> 16) & 0xFF);
    }
}

// Static member definition
const PluginInfo SetRenderModeEffect::effect_info {
    .name = "Set Render Mode",
    .category = "Misc",
    .description = "Control rendering pipeline",
    .author = "",
    .version = 1,
    .legacy_index = 40,  // R_LineMode
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<SetRenderModeEffect>();
    },
    .ui_layout = {
        {
            // From res.rc IDD_CFG_LINEMODE
            {
                .id = "enabled",
                .text = "Enable mode change",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 2, .w = 83, .h = 10,
                .default_val = true
            },
            {
                .id = "blend_group",
                .text = "Set blend mode to",
                .type = ControlType::GROUPBOX,
                .x = 0, .y = 15, .w = 136, .h = 37
            },
            {
                .id = "blend_mode",
                .text = "",
                .type = ControlType::DROPDOWN,
                .x = 5, .y = 25, .w = 128, .h = 184,
                .default_val = 0,
                .options = {
                    "Replace",
                    "Additive",
                    "Maximum Blend",
                    "50/50 Blend",
                    "Subtractive Blend 1",
                    "Subtractive Blend 2",
                    "Multiply Blend",
                    "Adjustable Blend",
                    "XOR",
                    "Minimum Blend"
                }
            },
            {
                .id = "alpha",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 1, .y = 39, .w = 132, .h = 11,
                .range = {0, 255},
                .default_val = 128
            },
            {
                .id = "line_width_label",
                .text = "Line width (pixels)",
                .type = ControlType::LABEL,
                .x = 0, .y = 55, .w = 56, .h = 8
            },
            {
                .id = "line_width",
                .text = "",
                .type = ControlType::TEXT_INPUT,
                .x = 58, .y = 53, .w = 20, .h = 12,
                .range = {1, 255},
                .default_val = 1
            },
            // Line style options (new extension - bits 24-26)
            {
                .id = "line_style_group",
                .text = "Line style",
                .type = ControlType::GROUPBOX,
                .x = 0, .y = 68, .w = 136, .h = 45
            },
            {
                .id = "anti_aliased",
                .text = "Anti-aliased",
                .type = ControlType::CHECKBOX,
                .x = 5, .y = 78, .w = 60, .h = 10,
                .default_val = 0
            },
            {
                .id = "angle_corrected",
                .text = "Angle-corrected thickness",
                .type = ControlType::CHECKBOX,
                .x = 5, .y = 88, .w = 100, .h = 10,
                .default_val = 0
            },
            {
                .id = "rounded_ends",
                .text = "Rounded endpoints",
                .type = ControlType::CHECKBOX,
                .x = 5, .y = 98, .w = 80, .h = 10,
                .default_val = 0
            }
        }
    }
};

// Register effect at startup
static bool register_set_render_mode = []() {
    PluginManager::instance().register_effect(SetRenderModeEffect::effect_info);
    return true;
}();

} // namespace avs
