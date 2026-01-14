// avs_lib - Portable Advanced Visualization Studio library
// Extended Set Render Mode - NOT part of original AVS
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "set_render_mode_ext.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include "core/blend_ext.h"
#include <algorithm>
#include <cmath>

namespace avs {

SetRenderModeExtEffect::SetRenderModeExtEffect() {
    init_parameters_from_layout(effect_info.ui_layout);

    // Initialize script variables to defaults
    engine_.set_variable("lw", 1.0);      // line width
    engine_.set_variable("bm", 0.0);      // blend mode
    engine_.set_variable("a", 128.0);     // alpha
}

int SetRenderModeExtEffect::render(AudioData visdata, int isBeat,
                                   uint32_t* framebuffer, uint32_t* fbout,
                                   int w, int h) {
    if (isBeat & 0x80000000) return 0;

    if (!parameters().get_bool("enabled")) return 0;

    // Get base values from UI
    int blend_mode = parameters().get_int("blend_mode");
    int alpha = parameters().get_int("alpha");
    int line_width = parameters().get_int("line_width");
    int line_style = 0;

    // Line style flags
    if (parameters().get_bool("anti_aliased")) line_style |= LINE_STYLE_AA;
    if (parameters().get_bool("angle_corrected")) line_style |= LINE_STYLE_ANGLE_CORRECT;
    if (parameters().get_bool("rounded_ends")) line_style |= LINE_STYLE_ROUNDED;
    if (parameters().get_bool("point_size")) line_style |= LINE_STYLE_POINTSIZE;

    // Get scripts
    std::string init_script = parameters().get_string("init_script");
    std::string frame_script = parameters().get_string("frame_script");
    std::string beat_script = parameters().get_string("beat_script");

    // Run scripts if any are defined
    bool use_scripts = !init_script.empty() || !frame_script.empty() || !beat_script.empty();

    if (use_scripts) {
        // Set up engine variables
        engine_.set_variable("w", static_cast<double>(w));
        engine_.set_variable("h", static_cast<double>(h));
        engine_.set_variable("b", isBeat ? 1.0 : 0.0);

        // Initialize from UI values once
        if (!inited_) {
            engine_.set_variable("lw", static_cast<double>(line_width));
            engine_.set_variable("bm", static_cast<double>(blend_mode));
            engine_.set_variable("a", static_cast<double>(alpha));

            // Run init script if present
            if (!init_script.empty()) {
                engine_.evaluate(init_script);
            }
            inited_ = true;
        }

        // Run frame script
        if (!frame_script.empty()) {
            engine_.evaluate(frame_script);
        }

        // Run beat script
        if (isBeat && !beat_script.empty()) {
            engine_.evaluate(beat_script);
        }

        // Get values from script
        line_width = static_cast<int>(std::clamp(engine_.get_variable("lw"), 1.0, 255.0));
        blend_mode = static_cast<int>(std::clamp(engine_.get_variable("bm"), 0.0, 9.0));
        alpha = static_cast<int>(std::clamp(engine_.get_variable("a"), 0.0, 255.0));
    }

    // Format: 0x80000000 | (line_style << 24) | (line_width << 16) | (alpha << 8) | blend_mode
    g_line_blend_mode = 0x80000000 | (line_style << 24) |
                       ((line_width & 0xFF) << 16) |
                       ((alpha & 0xFF) << 8) | (blend_mode & 0xFF);

    return 0;  // No framebuffer modification
}

void SetRenderModeExtEffect::load_parameters(const std::vector<uint8_t>& data) {
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

    // Reset init state when loading
    inited_ = false;
}

void SetRenderModeExtEffect::on_parameter_changed(const std::string& param_name) {
    // Re-init when any script changes
    if (param_name == "init_script" || param_name == "frame_script" ||
        param_name == "beat_script") {
        inited_ = false;
    }
}

// Extended UI controls (base + line styles + scripting)
static const std::vector<ControlLayout> ui_controls = {
    // Base controls (from res.rc IDD_CFG_LINEMODE)
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
        .id = "line_width",
        .text = "Line/point width",
        .type = ControlType::SLIDER,
        .x = 0, .y = 53, .w = 136, .h = 13,
        .range = {1, 100, 1},
        .default_val = 1
    },
    // Line style options (extension)
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
    },
    {
        .id = "point_size",
        .text = "Apply size to points",
        .type = ControlType::CHECKBOX,
        .x = 70, .y = 78, .w = 65, .h = 10,
        .default_val = 0
    },
    // Scripting section (extension)
    {
        .id = "script_group",
        .text = "Dynamic scripting",
        .type = ControlType::GROUPBOX,
        .x = 0, .y = 115, .w = 240, .h = 105
    },
    {
        .id = "init_label",
        .text = "init",
        .type = ControlType::LABEL,
        .x = 5, .y = 128, .w = 15, .h = 8
    },
    {
        .id = "init_script",
        .text = "",
        .type = ControlType::EDITTEXT,
        .x = 30, .y = 125, .w = 205, .h = 26
    },
    {
        .id = "frame_label",
        .text = "frame",
        .type = ControlType::LABEL,
        .x = 5, .y = 156, .w = 20, .h = 8
    },
    {
        .id = "frame_script",
        .text = "",
        .type = ControlType::EDITTEXT,
        .x = 30, .y = 153, .w = 205, .h = 26
    },
    {
        .id = "beat_label",
        .text = "beat",
        .type = ControlType::LABEL,
        .x = 5, .y = 184, .w = 15, .h = 8
    },
    {
        .id = "beat_script",
        .text = "",
        .type = ControlType::EDITTEXT,
        .x = 30, .y = 181, .w = 205, .h = 26
    }
};

// Static member definition
const PluginInfo SetRenderModeExtEffect::effect_info {
    .name = "Set Render Mode (extended)",
    .category = "Misc",
    .description = "Control rendering pipeline with scripting",
    .author = "",
    .version = 1,
    .legacy_index = -1,  // No legacy index - this is an extension
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<SetRenderModeExtEffect>();
    },
    .ui_layout = { ui_controls },
    .help_text =
        "Set Render Mode (Extended) - Scripting\n"
        "\n"
        "Variables:\n"
        "  lw   Line width (1-255)\n"
        "  bm   Blend mode (0-9)\n"
        "  a    Alpha for adjustable blend (0-255)\n"
        "  b    Beat (1 on beat, else 0)\n"
        "  w,h  Screen size\n"
        "\n"
        "Blend modes: 0=replace, 1=add, 2=max,\n"
        "3=50/50, 4=sub1, 5=sub2, 6=mul,\n"
        "7=adjustable, 8=xor, 9=min\n"
        "\n"
        "Examples:\n"
        "  init: lw=1; dir=1\n"
        "  frame: lw=lw+dir; if(lw>10,dir=-1,0)\n"
        "  beat: bm=rand(10)\n"
};

// Register effect at startup
static bool register_set_render_mode_ext = []() {
    PluginManager::instance().register_effect(SetRenderModeExtEffect::effect_info);
    return true;
}();

} // namespace avs
