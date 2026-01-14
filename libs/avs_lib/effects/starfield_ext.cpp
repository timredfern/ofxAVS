// avs_lib - Portable Advanced Visualization Studio library
// Extended Starfield - NOT part of original AVS
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Extensions: Uses styled point drawing from Set Render Mode
// for large/circular star points.

#include "starfield_ext.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/line_draw_ext.h"
#include <cstdlib>
#include <algorithm>
#include <cstring>

namespace avs {

StarfieldExtEffect::StarfieldExtEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    current_speed_ = 6.0f;
}

void StarfieldExtEffect::initialize_stars() {
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

void StarfieldExtEffect::create_star(int index) {
    stars_[index].x = (std::rand() % width_) - x_off_;
    stars_[index].y = (std::rand() % height_) - y_off_;
    stars_[index].z = static_cast<float>(z_off_);
}

int StarfieldExtEffect::render(AudioData visdata, int isBeat,
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

void StarfieldExtEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_stars.cpp
    int enabled = reader.read_u32();
    parameters().set_int("enabled", enabled);

    if (reader.remaining() >= 4) {
        uint32_t color = BinaryReader::bgr_add_alpha(reader.read_u32());
        parameters().set_color("color", color);
    }

    // Skip blend and blendavg - not used in extended version
    if (reader.remaining() >= 4) reader.read_u32();
    if (reader.remaining() >= 4) reader.read_u32();

    if (reader.remaining() >= 4) {
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

// Static member definition
const PluginInfo StarfieldExtEffect::effect_info {
    .name = "Starfield (extended)",
    .category = "Render",
    .description = "3D starfield with styled point drawing from Set Render Mode",
    .author = "",
    .version = 1,
    .legacy_index = -1,  // No legacy index - extension only
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<StarfieldExtEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enable Starfield",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 65, .h = 10,
                .default_val = 1
            },
            {
                .id = "warpSpeed",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 0, .y = 13, .w = 137, .h = 11,
                .range = {1, 500, 1},
                .default_val = 60
            },
            {
                .id = "speed_slow_label",
                .text = "Slow",
                .type = ControlType::LABEL,
                .x = 0, .y = 23, .w = 16, .h = 8
            },
            {
                .id = "speed_fast_label",
                .text = "Fast",
                .type = ControlType::LABEL,
                .x = 123, .y = 23, .w = 14, .h = 8
            },
            {
                .id = "maxStars",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 0, .y = 32, .w = 137, .h = 11,
                .range = {100, 4095, 1},
                .default_val = 350
            },
            {
                .id = "stars_fewer_label",
                .text = "Fewer stars",
                .type = ControlType::LABEL,
                .x = 0, .y = 42, .w = 37, .h = 8
            },
            {
                .id = "stars_more_label",
                .text = "More stars",
                .type = ControlType::LABEL,
                .x = 103, .y = 42, .w = 34, .h = 8
            },
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 52, .w = 37, .h = 15,
                .default_val = static_cast<int>(0xFFFFFFFF)
            },
            {
                .id = "onbeat",
                .text = "OnBeat Speed changes",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 72, .w = 92, .h = 10,
                .default_val = 0
            },
            {
                .id = "spdBeat",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 0, .y = 83, .w = 137, .h = 11,
                .range = {1, 500, 1},
                .default_val = 40
            },
            {
                .id = "beat_slower_label",
                .text = "Slower",
                .type = ControlType::LABEL,
                .x = 0, .y = 93, .w = 22, .h = 8
            },
            {
                .id = "beat_faster_label",
                .text = "Faster",
                .type = ControlType::LABEL,
                .x = 117, .y = 93, .w = 20, .h = 8
            },
            {
                .id = "durFrames",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 0, .y = 102, .w = 137, .h = 11,
                .range = {1, 100, 1},
                .default_val = 15
            },
            {
                .id = "dur_shorter_label",
                .text = "Shorter",
                .type = ControlType::LABEL,
                .x = 0, .y = 112, .w = 24, .h = 8
            },
            {
                .id = "dur_longer_label",
                .text = "Longer",
                .type = ControlType::LABEL,
                .x = 114, .y = 112, .w = 23, .h = 8
            },
            {
                .id = "render_mode_note",
                .text = "Use Set Render Mode (extended) for point style",
                .type = ControlType::LABEL,
                .x = 0, .y = 125, .w = 137, .h = 8
            }
        }
    }
};

// Register effect at startup
static bool register_starfield_ext = []() {
    PluginManager::instance().register_effect(StarfieldExtEffect::effect_info);
    return true;
}();

} // namespace avs
