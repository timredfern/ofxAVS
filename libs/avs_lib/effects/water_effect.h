// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"
#include <vector>

namespace avs {

/**
 * Water Effect - Creates water ripple distortion
 *
 * Averages neighboring pixels and subtracts the previous frame
 * to create a wave propagation effect.
 */
class WaterEffect : public EffectBase {
public:
    WaterEffect();

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    std::vector<uint32_t> lastframe_;
    int last_w_ = 0;
    int last_h_ = 0;
};

} // namespace avs
