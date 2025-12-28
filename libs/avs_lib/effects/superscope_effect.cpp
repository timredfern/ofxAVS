// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "superscope_effect.h"
#include "../core/plugin_manager.h"
#include "../core/ui.h"
#include <algorithm>
#include <cmath>

namespace avs {

SuperScopeEffect::SuperScopeEffect() {
    init_parameters_from_layout(effect_info.ui_layout);

    // Set default scripts (Spiral preset from original AVS)
    parameters().set_string("init_script", "n=800");
    parameters().set_string("frame_script", "t=t-0.05");
    parameters().set_string("beat_script", "");
    parameters().set_string("point_script", "d=i+v*0.2; r=t+i*$PI*4; x=cos(r)*d; y=sin(r)*d");

    // Initialize engine with default n
    engine_.set_variable("n", 800.0);
    engine_.set_variable("t", 0.0);
}

void SuperScopeEffect::draw_line(uint32_t* buffer, int w, int h,
                                  int x1, int y1, int x2, int y2, uint32_t color) {
    // Bresenham-style line drawing
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int steps = std::max(dx, dy);

    if (steps == 0) {
        if (x1 >= 0 && x1 < w && y1 >= 0 && y1 < h) {
            buffer[y1 * w + x1] = color;
        }
        return;
    }

    float x_inc = static_cast<float>(x2 - x1) / steps;
    float y_inc = static_cast<float>(y2 - y1) / steps;

    float x = static_cast<float>(x1);
    float y = static_cast<float>(y1);

    for (int i = 0; i <= steps; i++) {
        int px = static_cast<int>(x + 0.5f);
        int py = static_cast<int>(y + 0.5f);

        if (px >= 0 && px < w && py >= 0 && py < h) {
            buffer[py * w + px] = color;
        }

        x += x_inc;
        y += y_inc;
    }
}

int SuperScopeEffect::render(AudioData visdata, int isBeat,
                              uint32_t* framebuffer, uint32_t* fbout,
                              int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;

    // Get parameters
    std::string init_script = parameters().get_string("init_script");
    std::string frame_script = parameters().get_string("frame_script");
    std::string beat_script = parameters().get_string("beat_script");
    std::string point_script = parameters().get_string("point_script");
    int source_mode = parameters().get_int("source_mode");  // 0=wave, 1=spectrum
    int channel = parameters().get_int("channel");  // 0=left, 1=center, 2=right
    int draw_mode = parameters().get_int("draw_mode");  // 0=dots, 1=lines
    int num_colors = parameters().get_int("num_colors");
    uint32_t color = parameters().get_color("color");

    if (num_colors < 1) num_colors = 1;

    // Check if init script changed and needs re-running
    if (init_script != last_init_script_) {
        last_init_script_ = init_script;
        inited_ = false;
    }

    // Color cycling (simplified - single color for now)
    color_pos_++;
    int current_color = color & 0xFFFFFF;

    // Prepare audio data
    char* audio_data;
    static char center_channel[576];
    int ws = (source_mode == 1) ? 1 : 0;  // 0=waveform, 1=spectrum
    int xorv = (ws * 128) ^ 128;  // XOR value for signed/unsigned conversion

    if (channel == 1) {  // Center
        for (int i = 0; i < 576; i++) {
            center_channel[i] = visdata[ws ^ 1][0][i] / 2 + visdata[ws ^ 1][1][i] / 2;
        }
        audio_data = center_channel;
    } else if (channel == 2) {  // Right
        audio_data = &visdata[ws ^ 1][1][0];
    } else {  // Left (default)
        audio_data = &visdata[ws ^ 1][0][0];
    }

    // Set up engine variables
    engine_.set_variable("w", static_cast<double>(w));
    engine_.set_variable("h", static_cast<double>(h));
    engine_.set_variable("b", isBeat ? 1.0 : 0.0);
    engine_.set_variable("red", ((current_color >> 16) & 0xFF) / 255.0);
    engine_.set_variable("green", ((current_color >> 8) & 0xFF) / 255.0);
    engine_.set_variable("blue", (current_color & 0xFF) / 255.0);
    engine_.set_variable("skip", 0.0);
    engine_.set_variable("linesize", 1.0);
    engine_.set_variable("drawmode", draw_mode ? 1.0 : 0.0);

    // Run init script (once)
    if (!inited_ && !init_script.empty()) {
        engine_.evaluate(init_script);
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

    // Run point script for each point
    if (!point_script.empty()) {
        int n = static_cast<int>(engine_.get_variable("n"));
        if (n < 1) n = 1;
        if (n > 128 * 1024) n = 128 * 1024;

        bool can_draw = false;
        int last_x = 0, last_y = 0;

        for (int a = 0; a < n; a++) {
            // Calculate audio value with interpolation
            double r = (a * 576.0) / n;
            double s1 = r - static_cast<int>(r);
            int idx = static_cast<int>(r);
            if (idx >= 575) idx = 574;

            unsigned char sample1 = static_cast<unsigned char>(audio_data[idx]) ^ xorv;
            unsigned char sample2 = static_cast<unsigned char>(audio_data[idx + 1]) ^ xorv;
            double yr = sample1 * (1.0 - s1) + sample2 * s1;

            // Set per-point variables
            engine_.set_variable("v", yr / 128.0 - 1.0);  // -1 to 1
            engine_.set_variable("i", static_cast<double>(a) / static_cast<double>(n - 1));
            engine_.set_variable("skip", 0.0);

            // Execute point script
            engine_.evaluate(point_script);

            // Get output coordinates
            double var_x = engine_.get_variable("x");
            double var_y = engine_.get_variable("y");
            int px = static_cast<int>((var_x + 1.0) * w * 0.5);
            int py = static_cast<int>((var_y + 1.0) * h * 0.5);

            // Check skip
            if (engine_.get_variable("skip") < 0.00001) {
                // Get per-point color
                int point_color = (make_color_component(engine_.get_variable("red")) << 16) |
                                  (make_color_component(engine_.get_variable("green")) << 8) |
                                  make_color_component(engine_.get_variable("blue"));
                point_color |= 0xFF000000;  // Alpha

                double current_drawmode = engine_.get_variable("drawmode");

                if (current_drawmode < 0.00001) {
                    // Dots mode
                    if (py >= 0 && py < h && px >= 0 && px < w) {
                        framebuffer[px + py * w] = point_color;
                    }
                } else {
                    // Lines mode
                    if (can_draw) {
                        draw_line(framebuffer, w, h, last_x, last_y, px, py, point_color);
                    }
                }
            }

            can_draw = true;
            last_x = px;
            last_y = py;
        }
    }

    return 0;
}

// Static member definition
const PluginInfo SuperScopeEffect::effect_info {
    .name = "SuperScope",
    .description = "Advanced oscilloscope with scripting",
    .author = "",
    .version = 1,
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<SuperScopeEffect>();
    },
    .ui_layout = {
        {
            // Point script (per-point)
            {
                .id = "point_script",
                .text = "Point:",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 0, .w = 208, .h = 40
            },
            // Frame script
            {
                .id = "frame_script",
                .text = "Frame:",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 42, .w = 208, .h = 30
            },
            // Beat script
            {
                .id = "beat_script",
                .text = "Beat:",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 74, .w = 208, .h = 30
            },
            // Init script
            {
                .id = "init_script",
                .text = "Init:",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 106, .w = 208, .h = 26
            },
            // Source mode: Waveform vs Spectrum
            {
                .id = "source_mode",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Waveform", 5, 140, 49, 10},
                    {"Spectrum", 55, 140, 46, 10}
                },
                .default_val = 0  // Waveform
            },
            // Channel selection
            {
                .id = "channel",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Left", 5, 152, 28, 10},
                    {"Center", 36, 152, 37, 10},
                    {"Right", 76, 152, 33, 10}
                },
                .default_val = 1  // Center
            },
            // Draw mode: Dots vs Lines
            {
                .id = "draw_mode",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Dots", 166, 152, 31, 10},
                    {"Lines", 200, 152, 33, 10}
                },
                .default_val = 1  // Lines
            },
            // Number of colors
            {
                .id = "num_colors",
                .text = "Colors",
                .type = ControlType::TEXT_INPUT,
                .x = 47, .y = 166, .w = 19, .h = 12,
                .range = {1, 16},
                .default_val = 1
            },
            // Color
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 125, .y = 166, .w = 108, .h = 11,
                .default_val = static_cast<int>(0xFFFFFFFF)
            }
        }
    }
};

// Register effect at startup
static bool register_superscope = []() {
    PluginManager::instance().register_effect(SuperScopeEffect::effect_info);
    return true;
}();

} // namespace avs
