// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root

#include "onbeat_clear_effect.h"
#include "core/plugin_manager.h"
#include "core/blend.h"
#include "core/ui.h"

namespace avs {

OnBeatClearEffect::OnBeatClearEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int OnBeatClearEffect::render(AudioData visdata, int isBeat,
                               uint32_t* framebuffer, uint32_t* fbout, int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int nf = parameters().get_int("every_n_beats");
    uint32_t color = parameters().get_color("color") | 0xFF000000;
    bool blend = parameters().get_bool("blend");

    if (isBeat) {
        if (nf && ++beat_counter_ >= nf) {
            beat_counter_ = 0;
            int pixel_count = w * h;

            if (!blend) {
                for (int i = 0; i < pixel_count; i++) {
                    framebuffer[i] = color;
                }
            } else {
                for (int i = 0; i < pixel_count; i++) {
                    framebuffer[i] = BLEND_AVG(framebuffer[i], color);
                }
            }
        }
    }

    return 0;
}

const PluginInfo OnBeatClearEffect::effect_info {
    .name = "OnBeat Clear",
    .category = "Render",
    .description = "Clears screen on beat detection",
    .author = "",
    .version = 1,
    .legacy_index = 5,  // R_NFClear
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<OnBeatClearEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "beats_group",
                .text = "Clear every N beats",
                .type = ControlType::GROUPBOX,
                .x = 0, .y = 0, .w = 137, .h = 36
            },
            {
                .id = "every_n_beats",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 4, .y = 9, .w = 128, .h = 13,
                .range = {0, 100},
                .default_val = 1
            },
            {
                .id = "min_label",
                .text = "0",
                .type = ControlType::LABEL,
                .x = 7, .y = 24, .w = 8, .h = 8
            },
            {
                .id = "max_label",
                .text = "100",
                .type = ControlType::LABEL,
                .x = 115, .y = 24, .w = 13, .h = 8
            },
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 40, .w = 46, .h = 10,
                .default_val = 0xFFFFFF
            },
            {
                .id = "blend",
                .text = "Blend to color",
                .type = ControlType::CHECKBOX,
                .x = 49, .y = 40, .w = 59, .h = 10,
                .default_val = 0
            }
        }
    }
};

static bool register_onbeat_clear = []() {
    PluginManager::instance().register_effect(OnBeatClearEffect::effect_info);
    return true;
}();

} // namespace avs
