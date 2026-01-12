// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"

namespace avs {

/**
 * Mirror Effect - Mirror/flip transformations
 *
 * Copies pixels from one half of the screen to the other,
 * creating mirror reflections. Can operate in static mode
 * or randomly change on beat.
 */
class MirrorEffect : public EffectBase {
public:
    MirrorEffect();

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    // Smooth transition state
    int framecount_ = 0;
    int last_mode_ = 0;
    uint32_t divisors_ = 0;
    uint32_t inc_ = 0;
    int rbeat_ = 0;  // Random mode for onbeat

    // Blend helper for smooth transitions
    static inline uint32_t blend_adapt(uint32_t a, uint32_t b, int divisor);
};

} // namespace avs
