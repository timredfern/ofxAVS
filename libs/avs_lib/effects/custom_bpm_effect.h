// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"
#include <chrono>

namespace avs {

// Use common render return constants from effect_base.h:
// RENDER_SET_BEAT (0x10000000) - Force a beat
// RENDER_CLR_BEAT (0x20000000) - Cancel/clear beat

class CustomBpmEffect : public EffectBase {
public:
    CustomBpmEffect();
    virtual ~CustomBpmEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }
    static const PluginInfo effect_info;

private:
    // Runtime state (from original r_bpm.cpp)
    uint64_t arb_last_tc_ = 0;  // Last tick count for arbitrary beat
    int skip_count_ = 0;         // Beat counter for skipper
    int count_ = 0;              // Total beat count (for skipfirst)

    // Helper to get current time in milliseconds
    uint64_t get_tick_count();
};

} // namespace avs
