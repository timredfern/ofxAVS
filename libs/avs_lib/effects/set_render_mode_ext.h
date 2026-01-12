// avs_lib - Portable Advanced Visualization Studio library
// Extended Set Render Mode - NOT part of original AVS
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Extensions to original Set Render Mode:
//   - Line style flags (AA, angle correction, rounded endpoints)
//   - Dynamic scripting (init/frame/beat)
//   - Point size control

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"
#include "core/script/script_engine.h"

namespace avs {

/**
 * Extended Set Render Mode Effect
 *
 * Extends the original Set Render Mode with:
 * - Anti-aliased line drawing
 * - Angle-corrected thickness
 * - Rounded endpoints
 * - Point size control
 * - Dynamic scripting for line width, blend mode, alpha
 */
class SetRenderModeExtEffect : public EffectBase {
public:
    SetRenderModeExtEffect();

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;
    void on_parameter_changed(const std::string& param_name) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    ScriptEngine engine_;
    bool inited_ = false;
};

} // namespace avs
