// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "movement_effect.h"
#include "../core/parameter.h"
#include "../core/script/script_engine.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace avs {

MovementEffect::MovementEffect() 
    : table_valid_(false), last_width_(0), last_height_(0), 
      last_preset_index_(-1), last_rectangular_(true), 
      last_source_mapped_(false), last_wrap_(false), 
      last_blend_(false), last_subpixel_(true)
{
    setup_parameters();
}

void MovementEffect::setup_parameters() {
    auto& params = parameters();
    
    params.add_parameter(std::make_shared<Parameter>("enabled", ParameterType::BOOL, true));
    
    // Preset selection (0-23 = built-ins, 24 = custom)
    params.add_parameter(std::make_shared<Parameter>("preset", ParameterType::INT, 0, 0, 24));
    
    // Custom expression (used when preset = 24)
    params.add_parameter(std::make_shared<Parameter>("custom_expr", ParameterType::STRING, 
        std::string("d = d * 0.95; r = r + 0.1")));
    
    // Coordinate system
    params.add_parameter(std::make_shared<Parameter>("rectangular", ParameterType::BOOL, false)); // Default: polar
    
    // Rendering options (matching original AVS)
    params.add_parameter(std::make_shared<Parameter>("source_mapped", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("wrap", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("blend", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("subpixel", ParameterType::BOOL, true));
}

bool MovementEffect::needs_table_regeneration(int w, int h, AudioData visdata) const {
    int preset_index = parameters().get_int("preset", 0);
    std::string custom_expr = parameters().get_string("custom_expr");
    bool rectangular = parameters().get_bool("rectangular", false);
    bool source_mapped = parameters().get_bool("source_mapped", false);
    bool wrap = parameters().get_bool("wrap", false);
    bool blend = parameters().get_bool("blend", false);
    bool subpixel = parameters().get_bool("subpixel", true);
    
    return !table_valid_ || 
           w != last_width_ || 
           h != last_height_ ||
           preset_index != last_preset_index_ ||
           custom_expr != last_custom_expr_ ||
           rectangular != last_rectangular_ ||
           source_mapped != last_source_mapped_ ||
           wrap != last_wrap_ ||
           blend != last_blend_ ||
           subpixel != last_subpixel_;
}

void MovementEffect::generate_lookup_table(int w, int h, AudioData visdata) {
    // Resize lookup table for full resolution
    lookup_table_.resize(w * h);
    
    int preset_index = parameters().get_int("preset", 0);
    bool rectangular = parameters().get_bool("rectangular", false);
    
    printf("MovementEffect: Generating full-resolution lookup table %dx%d, preset=%d, rect=%s\n", 
           w, h, preset_index, rectangular ? "true" : "false");
    
    double max_d = sqrt(w*w + h*h) / 2.0; // Maximum distance from center
    
    // Generate lookup for every pixel
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // Convert to normalized coordinates (-1 to 1)
            double nx = (2.0 * x / (w - 1)) - 1.0;
            double ny = (2.0 * y / (h - 1)) - 1.0;
            
            // Convert to polar if needed
            double r = atan2(ny, nx);
            double d = sqrt(nx*nx + ny*ny);
            
            // Apply transformation
            double out_x = nx, out_y = ny, out_r = r, out_d = d;
            
            if (preset_index < 24) {
                // Use built-in preset script
                std::string script = get_preset_script(preset_index);
                if (!script.empty()) {
                    evaluate_movement_script(script, out_x, out_y, out_r, out_d, visdata, w, h);
                }
            } else {
                // Use custom script
                std::string custom_script = parameters().get_string("custom_expr");
                evaluate_movement_script(custom_script, out_x, out_y, out_r, out_d, visdata, w, h);
            }
            
            // Convert back to pixel coordinates
            int dest_x, dest_y;
            if (rectangular) {
                dest_x = (int)((out_x + 1.0) * (w - 1) / 2.0);
                dest_y = (int)((out_y + 1.0) * (h - 1) / 2.0);
            } else {
                dest_x = (int)((cos(out_r) * out_d + 1.0) * (w - 1) / 2.0);
                dest_y = (int)((sin(out_r) * out_d + 1.0) * (h - 1) / 2.0);
            }
            
            // Clamp to screen bounds
            dest_x = std::max(0, std::min(w - 1, dest_x));
            dest_y = std::max(0, std::min(h - 1, dest_y));
            
            // Store lookup (source pixel index)
            lookup_table_[y * w + x] = dest_y * w + dest_x;
        }
    }
    
    // Update state
    last_width_ = w;
    last_height_ = h;
    last_preset_index_ = preset_index;
    last_custom_expr_ = parameters().get_string("custom_expr");
    last_rectangular_ = rectangular;
    last_source_mapped_ = parameters().get_bool("source_mapped", false);
    last_wrap_ = parameters().get_bool("wrap", false);
    last_blend_ = parameters().get_bool("blend", false);
    last_subpixel_ = parameters().get_bool("subpixel", true);
    
    table_valid_ = true;
}

std::string MovementEffect::get_preset_script(int preset_index) const {
    // Preset scripts from original AVS r_trans.cpp descriptions[]
    static const std::vector<std::string> preset_scripts = {
        "",  // 0: none
        "",  // 1: slight fuzzify (needs special handling, not script-based)
        "x=x+1/32", // 2: shift rotate left
        "r = r + (0.1 - (0.2 * d)); d = d * 0.96", // 3: big swirl out
        "d = d * (0.99 * (1.0 - sin(r-$PI*0.5) / 32.0)); r = r + (0.03 * sin(d * $PI * 4))", // 4: medium swirl
        "d = d * (0.94 + (cos((r-$PI*0.5) * 32.0) * 0.06))", // 5: sunburster
        "d = d * (1.01 + (cos((r-$PI*0.5) * 4) * 0.04)); r = r + (0.03 * sin(d * $PI * 4))", // 6: swirl to center
        "",  // 7: blocky partial out (needs special handling)
        "r = r + (0.1 * sin(d * $PI * 5))", // 8: swirling around both ways at once
        "t = sin(d * $PI); d = d - (8*t*t*t*t*t)/sqrt((sw*sw+sh*sh)/4)", // 9: bubbling outward
        "t = sin(d * $PI); d = d - (8*t*t*t*t*t)/sqrt((sw*sw+sh*sh)/4); t=cos(d*$PI/2.0); r= r + 0.1*t*t*t", // 10: bubbling outward with swirl
        "d = d * (0.95 + (cos(((r-$PI*0.5) * 5.0) - ($PI / 2.50)) * 0.03))", // 11: 5 pointed distro
        "r = r + 0.04; d = d * (0.96 + cos(d * $PI) * 0.05)", // 12: tunneling
        "t = cos(d * $PI); r = r + (0.07 * t); d = d * (0.98 + t * 0.10)", // 13: bleedin'
        "d=sqrt(x*x+y*y); r=atan2(y,x); r=r+0.1-0.2*d; d=d*0.96; x=cos(r)*d + 8/128; y=sin(r)*d", // 14: shifted big swirl out
        "d = 0.15", // 15: psychotic beaming outward
        "r = cos(r * 3)", // 16: cosine radial 3-way
        "d = d * (1 - ((d - .35) * .5)); r = r + .1", // 17: spinny tube
        "d = d * (1 - (sin((r-$PI*0.5) * 7) * .03)); r = r + (cos(d * 12) * .03)", // 18: radial swirlies
        "d = d * (1 - (sin((r - $PI*0.5) * 12) * .05)); r = r + (cos(d * 18) * .05); d = d * (1-((d - .4) * .03)); r = r + ((d - .4) * .13)", // 19: swill
        "x = x + (cos(y * 18) * .02); y = y + (sin(x * 14) * .03)", // 20: gridley
        "x = x + (cos(abs(y-.5) * 8) * .02); y = y + (sin(abs(x-.5) * 8) * .05); x = x * .95; y = y * .95", // 21: grapevine
        "y = y * ( 1 + (sin(r + $PI/2) * .3) ); x = x * ( 1 + (cos(r + $PI/2) * .3) ); x = x * .995; y = y * .995", // 22: quadrant
        "y = (r*6)/($PI); x = d" // 23: 6-way kaleida
    };
    
    if (preset_index >= 0 && preset_index < static_cast<int>(preset_scripts.size())) {
        return preset_scripts[preset_index];
    }
    return "";
}

void MovementEffect::evaluate_movement_script(const std::string& script, double& x, double& y, double& r, double& d, 
                                             AudioData visdata, int w, int h) {
    if (script.empty()) {
        return; // Identity transform
    }
    
    // Set up script variables
    static ScriptEngine engine;
    
    // Set coordinate variables
    engine.set_variable("x", x);
    engine.set_variable("y", y);
    engine.set_variable("r", r);
    engine.set_variable("d", d);
    
    // Set constants
    engine.set_variable("$PI", M_PI);
    engine.set_variable("$E", M_E);
    engine.set_variable("sw", (double)w);  // screen width
    engine.set_variable("sh", (double)h);  // screen height
    
    // Set audio variables if available
    // TODO: Extract audio data from visdata
    
    // Execute script
    engine.evaluate(script);
    
    // Get results back
    x = engine.get_variable("x");
    y = engine.get_variable("y");
    r = engine.get_variable("r");
    d = engine.get_variable("d");
}

void MovementEffect::apply_transformation(uint32_t* input, uint32_t* output, int w, int h) {
    bool source_mapped = parameters().get_bool("source_mapped", false);
    
    if (source_mapped) {
        // Forward mapping: for each source pixel, place it at the transformed location
        memset(output, 0, w * h * sizeof(uint32_t)); // Clear output
        
        for (int i = 0; i < w * h; i++) {
            int dest_index = lookup_table_[i];
            if (dest_index >= 0 && dest_index < w * h) {
                output[dest_index] = input[i];
            }
        }
    } else {
        // Inverse mapping (default): for each output pixel, pull from the transformed location
        for (int i = 0; i < w * h; i++) {
            int src_index = lookup_table_[i];
            if (src_index >= 0 && src_index < w * h) {
                output[i] = input[src_index];
            } else {
                output[i] = 0; // Black for out-of-bounds
            }
        }
    }
}

int MovementEffect::render(AudioData visdata, int isBeat,
                          uint32_t* framebuffer, uint32_t* fbout,
                          int w, int h) {
    if (!is_enabled()) return 0;
    
    // Check if we need to regenerate the lookup table
    if (needs_table_regeneration(w, h, visdata)) {
        generate_lookup_table(w, h, visdata);
    }
    
    // Apply transformation
    apply_transformation(framebuffer, fbout, w, h);
    
    return 1; // Use fbout
}

} // namespace avs