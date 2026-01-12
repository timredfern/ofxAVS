// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "scatter.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include <cstdlib>

namespace avs {

ScatterEffect::ScatterEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int ScatterEffect::render(AudioData visdata, int isBeat,
                          uint32_t* framebuffer, uint32_t* fbout,
                          int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;

    // Rebuild fudge table if width changed
    if (table_width_ != w) {
        for (int x = 0; x < 512; x++) {
            // Create 8x8 grid offset pattern
            int xp = (x % 8) - 4;
            int yp = (x / 8) % 8 - 4;
            // Adjust for asymmetric offset (original AVS behavior)
            if (xp < 0) xp++;
            if (yp < 0) yp++;
            fudge_table_[x] = w * yp + xp;
        }
        table_width_ = w;
    }

    // Copy top 4 rows directly
    int l = w * 4;
    while (l-- > 0) {
        *fbout++ = *framebuffer++;
    }

    // Scatter middle rows (h-8 rows)
    l = w * (h - 8);
    while (l-- > 0) {
        *fbout++ = framebuffer[fudge_table_[rand() & 511]];
        framebuffer++;
    }

    // Copy bottom 4 rows directly
    l = w * 4;
    while (l-- > 0) {
        *fbout++ = *framebuffer++;
    }

    return 1;  // Request buffer swap
}

void ScatterEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_scat.cpp:
    // enabled (int32)
    if (reader.remaining() >= 4) {
        parameters().set_bool("enabled", reader.read_u32() != 0);
    }
}

// Static member definition
const PluginInfo ScatterEffect::effect_info {
    .name = "Scatter",
    .category = "Trans",
    .description = "Random pixel offset scatter effect",
    .author = "",
    .version = 1,
    .legacy_index = 16,  // R_Scat
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<ScatterEffect>();
    },
    .ui_layout = {
        {
            // Original UI from r_scat.cpp g_DlgProc:
            // IDC_CHECK1: enabled checkbox
            {
                .id = "enabled",
                .text = "Enabled",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 41, .h = 10,
                .default_val = 1
            }
        }
    }
};

// Register effect at startup
static bool register_scatter = []() {
    PluginManager::instance().register_effect(ScatterEffect::effect_info);
    return true;
}();

} // namespace avs
