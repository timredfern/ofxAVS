// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Port of r_dotpln.cpp from original AVS

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"

namespace avs {

/**
 * Dot Plane Effect - 3D rotating plane of dots
 *
 * Displays a 64x64 grid of dots that react to audio.
 * The plane rotates around the Y axis and can be tilted.
 * Dot colors form a gradient based on amplitude.
 */
class DotPlaneEffect : public EffectBase {
public:
    DotPlaneEffect();

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;
    void on_parameter_changed(const std::string& name) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    static constexpr int NUM_WIDTH = 64;

    void init_color_table();

    float rotation_;                          // Current rotation angle
    float atable_[NUM_WIDTH * NUM_WIDTH];     // Amplitude table
    float vtable_[NUM_WIDTH * NUM_WIDTH];     // Velocity table
    int ctable_[NUM_WIDTH * NUM_WIDTH];       // Color table (per-dot)
    int color_tab_[64];                       // Gradient color lookup
};

} // namespace avs
