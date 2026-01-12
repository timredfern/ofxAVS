// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Port of r_timescope.cpp from original AVS

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"

namespace avs {

/**
 * Timescope Effect - Scrolling spectrogram visualization
 *
 * Draws spectrum data as vertical lines that scroll horizontally,
 * creating a spectrogram view of audio over time.
 */
class TimescopeEffect : public EffectBase {
public:
    TimescopeEffect();

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    int x_ = 0;  // Current x position for drawing
};

} // namespace avs
