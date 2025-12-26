// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "oscilloscope_effect.h"
#include "../core/plugin_manager.h"
#include "../core/ui.h"
#include <algorithm>
#include <cmath>

namespace avs {

OscilloscopeEffect::OscilloscopeEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

void OscilloscopeEffect::draw_line(uint32_t* buffer, int w, int h, int x1, int y1, int x2, int y2, uint32_t color) {
    // Simple line drawing (Bresenham's line algorithm would be better)
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int steps = std::max(dx, dy);
    
    if (steps == 0) {
        if (x1 >= 0 && x1 < w && y1 >= 0 && y1 < h) {
            buffer[y1 * w + x1] = color;
        }
        return;
    }
    
    float x_inc = (float)(x2 - x1) / steps;
    float y_inc = (float)(y2 - y1) / steps;
    
    float x = x1;
    float y = y1;
    
    for (int i = 0; i <= steps; i++) {
        int px = (int)(x + 0.5f);
        int py = (int)(y + 0.5f);
        
        if (px >= 0 && px < w && py >= 0 && py < h) {
            buffer[py * w + px] = color;
        }
        
        x += x_inc;
        y += y_inc;
    }
}

int OscilloscopeEffect::render(AudioData visdata, int isBeat,
                              uint32_t* framebuffer, uint32_t* fbout,
                              int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;

    // Get parameters
    uint32_t color = parameters().get_color("color");
    auto channel = static_cast<AudioChannel>(parameters().get_int("channel"));
    auto draw_style = static_cast<DrawStyle>(parameters().get_int("draw_style"));
    auto position = static_cast<VerticalPosition>(parameters().get_int("position"));
    // auto mode = static_cast<RenderMode>(parameters().get_int("mode")); // TODO: implement spectrum mode

    // Scale factors matching original AVS (uses 288 samples, scales to screen width)
    const int sample_count = 288;
    float x_scale = (float)sample_count / w;
    float y_scale = h / 2.0f / 256.0f;

    // Calculate vertical position based on position setting
    int y_base;
    if (position == VerticalPosition::TOP) {
        y_base = 0;
    } else if (position == VerticalPosition::BOTTOM) {
        y_base = h / 2;
    } else {  // CENTER
        y_base = h / 4;
    }
    int y_center = y_base + static_cast<int>(y_scale * 128.0f);

    // Choose which channel to display
    char* audio_data;
    static char mixed_data[576];

    if (channel == AudioChannel::RIGHT) {
        audio_data = &visdata[0][1][0];
    } else if (channel == AudioChannel::CENTER) {
        // Mix both channels
        for (int i = 0; i < 576; i++) {
            mixed_data[i] = (visdata[0][0][i] + visdata[0][1][i]) / 2;
        }
        audio_data = mixed_data;
    } else {  // LEFT (default)
        audio_data = &visdata[0][0][0];
    }

    if (draw_style == DrawStyle::SOLID) {
        // Solid scope: draw vertical bars from center to waveform value
        for (int x = 0; x < w; x++) {
            float r = x * x_scale;
            int idx = static_cast<int>(r);
            float frac = r - idx;
            if (idx >= sample_count - 1) idx = sample_count - 2;

            float yr = audio_data[idx] * (1.0f - frac) + audio_data[idx + 1] * frac;
            int y = y_base + static_cast<int>(yr * y_scale);

            draw_line(framebuffer, w, h, x, y_center, x, y, color);
        }
    } else if (draw_style == DrawStyle::LINES) {
        // Line scope: draw connected line segments
        float xs = 1.0f / x_scale;  // w / 288.0
        int lx = 0;
        int ly = y_base + static_cast<int>(audio_data[0] * y_scale);

        for (int i = 1; i < sample_count; i++) {
            int ox = static_cast<int>(i * xs);
            int oy = y_base + static_cast<int>(audio_data[i] * y_scale);

            draw_line(framebuffer, w, h, lx, ly, ox, oy, color);

            lx = ox;
            ly = oy;
        }
    } else {  // DOTS
        // Dot scope: draw individual dots with interpolation
        for (int x = 0; x < w; x++) {
            float r = x * x_scale;
            int idx = static_cast<int>(r);
            float frac = r - idx;
            if (idx >= sample_count - 1) idx = sample_count - 2;

            float yr = audio_data[idx] * (1.0f - frac) + audio_data[idx + 1] * frac;
            int y = y_base + static_cast<int>(yr * y_scale);

            if (y >= 0 && y < h) {
                framebuffer[y * w + x] = color;
            }
        }
    }

    color_pos_++;
    return 0;
}

// Static member definition
const PluginInfo OscilloscopeEffect::effect_info {
    .name = "Oscilloscope",
    .description = "",
    .author = "",
    .version = 1,
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<OscilloscopeEffect>();
    },
    .ui_layout = {
        {
            // Render mode: Spectrum vs Oscilloscope
            {
                .id = "mode",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Spectrum", 6, 9, 46, 10},
                    {"Oscilloscope", 53, 9, 53, 10}
                },
                .default_val = 1  // Oscilloscope
            },
            // Draw style: Lines, Solid, Dots
            {
                .id = "draw_style",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Lines", 6, 24, 33, 10},
                    {"Solid", 40, 24, 31, 10},
                    {"Dots", 72, 24, 31, 10}
                },
                .default_val = 0  // Lines
            },
            // Channel selection
            {
                .id = "channel",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Left channel", 8, 68, 56, 10},
                    {"Right channel", 8, 78, 61, 10},
                    {"Center channel", 8, 88, 65, 10}
                },
                .default_val = 0  // Left
            },
            // Vertical position
            {
                .id = "position",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Top", 86, 68, 29, 10},
                    {"Bottom", 86, 78, 38, 10},
                    {"Center", 86, 88, 37, 10}
                },
                .default_val = 2  // Center
            },
            // Color controls
            {
                .id = "num_colors",
                .text = "Colors",
                .type = ControlType::TEXT_INPUT,
                .x = 53, .y = 107, .w = 19, .h = 12,
                .range = {1, 16},
                .default_val = 1
            },
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 6, .y = 122, .w = 127, .h = 11,
                .default_val = static_cast<int>(0xFFFFFFFF)
            }
        }
    }
};

// Register effect at startup
static bool register_oscilloscope = []() {
    PluginManager::instance().register_effect(OscilloscopeEffect::effect_info);
    return true;
}();

} // namespace avs