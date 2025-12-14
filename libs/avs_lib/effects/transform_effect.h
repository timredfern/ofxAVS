#pragma once

#include "../core/effect_base.h"
#include "../core/script/script_engine.h"
#include <memory>

namespace avs {

/**
 * Transform Effect - One of AVS's most powerful effects
 * 
 * Allows mathematical transformation of pixel coordinates using NS-EEL expressions.
 * Common uses include:
 * - Coordinate transformation (rotation, scaling, translation)
 * - Distortion effects (fisheye, wave, ripple)
 * - Polar/cartesian coordinate conversion
 * - Time-based transformations using beat detection
 * 
 * Variables available in expressions:
 * - x, y: current pixel coordinates (normalized 0-1)
 * - w, h: screen dimensions
 * - r, g, b: current pixel color values (0-1)
 * 
 * The effect evaluates expressions for each pixel to determine new coordinates,
 * then samples from the source image at those transformed positions.
 */
class TransformEffect : public EffectBase {
public:
    TransformEffect();
    virtual ~TransformEffect() = default;
    
    int render(AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override;
    
    std::string get_name() const override { return "Transform"; }
    std::string get_description() const override { return "Mathematical coordinate transformation"; }
    
    // For testing - access to script engine
    ScriptEngine* get_script_engine() { return script_engine_.get(); }

private:
    std::unique_ptr<ScriptEngine> script_engine_;
    
    // Compiled expressions for performance
    std::string last_x_expr_;
    std::string last_y_expr_;
    bool expressions_valid_;
    
    void setup_parameters();
    void update_expressions();
    bool compile_expressions();
    
    // Coordinate transformation helpers
    struct TransformPoint {
        double x, y;
    };
    
    TransformPoint transform_pixel(int px, int py, int w, int h, uint32_t color);
    uint32_t sample_pixel(uint32_t* framebuffer, double x, double y, int w, int h);
};

} // namespace avs