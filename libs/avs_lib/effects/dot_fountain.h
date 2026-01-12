// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"
#include <cmath>

namespace avs {

// Particle structure for fountain
struct FountainPoint {
    float r, dr;    // radius and delta radius
    float h, dh;    // height and delta height
    float ax, ay;   // axis coordinates
    int c;          // color
};

constexpr int NUM_ROT_DIV = 30;
constexpr int NUM_ROT_HEIGHT = 256;

class DotFountainEffect : public EffectBase {
public:
    DotFountainEffect();

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;
    void on_parameter_changed(const std::string& name) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    void init_color_table();

    FountainPoint points_[NUM_ROT_HEIGHT][NUM_ROT_DIV];
    int color_table_[64];
    float rotation_ = 0.0f;
    int rotvel_ = 16;      // rotation velocity (-50 to 50)
    int angle_ = -20;      // viewing angle (-90 to 90)
    uint32_t colors_[5];   // gradient colors (bottom to top)
};

} // namespace avs
