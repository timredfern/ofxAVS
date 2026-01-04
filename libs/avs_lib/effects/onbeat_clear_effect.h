// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root

#pragma once

#include "core/effect_base.h"

namespace avs {
struct PluginInfo;
}

namespace avs {

class OnBeatClearEffect : public EffectBase {
public:
    OnBeatClearEffect();
    virtual ~OnBeatClearEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout, int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    int beat_counter_ = 0;
};

} // namespace avs
