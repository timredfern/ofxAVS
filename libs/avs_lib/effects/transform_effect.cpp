#include "transform_effect.h"
#include "../core/plugin_manager.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace avs {

TransformEffect::TransformEffect() : expressions_valid_(false) {
    script_engine_ = std::make_unique<ScriptEngine>();
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
    params.add_parameter(std::make_shared<Parameter>("bilinear", ParameterType::BOOL, true));
    params.add_parameter(std::make_shared<Parameter>("wrap", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("blend_mode", ParameterType::INT, 0, 0, 3));
    
    // Beat-reactive variables (for future use)
    params.add_parameter(std::make_shared<Parameter>("beat_sensitivity", ParameterType::FLOAT, 1.0f, 0.0f, 2.0f));
}

bool TransformEffect::compile_expressions() {
    std::string x_expr = parameters().get_string("x_expr");
    std::string y_expr = parameters().get_string("y_expr");
    
    // Only recompile if expressions changed
    if (x_expr == last_x_expr_ && y_expr == last_y_expr_ && expressions_valid_) {
        return true;
    }
    
    // Test expressions by evaluating them with dummy values
    script_engine_->set_pixel_context(50, 50, 100, 100);
    script_engine_->set_color_context(0.5, 0.5, 0.5);
    
    // Set dummy audio context for testing
    AudioData dummy_audio = {};
    script_engine_->set_audio_context(dummy_audio, false);
    
    double test_x = script_engine_->evaluate(x_expr);
    if (script_engine_->has_error()) {
        expressions_valid_ = false;
        return false;
    }
    
    double test_y = script_engine_->evaluate(y_expr);
    if (script_engine_->has_error()) {
        expressions_valid_ = false;
        return false;
    }
    
    last_x_expr_ = x_expr;
    last_y_expr_ = y_expr;
    expressions_valid_ = true;
    
    return true;
}

TransformEffect::TransformPoint TransformEffect::transform_pixel(int px, int py, int w, int h, uint32_t color) {
    // Extract color components
    double r = ((color >> 16) & 0xFF) / 255.0;
    double g = ((color >> 8) & 0xFF) / 255.0;
    double b = (color & 0xFF) / 255.0;
    
    // Set context for this pixel
    script_engine_->set_pixel_context(px, py, w, h);
    script_engine_->set_color_context(r, g, b);
    
    // Evaluate transformation expressions
    double new_x = script_engine_->evaluate(last_x_expr_);
    double new_y = script_engine_->evaluate(last_y_expr_);
    
    return {new_x, new_y};
}

uint32_t TransformEffect::sample_pixel(uint32_t* framebuffer, double x, double y, int w, int h) {
    bool wrap = parameters().get_bool("wrap");
    bool bilinear = parameters().get_bool("bilinear");
    
    // Handle wrapping or clamping
    if (wrap) {
        x = x - std::floor(x);  // Wrap to [0,1]
        y = y - std::floor(y);
    } else {
        x = std::clamp(x, 0.0, 1.0);
        y = std::clamp(y, 0.0, 1.0);
    }
    
    // Convert normalized coordinates to pixel coordinates
    double pixel_x = x * (w - 1);
    double pixel_y = y * (h - 1);
    
    if (!bilinear) {
        // Nearest neighbor sampling
        int ix = (int)(pixel_x + 0.5);
        int iy = (int)(pixel_y + 0.5);
        
        if (ix >= 0 && ix < w && iy >= 0 && iy < h) {
            return framebuffer[iy * w + ix];
        }
        return 0xFF000000; // Black with alpha for out-of-bounds
    }
    
    // Bilinear interpolation
    int ix1 = (int)pixel_x;
    int iy1 = (int)pixel_y;
    int ix2 = ix1 + 1;
    int iy2 = iy1 + 1;
    
    double fx = pixel_x - ix1;
    double fy = pixel_y - iy1;
    
    // Sample four neighboring pixels
    uint32_t p11 = 0, p12 = 0, p21 = 0, p22 = 0;
    
    if (ix1 >= 0 && ix1 < w && iy1 >= 0 && iy1 < h) p11 = framebuffer[iy1 * w + ix1];
    if (ix2 >= 0 && ix2 < w && iy1 >= 0 && iy1 < h) p21 = framebuffer[iy1 * w + ix2];
    if (ix1 >= 0 && ix1 < w && iy2 >= 0 && iy2 < h) p12 = framebuffer[iy2 * w + ix1];
    if (ix2 >= 0 && ix2 < w && iy2 >= 0 && iy2 < h) p22 = framebuffer[iy2 * w + ix2];
    
    // Interpolate each color channel
    auto interp_channel = [](uint32_t p11, uint32_t p12, uint32_t p21, uint32_t p22, 
                           double fx, double fy, int shift) -> int {
        int c11 = (p11 >> shift) & 0xFF;
        int c12 = (p12 >> shift) & 0xFF;
        int c21 = (p21 >> shift) & 0xFF;
        int c22 = (p22 >> shift) & 0xFF;
        
        double c1 = c11 * (1.0 - fx) + c21 * fx;
        double c2 = c12 * (1.0 - fx) + c22 * fx;
        double result = c1 * (1.0 - fy) + c2 * fy;
        
        return (int)(result + 0.5);
    };
    
    int r = interp_channel(p11, p12, p21, p22, fx, fy, 16);
    int g = interp_channel(p11, p12, p21, p22, fx, fy, 8);
    int b = interp_channel(p11, p12, p21, p22, fx, fy, 0);
    
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

int TransformEffect::render(AudioData visdata, int isBeat,
                          uint32_t* framebuffer, uint32_t* fbout,
                          int w, int h) {
    if (!is_enabled()) return 0;
    
    // Set audio context for all expressions in this frame
    script_engine_->set_audio_context(*reinterpret_cast<const AudioData*>(visdata), isBeat != 0);
    
    // Compile expressions if needed
    if (!compile_expressions()) {
        // If expressions are invalid, just copy the input
        if (framebuffer != fbout) {
            memcpy(fbout, framebuffer, w * h * sizeof(uint32_t));
        }
        return 1; // Use output buffer
    }
    
    // Transform each pixel
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            uint32_t pixel_color = framebuffer[idx];
            
            // Get transformed coordinates
            TransformPoint transformed = transform_pixel(x, y, w, h, pixel_color);
            
            // Sample from the transformed location
            uint32_t new_pixel = sample_pixel(framebuffer, transformed.x, transformed.y, w, h);
            
            fbout[idx] = new_pixel;
        }
    }
    
    return 1; // Use output buffer
}

// Register the effect
REGISTER_AVS_EFFECT("transform", TransformEffect);

} // namespace avs