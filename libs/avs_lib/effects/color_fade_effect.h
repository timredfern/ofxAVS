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

class ColorFadeEffect : public EffectBase {
public:
    ColorFadeEffect();
    virtual ~ColorFadeEffect() = default;

    int render(AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    // Current fade positions (interpolate toward targets)
    int faderpos_[3] = {8, -8, -8};

    // Lookup table: maps (g-b, b-r) to fade category (0-3)
    unsigned char c_tab_[512][512];

    // Clipping table for clamping results to 0-255
    unsigned char clip_[256 + 40 + 40];

    void init_tables();
};

} // namespace avs
