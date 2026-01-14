// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include <cstdint>
#include <array>

namespace avs {

class StarfieldEffect : public EffectBase {
public:
    StarfieldEffect();
    virtual ~StarfieldEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    void load_parameters(const std::vector<uint8_t>& data) override;

    static const PluginInfo effect_info;

private:
    struct Star {
        int x, y;
        float z;
        float speed;
        int ox, oy;
    };

    void initialize_stars();
    void create_star(int index);

    std::array<Star, 4096> stars_;
    int max_stars_ = 350;
    int width_ = 0;
    int height_ = 0;
    int x_off_ = 0;
    int y_off_ = 0;
    int z_off_ = 255;
    float current_speed_ = 6.0f;
    float inc_beat_ = 0.0f;
    int nc_ = 0;
};

} // namespace avs
