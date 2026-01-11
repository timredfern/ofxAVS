// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "dynamic_movement_effect.h"
#include "core/binary_reader.h"
#include "core/parameter.h"
#include "core/plugin_manager.h"
#include "core/ui.h"
#include <cmath>
#include <cstring>

namespace avs {

// Example presets from original r_dmove.cpp
struct DMovPreset {
    const char* name;
    bool rect;      // rectangular coordinates
    bool wrap;
    int grid_w;
    int grid_h;
    const char* init;
    const char* pixel;
    const char* frame;
    const char* beat;
};

static const DMovPreset dmov_presets[] = {
    {"(current)", false, false, 16, 16, "", "d=d*0.9", "", ""},
    {"Random Rotate", false, true, 2, 2, "", "r = r + dr;", "", "dr = (rand(100) / 100) * $PI;\r\nd = d * .95;"},
    {"Random Direction", true, true, 2, 2, "speed=.05;dr = (rand(200) / 100) * $PI;", "x = x + dx;\r\ny = y + dy;", "dx = cos(dr) * speed;\r\ndy = sin(dr) * speed;", "dr = (rand(200) / 100) * $PI;"},
    {"In and Out", false, true, 2, 2, "speed=.2;c=0;", "d = d * dd;", "", "c = c + ($PI/2);\r\ndd = 1 - (sin(c) * speed);"},
    {"Unspun Kaleida", false, true, 33, 33, "c=200;f=0;dt=0;dl=0;beatdiv=8", "r=cos(r*dr);", "f = f + 1;\r\nt = ((f * $pi * 2)/c)/beatdiv;\r\ndt = dl + t;\r\ndr = 4+(cos(dt)*2);", "c=f;f=0;dl=dt"},
    {"Roiling Gridley", true, true, 32, 32, "c=200;f=0;dt=0;dl=0;beatdiv=8", "x=x+(sin(y*dx) * .03);\r\ny=y-(cos(x*dy) * .03);", "f = f + 1;\r\nt = ((f * $pi * 2)/c)/beatdiv;\r\ndt = dl + t;\r\ndx = 14+(cos(dt)*8);\r\ndy = 10+(sin(dt*2)*4);", "c=f;f=0;dl=dt"},
    {"6-Way Outswirl", false, false, 32, 32, "c=200;f=0;dt=0;dl=0;beatdiv=8", "d=d*(1+(cos(r*6) * .05));\r\nr=r-(sin(d*dr) * .05);\r\nd = d * .98;", "f = f + 1;\r\nt = ((f * $pi * 2)/c)/beatdiv;\r\ndt = dl + t;\r\ndr = 18+(cos(dt)*12);", "c=f;f=0;dl=dt"},
    {"Wavy", true, true, 6, 6, "c=200;f=0;dx=0;dl=0;beatdiv=16;speed=.05", "y = y + ((sin((x+dx) * $PI))*speed);\r\nx = x + .025", "f = f + 1;\r\nt = ( (f * 2 * 3.1415) / c ) / beatdiv;\r\ndx = dl + t;", "c = f;\r\nf = 0;\r\ndl = dx;"},
    {"Smooth Rotoblitter", false, true, 2, 2, "c=200;f=0;dt=0;dl=0;beatdiv=4;speed=.15", "r = r + dr;\r\nd = d * dd;", "f = f + 1;\r\nt = ((f * $pi * 2)/c)/beatdiv;\r\ndt = dl + t;\r\ndr = cos(dt)*speed*2;\r\ndd = 1 - (sin(dt)*speed);", "c=f;f=0;dl=dt"},
};

static constexpr int NUM_DMOV_PRESETS = 9;  // Number of presets in dmov_presets array

DynamicMovementEffect::DynamicMovementEffect()
{
    init_parameters_from_layout(effect_info.ui_layout);
}

void DynamicMovementEffect::on_parameter_changed(const std::string& param_name) {
    // Run init script when it changes
    if (param_name == "init_script") {
        engine_.evaluate(parameters().get_string("init_script"));
    }

    // Load preset when selected
    if (param_name == "example") {
        int preset_idx = parameters().get_int("example");
        if (preset_idx > 0 && preset_idx < NUM_DMOV_PRESETS) {
            const auto& preset = dmov_presets[preset_idx];
            parameters().set_string("init_script", preset.init);
            parameters().set_string("frame_script", preset.frame);
            parameters().set_string("beat_script", preset.beat);
            parameters().set_string("pixel_script", preset.pixel);
            parameters().set_int("grid_width", preset.grid_w);
            parameters().set_int("grid_height", preset.grid_h);
            parameters().set_bool("rectangular", preset.rect);
            parameters().set_bool("wrap", preset.wrap);
            // Run init script (set_string doesn't trigger on_parameter_changed)
            engine_.evaluate(preset.init);
        }
        // Reset to (current) after loading
        parameters().set_int("example", 0);
    }
}

int DynamicMovementEffect::render(AudioData visdata, int isBeat,
                                 uint32_t* framebuffer, uint32_t* fbout,
                                 int w, int h) {
    if (!is_enabled()) return 0;

    if (parameters().get_bool("no_movement")) {
        memcpy(fbout, framebuffer, w * h * sizeof(uint32_t));
        return 1;
    }

    engine_.set_audio_context(visdata, isBeat);
    engine_.evaluate(parameters().get_string("frame_script"));
    if (isBeat) {
        engine_.evaluate(parameters().get_string("beat_script"));
    }

    CoordMode mode = parameters().get_bool("rectangular") ? CoordMode::RECTANGULAR : CoordMode::POLAR;
    grid_.generate(engine_,
                   parameters().get_int("grid_width"),
                   parameters().get_int("grid_height"),
                   w, h,
                   parameters().get_string("pixel_script"),
                   mode, visdata);

    grid_.apply(framebuffer, fbout, w, h,
                parameters().get_bool("bilinear"),
                parameters().get_bool("wrap"),
                parameters().get_bool("blend"));

    return 1;
}

void DynamicMovementEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Binary format from r_dmove.cpp:
    // If first byte == 1: new format with length-prefixed strings
    // Else: old format with 4 x 256 byte fixed strings
    std::string init_script, pixel_script, frame_script, beat_script;

    if (reader.read_u8() == 1) {
        // New format: length-prefixed strings
        init_script = reader.read_length_prefixed_string();
        pixel_script = reader.read_length_prefixed_string();
        frame_script = reader.read_length_prefixed_string();
        beat_script = reader.read_length_prefixed_string();
    } else {
        // Old format: 4 x 256 byte fixed strings (total 1024 bytes)
        reader.skip(static_cast<size_t>(-1));  // Go back to start
        BinaryReader reader2(data);  // Fresh reader from start
        if (reader2.remaining() >= 1024) {
            init_script = reader2.read_string_fixed(256);
            pixel_script = reader2.read_string_fixed(256);
            frame_script = reader2.read_string_fixed(256);
            beat_script = reader2.read_string_fixed(256);
        }
    }

    parameters().set_string("init_script", init_script);
    parameters().set_string("pixel_script", pixel_script);
    parameters().set_string("frame_script", frame_script);
    parameters().set_string("beat_script", beat_script);

    // After strings: subpixel, rectcoords, m_xres, m_yres, blend, wrap, buffern, nomove
    if (reader.remaining() >= 4) {
        int subpixel = reader.read_u32();
        parameters().set_bool("bilinear", subpixel != 0);
    }
    if (reader.remaining() >= 4) {
        int rectcoords = reader.read_u32();
        parameters().set_bool("rectangular", rectcoords != 0);
    }
    if (reader.remaining() >= 4) {
        int m_xres = reader.read_u32();
        parameters().set_int("grid_width", m_xres);
    }
    if (reader.remaining() >= 4) {
        int m_yres = reader.read_u32();
        parameters().set_int("grid_height", m_yres);
    }
    if (reader.remaining() >= 4) {
        int blend = reader.read_u32();
        parameters().set_bool("blend", blend != 0);
    }
    if (reader.remaining() >= 4) {
        int wrap = reader.read_u32();
        parameters().set_bool("wrap", wrap != 0);
    }
    // Skip buffern (internal buffer tracking)
    if (reader.remaining() >= 4) reader.read_u32();
    if (reader.remaining() >= 4) {
        int nomove = reader.read_u32();
        parameters().set_bool("no_movement", nomove != 0);
    }

    // Run init script
    engine_.evaluate(init_script);
}

// Static member definition
const PluginInfo DynamicMovementEffect::effect_info {
    .name = "Dynamic Movement",
    .category = "Trans",
    .description = "Grid-based transformations with scripting",
    .author = "",
    .version = 1,
    .legacy_index = 43,  // R_DMove
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<DynamicMovementEffect>();
    },
    .ui_layout = {
        {
            // Labels for script boxes (from res.rc IDD_CFG_DMOVE)
            {
                .id = "init_label",
                .text = "init",
                .type = ControlType::LABEL,
                .x = 0, .y = 3, .w = 10, .h = 8
            },
            {
                .id = "frame_label",
                .text = "frame",
                .type = ControlType::LABEL,
                .x = 0, .y = 36, .w = 18, .h = 8
            },
            {
                .id = "beat_label",
                .text = "beat",
                .type = ControlType::LABEL,
                .x = 0, .y = 89, .w = 15, .h = 8
            },
            {
                .id = "pixel_label",
                .text = "pixel",
                .type = ControlType::LABEL,
                .x = 0, .y = 142, .w = 15, .h = 8
            },
            // Script editors - IDC_EDIT4, IDC_EDIT2, IDC_EDIT3, IDC_EDIT1
            {
                .id = "init_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 0, .w = 208, .h = 14
            },
            {
                .id = "frame_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 14, .w = 208, .h = 53
            },
            {
                .id = "beat_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 67, .w = 208, .h = 53
            },
            {
                .id = "pixel_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 120, .w = 208, .h = 53,
                .default_val = "d=d*0.9"
            },
            // Example preset dropdown - "source" label with dropdown
            {
                .id = "example_label",
                .text = "source",
                .type = ControlType::LABEL,
                .x = 0, .y = 177, .w = 22, .h = 8
            },
            {
                .id = "example",
                .text = "",
                .type = ControlType::DROPDOWN,
                .x = 25, .y = 175, .w = 85, .h = 56,
                .default_val = 0,
                .options = {
                    "(current)", "Random Rotate", "Random Direction", "In and Out",
                    "Unspun Kaleida", "Roiling Gridley", "6-Way Outswirl", "Wavy",
                    "Smooth Rotoblitter"
                }
            },
            // Grid size labels - from res.rc
            {
                .id = "gridsize_label",
                .text = "Grid size:",
                .type = ControlType::LABEL,
                .x = 78, .y = 192, .w = 30, .h = 8
            },
            {
                .id = "gridx_label",
                .text = "x",
                .type = ControlType::LABEL,
                .x = 128, .y = 191, .w = 8, .h = 8
            },
            // Grid size inputs - IDC_EDIT5, IDC_EDIT6
            {
                .id = "grid_width",
                .text = "",
                .type = ControlType::TEXT_INPUT,
                .x = 108, .y = 190, .w = 18, .h = 12,
                .range = {2, 256},
                .default_val = 16
            },
            {
                .id = "grid_height",
                .text = "",
                .type = ControlType::TEXT_INPUT,
                .x = 136, .y = 190, .w = 18, .h = 12,
                .range = {2, 256},
                .default_val = 16
            },
            // Checkboxes - bottom row
            {
                .id = "blend",
                .text = "Blend",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 190, .w = 34, .h = 10,
                .default_val = 0
            },
            {
                .id = "wrap",
                .text = "Wrap",
                .type = ControlType::CHECKBOX,
                .x = 35, .y = 190, .w = 33, .h = 10,
                .default_val = 0
            },
            {
                .id = "no_movement",
                .text = "No movement (just blend)",
                .type = ControlType::CHECKBOX,
                .x = 113, .y = 176, .w = 100, .h = 10,
                .default_val = 0
            },
            {
                .id = "rectangular",
                .text = "Rectangular coords",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 204, .w = 90, .h = 10,
                .default_val = 0
            },
            {
                .id = "bilinear",
                .text = "Bilinear",
                .type = ControlType::CHECKBOX,
                .x = 94, .y = 204, .w = 63, .h = 10,
                .default_val = 1
            }
        },
        "Dynamic Movement\n"
        "\n"
        "Variables (polar mode):\n"
        "d = distance from center (0=center, 1=edge)\n"
        "r = angle in radians\n"
        "\n"
        "Variables (rectangular mode):\n"
        "x, y = coordinates (-1 to 1)\n"
        "\n"
        "Common variables:\n"
        "w, h = width, height (in pixels)\n"
        "b = isBeat (1 if beat, 0 otherwise)\n"
        "alpha = blend amount (0.0-1.0)\n"
        "\n"
        "Scripts:\n"
        "init: runs once at start\n"
        "frame: runs once per frame\n"
        "beat: runs on beat\n"
        "pixel: runs for each grid point\n"
    }
};

// Register effect at startup
static bool register_dynamic_movement = []() {
    PluginManager::instance().register_effect(DynamicMovementEffect::effect_info);
    return true;
}();

} // namespace avs
