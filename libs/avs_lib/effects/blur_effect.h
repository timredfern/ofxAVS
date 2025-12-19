// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "../core/effect_base.h"

namespace avs {
struct PluginInfo;
}

namespace avs {

class BlurEffect : public EffectBase {
public:
    BlurEffect();
    virtual ~BlurEffect() = default;
    
    // Core render function - ported from original r_blur.cpp
    int render(AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override;
    
    std::string get_name() const override { return "Blur"; }
    std::string get_description() const override { return "Blur effect"; }
    const PluginInfo& get_plugin_info() const override { return effect_info; }
    
    static const PluginInfo effect_info;

private:
    void setup_parameters();
    void apply_box_blur(uint32_t* input, uint32_t* output, int w, int h, int radius);
};

} // namespace avs