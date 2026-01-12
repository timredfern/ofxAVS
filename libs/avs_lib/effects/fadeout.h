// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include <cstdint>

namespace avs {

class FadeoutEffect : public EffectBase {
public:
    FadeoutEffect();
    virtual ~FadeoutEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    void on_parameter_changed(const std::string& param_name) override;
    void load_parameters(const std::vector<uint8_t>& data) override;

    static const PluginInfo effect_info;

private:
    void make_table();

    // Fade lookup tables for R, G, B channels
    unsigned char fadtab_[3][256];

    // Cached parameters for detecting changes
    int last_fadelen_ = -1;
    uint32_t last_color_ = 0xFFFFFFFF;
};

} // namespace avs
