// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include <cstdint>

namespace avs {

class BassSpinEffect : public EffectBase {
public:
    BassSpinEffect();
    virtual ~BassSpinEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    void load_parameters(const std::vector<uint8_t>& data) override;

    static const PluginInfo effect_info;

private:
    void draw_triangle(uint32_t* fb, int points[6], int width, int height, uint32_t color);

    int last_a_ = 0;
    int lx_[2][2] = {{0}};
    int ly_[2][2] = {{0}};
    double r_v_[2] = {3.14159, 0.0};
    double v_[2] = {0.0, 0.0};
    double dir_[2] = {-1.0, 1.0};
};

} // namespace avs
