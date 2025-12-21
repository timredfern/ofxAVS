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

class OscilloscopeEffect : public EffectBase {
public:
    OscilloscopeEffect();
    virtual ~OscilloscopeEffect() = default;
    
    // Core render function - simplified oscilloscope
    int render(AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override;
    
    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    int color_pos_ = 0;

    void draw_line(uint32_t* buffer, int w, int h, int x1, int y1, int x2, int y2, uint32_t color);
};

} // namespace avs