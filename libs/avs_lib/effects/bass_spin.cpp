// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "bass_spin.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/line_draw.h"
#include <cmath>
#include <algorithm>

namespace avs {

BassSpinEffect::BassSpinEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

void BassSpinEffect::draw_triangle(uint32_t* fb, int points[6], int width, int height, uint32_t color) {
    // Sort points by Y coordinate
    for (int pass = 0; pass < 2; pass++) {
        if (points[1] > points[3]) {
            std::swap(points[0], points[2]);
            std::swap(points[1], points[3]);
        }
        if (points[3] > points[5]) {
            std::swap(points[2], points[4]);
            std::swap(points[3], points[5]);
        }
    }

    // Fixed point math (16.16)
    int x1 = points[0] << 16;
    int x2 = x1;
    int dx1, dx2;

    if (points[1] < points[3]) {
        dx1 = ((points[2] - points[0]) << 16) / (points[3] - points[1]);
    } else {
        dx1 = 0;
    }

    if (points[1] < points[5]) {
        dx2 = ((points[4] - points[0]) << 16) / (points[5] - points[1]);
    } else {
        dx2 = 0;
    }

    int ymax = std::min(points[5], height);

    for (int y = points[1]; y < ymax; y++) {
        if (y == points[3]) {
            if (y == points[5]) return;
            x1 = points[2] << 16;
            dx1 = ((points[4] - points[2]) << 16) / (points[5] - points[3]);
        }

        if (y >= 0) {
            int xa = (std::min(x1, x2) - 32768) >> 16;
            int xb = (std::max(x1, x2) + 32768) >> 16;
            int xl = xb - xa;
            if (xl < 0) xl = -xl;
            if (!xl) xl = 1;

            uint32_t* t = fb + y * width + xa;
            if (xa < 0) { t -= xa; xl += xa; xa = 0; }
            if (xa + xl >= width) xl = width - xa;

            if (xl > 0) {
                while (xl--) {
                    BLEND_LINE(t++, color);
                }
            }
        }

        x1 += dx1;
        x2 += dx2;
    }
}

int BassSpinEffect::render(AudioData visdata, int isBeat,
                            uint32_t* framebuffer, uint32_t* fbout,
                            int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int enabled = parameters().get_int("enabled");
    int mode = parameters().get_int("mode");
    uint32_t left_color = parameters().get_color("left_color");
    uint32_t right_color = parameters().get_color("right_color");

    for (int channel = 0; channel < 2; channel++) {
        if (!(enabled & (1 << channel))) continue;

        uint32_t color = (channel == 0) ? left_color : right_color;

        // Get spectrum data for this channel
        unsigned char* fa_data = reinterpret_cast<unsigned char*>(visdata[0][channel]);

        int ss = std::min(h / 2, (w * 3) / 8);
        double s = static_cast<double>(ss);
        int c_x = (channel == 0) ? w / 2 - ss / 2 : w / 2 + ss / 2;

        // Sum bass frequencies (first 44 bands)
        int d = 0;
        for (int x = 0; x < 44; x++) {
            d += fa_data[x];
        }

        int a = (d * 512) / (last_a_ + 30 * 256);
        last_a_ = d;

        if (a > 255) a = 255;
        v_[channel] = 0.7 * (std::max(a - 104, 12) / 96.0) + 0.3 * v_[channel];
        r_v_[channel] += 3.14159 / 6.0 * v_[channel] * dir_[channel];

        s *= a * 1.0 / 256.0;
        int yp = static_cast<int>(std::sin(r_v_[channel]) * s);
        int xp = static_cast<int>(std::cos(r_v_[channel]) * s);

        if (mode == 0) {
            // Lines mode
            if (lx_[0][channel] || ly_[0][channel]) {
                draw_line(framebuffer, lx_[0][channel], ly_[0][channel],
                          xp + c_x, yp + h / 2, w, h, color);
            }
            lx_[0][channel] = xp + c_x;
            ly_[0][channel] = yp + h / 2;
            draw_line(framebuffer, c_x, h / 2, c_x + xp, h / 2 + yp, w, h, color);

            if (lx_[1][channel] || ly_[1][channel]) {
                draw_line(framebuffer, lx_[1][channel], ly_[1][channel],
                          c_x - xp, h / 2 - yp, w, h, color);
            }
            lx_[1][channel] = c_x - xp;
            ly_[1][channel] = h / 2 - yp;
            draw_line(framebuffer, c_x, h / 2, c_x - xp, h / 2 - yp, w, h, color);
        } else if (mode == 1) {
            // Triangles mode
            if (lx_[0][channel] || ly_[0][channel]) {
                int pts[6] = {c_x, h / 2, lx_[0][channel], ly_[0][channel], xp + c_x, yp + h / 2};
                draw_triangle(framebuffer, pts, w, h, color);
            }
            lx_[0][channel] = xp + c_x;
            ly_[0][channel] = yp + h / 2;

            if (lx_[1][channel] || ly_[1][channel]) {
                int pts[6] = {c_x, h / 2, lx_[1][channel], ly_[1][channel], c_x - xp, h / 2 - yp};
                draw_triangle(framebuffer, pts, w, h, color);
            }
            lx_[1][channel] = c_x - xp;
            ly_[1][channel] = h / 2 - yp;
        }
    }

    return 0;
}

void BassSpinEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_bspin.cpp:
    // enabled, colors[0], colors[1], mode
    int enabled = reader.read_u32();
    parameters().set_int("enabled", enabled);

    if (reader.remaining() >= 4) {
        uint32_t color = BinaryReader::bgr_add_alpha(reader.read_u32());
        parameters().set_color("left_color", color);
    }

    if (reader.remaining() >= 4) {
        uint32_t color = BinaryReader::bgr_add_alpha(reader.read_u32());
        parameters().set_color("right_color", color);
    }

    if (reader.remaining() >= 4) {
        int mode = reader.read_u32();
        parameters().set_int("mode", mode);
    }
}

// Static member definition - UI layout from res.rc IDD_CFG_BSPIN
const PluginInfo BassSpinEffect::effect_info {
    .name = "Bass Spin",
    .category = "Render",
    .description = "Rotating shapes driven by bass frequencies",
    .author = "",
    .version = 1,
    .legacy_index = 16,  // R_BSpin from rlib.cpp
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<BassSpinEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "",
                .type = ControlType::RADIO_GROUP,
                .x = 0, .y = 0, .w = 100, .h = 40,
                .radio_options = {
                    {"Off", 0, 0, 90, 10},
                    {"Left channel", 0, 10, 90, 10},
                    {"Right channel", 0, 20, 90, 10},
                    {"Both channels", 0, 30, 90, 10}
                },
                .default_val = 3
            },
            {
                .id = "left_color",
                .text = "Left Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 45, .w = 78, .h = 20,
                .default_val = static_cast<int>(0xFFFFFFFF)
            },
            {
                .id = "right_color",
                .text = "Right Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 70, .w = 78, .h = 20,
                .default_val = static_cast<int>(0xFFFFFFFF)
            },
            {
                .id = "mode",
                .text = "",
                .type = ControlType::RADIO_GROUP,
                .x = 0, .y = 95, .w = 100, .h = 30,
                .radio_options = {
                    {"Lines", 0, 0, 90, 10},
                    {"Triangles", 0, 14, 90, 10}
                },
                .default_val = 1
            }
        }
    }
};

// Register effect at startup
static bool register_bass_spin = []() {
    PluginManager::instance().register_effect(BassSpinEffect::effect_info);
    return true;
}();

} // namespace avs
