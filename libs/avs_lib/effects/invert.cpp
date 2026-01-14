// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "invert.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"

namespace avs {

InvertEffect::InvertEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int InvertEffect::render(AudioData visdata, int isBeat,
                         uint32_t* framebuffer, uint32_t* fbout,
                         int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int enabled = parameters().get_int("enabled");
    if (!enabled) return 0;

    int count = w * h;
    for (int i = 0; i < count; i++) {
        // XOR with 0xFFFFFF to invert RGB, preserve alpha
        framebuffer[i] ^= 0x00FFFFFF;
    }

    return 0;
}

void InvertEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);
    int enabled = reader.read_u32();
    parameters().set_int("enabled", enabled);
}

// Static member definition - UI layout from res.rc IDD_CFG_INVERT
const PluginInfo InvertEffect::effect_info {
    .name = "Invert",
    .category = "Trans",
    .description = "Inverts all pixel colors",
    .author = "",
    .version = 1,
    .legacy_index = 9,  // R_Invert from rlib.cpp
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<InvertEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enable",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 78, .h = 10,
                .default_val = 1
            }
        }
    }
};

// Register effect at startup
static bool register_invert = []() {
    PluginManager::instance().register_effect(InvertEffect::effect_info);
    return true;
}();

} // namespace avs
