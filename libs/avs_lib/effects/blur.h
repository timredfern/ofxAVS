// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "../core/effect_base.h"

namespace avs {
struct PluginInfo;

// Blur level presets matching original AVS
enum class BlurLevel { NONE = 0, LIGHT = 1, MEDIUM = 2, HEAVY = 3 };
enum class RoundMode { DOWN = 0, UP = 1 };

class BlurEffect : public EffectBase {
public:
    BlurEffect();
    virtual ~BlurEffect() = default;

    // Core render function - ported from original r_blur.cpp
    int render(AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    void load_parameters(const std::vector<uint8_t>& data) override;

    static const PluginInfo effect_info;
};

} // namespace avs
