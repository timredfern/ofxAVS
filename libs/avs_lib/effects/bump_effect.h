// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/script/script_engine.h"
#include <cstdint>

namespace avs {

class BumpEffect : public EffectBase {
public:
    BumpEffect();
    virtual ~BumpEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    void on_parameter_changed(const std::string& param_name) override;

    static const PluginInfo effect_info;

private:
    // Get depth value from pixel (max of RGB channels)
    inline int depthof(int c, bool invert);

    // Set depth on pixel
    static inline int setdepth(int l, int c);
    static inline int setdepth0(int c);

    ScriptEngine script_engine_;
    bool inited_ = false;

    // On-beat transition state
    int this_depth_ = 30;
    int nF_ = 0;  // Frames remaining in on-beat transition
};

} // namespace avs
