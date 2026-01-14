// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include <cstdint>

namespace avs {

class BlitterFeedbackEffect : public EffectBase {
public:
    BlitterFeedbackEffect();
    virtual ~BlitterFeedbackEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    void load_parameters(const std::vector<uint8_t>& data) override;

    static const PluginInfo effect_info;

private:
    int blitter_normal(uint32_t* framebuffer, uint32_t* fbout, int w, int h, int f_val);
    int blitter_out(uint32_t* framebuffer, uint32_t* fbout, int w, int h, int f_val);

    int fpos_ = 30;  // Current position (animated between scale and scale2)
};

} // namespace avs
