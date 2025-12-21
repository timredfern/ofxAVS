// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "../core/effect_base.h"
#include "../core/coordinate_lookup_table.h"
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

    static const PluginInfo effect_info;

private:
    void setup_parameters();
    bool needs_grid_regeneration(int w, int h, AudioData visdata) const;
    void generate_grid(int w, int h, AudioData visdata, int isBeat);
    
    // Script execution phases
    void execute_init_script(AudioData visdata, int w, int h);
    void execute_frame_script(AudioData visdata, int w, int h);  
    void execute_beat_script(AudioData visdata, int w, int h);
    void execute_pixel_script(double& x, double& y, double& r, double& d, 
                             AudioData visdata, int w, int h);
    
    // Grid-based coordinate lookup with interpolation
    CoordinateLookupTable grid_table_;
    
    // State tracking for regeneration
    int last_width_, last_height_;
    int last_grid_width_, last_grid_height_;
    std::string last_init_script_;
    std::string last_frame_script_;
    std::string last_beat_script_;
    std::string last_pixel_script_;
    bool last_rectangular_;
    InterpolationMode last_interp_mode_;
    bool last_wrap_;
    bool last_blend_;
    int last_buffer_source_; 
    
    // Script variable storage (persistent across frames)
    double script_vars_[32]; // User variables
    bool script_initialized_;
};

} // namespace avs