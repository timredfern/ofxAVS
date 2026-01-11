// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"

namespace avs {

class InterferencesEffect : public EffectBase {
public:
    InterferencesEffect();
    virtual ~InterferencesEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }
    static const PluginInfo effect_info;

private:
    // Runtime state (from original r_interf.cpp)
    float a_ = 0.0f;           // Current rotation angle (radians)
    float status_ = 3.14159f;  // On-beat transition status (0 to PI)

    // Interpolated values (computed each frame)
    int _distance_ = 0;
    int _alpha_ = 0;
    int _rotationinc_ = 0;

    // Pre-computed blend table (256 entries per alpha level)
    void update_blend_table(int alpha);
    uint8_t blend_table_[256];
    int last_alpha_ = -1;
};

} // namespace avs
