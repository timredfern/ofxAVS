// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "clear_effect.h"
#include "core/plugin_manager.h"
#include "core/blend.h"
#include <memory>

namespace avs {

ClearEffect::ClearEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
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
    auto blend_mode = static_cast<BlendMode>(parameters().get_int("blend_mode"));

    int pixel_count = w * h;
    uint32_t* p = framebuffer;

    // Apply clearing operation based on blend mode
    if (blend_mode == BlendMode::ADDITIVE) {
        for (int i = 0; i < pixel_count; i++) {
            p[i] = BLEND(p[i], color);
        }
    } else if (blend_mode == BlendMode::BLEND_5050) {
        for (int i = 0; i < pixel_count; i++) {
            p[i] = BLEND_AVG(p[i], color);
        }
    } else {  // REPLACE or DEFAULT
        for (int i = 0; i < pixel_count; i++) {
            p[i] = color;
        }
    }
    
    return 0; // Use input buffer (modified in place)
}

// Static member definition
const PluginInfo ClearEffect::effect_info {
    .name = "Clear",
    .category = "Render",
    .description = "",
    .author = "",
    .version = 1,
    .legacy_index = 25,  // R_Clear
    .factory = []() -> std::unique_ptr<EffectBase> {
        return std::make_unique<ClearEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enable Clear screen",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 79, .h = 10,
                .default_val = 1
            },
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 15, .w = 137, .h = 13,
                .default_val = static_cast<int>(0xFF000000)
            },
            {
                .id = "only_first",
                .text = "First frame only",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 30, .w = 63, .h = 10,
                .default_val = 0
            },
            {
                .id = "blend_mode",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Replace blend mode", 0, 41, 43, 10},
                    {"Additive blend mode", 0, 51, 61, 10},
                    {"Blend 50/50 mode", 0, 61, 55, 10},
                    {"Default render blend mode", 0, 71, 99, 10}
                },
                .default_val = 0  // Replace
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