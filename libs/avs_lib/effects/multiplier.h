// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include <cstdint>

namespace avs {

class MultiplierEffect : public EffectBase {
public:
    MultiplierEffect();
    virtual ~MultiplierEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    void load_parameters(const std::vector<uint8_t>& data) override;

    static const PluginInfo effect_info;

    // Mode constants matching original
    static constexpr int MD_XI = 0;     // Infinite root (non-black to white)
    static constexpr int MD_X8 = 1;     // 8x brightness
    static constexpr int MD_X4 = 2;     // 4x brightness
    static constexpr int MD_X2 = 3;     // 2x brightness
    static constexpr int MD_X05 = 4;    // 0.5x brightness
    static constexpr int MD_X025 = 5;   // 0.25x brightness
    static constexpr int MD_X0125 = 6;  // 0.125x brightness
    static constexpr int MD_XS = 7;     // Square (white stays white, rest black)
};

} // namespace avs
