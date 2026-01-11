// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"

namespace avs {

class RotoBlitterEffect : public EffectBase {
public:
    RotoBlitterEffect();
    virtual ~RotoBlitterEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }
    static const PluginInfo effect_info;

private:
    // Runtime state
    int64_t current_zoom_ = 0;
    int direction_ = 1;
    double current_rotation_ = 1.0;

    // Cached width multiplier table for fast y*w calculation
    int last_w_ = 0;
    int last_h_ = 0;
    std::vector<int> w_mul_;
};

} // namespace avs
