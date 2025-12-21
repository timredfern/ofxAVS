// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "blur_effect.h"
#include "../core/plugin_manager.h"
#include "../core/ui.h"
#include <algorithm>
#include <cstring>

namespace avs {

BlurEffect::BlurEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

void BlurEffect::apply_box_blur(uint32_t* input, uint32_t* output, int w, int h, int radius) {
    // Simple box blur implementation
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int r_sum = 0, g_sum = 0, b_sum = 0, count = 0;
            
            // Sample in radius around current pixel
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int sx = x + dx;
                    int sy = y + dy;
                    
                    // Clamp to image bounds
                    sx = std::max(0, std::min(w - 1, sx));
                    sy = std::max(0, std::min(h - 1, sy));
                    
                    uint32_t pixel = input[sy * w + sx];
                    
                    r_sum += (pixel >> 16) & 0xFF;
                    g_sum += (pixel >> 8) & 0xFF;
                    b_sum += pixel & 0xFF;
                    count++;
                }
            }
            
            // Average the colors
            if (count > 0) {
                int r = r_sum / count;
                int g = g_sum / count;
                int b = b_sum / count;
                
                output[y * w + x] = (r << 16) | (g << 8) | b;
            } else {
                output[y * w + x] = input[y * w + x];
            }
        }
    }
}

int BlurEffect::render(AudioData visdata, int isBeat,
                      uint32_t* framebuffer, uint32_t* fbout,
                      int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;
    
    double strength = parameters().get_int("strength") / 100.0;  // Convert 0-100 to 0.0-1.0
    int radius = parameters().get_int("radius");

    if (strength <= 0.0 || radius <= 0) {
        // No blur - just copy input to output
        std::memcpy(fbout, framebuffer, w * h * sizeof(uint32_t));
        return 1;
    }
    
    // Apply blur
    apply_box_blur(framebuffer, fbout, w, h, radius);
    
    // Blend with original based on strength
    if (strength < 1.0) {
        for (int i = 0; i < w * h; i++) {
            uint32_t orig = framebuffer[i];
            uint32_t blur = fbout[i];
            
            // Linear interpolation between original and blurred
            int orig_r = (orig >> 16) & 0xFF;
            int orig_g = (orig >> 8) & 0xFF;
            int orig_b = orig & 0xFF;
            
            int blur_r = (blur >> 16) & 0xFF;
            int blur_g = (blur >> 8) & 0xFF;
            int blur_b = blur & 0xFF;
            
            int final_r = (int)(orig_r * (1.0 - strength) + blur_r * strength);
            int final_g = (int)(orig_g * (1.0 - strength) + blur_g * strength);
            int final_b = (int)(orig_b * (1.0 - strength) + blur_b * strength);
            
            fbout[i] = (final_r << 16) | (final_g << 8) | final_b;
        }
    }
    
    return 1; // Use output buffer
}

// Static member definition
const PluginInfo BlurEffect::effect_info {
    .name = "Blur",
    .description = "Blur effect",
    .author = "AVS Port",
    .version = 1,
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<BlurEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enable Blur",
                .type = ControlType::CHECKBOX,
                .x = 10, .y = 5, .w = 80, .h = 10,
                .default_val = 1
            },
            {
                .id = "strength",
                .text = "Strength",
                .type = ControlType::SLIDER,
                .x = 10, .y = 20, .w = 100, .h = 15,
                .range = {0, 100},
                .default_val = 50
            },
            {
                .id = "radius",
                .text = "Radius",
                .type = ControlType::SLIDER,
                .x = 10, .y = 40, .w = 100, .h = 15,
                .range = {1, 10},
                .default_val = 2
            }
        }
    }
};

// Register effect at startup
static bool register_blur = []() {
    PluginManager::instance().register_effect(BlurEffect::effect_info);
    return true;
}();

} // namespace avs