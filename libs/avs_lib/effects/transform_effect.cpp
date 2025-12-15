#include "transform_effect.h"
#include "../core/plugin_manager.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace avs {

TransformEffect::TransformEffect() 
    : last_rectangular_(true), last_subpixel_(true), last_wrap_(false),
      last_width_(0), last_height_(0), table_valid_(false)
{
    setup_parameters();
}

void TransformEffect::setup_parameters() {
    auto& params = parameters();
    
    params.add_parameter(std::make_shared<Parameter>("enabled", ParameterType::BOOL, true));
    
    // Transform expressions - classic AVS defaults
    params.add_parameter(std::make_shared<Parameter>("x_expr", ParameterType::STRING, 
        std::string("x"))); // Default: no transformation
    params.add_parameter(std::make_shared<Parameter>("y_expr", ParameterType::STRING, 
        std::string("y"))); // Default: no transformation
    
    // Transform options
    params.add_parameter(std::make_shared<Parameter>("polar", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("subpixel", ParameterType::BOOL, true));
    params.add_parameter(std::make_shared<Parameter>("wrap", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("blend", ParameterType::BOOL, false));
    
    // Beat-reactive variables (for future use)
    params.add_parameter(std::make_shared<Parameter>("beat_sensitivity", ParameterType::FLOAT, 1.0f, 0.0f, 2.0f));
}

bool TransformEffect::needs_table_regeneration(int w, int h, const AudioData& visdata) const {
    std::string x_expr = parameters().get_string("x_expr");
    std::string y_expr = parameters().get_string("y_expr");
    bool rectangular = !parameters().get_bool("polar", false);
    bool subpixel = parameters().get_bool("subpixel", true);
    bool wrap = parameters().get_bool("wrap", false);
    
    return !table_valid_ || 
           x_expr != last_x_expr_ || 
           y_expr != last_y_expr_ ||
           rectangular != last_rectangular_ ||
           subpixel != last_subpixel_ ||
           wrap != last_wrap_ ||
           w != last_width_ || 
           h != last_height_;
}


int TransformEffect::render(AudioData visdata, int isBeat,
                          uint32_t* framebuffer, uint32_t* fbout,
                          int w, int h) {
    if (!is_enabled()) return 0;
    
    // Check if we need to regenerate the lookup table
    if (needs_table_regeneration(w, h, *reinterpret_cast<const AudioData*>(visdata))) {
        
        // Get current parameters
        std::string x_expr = parameters().get_string("x_expr");
        std::string y_expr = parameters().get_string("y_expr");
        bool rectangular = !parameters().get_bool("polar", false);
        bool subpixel = parameters().get_bool("subpixel", true);
        bool wrap = parameters().get_bool("wrap", false);
        
        // Generate new lookup table
        lookup_table_.generate(w, h, x_expr, y_expr, rectangular, subpixel, 
                             *reinterpret_cast<const AudioData*>(visdata), wrap);
        
        // Update state
        last_x_expr_ = x_expr;
        last_y_expr_ = y_expr;
        last_rectangular_ = rectangular;
        last_subpixel_ = subpixel;
        last_wrap_ = wrap;
        last_width_ = w;
        last_height_ = h;
        table_valid_ = true;
    }
    
    // Apply the lookup table transformation
    bool blend = parameters().get_bool("blend", false);
    lookup_table_.apply(framebuffer, fbout, w, h, blend);
    
    return 1; // Use output buffer
}

// Register the effect
REGISTER_AVS_EFFECT("transform", TransformEffect);

} // namespace avs