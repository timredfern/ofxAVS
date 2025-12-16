#include "dynamic_movement_effect.h"
#include "../core/parameter.h"
#include <cmath>
#include <cstring>

namespace avs {

DynamicMovementEffect::DynamicMovementEffect()
    : last_width_(0), last_height_(0), last_grid_width_(0), last_grid_height_(0),
      last_rectangular_(false), last_interp_mode_(InterpolationMode::NONE),
      last_wrap_(false), last_blend_(false), script_initialized_(false)
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
        std::string("d=d*0.95; r=r+0.1")));
    
    // Grid configuration
    params.add_parameter(std::make_shared<Parameter>("grid_width", ParameterType::INT, 16, 2, 256));
    params.add_parameter(std::make_shared<Parameter>("grid_height", ParameterType::INT, 16, 2, 256));
    
    // Coordinate system
    params.add_parameter(std::make_shared<Parameter>("rectangular", ParameterType::BOOL, false)); // Default: polar
    
    // Interpolation mode
    std::vector<std::string> interp_options = {"None (Stepped)", "Linear", "Nearest"};
    params.add_parameter(std::make_shared<Parameter>("interpolation", ParameterType::ENUM, 0, interp_options));
    
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

void DynamicMovementEffect::execute_init_script(AudioData visdata, int w, int h) {
    // TODO: Implement EEL script execution for init phase
    // For now, initialize script variables
    if (!script_initialized_) {
        for (int i = 0; i < 32; i++) {
            script_vars_[i] = 0.0;
        }
        script_initialized_ = true;
    }
}

void DynamicMovementEffect::execute_frame_script(AudioData visdata, int w, int h) {
    // TODO: Implement EEL script execution for frame phase
    // This would update persistent variables based on current frame state
}

void DynamicMovementEffect::execute_beat_script(AudioData visdata, int w, int h) {
    // TODO: Implement EEL script execution for beat phase  
    // This would update variables based on beat detection
}

void DynamicMovementEffect::execute_pixel_script(double& x, double& y, double& r, double& d,
                                                AudioData visdata, int w, int h) {
    // TODO: Implement full EEL script execution for pixel phase
    // For now, implement basic pattern matching for common transformations
    std::string pixel_script = parameters().get_string("pixel_script");
    
    // Handle common polar transformations
    if (pixel_script.find("d*0.95") != std::string::npos && 
        pixel_script.find("r+0.1") != std::string::npos) {
        // Classic spiral: shrink distance, increase angle
        d *= 0.95;
        r += 0.1;
    } else if (pixel_script.find("d*0.5") != std::string::npos) {
        // Zoom in effect
        d *= 0.5;
    } else if (pixel_script.find("r+") != std::string::npos) {
        // Simple rotation - extract angle
        size_t pos = pixel_script.find("r+");
        if (pos != std::string::npos) {
            std::string angle_str = pixel_script.substr(pos + 2);
            double angle = std::stod(angle_str);
            r += angle;
        }
    }
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
    
    // Generate appropriate coordinate transformation expressions
    std::string x_expr, y_expr;
    
    if (rectangular) {
        // In rectangular mode, use script directly (should modify x,y)
        x_expr = pixel_script;
        y_expr = pixel_script;
    } else {
        // In polar mode, need to convert the script to coordinate expressions
        // For default "d=d*0.95; r=r+0.1", generate expressions that convert polar->cartesian
        if (pixel_script.find("d*0.95") != std::string::npos && 
            pixel_script.find("r+0.1") != std::string::npos) {
            // Generate expressions for the classic spiral transformation
            x_expr = "cos(atan2(y-0.5,x-0.5) + 0.1) * sqrt((x-0.5)*(x-0.5)+(y-0.5)*(y-0.5)) * 0.95 + 0.5";
            y_expr = "sin(atan2(y-0.5,x-0.5) + 0.1) * sqrt((x-0.5)*(x-0.5)+(y-0.5)*(y-0.5)) * 0.95 + 0.5";
        } else if (pixel_script.find("d*0.5") != std::string::npos) {
            // Simple zoom transformation
            x_expr = "0.5 + (x-0.5) * 0.5";
            y_expr = "0.5 + (y-0.5) * 0.5";
        } else if (pixel_script.find("d*0.9") != std::string::npos) {
            // Milder zoom transformation
            x_expr = "0.5 + (x-0.5) * 0.9";
            y_expr = "0.5 + (y-0.5) * 0.9";
        } else {
            // For other scripts, use identity transformation for now
            x_expr = "x";
            y_expr = "y";
        }
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