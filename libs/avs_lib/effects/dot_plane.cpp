// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Port of r_dotpln.cpp from original AVS

#include "dot_plane.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include "core/line_draw_ext.h"
#include "core/matrix.h"
#include <algorithm>
#include <cstring>

namespace avs {

DotPlaneEffect::DotPlaneEffect() : rotation_(0.0f) {
    init_parameters_from_layout(effect_info.ui_layout);

    std::memset(atable_, 0, sizeof(atable_));
    std::memset(vtable_, 0, sizeof(vtable_));
    std::memset(ctable_, 0, sizeof(ctable_));

    init_color_table();
}

void DotPlaneEffect::init_color_table() {
    // Build gradient from 5 colors (64 entries)
    uint32_t colors[5];
    colors[0] = parameters().get_color("color_bottom");
    colors[1] = parameters().get_color("color_low");
    colors[2] = parameters().get_color("color_mid");
    colors[3] = parameters().get_color("color_high");
    colors[4] = parameters().get_color("color_top");

    for (int t = 0; t < 4; t++) {
        int r = (colors[t] & 0xff) << 16;
        int g = ((colors[t] >> 8) & 0xff) << 16;
        int b = ((colors[t] >> 16) & 0xff) << 16;
        int dr = (((colors[t + 1] & 0xff) - (colors[t] & 0xff)) << 16) / 16;
        int dg = ((((colors[t + 1] >> 8) & 0xff) - ((colors[t] >> 8) & 0xff)) << 16) / 16;
        int db = ((((colors[t + 1] >> 16) & 0xff) - ((colors[t] >> 16) & 0xff)) << 16) / 16;

        for (int x = 0; x < 16; x++) {
            color_tab_[t * 16 + x] = (r >> 16) | ((g >> 16) << 8) | ((b >> 16) << 16);
            r += dr;
            g += dg;
            b += db;
        }
    }
}

int DotPlaneEffect::render(AudioData visdata, int isBeat,
                           uint32_t* framebuffer, uint32_t* fbout,
                           int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int rotvel = parameters().get_int("rotation_speed");
    int angle = parameters().get_int("angle");

    // Build transformation matrix
    float matrix[16], matrix2[16];
    matrix_rotate(matrix, 2, rotation_);           // Rotate around Z (actually Y in AVS coords)
    matrix_rotate(matrix2, 1, static_cast<float>(angle));  // Tilt
    matrix_multiply(matrix, matrix2);
    matrix_translate(matrix2, 0.0f, -20.0f, 400.0f);  // Move back into view
    matrix_multiply(matrix, matrix2);

    // Store first row for wraparound
    float btable[NUM_WIDTH];
    std::memcpy(btable, &atable_[0], sizeof(float) * NUM_WIDTH);

    // Update amplitude/velocity tables - shift data and add new audio
    for (int fo = 0; fo < NUM_WIDTH; fo++) {
        int t = (NUM_WIDTH - (fo + 2)) * NUM_WIDTH;
        float* i = &atable_[t];
        float* o = &atable_[t + NUM_WIDTH];
        float* v = &vtable_[t];
        float* ov = &vtable_[t + NUM_WIDTH];
        int* c = &ctable_[t];
        int* oc = &ctable_[t + NUM_WIDTH];

        if (fo == NUM_WIDTH - 1) {
            // Last row: sample from audio data
            // AudioData is char[2][2][576] - visdata[0][0] is waveform left
            const unsigned char* sd = reinterpret_cast<const unsigned char*>(&visdata[0][0][0]);
            i = btable;
            for (int p = 0; p < NUM_WIDTH; p++) {
                // Sample 3 consecutive values and take max
                int val = std::max({static_cast<int>(sd[0]),
                                   static_cast<int>(sd[1]),
                                   static_cast<int>(sd[2])});
                *o = static_cast<float>(val);

                // Map to color index (0-63)
                int ci = val >> 2;
                if (ci > 63) ci = 63;
                *oc++ = color_tab_[ci];

                *ov++ = (*o++ - *i++) / 90.0f;
                sd += 3;
            }
        } else {
            // Other rows: propagate with decay
            for (int p = 0; p < NUM_WIDTH; p++) {
                *o = *i++ + *v;
                if (*o < 0.0f) *o = 0.0f;
                *ov++ = *v++ - 0.15f * (*o++ / 255.0f);
                *oc++ = *c++;
            }
        }
    }

    // Projection scale factor
    float adj = w * 440.0f / 640.0f;
    float adj2 = h * 440.0f / 480.0f;
    if (adj2 < adj) adj = adj2;

    // Render dots with depth sorting based on rotation
    for (int fo = 0; fo < NUM_WIDTH; fo++) {
        int f = (rotation_ < 90.0f || rotation_ > 270.0f) ? NUM_WIDTH - fo - 1 : fo;
        float dw = 350.0f / static_cast<float>(NUM_WIDTH);
        float xpos = -(NUM_WIDTH * 0.5f) * dw;
        float zpos = (f - NUM_WIDTH * 0.5f) * dw;

        int* ct = &ctable_[f * NUM_WIDTH];
        float* at = &atable_[f * NUM_WIDTH];
        int da = 1;

        // Reverse draw order based on rotation for proper depth
        if (rotation_ < 180.0f) {
            da = -1;
            dw = -dw;
            xpos = -xpos + dw;
            ct += NUM_WIDTH - 1;
            at += NUM_WIDTH - 1;
        }

        for (int p = 0; p < NUM_WIDTH; p++) {
            float ox, oy, oz;
            matrix_apply(matrix, xpos, 64.0f - *at, zpos, &ox, &oy, &oz);

            float scale = adj / oz;
            int ix = static_cast<int>(ox * scale) + (w / 2);
            int iy = static_cast<int>(oy * scale) + (h / 2);

            // Use styled point drawing for Set Render Mode compatibility
            draw_point_styled(framebuffer, ix, iy, w, h, *ct);

            xpos += dw;
            ct += da;
            at += da;
        }
    }

    // Update rotation
    rotation_ += rotvel / 5.0f;
    if (rotation_ >= 360.0f) rotation_ -= 360.0f;
    if (rotation_ < 0.0f) rotation_ += 360.0f;

    return 0;
}

void DotPlaneEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Binary format from r_dotpln.cpp:
    // rotvel (int32)
    // colors[5] (5 x int32)
    // angle (int32)
    // r * 32 (int32)

    if (reader.remaining() >= 4) {
        parameters().set_int("rotation_speed", reader.read_i32());
    }

    if (reader.remaining() >= 20) {
        parameters().set_color("color_top", reader.read_u32());
        parameters().set_color("color_high", reader.read_u32());
        parameters().set_color("color_mid", reader.read_u32());
        parameters().set_color("color_low", reader.read_u32());
        parameters().set_color("color_bottom", reader.read_u32());
    }

    if (reader.remaining() >= 4) {
        parameters().set_int("angle", reader.read_i32());
    }

    if (reader.remaining() >= 4) {
        int rr = reader.read_i32();
        rotation_ = rr / 32.0f;
    }

    init_color_table();
}

void DotPlaneEffect::on_parameter_changed(const std::string& name) {
    // Rebuild color gradient when any color parameter changes
    if (name.find("color_") == 0) {
        init_color_table();
    }
}

// UI controls from res.rc IDD_CFG_DOTPLANE
static const std::vector<ControlLayout> ui_controls = {
    {
        .id = "rotation_group",
        .text = "Rotation",
        .type = ControlType::GROUPBOX,
        .x = 0, .y = 0, .w = 137, .h = 36
    },
    {
        .id = "left_label",
        .text = "Left",
        .type = ControlType::LABEL,
        .x = 12, .y = 9, .w = 13, .h = 8
    },
    {
        .id = "right_label",
        .text = "Right",
        .type = ControlType::LABEL,
        .x = 80, .y = 9, .w = 18, .h = 8
    },
    {
        .id = "rotation_speed",
        .text = "",
        .type = ControlType::SLIDER,
        .x = 6, .y = 17, .w = 95, .h = 15,
        .range = {-50, 50},
        .default_val = 16
    },
    {
        .id = "zero_button",
        .text = "zero",
        .type = ControlType::BUTTON,
        .x = 103, .y = 15, .w = 28, .h = 10
    },
    {
        .id = "angle_group",
        .text = "Angle",
        .type = ControlType::GROUPBOX,
        .x = 0, .y = 36, .w = 137, .h = 29
    },
    {
        .id = "angle",
        .text = "",
        .type = ControlType::SLIDER,
        .x = 6, .y = 45, .w = 126, .h = 15,
        .range = {-90, 90},
        .default_val = -20
    },
    {
        .id = "colors_group",
        .text = "Dot colors",
        .type = ControlType::GROUPBOX,
        .x = 0, .y = 69, .w = 53, .h = 68
    },
    {
        .id = "color_top",
        .text = "Top",
        .type = ControlType::COLOR_BUTTON,
        .x = 7, .y = 79, .w = 41, .h = 10,
        .default_val = static_cast<int>(0x6B8818)  // RGB(24, 136, 107) reversed
    },
    {
        .id = "color_high",
        .text = "High",
        .type = ControlType::COLOR_BUTTON,
        .x = 7, .y = 90, .w = 41, .h = 10,
        .default_val = static_cast<int>(0x9036D9)  // RGB(217, 54, 144) reversed
    },
    {
        .id = "color_mid",
        .text = "Mid",
        .type = ControlType::COLOR_BUTTON,
        .x = 7, .y = 101, .w = 41, .h = 10,
        .default_val = static_cast<int>(0x2A1D74)  // RGB(116, 29, 42) reversed
    },
    {
        .id = "color_low",
        .text = "Low",
        .type = ControlType::COLOR_BUTTON,
        .x = 7, .y = 112, .w = 41, .h = 10,
        .default_val = static_cast<int>(0xFF0A23)  // RGB(35, 10, 255) reversed
    },
    {
        .id = "color_bottom",
        .text = "Bottom",
        .type = ControlType::COLOR_BUTTON,
        .x = 7, .y = 123, .w = 41, .h = 10,
        .default_val = static_cast<int>(0x1C6B18)  // RGB(24, 107, 28) reversed
    }
};

const PluginInfo DotPlaneEffect::effect_info{
    .name = "Dot Plane",
    .category = "Render",
    .description = "3D rotating plane of audio-reactive dots",
    .author = "",
    .version = 1,
    .legacy_index = 29,  // R_DotPlane
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<DotPlaneEffect>();
    },
    .ui_layout = {ui_controls},
    .help_text = ""
};

// Register effect at startup
static bool register_dot_plane = []() {
    PluginManager::instance().register_effect(DotPlaneEffect::effect_info);
    return true;
}();

} // namespace avs
