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
    setup_parameters();
}

void OscilloscopeEffect::setup_parameters() {
    auto& params = parameters();
    
    params.add_parameter(std::make_shared<Parameter>("enabled", ParameterType::BOOL, true));
    params.add_parameter(std::make_shared<Parameter>("color", ParameterType::COLOR, uint32_t(0xFFFFFFFF))); // White with full alpha
    params.add_parameter(std::make_shared<Parameter>("channel_left", ParameterType::BOOL, true));
    params.add_parameter(std::make_shared<Parameter>("channel_right", ParameterType::BOOL, false)); 
    params.add_parameter(std::make_shared<Parameter>("channel_both", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("solid", ParameterType::BOOL, false)); // false=dots, true=lines
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
    
    uint32_t color = parameters().get_color("color");
    bool channel_left = parameters().get_bool("channel_left");
    bool channel_right = parameters().get_bool("channel_right");
    bool channel_both = parameters().get_bool("channel_both");
    bool solid = parameters().get_bool("solid");
    
    // Scale factors
    float x_scale = 576.0f / w;  // Map 576 samples to screen width
    float y_scale = h / 512.0f;  // Map audio range to screen height
    int y_center = h / 2;
    
    // Choose which channel to display
    unsigned char* audio_data;
    if (channel_left) {
        audio_data = (unsigned char*)&visdata[0][0][0]; // Left channel waveform
    } else if (channel_right) {
        audio_data = (unsigned char*)&visdata[0][1][0]; // Right channel waveform
    } else if (channel_both) {
        // Mix both channels - simplified
        static char mixed_data[576];
        for (int i = 0; i < 576; i++) {
            mixed_data[i] = (visdata[0][0][i] + visdata[0][1][i]) / 2;
        }
        audio_data = (unsigned char*)mixed_data;
    } else {
        // Default to left if no channel selected
        audio_data = (unsigned char*)&visdata[0][0][0];
    }
    
    if (solid) {
        // Draw connected line segments
        int prev_x = 0;
        int prev_y = y_center + (int)((audio_data[0] ^ 128) * y_scale) - (int)(128 * y_scale);
        
        for (int x = 1; x < w; x++) {
            // Sample audio data with interpolation
            float sample_pos = x * x_scale;
            int sample_idx = (int)sample_pos;
            float frac = sample_pos - sample_idx;
            
            if (sample_idx >= 575) sample_idx = 575;
            
            // Linear interpolation between samples
            float sample_val = (audio_data[sample_idx] ^ 128) * (1.0f - frac) + 
                              (audio_data[sample_idx + 1] ^ 128) * frac;
            
            int y = y_center + (int)(sample_val * y_scale) - (int)(128 * y_scale);
            
            // Draw line from previous point to current point
            draw_line(framebuffer, w, h, prev_x, prev_y, x, y, color);
            
            prev_x = x;
            prev_y = y;
        }
    } else {
        // Draw dots
        for (int x = 0; x < w; x++) {
            // Sample audio data
            float sample_pos = x * x_scale;
            int sample_idx = (int)sample_pos;
            
            if (sample_idx >= 576) sample_idx = 575;
            
            // Convert audio sample to screen Y coordinate
            int audio_val = audio_data[sample_idx] ^ 128; // Convert from unsigned to signed
            int y = y_center + (int)((audio_val - 128) * y_scale);
            
            // Draw pixel
            if (y >= 0 && y < h) {
                framebuffer[y * w + x] = color;
            }
        }
    }
    
    color_pos_++;
    return 0; // Modified input buffer in place
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
        "Oscilloscope",
        {
            // Effect color button
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 4, .y = 22, .w = 40, .h = 16
            },
            
            // Channel selection - radio buttons
            {
                .id = "channel_left",
                .text = "Left",
                .type = ControlType::RADIO_BUTTON,
                .x = 3, .y = 42, .w = 30, .h = 10
            },
            {
                .id = "channel_right", 
                .text = "Right",
                .type = ControlType::RADIO_BUTTON,
                .x = 35, .y = 42, .w = 30, .h = 10
            },
            {
                .id = "channel_both",
                .text = "Both", 
                .type = ControlType::RADIO_BUTTON,
                .x = 67, .y = 42, .w = 30, .h = 10
            },
            
            // Drawing mode
            {
                .id = "solid",
                .text = "Solid",
                .type = ControlType::CHECKBOX,
                .x = 3, .y = 60, .w = 30, .h = 10
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