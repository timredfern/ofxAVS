// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "brightness_effect.h"
#include "../core/plugin_manager.h"
#include "../core/ui.h"
#include <algorithm>

namespace avs {

BrightnessEffect::BrightnessEffect() {
    setup_parameters();
}

void BrightnessEffect::setup_parameters() {
    auto& params = parameters();
    
    params.add_parameter(std::make_shared<Parameter>("enabled", ParameterType::BOOL, true));
    
    // Brightness adjustment: -255 to +255
    // Negative values darken, positive values brighten
    params.add_parameter(std::make_shared<Parameter>("brightness", ParameterType::INT, 0, -255, 255));
    
    // Separate color channel control
    params.add_parameter(std::make_shared<Parameter>("red_adjust", ParameterType::INT, 0, -255, 255));
    params.add_parameter(std::make_shared<Parameter>("green_adjust", ParameterType::INT, 0, -255, 255));
    params.add_parameter(std::make_shared<Parameter>("blue_adjust", ParameterType::INT, 0, -255, 255));
    
    // Mode: 0 = all channels, 1 = separate channels
    params.add_parameter(std::make_shared<Parameter>("mode", ParameterType::INT, 0, 0, 1));
    
    // UI button parameters
    params.add_parameter(std::make_shared<Parameter>("red_reset", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("green_reset", ParameterType::BOOL, false)); 
    params.add_parameter(std::make_shared<Parameter>("blue_reset", ParameterType::BOOL, false));
    
    // Mode parameters  
    params.add_parameter(std::make_shared<Parameter>("dissoc", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("replace", ParameterType::BOOL, true));
    params.add_parameter(std::make_shared<Parameter>("additive", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("5050", ParameterType::BOOL, false));
}

int BrightnessEffect::render(AudioData visdata, int isBeat,
                            uint32_t* framebuffer, uint32_t* fbout,
                            int w, int h) {
    if (!is_enabled()) return 0;
    
    if (isBeat & 0x80000000) return 0;
    
    int mode = parameters().get_int("mode");
    int pixel_count = w * h;
    
    if (mode == 0) {
        // All channels mode - single brightness value
        int brightness = parameters().get_int("brightness");
        
        if (brightness == 0) return 0; // No change needed
        
        for (int i = 0; i < pixel_count; i++) {
            uint32_t pixel = framebuffer[i];
            
            // Extract channels
            int a = (pixel >> 24) & 0xFF;
            int r = (pixel >> 16) & 0xFF;
            int g = (pixel >> 8) & 0xFF;
            int b = pixel & 0xFF;
            
            // Apply brightness
            r = std::clamp(r + brightness, 0, 255);
            g = std::clamp(g + brightness, 0, 255);
            b = std::clamp(b + brightness, 0, 255);
            
            // Reassemble pixel
            framebuffer[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    } else {
        // Separate channels mode
        int red_adjust = parameters().get_int("red_adjust");
        int green_adjust = parameters().get_int("green_adjust");
        int blue_adjust = parameters().get_int("blue_adjust");
        
        if (red_adjust == 0 && green_adjust == 0 && blue_adjust == 0) {
            return 0; // No change needed
        }
        
        for (int i = 0; i < pixel_count; i++) {
            uint32_t pixel = framebuffer[i];
            
            // Extract channels
            int a = (pixel >> 24) & 0xFF;
            int r = (pixel >> 16) & 0xFF;
            int g = (pixel >> 8) & 0xFF;
            int b = pixel & 0xFF;
            
            // Apply per-channel brightness
            r = std::clamp(r + red_adjust, 0, 255);
            g = std::clamp(g + green_adjust, 0, 255);
            b = std::clamp(b + blue_adjust, 0, 255);
            
            // Reassemble pixel
            framebuffer[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    return 0; // Modified in place
}

// Static member definition
const PluginInfo BrightnessEffect::effect_info {
    .name = "Brightness",
    .description = "",
    .author = "",
    .version = 1,
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<BrightnessEffect>();
    },
    .ui_layout = {
        "Brightness",
        {
            // Enable checkbox
            {
                .id = "enabled",
                .text = "Enable Brightness filter", 
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 87, .h = 10
            },
            
            // Red slider  
            {
                .id = "red_adjust",
                .text = "Red",
                .type = ControlType::SLIDER,
                .x = 25, .y = 13, .w = 97, .h = 13,
                .range = {-255, 255, 0, 25}
            },
            
            // Red reset button (><)
            {
                .id = "red_reset", 
                .text = "><",
                .type = ControlType::BUTTON,
                .x = 125, .y = 12, .w = 12, .h = 14
            },
            
            // Green slider
            {
                .id = "green_adjust",
                .text = "Green", 
                .type = ControlType::SLIDER,
                .x = 25, .y = 28, .w = 97, .h = 13,
                .range = {-255, 255, 0, 25}
            },
            
            // Green reset button
            {
                .id = "green_reset",
                .text = "><", 
                .type = ControlType::BUTTON,
                .x = 125, .y = 28, .w = 12, .h = 14
            },
            
            // Blue slider
            {
                .id = "blue_adjust",
                .text = "Blue",
                .type = ControlType::SLIDER, 
                .x = 25, .y = 44, .w = 97, .h = 13,
                .range = {-255, 255, 0, 25}
            },
            
            // Blue reset button
            {
                .id = "blue_reset",
                .text = "><",
                .type = ControlType::BUTTON,
                .x = 125, .y = 44, .w = 12, .h = 14
            },
            
            // Dissociate RGB checkbox
            {
                .id = "dissoc",
                .text = "Dissociate RGB values",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 60, .w = 89, .h = 10
            },
            
            // Blend mode radio buttons
            {
                .id = "replace",
                .text = "Replace",
                .type = ControlType::RADIO_BUTTON,
                .x = 0, .y = 71, .w = 43, .h = 10
            },
            
            {
                .id = "additive", 
                .text = "Additive blend",
                .type = ControlType::RADIO_BUTTON,
                .x = 0, .y = 81, .w = 61, .h = 10
            },
            
            {
                .id = "5050",
                .text = "50/50 blend", 
                .type = ControlType::RADIO_BUTTON,
                .x = 0, .y = 91, .w = 53, .h = 10
            }
        }
    }
};

// Register effect at startup
static bool register_brightness = []() {
    PluginManager::instance().register_effect(BrightnessEffect::effect_info);
    return true;
}();

} // namespace avs