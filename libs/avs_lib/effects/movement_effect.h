// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "../core/effect_base.h"
#include <vector>
#include <string>

namespace avs {

// Forward declarations
typedef char AudioData[2][2][576];

/**
 * Movement Effect - Trans/Movement from r_trans.cpp
 * 
 * This effect exactly matches the original AVS Trans/Movement behavior:
 * - 23 built-in preset transformations
 * - Custom scripting with single expression evaluation
 * - Full-resolution lookup table (one entry per pixel)
 * - Supports both rectangular and polar coordinate systems
 * - Source mapping, wrap, and blend options
 */
class MovementEffect : public EffectBase {
public:
    MovementEffect();
    ~MovementEffect() = default;

    // EffectBase interface
    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;
    
    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

    // Public for testing
    std::string get_preset_script(int preset_index) const;

private:
    void setup_parameters();
    bool needs_table_regeneration(int w, int h, AudioData visdata) const;
    void generate_lookup_table(int w, int h, AudioData visdata);
    void apply_transformation(uint32_t* input, uint32_t* output, int w, int h);
    
    // Script execution for both presets and custom expressions
    void evaluate_movement_script(const std::string& script, double& x, double& y, double& r, double& d, 
                                 AudioData visdata, int w, int h);
    
    // Full-resolution lookup table
    std::vector<int> lookup_table_;
    bool table_valid_;
    
    // State tracking for regeneration
    int last_width_, last_height_;
    int last_preset_index_;
    std::string last_custom_expr_;
    bool last_rectangular_;
    bool last_source_mapped_;
    bool last_wrap_;
    bool last_blend_;
    bool last_subpixel_;
};

} // namespace avs