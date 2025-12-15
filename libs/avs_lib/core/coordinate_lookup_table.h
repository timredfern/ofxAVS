#pragma once

#include "effect_base.h"
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace avs {

class ScriptEngine;

/**
 * Transform Lookup Table Utility
 * 
 * Generates and applies coordinate transformation lookup tables for AVS effects.
 * Based on the original AVS Transform effect implementation, this creates a
 * pre-computed mapping from output pixel coordinates to input pixel coordinates.
 * 
 * The lookup table approach creates the characteristic "stepped" artifacts of
 * classic AVS by evaluating expressions at discrete pixel centers rather than
 * continuously.
 */
class TransformLookupTable {
public:
    TransformLookupTable();
    ~TransformLookupTable();
    
    /**
     * Generate lookup table from transformation expressions
     * 
     * @param width Output image width
     * @param height Output image height  
     * @param x_expr Expression for x coordinate transformation
     * @param y_expr Expression for y coordinate transformation
     * @param rectangular If true, use rectangular coordinates (x,y in [-1,1])
     *                   If false, use polar coordinates (r, d)
     * @param subpixel If true, enable subpixel interpolation
     * @param audio_data Audio data for expressions (beat, v1-v8, etc)
     * @param wrap If true, coordinates wrap around image boundaries
     */
    void generate(int width, int height, 
                 const std::string& x_expr, const std::string& y_expr,
                 bool rectangular, bool subpixel,
                 const AudioData& audio_data, bool wrap = false);
    
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
    int width_;
    int height_;
    bool subpixel_;
    bool wrap_;
    
    // Helper methods
    void generate_rectangular(const std::string& x_expr, const std::string& y_expr,
                            const AudioData& audio_data);
    void generate_polar(const std::string& x_expr, const std::string& y_expr,
                       const AudioData& audio_data);
    
    uint32_t encode_lookup(double x, double y) const;
    uint32_t sample_with_interpolation(const uint32_t* input, uint32_t base_offset,
                                      uint32_t x_partial, uint32_t y_partial) const;
    
    // Clamp or wrap coordinates based on settings
    void clamp_or_wrap(double& x, double& y) const;
};

} // namespace avs