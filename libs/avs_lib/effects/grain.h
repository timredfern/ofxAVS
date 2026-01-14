// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include <cstdint>
#include <vector>

namespace avs {

class GrainEffect : public EffectBase {
public:
    GrainEffect();
    virtual ~GrainEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    void load_parameters(const std::vector<uint8_t>& data) override;

    static const PluginInfo effect_info;

private:
    unsigned char fast_rand_byte();
    void reinit_depth_buffer(int w, int h);

    // Fast random table (matches original)
    unsigned char randtab_[491];
    int randtab_pos_ = 0;

    // Static grain depth buffer
    std::vector<unsigned char> depth_buffer_;
    int old_w_ = 0;
    int old_h_ = 0;
};

} // namespace avs
