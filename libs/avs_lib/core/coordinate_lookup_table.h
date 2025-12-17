#pragma once

#include "effect_base.h"
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace avs {

class ScriptEngine;

/**
 * Interpolation modes for grid-based coordinate lookup
 * 
 * These modes control how the discrete grid values are interpolated
 * when applied to the full-resolution output image:
 * 
 * NONE: No interpolation - each output pixel uses the nearest grid point.
 *       Creates the characteristic "stepped" or "quantized" artifacts
 *       that were a hallmark of classic AVS Transform effects.
 *       Best for: Authentic retro AVS look, dramatic stepped distortions
 * 
 * LINEAR: Bilinear interpolation between grid points for smooth gradients.
 *         Eliminates stepping artifacts but maintains the performance
 *         benefits of grid-based evaluation.
 *         Best for: Modern smooth transforms, subtle distortions
 * 
 * NEAREST: Nearest neighbor interpolation - sharper than linear but
 *          less blocky than none. Each pixel uses the closest grid value
 *          but transitions are still abrupt.
 *          Best for: Pixelated/retro effects with sharp transitions
 */
enum class InterpolationMode {
    NONE,      // No interpolation - blocky/stepped effect (classic AVS)
    LINEAR,    // Bilinear interpolation - smooth transforms  
    NEAREST    // Nearest neighbor - sharp but less blocky than none
};

/**
 * Coordinate Lookup Table Utility
 * 
 * Generates grid-based coordinate transformation lookup tables for AVS effects.
 * This utility stores transformed coordinates in a low-resolution grid and
 * interpolates them for final output, creating the characteristic stepped
 * distortions of classic AVS when interpolation is disabled.
 * 
 * Key differences from full-resolution tables:
 * - Stores coordinates, not pixel indices
 * - Grid-based evaluation with interpolation support
 * - Memory efficient: 16x16 grid = 1KB vs full res = width*height*4 bytes
 * - Configurable interpolation modes for different visual effects
 * 
 * Typical grid sizes:
 * - 8x8: Very blocky, extreme classic AVS look
 * - 16x16: Heavy stepping, classic AVS look, 1KB memory
 * - 32x32: Moderate stepping, 4KB memory  
 * - 64x64: Subtle stepping, 16KB memory
 */
class CoordinateLookupTable {
public:
    CoordinateLookupTable();
    ~CoordinateLookupTable();
    
    /**
     * Generate lookup table from transformation expressions
     * 
     * @param width Output image width
     * @param height Output image height
     * @param grid_width Lookup table grid width (e.g. 16 for 16x16 grid)
     * @param grid_height Lookup table grid height (e.g. 16 for 16x16 grid)
     * @param x_expr Expression for x coordinate transformation
     * @param y_expr Expression for y coordinate transformation
     * @param rectangular If true, use rectangular coordinates (x,y in [-1,1])
     *                   If false, use polar coordinates (r, d)
     * @param subpixel If true, enable subpixel interpolation
     * @param audio_data Audio data for expressions (beat, v1-v8, etc)
     * @param wrap If true, coordinates wrap around image boundaries
     * @param interp_mode Interpolation mode for grid upsampling
     */
    void generate(int width, int height, int grid_width, int grid_height,
                 const std::string& x_expr, const std::string& y_expr,
                 bool rectangular, bool subpixel,
                 AudioData audio_data, bool wrap = false,
                 InterpolationMode interp_mode = InterpolationMode::LINEAR);
    
    /**
     * Apply the lookup table transformation
     * 
     * @param input Input framebuffer
     * @param output Output framebuffer  
     * @param width Image width
     * @param height Image height
     * @param blend If true, blend with existing output
     */
    void apply(const uint32_t* input, uint32_t* output, 
               int width, int height, bool blend = false) const;
    
    /**
     * Get interpolated coordinates for a specific grid position (for testing)
     */
    std::pair<double, double> get_interpolated_coordinates(double grid_x, double grid_y) const;
    
    /**
     * Set interpolation mode (for testing)
     */
    void set_interpolation_mode(InterpolationMode mode) { interp_mode_ = mode; }
    
    /**
     * Check if table was generated with subpixel interpolation
     */
    bool has_subpixel() const { return subpixel_; }
    
    /**
     * Check if table is valid/generated
     */
    bool is_valid() const { return !coordinate_grid_.empty(); }
    
    /**
     * Test pixel interpolation function directly (for debugging color corruption)
     */
    uint32_t interpolate_pixels(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11, 
                               double fx, double fy) const;
    
private:
    // Grid of coordinate pairs (x,y) stored as pairs of doubles
    std::vector<std::pair<double, double>> coordinate_grid_;
    int output_width_;
    int output_height_;
    int grid_width_;
    int grid_height_;
    bool subpixel_;
    bool wrap_;
    InterpolationMode interp_mode_;
    
    // Helper methods
    void generate_rectangular(const std::string& x_expr, const std::string& y_expr,
                            AudioData audio_data);
    void generate_polar(const std::string& x_expr, const std::string& y_expr,
                       AudioData audio_data);
    
    // Coordinate interpolation methods
    std::pair<double, double> interpolate_coordinates(double grid_x, double grid_y) const;
    std::pair<double, double> get_grid_coordinates(int gx, int gy) const;
    
    // Pixel sampling and blending methods
    uint32_t sample_pixel(const uint32_t* input, double x, double y) const;
    uint32_t blend_max(uint32_t a, uint32_t b) const;
    
    // Coordinate transformation utilities
    void clamp_or_wrap(double& x, double& y) const;
    std::pair<double, double> normalize_coordinates(int pixel_x, int pixel_y) const;
    std::pair<double, double> denormalize_coordinates(double norm_x, double norm_y) const;
};

} // namespace avs