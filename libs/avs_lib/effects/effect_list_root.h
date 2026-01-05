// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root

#pragma once

#include "../core/effect_container.h"
#include "../core/plugin_manager.h"
#include <cstring>

namespace avs {

// Root Effect List - the "Main" container at the top of the effect tree
// This is a simplified version with minimal controls (just clear every frame)
// Not registered with PluginManager - only created internally by Renderer
class EffectListRoot : public EffectContainer {
public:
    EffectListRoot() {
        init_parameters_from_layout(effect_info.ui_layout);
    }
    virtual ~EffectListRoot() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override {
        // Clear framebuffer if requested
        bool clear_each_frame = parameters().get_bool("clear_each_frame");
        if (clear_each_frame) {
            std::memset(framebuffer, 0, w * h * sizeof(uint32_t));
        }

        // Render each child effect in sequence
        uint32_t* current_in = framebuffer;
        uint32_t* current_out = fbout;

        for (size_t i = 0; i < children_.size(); i++) {
            EffectBase* child = children_[i].get();
            if (!child) continue;

            int result = child->render(visdata, isBeat, current_in, current_out, w, h);

            if (result == 1) {
                std::swap(current_in, current_out);
            }
        }

        // If we ended with current_in != framebuffer, copy back
        if (current_in != framebuffer) {
            std::memcpy(framebuffer, current_in, w * h * sizeof(uint32_t));
        }

        return 0;
    }

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;
};

// Static definition - minimal UI for root
inline const PluginInfo EffectListRoot::effect_info {
    .name = "Main",
    .category = "Misc",
    .description = "Root effect container",
    .author = "",
    .version = 1,
    .factory = nullptr,  // Not user-creatable
    .ui_layout = {
        {
            {
                .id = "clear_each_frame",
                .text = "Clear every frame",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 100, .h = 10,
                .default_val = 0
            }
        }
    }
};

} // namespace avs
