// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "dot_fountain.h"
#include "core/binary_reader.h"
#include "core/plugin_manager.h"
#include "core/blend.h"
#include <cstring>
#include <cmath>

namespace avs {

// 4x4 matrix operations (ported from matrix.cpp)
namespace {

void matrix_rotate(float matrix[], int axis, float deg) {
    int m1, m2;
    float c, s;
    deg *= 3.141592653589f / 180.0f;
    std::memset(matrix, 0, sizeof(float) * 16);
    matrix[((axis - 1) << 2) + axis - 1] = matrix[15] = 1.0f;
    m1 = (axis % 3);
    m2 = ((m1 + 1) % 3);
    c = std::cos(deg);
    s = std::sin(deg);
    matrix[(m1 << 2) + m1] = c;
    matrix[(m1 << 2) + m2] = s;
    matrix[(m2 << 2) + m2] = c;
    matrix[(m2 << 2) + m1] = -s;
}

void matrix_translate(float m[], float x, float y, float z) {
    std::memset(m, 0, sizeof(float) * 16);
    m[0] = m[4 + 1] = m[8 + 2] = m[12 + 3] = 1.0f;
    m[0 + 3] = x;
    m[4 + 3] = y;
    m[8 + 3] = z;
}

void matrix_multiply(float* dest, float src[]) {
    float temp[16];
    std::memcpy(temp, dest, sizeof(float) * 16);
    for (int i = 0; i < 16; i += 4) {
        *dest++ = src[i + 0] * temp[(0 << 2) + 0] + src[i + 1] * temp[(1 << 2) + 0] +
                  src[i + 2] * temp[(2 << 2) + 0] + src[i + 3] * temp[(3 << 2) + 0];
        *dest++ = src[i + 0] * temp[(0 << 2) + 1] + src[i + 1] * temp[(1 << 2) + 1] +
                  src[i + 2] * temp[(2 << 2) + 1] + src[i + 3] * temp[(3 << 2) + 1];
        *dest++ = src[i + 0] * temp[(0 << 2) + 2] + src[i + 1] * temp[(1 << 2) + 2] +
                  src[i + 2] * temp[(2 << 2) + 2] + src[i + 3] * temp[(3 << 2) + 2];
        *dest++ = src[i + 0] * temp[(0 << 2) + 3] + src[i + 1] * temp[(1 << 2) + 3] +
                  src[i + 2] * temp[(2 << 2) + 3] + src[i + 3] * temp[(3 << 2) + 3];
    }
}

void matrix_apply(float* m, float x, float y, float z,
                  float* outx, float* outy, float* outz) {
    *outx = x * m[0] + y * m[1] + z * m[2] + m[3];
    *outy = x * m[4] + y * m[5] + z * m[6] + m[7];
    *outz = x * m[8] + y * m[9] + z * m[10] + m[11];
}

}  // anonymous namespace

DotFountainEffect::DotFountainEffect() {
    init_parameters_from_layout(effect_info.ui_layout);

    // Default colors (BGR format in original, we use ABGR)
    colors_[0] = 0xFF1C6B1C;  // dark green (bottom)
    colors_[1] = 0xFFFF0A23;  // blue-ish
    colors_[2] = 0xFF2A1D74;  // reddish
    colors_[3] = 0xFF9036D9;  // purple
    colors_[4] = 0xFF6B88FF;  // light orange (top)

    init_color_table();
    std::memset(points_, 0, sizeof(points_));

    angle_ = -20;
    rotvel_ = 16;
}

void DotFountainEffect::init_color_table() {
    for (int t = 0; t < 4; t++) {
        int r = (colors_[t] & 0xFF) << 16;
        int g = ((colors_[t] >> 8) & 0xFF) << 16;
        int b = ((colors_[t] >> 16) & 0xFF) << 16;

        int dr = (((colors_[t + 1] & 0xFF) - (colors_[t] & 0xFF)) << 16) / 16;
        int dg = ((((colors_[t + 1] >> 8) & 0xFF) - ((colors_[t] >> 8) & 0xFF)) << 16) / 16;
        int db = ((((colors_[t + 1] >> 16) & 0xFF) - ((colors_[t] >> 16) & 0xFF)) << 16) / 16;

        for (int x = 0; x < 16; x++) {
            color_table_[t * 16 + x] = (r >> 16) | ((g >> 16) << 8) | ((b >> 16) << 16);
            r += dr;
            g += dg;
            b += db;
        }
    }
}

int DotFountainEffect::render(AudioData visdata, int isBeat,
                               uint32_t* framebuffer, uint32_t* fbout,
                               int w, int h) {
    if (isBeat & 0x80000000) return 0;

    // Get parameters
    rotvel_ = parameters().get_int("rotvel");
    angle_ = parameters().get_int("angle");

    // Update colors from parameters
    colors_[0] = parameters().get_color("color_bottom");
    colors_[1] = parameters().get_color("color_low");
    colors_[2] = parameters().get_color("color_mid");
    colors_[3] = parameters().get_color("color_high");
    colors_[4] = parameters().get_color("color_top");
    init_color_table();

    // Build transformation matrix
    float matrix[16], matrix2[16];
    matrix_rotate(matrix, 2, rotation_);
    matrix_rotate(matrix2, 1, static_cast<float>(angle_));
    matrix_multiply(matrix, matrix2);
    matrix_translate(matrix2, 0.0f, -20.0f, 400.0f);
    matrix_multiply(matrix, matrix2);

    // Save bottom row for creating new points
    FountainPoint pb[NUM_ROT_DIV];
    std::memcpy(pb, &points_[0], sizeof(pb));

    // Transform existing points (move upward through the array)
    for (int fo = NUM_ROT_HEIGHT - 2; fo >= 0; fo--) {
        float booga = 1.3f / (fo + 100);
        FountainPoint* in = &points_[fo][0];
        FountainPoint* out = &points_[fo + 1][0];

        for (int p = 0; p < NUM_ROT_DIV; p++) {
            *out = *in;
            out->r += out->dr;
            out->dh += 0.05f;
            out->dr += booga;
            out->h += out->dh;
            out++;
            in++;
        }
    }

    // Create new points at bottom from audio
    FountainPoint* out = &points_[0][0];
    FountainPoint* in = pb;
    unsigned char* sd = reinterpret_cast<unsigned char*>(visdata[1][0]);

    for (int p = 0; p < NUM_ROT_DIV; p++) {
        int t = (*sd ^ 128);
        sd++;
        t = (t * 5) / 4 - 64;
        if (isBeat) t += 128;
        if (t > 255) t = 255;

        out->r = 1.0f;
        float dr = t / 200.0f;
        if (dr < 0) dr = -dr;
        out->h = 250;
        dr += 1.0f;
        out->dh = -dr * (100.0f + (out->dh - in->dh)) / 100.0f * 2.8f;
        t = t / 4;
        if (t > 63) t = 63;
        if (t < 0) t = 0;
        out->c = color_table_[t];

        float a = p * 3.14159f * 2.0f / NUM_ROT_DIV;
        out->ax = std::sin(a);
        out->ay = std::cos(a);
        out->dr = 0.0f;
        out++;
        in++;
    }

    // Calculate perspective adjustment
    float adj = w * 440.0f / 640.0f;
    float adj2 = h * 440.0f / 480.0f;
    if (adj2 < adj) adj = adj2;

    // Render all points
    FountainPoint* pt = &points_[0][0];
    for (int fo = 0; fo < NUM_ROT_HEIGHT; fo++) {
        for (int p = 0; p < NUM_ROT_DIV; p++) {
            float x, y, z;
            matrix_apply(matrix, pt->ax * pt->r, pt->h, pt->ay * pt->r, &x, &y, &z);
            z = adj / z;

            if (z > 0.0000001f) {
                int ix = static_cast<int>(x * z) + w / 2;
                int iy = static_cast<int>(y * z) + h / 2;
                if (iy >= 0 && iy < h && ix >= 0 && ix < w) {
                    BLEND_LINE(&framebuffer[iy * w + ix], pt->c);
                }
            }
            pt++;
        }
    }

    // Update rotation
    rotation_ += rotvel_ / 5.0f;
    if (rotation_ >= 360.0f) rotation_ -= 360.0f;
    if (rotation_ < 0.0f) rotation_ += 360.0f;

    return 0;
}

void DotFountainEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Format: rotvel (4), colors[5] (20), angle (4), r*32 (4)
    if (reader.remaining() >= 4) {
        rotvel_ = static_cast<int>(reader.read_u32());
        parameters().set_int("rotvel", rotvel_);
    }

    for (int i = 0; i < 5 && reader.remaining() >= 4; i++) {
        colors_[i] = BinaryReader::bgr_add_alpha(reader.read_u32());
    }
    parameters().set_color("color_bottom", colors_[4]);  // IDC_C5 = bottom
    parameters().set_color("color_low", colors_[3]);     // IDC_C4
    parameters().set_color("color_mid", colors_[2]);     // IDC_C3
    parameters().set_color("color_high", colors_[1]);    // IDC_C2
    parameters().set_color("color_top", colors_[0]);     // IDC_C1 = top

    if (reader.remaining() >= 4) {
        angle_ = static_cast<int>(reader.read_u32());
        parameters().set_int("angle", angle_);
    }

    if (reader.remaining() >= 4) {
        int rr = static_cast<int>(reader.read_u32());
        rotation_ = rr / 32.0f;
    }

    init_color_table();
}

void DotFountainEffect::on_parameter_changed(const std::string& name) {
    // Rebuild color gradient when any color parameter changes
    if (name.find("color_") == 0) {
        init_color_table();
    }
}

const PluginInfo DotFountainEffect::effect_info{
    .name = "Dot Fountain",
    .category = "Render",
    .description = "3D rotating dot fountain particle effect",
    .author = "",
    .version = 1,
    .legacy_index = 19,
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<DotFountainEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "speed_label",
                .text = "Rotation speed",
                .type = ControlType::LABEL,
                .x = 5, .y = 0, .w = 60, .h = 10
            },
            {
                .id = "rotvel",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 5, .y = 11, .w = 97, .h = 21,
                .range = {-50, 50},
                .default_val = 16
            },
            {
                .id = "zero_button",
                .text = "Zero",
                .type = ControlType::BUTTON,
                .x = 103, .y = 15, .w = 28, .h = 10
            },
            {
                .id = "angle_label",
                .text = "Angle",
                .type = ControlType::LABEL,
                .x = 6, .y = 35, .w = 30, .h = 8
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
                .id = "colors_label",
                .text = "Colors (top to bottom)",
                .type = ControlType::LABEL,
                .x = 7, .y = 65, .w = 80, .h = 8
            },
            {
                .id = "color_top",
                .text = "Top",
                .type = ControlType::COLOR_BUTTON,
                .x = 7, .y = 79, .w = 41, .h = 10,
                .default_val = static_cast<int>(0xFF6B88FF)
            },
            {
                .id = "color_high",
                .text = "High",
                .type = ControlType::COLOR_BUTTON,
                .x = 7, .y = 90, .w = 41, .h = 10,
                .default_val = static_cast<int>(0xFF9036D9)
            },
            {
                .id = "color_mid",
                .text = "Mid",
                .type = ControlType::COLOR_BUTTON,
                .x = 7, .y = 101, .w = 41, .h = 10,
                .default_val = static_cast<int>(0xFF2A1D74)
            },
            {
                .id = "color_low",
                .text = "Low",
                .type = ControlType::COLOR_BUTTON,
                .x = 7, .y = 112, .w = 41, .h = 10,
                .default_val = static_cast<int>(0xFFFF0A23)
            },
            {
                .id = "color_bottom",
                .text = "Bottom",
                .type = ControlType::COLOR_BUTTON,
                .x = 7, .y = 123, .w = 41, .h = 10,
                .default_val = static_cast<int>(0xFF1C6B1C)
            }
        }
    }
};

// Register effect at startup
static bool register_dot_fountain = []() {
    PluginManager::instance().register_effect(DotFountainEffect::effect_info);
    return true;
}();

}  // namespace avs
