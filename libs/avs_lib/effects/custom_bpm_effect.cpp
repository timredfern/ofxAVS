// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "custom_bpm_effect.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include <chrono>

namespace avs {

CustomBpmEffect::CustomBpmEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    arb_last_tc_ = get_tick_count();
}

uint64_t CustomBpmEffect::get_tick_count() {
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());
    return ms.count();
}

int CustomBpmEffect::render(AudioData visdata, int isBeat,
                            uint32_t* framebuffer, uint32_t* fbout,
                            int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;

    int arbitrary = parameters().get_bool("arbitrary") ? 1 : 0;
    int skip = parameters().get_bool("skip") ? 1 : 0;
    int invert = parameters().get_bool("invert") ? 1 : 0;
    int arb_val = parameters().get_int("arb_val");
    int skip_val = parameters().get_int("skip_val");
    int skipfirst = parameters().get_int("skipfirst");

    // Count incoming beats (for skipfirst feature)
    if (isBeat) {
        count_++;
    }

    // Skip first N beats
    if (skipfirst != 0 && count_ <= skipfirst) {
        return isBeat ? RENDER_CLR_BEAT : 0;
    }

    // Mode: Arbitrary (fixed BPM)
    if (arbitrary) {
        uint64_t tc_now = get_tick_count();
        if (tc_now > arb_last_tc_ + arb_val) {
            arb_last_tc_ = tc_now;
            return RENDER_SET_BEAT;
        }
        return RENDER_CLR_BEAT;
    }

    // Mode: Skip beats
    if (skip) {
        if (isBeat && ++skip_count_ >= skip_val + 1) {
            skip_count_ = 0;
            return RENDER_SET_BEAT;
        }
        return RENDER_CLR_BEAT;
    }

    // Mode: Invert beats
    if (invert) {
        if (isBeat) {
            return RENDER_CLR_BEAT;
        } else {
            return RENDER_SET_BEAT;
        }
    }

    return 0;
}

void CustomBpmEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_bpm.cpp:
    // enabled (int32)
    // arbitrary (int32) - fixed BPM mode
    // skip (int32) - skip beats mode
    // invert (int32) - invert mode
    // arbVal (int32) - milliseconds between beats (200-10000)
    // skipVal (int32) - beats to skip (1-16)
    // skipfirst (int32) - skip first N beats (0-64)

    if (reader.remaining() >= 4) {
        parameters().set_bool("enabled", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("arbitrary", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("skip", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("invert", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("arb_val", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("skip_val", reader.read_u32());
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("skipfirst", reader.read_u32());
    }

    arb_last_tc_ = get_tick_count();
}

// Static member definition
const PluginInfo CustomBpmEffect::effect_info {
    .name = "Custom BPM",
    .category = "Misc",
    .description = "Override automatic BPM detection",
    .author = "",
    .version = 1,
    .legacy_index = 33,  // R_Bpm
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<CustomBpmEffect>();
    },
    .ui_layout = {
        {
            // Original UI from r_bpm.cpp g_DlgProc:
            // IDC_CHECK1: enabled
            // IDC_ARBITRARY: arbitrary mode radio
            // IDC_ARBVAL: arbVal slider, range 200-10000
            // IDC_SKIP: skip mode radio
            // IDC_SKIPVAL: skipVal slider, range 1-16
            // IDC_INVERT: invert mode radio
            // IDC_SKIPFIRST: skipfirst slider, range 0-64
            {
                .id = "enabled",
                .text = "Enable BPM Customizer",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 137, .h = 10,
                .default_val = 1
            },
            {
                .id = "arbitrary",
                .text = "Arbitrary",
                .type = ControlType::CHECKBOX,  // Original uses radio
                .x = 0, .y = 14, .w = 41, .h = 10,
                .default_val = 1
            },
            {
                .id = "arb_val",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 38, .y = 14, .w = 65, .h = 11,
                .range = {200, 10000, 100},
                .default_val = 500
            },
            {
                .id = "skip",
                .text = "Skip",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 24, .w = 30, .h = 10,
                .default_val = 0
            },
            {
                .id = "skip_val",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 38, .y = 24, .w = 65, .h = 11,
                .range = {1, 16, 1},
                .default_val = 1
            },
            {
                .id = "invert",
                .text = "Reverse",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 34, .w = 43, .h = 10,
                .default_val = 0
            },
            {
                .id = "skipfirst",
                .text = "Skip first beats",
                .type = ControlType::SLIDER,
                .x = 0, .y = 48, .w = 100, .h = 11,
                .range = {0, 64, 1},
                .default_val = 0
            }
        }
    }
};

// Register effect at startup
static bool register_custom_bpm = []() {
    PluginManager::instance().register_effect(CustomBpmEffect::effect_info);
    return true;
}();

} // namespace avs
