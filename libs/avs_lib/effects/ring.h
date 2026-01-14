// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Port of r_oscring.cpp from original AVS

#pragma once
#include "core/effect_base.h"

namespace avs {

class RingEffect : public EffectBase {
public:
    RingEffect();
    virtual ~RingEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    // Runtime state
    int color_pos_ = 0;
};

} // namespace avs
