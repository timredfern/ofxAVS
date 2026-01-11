// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"
#include "core/script/script_engine.h"

namespace avs {

/**
 * Set Render Mode Effect - Control rendering pipeline
 *
 * Sets global line blend mode and line width for subsequent
 * rendering effects (SuperScope, etc.)
 *
 * Extended with frame/beat scripting for dynamic control.
 */
class SetRenderModeEffect : public EffectBase {
public:
    SetRenderModeEffect();

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;
    void on_parameter_changed(const std::string& param_name) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

    // Blend mode constants matching original AVS
    enum BlendMode {
        BLEND_REPLACE = 0,
        BLEND_ADDITIVE = 1,
        BLEND_MAXIMUM = 2,
        BLEND_5050 = 3,
        BLEND_SUB1 = 4,
        BLEND_SUB2 = 5,
        BLEND_MULTIPLY = 6,
        BLEND_ADJUSTABLE = 7,
        BLEND_XOR = 8,
        BLEND_MINIMUM = 9
    };

private:
    ScriptEngine engine_;
    bool inited_ = false;
};

} // namespace avs
