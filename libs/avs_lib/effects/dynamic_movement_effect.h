// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "../core/effect_base.h"
#include "../core/coordinate_grid.h"
#include "../core/script/script_engine.h"
#include <string>

namespace avs {

// Forward declarations
typedef char AudioData[2][2][576];

/**
 * Dynamic Movement Effect - Trans/Dynamic Movement from r_dmove.cpp
 *
 * This effect exactly matches the original AVS Trans/Dynamic Movement behavior:
 * - Multi-phase script execution (init/frame/beat/pixel)
 * - Grid-based coordinate evaluation with interpolation
 * - Configurable grid resolution (2x2 to 256x256)
 * - Creates the classic "stepped" transformation artifacts
 * - Full scripting support with persistent variables
 */
class DynamicMovementEffect : public EffectBase {
public:
    DynamicMovementEffect();
    ~DynamicMovementEffect() = default;

    // EffectBase interface
    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;
    
    const PluginInfo& get_plugin_info() const override { return effect_info; }

    // Respond to UI parameter changes
    void on_parameter_changed(const std::string& param_name) override;
    void load_parameters(const std::vector<uint8_t>& data) override;

    static const PluginInfo effect_info;

private:
    // Grid-based coordinate lookup with interpolation
    CoordinateGrid grid_;

    // Script engine with persistent variables across frames
    ScriptEngine engine_;
};

} // namespace avs