// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root

#include "effect_list.h"
#include "../core/plugin_manager.h"
#include "../core/ui.h"
#include <memory>
#include <cstring>

namespace avs {

EffectList::EffectList() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int EffectList::render(AudioData visdata, int isBeat,
                       uint32_t* framebuffer, uint32_t* fbout,
                       int w, int h) {
    // Check if enabled (sub-lists only - root is always enabled)
    if (!is_root_ && !is_enabled()) {
        return 0;
    }

    // Clear framebuffer if requested
    bool clear_each_frame = parameters().get_bool("clear_each_frame");
    if (clear_each_frame) {
        std::memset(framebuffer, 0, w * h * sizeof(uint32_t));
    }

    // Render each child effect in sequence
    // Effects can swap between framebuffer and fbout
    uint32_t* current_in = framebuffer;
    uint32_t* current_out = fbout;

    for (size_t i = 0; i < children_.size(); i++) {
        EffectBase* child = children_[i].get();
        if (!child) continue;

        // Call child's render
        int result = child->render(visdata, isBeat, current_in, current_out, w, h);

        // If result is 1, child wrote to fbout - swap buffers for next effect
        if (result == 1) {
            std::swap(current_in, current_out);
        }
    }

    // If we ended with current_in != framebuffer, we need to copy back
    if (current_in != framebuffer) {
        std::memcpy(framebuffer, current_in, w * h * sizeof(uint32_t));
    }

    return 0;  // Result is always in framebuffer
}

// Static member definition - UI layout for sub-lists
// Root lists use a simpler UI (just "clear each frame")
const PluginInfo EffectList::effect_info {
    .name = "Effect List",
    .description = "Container for grouping effects with optional scripted control",
    .author = "",
    .version = 1,
    .factory = []() -> std::unique_ptr<EffectBase> {
        return std::make_unique<EffectList>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enabled",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 2, .w = 56, .h = 10,
                .default_val = 1
            },
            {
                .id = "clear_each_frame",
                .text = "Clear every frame",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 13, .w = 71, .h = 10,
                .default_val = 0
            }
            // TODO: Add remaining controls in Phase 4:
            // - Input/output blending dropdowns and sliders
            // - OnBeat enable checkbox and frames input
            // - Use evaluation override checkbox
            // - Init script textarea
            // - Frame script textarea
        }
    }
};

// Register effect at startup
static bool register_effect_list = []() {
    PluginManager::instance().register_effect(EffectList::effect_info);
    return true;
}();

} // namespace avs
