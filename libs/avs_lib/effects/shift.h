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

class ShiftEffect : public EffectBase {
public:
    ShiftEffect();
    virtual ~ShiftEffect() = default;

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
    int last_w_ = 0;
    int last_h_ = 0;
};

} // namespace avs
