#pragma once

#include "../core/effect_base.h"
#include "../core/coordinate_lookup_table.h"
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
    
    // For testing - access to lookup table
    const CoordinateLookupTable& get_lookup_table() const { return lookup_table_; }

private:
    CoordinateLookupTable lookup_table_;
    
    // Expression state tracking
    std::string last_x_expr_;
    std::string last_y_expr_;
    bool last_rectangular_;
    bool last_subpixel_;
    bool last_wrap_;
    int last_grid_width_;
    int last_grid_height_;
    InterpolationMode last_interp_mode_;
    int last_width_;
    int last_height_;
    bool table_valid_;
    
    void setup_parameters();
    bool needs_table_regeneration(int w, int h, const AudioData& visdata) const;
};

} // namespace avs