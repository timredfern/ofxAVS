// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "clear_effect.h"
#include "../core/plugin_manager.h"
#include "../core/ui.h"
#include <memory>

namespace avs {

ClearEffect::ClearEffect() {
    setup_parameters();
}

void ClearEffect::setup_parameters() {
    // Set up parameters matching original clear effect
    auto& params = parameters();
    
    params.add_parameter(std::make_shared<Parameter>("enabled", ParameterType::BOOL, true));
    params.add_parameter(std::make_shared<Parameter>("color", ParameterType::COLOR, uint32_t(0xFF000000))); // Black with full alpha
    params.add_parameter(std::make_shared<Parameter>("only_first", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("blend_replace", ParameterType::BOOL, true));
    params.add_parameter(std::make_shared<Parameter>("blend_additive", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("blend_5050", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("default_render_blend", ParameterType::BOOL, false));
}

// Blend functions for different modes
static uint32_t blend_replace(uint32_t dest, uint32_t src) { 
    return src; 
}

static uint32_t blend_add(uint32_t dest, uint32_t src) {
    int r = ((dest >> 16) & 0xFF) + ((src >> 16) & 0xFF);
    int g = ((dest >> 8) & 0xFF) + ((src >> 8) & 0xFF);
    int b = (dest & 0xFF) + (src & 0xFF);
    int a = ((dest >> 24) & 0xFF);
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static uint32_t blend_max(uint32_t dest, uint32_t src) {
    int r = std::max((dest >> 16) & 0xFF, (src >> 16) & 0xFF);
    int g = std::max((dest >> 8) & 0xFF, (src >> 8) & 0xFF);
    int b = std::max(dest & 0xFF, src & 0xFF);
    int a = ((dest >> 24) & 0xFF);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static uint32_t blend_avg(uint32_t dest, uint32_t src) {
    int r = (((dest >> 16) & 0xFF) + ((src >> 16) & 0xFF)) / 2;
    int g = (((dest >> 8) & 0xFF) + ((src >> 8) & 0xFF)) / 2;
    int b = ((dest & 0xFF) + (src & 0xFF)) / 2;
    int a = ((dest >> 24) & 0xFF);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static uint32_t blend_sub(uint32_t dest, uint32_t src) {
    int r = ((dest >> 16) & 0xFF) - ((src >> 16) & 0xFF);
    int g = ((dest >> 8) & 0xFF) - ((src >> 8) & 0xFF);
    int b = (dest & 0xFF) - (src & 0xFF);
    int a = ((dest >> 24) & 0xFF);
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

int ClearEffect::render(AudioData visdata, int isBeat,
                       uint32_t* framebuffer, uint32_t* fbout,
                       int w, int h) {
    // Port of original render logic with minimal changes
    if (!is_enabled()) return 0;
    
    bool only_first = parameters().get_bool("only_first");
    if (only_first && frame_counter_ > 0) return 0;
    
    if (isBeat & 0x80000000) return 0; // Original beat check
    
    frame_counter_++;
    
    uint32_t color = parameters().get_color("color");
    bool blend_replace = parameters().get_bool("blend_replace");
    bool blend_additive = parameters().get_bool("blend_additive");
    bool blend_5050 = parameters().get_bool("blend_5050");
    
    int pixel_count = w * h;
    uint32_t* p = framebuffer;
    
    // Apply clearing operation based on blend mode
    if (blend_additive) {
        for (int i = 0; i < pixel_count; i++) {
            p[i] = blend_add(p[i], color);
        }
    } else if (blend_5050) {
        for (int i = 0; i < pixel_count; i++) {
            p[i] = blend_avg(p[i], color);
        }
    } else { // Default to replace
        for (int i = 0; i < pixel_count; i++) {
            p[i] = color;
        }
    }
    
    return 0; // Use input buffer (modified in place)
}

// Static member definition
const PluginInfo ClearEffect::effect_info {
    .name = "Clear",
    .description = "",
    .author = "",
    .version = 1,
    .factory = []() -> std::unique_ptr<EffectBase> {
        return std::make_unique<ClearEffect>();
    },
    .ui_layout = {
        "Clear",
        {
            // Enable Clear screen checkbox (corresponds to general 'enabled' parameter)
            {
                .id = "enabled",
                .text = "Enable Clear screen",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 79, .h = 10
            },
            // Color selection button
            {
                .id = "color",
                .text = "Color", 
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 15, .w = 137, .h = 13
            },
            
            // Only on first frame checkbox
            {
                .id = "only_first",
                .text = "First frame only", 
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 30, .w = 63, .h = 10
            },
            // Blend mode selection
            {
                .id = "blend_replace",
                .text = "Replace blend mode",
                .type = ControlType::RADIO_BUTTON,
                .x = 0, .y = 41, .w = 43, .h = 10
            },
            
            {
                .id = "blend_additive",
                .text = "Additive blend mode",
                .type = ControlType::RADIO_BUTTON, 
                .x = 0, .y = 51, .w = 61, .h = 10
            },
            
            {
                .id = "blend_5050", 
                .text = "Blend 50/50 mode",
                .type = ControlType::RADIO_BUTTON,
                .x = 0, .y = 61, .w = 55, .h = 10
            },
            
            {
                .id = "default_render_blend", 
                .text = "Default render blend mode",
                .type = ControlType::RADIO_BUTTON,
                .x = 0, .y = 71, .w = 99, .h = 10
            }
        }
    }
};

// Register effect at startup
static bool register_clear = []() {
    PluginManager::instance().register_effect(ClearEffect::effect_info);
    return true;
}();

} // namespace avs