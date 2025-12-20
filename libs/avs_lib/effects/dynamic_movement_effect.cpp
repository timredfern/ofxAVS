// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "dynamic_movement_effect.h"
#include "../core/parameter.h"
#include <cmath>
#include <cstring>

namespace avs {

DynamicMovementEffect::DynamicMovementEffect()
    : last_width_(0), last_height_(0), last_grid_width_(0), last_grid_height_(0),
      last_rectangular_(false), last_interp_mode_(InterpolationMode::LINEAR),
      last_wrap_(false), last_blend_(false), last_buffer_source_(0), script_initialized_(false)
{
    // Initialize script variables  
    memset(script_vars_, 0, sizeof(script_vars_));
    setup_parameters();
}

void DynamicMovementEffect::setup_parameters() {
    auto& params = parameters();
    
    params.add_parameter(std::make_shared<Parameter>("enabled", ParameterType::BOOL, true));
    
    // Multi-phase scripts (matching original r_dmove.cpp)
    params.add_parameter(std::make_shared<Parameter>("init_script", ParameterType::STRING,
        std::string("// Init phase - run once")));
    params.add_parameter(std::make_shared<Parameter>("frame_script", ParameterType::STRING,
        std::string("// Frame phase - run per frame")));
    params.add_parameter(std::make_shared<Parameter>("beat_script", ParameterType::STRING,
        std::string("// Beat phase - run on beat")));
    params.add_parameter(std::make_shared<Parameter>("pixel_script", ParameterType::STRING,
        std::string("x=x; y=y-0.01")));
    
    // Grid configuration
    params.add_parameter(std::make_shared<Parameter>("grid_width", ParameterType::INT, 16, 2, 256));
    params.add_parameter(std::make_shared<Parameter>("grid_height", ParameterType::INT, 16, 2, 256));
    
    // Coordinate system
    params.add_parameter(std::make_shared<Parameter>("rectangular", ParameterType::BOOL, true)); // Default: rectangular for x,y
    
    // Interpolation mode
    std::vector<std::string> interp_options = {"None (Stepped)", "Linear", "Nearest"};
    params.add_parameter(std::make_shared<Parameter>("interpolation", ParameterType::ENUM, 1, interp_options));
    
    // Rendering options
    params.add_parameter(std::make_shared<Parameter>("wrap", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("blend", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("no_movement", ParameterType::BOOL, false));
}

bool DynamicMovementEffect::needs_grid_regeneration(int w, int h, AudioData visdata) const {
    int grid_width = parameters().get_int("grid_width", 16);
    int grid_height = parameters().get_int("grid_height", 16);
    std::string init_script = parameters().get_string("init_script");
    std::string frame_script = parameters().get_string("frame_script");
    std::string beat_script = parameters().get_string("beat_script");
    std::string pixel_script = parameters().get_string("pixel_script");
    bool rectangular = parameters().get_bool("rectangular", false);
    int interp_int = parameters().get_int("interpolation", 0);
    InterpolationMode interp_mode = static_cast<InterpolationMode>(interp_int);
    bool wrap = parameters().get_bool("wrap", false);
    bool blend = parameters().get_bool("blend", false);
    
    return w != last_width_ || 
           h != last_height_ ||
           grid_width != last_grid_width_ ||
           grid_height != last_grid_height_ ||
           init_script != last_init_script_ ||
           frame_script != last_frame_script_ ||
           beat_script != last_beat_script_ ||
           pixel_script != last_pixel_script_ ||
           rectangular != last_rectangular_ ||
           interp_mode != last_interp_mode_ ||
           wrap != last_wrap_ ||
           blend != last_blend_;
}

void DynamicMovementEffect::generate_grid(int w, int h, AudioData visdata, int isBeat) {
    int grid_width = parameters().get_int("grid_width", 16);
    int grid_height = parameters().get_int("grid_height", 16);
    bool rectangular = parameters().get_bool("rectangular", false);
    int interp_int = parameters().get_int("interpolation", 0);
    InterpolationMode interp_mode = static_cast<InterpolationMode>(interp_int);
    bool wrap = parameters().get_bool("wrap", false);
    std::string pixel_script = parameters().get_string("pixel_script");
    
    // Execute script phases in order
    execute_init_script(visdata, w, h);
    execute_frame_script(visdata, w, h);
    if (isBeat) {
        execute_beat_script(visdata, w, h);
    }
    
    // For rectangular mode, just pass the script directly to the grid generator
    // The CoordinateLookupTable will execute it properly using the script engine
    std::string x_expr, y_expr;
    
    if (rectangular) {
        // In rectangular mode, the pixel script should contain assignments like "x=x; y=y-0.01"
        // Pass the script to be executed by the script engine in CoordinateLookupTable
        x_expr = pixel_script;
        y_expr = pixel_script;
    } else {
        // In polar mode, similar approach but for d,r coordinates
        x_expr = pixel_script;
        y_expr = pixel_script;
    }
    
    grid_table_.generate(w, h, grid_width, grid_height, 
                        x_expr, y_expr,
                        rectangular, false, // subpixel disabled for grid mode
                        visdata, wrap, interp_mode);
    
    // Update state
    last_width_ = w;
    last_height_ = h;
    last_grid_width_ = grid_width;
    last_grid_height_ = grid_height;
    last_init_script_ = parameters().get_string("init_script");
    last_frame_script_ = parameters().get_string("frame_script");
    last_beat_script_ = parameters().get_string("beat_script");
    last_pixel_script_ = pixel_script;
    last_rectangular_ = rectangular;
    last_interp_mode_ = interp_mode;
    last_wrap_ = wrap;
    last_blend_ = parameters().get_bool("blend", false);
}

DynamicMovementEffect::DynamicMovementEffect()
    : last_width_(0), last_height_(0), last_grid_width_(0), last_grid_height_(0),
      last_rectangular_(false), last_interp_mode_(InterpolationMode::LINEAR),
      last_wrap_(false), last_blend_(false), script_initialized_(false)
{
    // Initialize script variables  
    memset(script_vars_, 0, sizeof(script_vars_));
    setup_parameters();
}

int DynamicMovementEffect::render(AudioData visdata, int isBeat,
                                 uint32_t* framebuffer, uint32_t* fbout,
                                 int w, int h) {
    if (!is_enabled()) return 0;
    
    bool no_movement = parameters().get_bool("no_movement", false);
    if (no_movement) {
        // Just copy input to output
        memcpy(fbout, framebuffer, w * h * sizeof(uint32_t));
        return 1;
    }
    
    // Check if we need to regenerate the grid
    if (needs_grid_regeneration(w, h, visdata)) {
        generate_grid(w, h, visdata, isBeat);
    }
    
    // Apply grid transformation with interpolation
    bool blend = parameters().get_bool("blend", false);
    grid_table_.apply(framebuffer, fbout, w, h, blend);
    
    return 1; // Use fbout
}

} // namespace avs