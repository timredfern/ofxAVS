// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "starfield.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/line_draw_ext.h"
#include <cstdlib>
#include <algorithm>
#include <cstring>

namespace avs {

StarfieldEffect::StarfieldEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    current_speed_ = 6.0f;
}

void StarfieldEffect::initialize_stars() {
    int max_stars_set = parameters().get_int("maxStars");

    // Scale star count by screen size
    max_stars_ = (max_stars_set * width_ * height_) / (512 * 384);
    if (max_stars_ > 4095) max_stars_ = 4095;

    for (int i = 0; i < max_stars_; i++) {
        stars_[i].x = (std::rand() % width_) - x_off_;
        stars_[i].y = (std::rand() % height_) - y_off_;
        stars_[i].z = static_cast<float>(std::rand() % 255);
        stars_[i].speed = static_cast<float>(std::rand() % 9 + 1) / 10.0f;
    }
}

void StarfieldEffect::create_star(int index) {
    stars_[index].x = (std::rand() % width_) - x_off_;
    stars_[index].y = (std::rand() % height_) - y_off_;
    stars_[index].z = static_cast<float>(z_off_);
}

int StarfieldEffect::render(AudioData visdata, int isBeat,
                             uint32_t* framebuffer, uint32_t* fbout,
                             int w, int h) {
    int enabled = parameters().get_int("enabled");
    if (!enabled) return 0;

    float warp_speed = parameters().get_int("warpSpeed") / 10.0f;
    float spd_beat = parameters().get_int("spdBeat") / 10.0f;
    int dur_frames = parameters().get_int("durFrames");
    int onbeat = parameters().get_int("onbeat");
    uint32_t color = parameters().get_color("color");

    // Handle beat speed change
    if (onbeat && isBeat) {
        current_speed_ = spd_beat;
        inc_beat_ = (warp_speed - current_speed_) / static_cast<float>(dur_frames);
        nc_ = dur_frames;
    }

    // Reinitialize if screen size changed
    if (width_ != w || height_ != h) {
        width_ = w;
        height_ = h;
        x_off_ = width_ / 2;
        y_off_ = height_ / 2;
        initialize_stars();
    }

    if (isBeat & 0x80000000) return 0;

    // Render stars
    for (int i = 0; i < max_stars_; i++) {
        if (static_cast<int>(stars_[i].z) > 0) {
            // Project 3D to 2D
            int nx = ((stars_[i].x << 7) / static_cast<int>(stars_[i].z)) + x_off_;
            int ny = ((stars_[i].y << 7) / static_cast<int>(stars_[i].z)) + y_off_;

            if (nx > 0 && nx < w && ny > 0 && ny < h) {
                // Calculate star intensity based on depth
                int c = static_cast<int>((255 - static_cast<int>(stars_[i].z)) * stars_[i].speed);

                uint32_t star_color;
                if (color != 0x00FFFFFF) {
                    // Blend with target color based on intensity
                    int divisor = c >> 4;
                    if (divisor > 15) divisor = 15;
                    uint32_t gray = c | (c << 8) | (c << 16);
                    // Simple blend toward target color
                    int cr = ((gray & 0xff) * (16 - divisor) + (color & 0xff) * divisor) >> 4;
                    int cg = (((gray >> 8) & 0xff) * (16 - divisor) + ((color >> 8) & 0xff) * divisor) >> 4;
                    int cb = (((gray >> 16) & 0xff) * (16 - divisor) + ((color >> 16) & 0xff) * divisor) >> 4;
                    star_color = cr | (cg << 8) | (cb << 16);
                } else {
                    star_color = c | (c << 8) | (c << 16);
                }

                draw_point_styled(framebuffer, nx, ny, w, h, star_color);

                stars_[i].ox = nx;
                stars_[i].oy = ny;
                stars_[i].z -= stars_[i].speed * current_speed_;
            } else {
                create_star(i);
            }
        } else {
            create_star(i);
        }
    }

    // Update speed
    if (!nc_) {
        current_speed_ = warp_speed;
    } else {
        current_speed_ = std::max(0.0f, current_speed_ + inc_beat_);
        nc_--;
    }

    return 0;
}

void StarfieldEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_stars.cpp
    int enabled = reader.read_u32();
    parameters().set_int("enabled", enabled);

    if (reader.remaining() >= 4) {
        uint32_t color = BinaryReader::bgr_add_alpha(reader.read_u32());
        parameters().set_color("color", color);
    }

    if (reader.remaining() >= 4) {
        int blend = reader.read_u32();
        parameters().set_int("blend", blend);
    }

    if (reader.remaining() >= 4) {
        int blendavg = reader.read_u32();
        parameters().set_int("blendavg", blendavg);
    }

    if (reader.remaining() >= 4) {
        // Original stores speed as integer (scaled by 10)
        int speed = reader.read_u32();
        parameters().set_int("warpSpeed", speed);
        current_speed_ = speed / 10.0f;
    }

    if (reader.remaining() >= 4) {
        int max_stars = reader.read_u32();
        parameters().set_int("maxStars", max_stars);
    }

    if (reader.remaining() >= 4) {
        int onbeat = reader.read_u32();
        parameters().set_int("onbeat", onbeat);
    }

    if (reader.remaining() >= 4) {
        int spd_beat = reader.read_u32();
        parameters().set_int("spdBeat", spd_beat);
    }

    if (reader.remaining() >= 4) {
        int dur_frames = reader.read_u32();
        parameters().set_int("durFrames", dur_frames);
    }
}

// Static member definition - UI layout from res.rc IDD_CFG_STARFIELD
const PluginInfo StarfieldEffect::effect_info {
    .name = "Starfield",
    .category = "Render",
    .description = "3D starfield flying through space",
    .author = "",
    .version = 1,
    .legacy_index = 14,  // R_StarField from rlib.cpp
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<StarfieldEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enable",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 78, .h = 10,
                .default_val = 1
            },
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 15, .w = 78, .h = 20,
                .default_val = static_cast<int>(0xFFFFFFFF)
            },
            {
                .id = "warpSpeed",
                .text = "Speed",
                .type = ControlType::SLIDER,
                .x = 0, .y = 40, .w = 150, .h = 20,
                .range = {1, 500, 1},
                .default_val = 60
            },
            {
                .id = "maxStars",
                .text = "Stars",
                .type = ControlType::SLIDER,
                .x = 0, .y = 65, .w = 150, .h = 20,
                .range = {100, 4095, 1},
                .default_val = 350
            },
            {
                .id = "onbeat",
                .text = "On beat",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 90, .w = 78, .h = 10,
                .default_val = 0
            },
            {
                .id = "spdBeat",
                .text = "Beat Speed",
                .type = ControlType::SLIDER,
                .x = 0, .y = 105, .w = 150, .h = 20,
                .range = {1, 500, 1},
                .default_val = 40
            },
            {
                .id = "durFrames",
                .text = "Duration",
                .type = ControlType::SLIDER,
                .x = 0, .y = 130, .w = 150, .h = 20,
                .range = {1, 100, 1},
                .default_val = 15
            },
            {
                .id = "blend",
                .text = "Additive",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 155, .w = 78, .h = 10,
                .default_val = 0
            },
            {
                .id = "blendavg",
                .text = "50/50",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 170, .w = 78, .h = 10,
                .default_val = 0
            }
        }
    }
};

// Register effect at startup
static bool register_starfield = []() {
    PluginManager::instance().register_effect(StarfieldEffect::effect_info);
    return true;
}();

} // namespace avs
