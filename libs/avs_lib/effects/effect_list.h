// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root

#pragma once

#include "../core/effect_container.h"

namespace avs {
struct PluginInfo;
}

namespace avs {

// Effect List - the classic AVS container effect for nested groups
// Matches original r_list.cpp behavior with:
// - Input/output blending with adjustable alpha
// - Optional init/frame scripts for dynamic control
// - OnBeat enable for N frames
// - Clear framebuffer option
// - Enabled checkbox (can be disabled unlike root)
class EffectList : public EffectContainer {
public:
    EffectList();
    virtual ~EffectList() = default;

    // Core render function - renders all children in sequence
    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

    // For future scripting support:
    // std::string init_script_;
    // std::string frame_script_;
    // bool use_code_ = false;
    // int beat_render_frames_ = 0;
    // etc.
};

} // namespace avs
