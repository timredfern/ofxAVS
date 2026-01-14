// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include <cstdint>

namespace avs {

class ChannelShiftEffect : public EffectBase {
public:
    ChannelShiftEffect();
    virtual ~ChannelShiftEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    void load_parameters(const std::vector<uint8_t>& data) override;

    static const PluginInfo effect_info;

    // Mode constants (values match original IDC_* resource IDs)
    static constexpr int MODE_RGB = 0;  // No change
    static constexpr int MODE_RBG = 1;  // Swap G and B
    static constexpr int MODE_GRB = 2;  // Swap R and G
    static constexpr int MODE_GBR = 3;  // Rotate left
    static constexpr int MODE_BRG = 4;  // Rotate right
    static constexpr int MODE_BGR = 5;  // Swap R and B
};

} // namespace avs
