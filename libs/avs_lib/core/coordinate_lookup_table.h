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
 * Generates and applies coordinate transformation lookup tables for AVS effects.
 * This utility can be used by Transform, Movement, and other coordinate-mapping effects.
 * 
 * Based on the original AVS approach, this creates a pre-computed grid mapping from 
 * output coordinates to input coordinates. The grid resolution is configurable, with
 * smaller grids creating more pronounced stepping artifacts characteristic of classic AVS.
 * 
 * Typical grid sizes:
 * - 16x16: Heavy stepping, classic AVS look, 1KB memory
 * - 32x32: Moderate stepping, 4KB memory  
 * - 64x64: Subtle stepping, 16KB memory
 * - Full resolution: No stepping, smooth transforms, high memory usage
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
                 const AudioData& audio_data, bool wrap = false,
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
     * Get lookup value for a specific output coordinate (for testing)
     */
    uint32_t get_lookup(int x, int y) const;
    
    /**
     * Check if table was generated with subpixel interpolation
     */
    bool has_subpixel() const { return subpixel_; }
    
    /**
     * Check if table is valid/generated
     */
    bool is_valid() const { return !lookup_table_.empty(); }
    
private:
    std::vector<uint32_t> lookup_table_;
    int output_width_;
    int output_height_;
    int grid_width_;
    int grid_height_;
    bool subpixel_;
    bool wrap_;
    InterpolationMode interp_mode_;
    
    // Helper methods
    void generate_rectangular(const std::string& x_expr, const std::string& y_expr,
                            const AudioData& audio_data);
    void generate_polar(const std::string& x_expr, const std::string& y_expr,
                       const AudioData& audio_data);
    
    uint32_t encode_lookup(double x, double y) const;
    uint32_t sample_with_interpolation(const uint32_t* input, uint32_t base_offset,
                                       uint32_t x_partial, uint32_t y_partial) const;
    uint32_t interpolate_pixels(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11, 
                              double fx, double fy) const;
    
    void apply_subpixel_write(uint32_t pixel, uint32_t* output, 
                             uint32_t base_offset, uint32_t x_partial, uint32_t y_partial,
                             bool blend) const;
    
    uint32_t apply_weight(uint32_t pixel, uint32_t weight) const;
    uint32_t blend_max(uint32_t a, uint32_t b) const;
    
    // Clamp or wrap coordinates based on settings
    void clamp_or_wrap(double& x, double& y) const;
};

} // namespace avs