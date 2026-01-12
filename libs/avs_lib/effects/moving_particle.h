// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"

namespace avs {

class MovingParticleEffect : public EffectBase {
public:
    MovingParticleEffect();
    virtual ~MovingParticleEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }
    static const PluginInfo effect_info;

private:
    // Physics state (from original r_parts.cpp)
    double c_[2] = {0.0, 0.0};     // Target position (set on beat)
    double v_[2] = {-0.01551, 0.0}; // Velocity
    double p_[2] = {-0.6, 0.3};    // Current position
    int s_pos_ = 8;                 // Current size (interpolates toward target)
};

} // namespace avs
