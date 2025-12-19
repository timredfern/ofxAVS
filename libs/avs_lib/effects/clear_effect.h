// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "../core/effect_base.h"

namespace avs {
struct PluginInfo;
}

namespace avs {

class ClearEffect : public EffectBase {
public:
    ClearEffect();
    virtual ~ClearEffect() = default;
    
    // Core render function - ported from original r_clear.cpp
    int render(AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override;
    
    std::string get_name() const override { return "Clear"; }
    std::string get_description() const override { return "Render / Clear screen"; }
    const PluginInfo& get_plugin_info() const override { return effect_info; }
    
    static const PluginInfo effect_info;
    

private:
    int frame_counter_ = 0;
    
    void setup_parameters();
};

} // namespace avs