// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "effect_base.h"
#include <vector>
#include <string>
#include <cstdint>
#include <utility>

namespace avs {

// Forward declaration
class ScriptEngine;

/**
 * Coordinate mode for grid generation
 */
enum class CoordMode {
    RECTANGULAR,  // x, y in [-1, 1]
    POLAR         // d (distance), r (angle)
};

/**
 * CoordinateGrid - Grid-based coordinate transformation
 *
 * Implements the two-stage transformation process from original AVS r_dmove.cpp:
 *
 * Stage 1: Grid Generation + Interpolation
 *   - Script runs at each grid point (e.g., 16x16)
 *   - Stores source coordinates as 16.16 fixed-point
 *   - For each output pixel, bilinearly interpolates between grid points
 *   - Grid interpolation is ALWAYS bilinear (no option to disable)
 *
 * Stage 2: Pixel Sampling
 *   - Reads from source image at the interpolated coordinates
 *   - subpixel=true: bilinear sample from source
 *   - subpixel=false: nearest neighbor from source
 *
 * Grid dimensions control the coarseness of the transformation:
 *   - Small grid (2x2): Visible stepping artifacts due to coarse interpolation
 *   - Large grid (32x32): Smooth transformation
 */
class CoordinateGrid {
public:
    CoordinateGrid();
    ~CoordinateGrid() = default;

    /**
     * Resize the grid storage
     *
     * @param grid_width Grid width (number of grid points horizontally)
     * @param grid_height Grid height (number of grid points vertically)
     */
    void resize(int grid_width, int grid_height);

    /**
     * Set source coordinates for a grid point
     *
     * @param gx Grid X index (0 to grid_width-1)
     * @param gy Grid Y index (0 to grid_height-1)
     * @param src_x Source X coordinate in pixels
     * @param src_y Source Y coordinate in pixels
     */
    void set(int gx, int gy, double src_x, double src_y);

    /**
     * Get source coordinates for a grid point
     *
     * @param gx Grid X index
     * @param gy Grid Y index
     * @return Pair of (src_x, src_y) in pixels
     */
    std::pair<double, double> get(int gx, int gy) const;

    /**
     * Sample the grid with bilinear interpolation
     *
     * This performs the grid interpolation that happens for every output pixel.
     * Always uses bilinear interpolation (matching original AVS behavior).
     *
     * @param norm_x Normalized X position [0, 1] across the output
     * @param norm_y Normalized Y position [0, 1] across the output
     * @return Pair of interpolated (src_x, src_y) in pixels
     */
    std::pair<double, double> sample(double norm_x, double norm_y) const;

    /**
     * Generate grid by evaluating script at each grid point
     *
     * Creates a temporary ScriptEngine for evaluation. Use the overload
     * accepting ScriptEngine& if you need persistent variables from
     * frame/beat scripts (e.g., DynamicMovementEffect).
     *
     * @param grid_width Grid width
     * @param grid_height Grid height
     * @param output_width Output image width
     * @param output_height Output image height
     * @param script Script to evaluate at each grid point
     * @param mode RECTANGULAR or POLAR coordinate mode
     * @param audio Audio data for script variables
     */
    void generate(int grid_width, int grid_height,
                  int output_width, int output_height,
                  const std::string& script,
                  CoordMode mode,
                  AudioData audio);

    /**
     * Generate grid using an external ScriptEngine
     *
     * Use this when the pixel script needs access to variables set by
     * init/frame/beat scripts. The engine's existing variable values
     * are preserved and used during evaluation.
     *
     * @param engine ScriptEngine with pre-set variables from frame/beat scripts
     * @param grid_width Grid width
     * @param grid_height Grid height
     * @param output_width Output image width
     * @param output_height Output image height
     * @param script Script to evaluate at each grid point
     * @param mode RECTANGULAR or POLAR coordinate mode
     * @param audio Audio data for script variables
     */
    void generate(ScriptEngine& engine,
                  int grid_width, int grid_height,
                  int output_width, int output_height,
                  const std::string& script,
                  CoordMode mode,
                  AudioData audio);

    /**
     * Apply transformation to framebuffer
     *
     * Stage 1: For each output pixel, bilinearly interpolate grid to get source coords
     * Stage 2: Sample source image at those coords (bilinear or nearest based on subpixel)
     *
     * @param input Input framebuffer
     * @param output Output framebuffer
     * @param width Image width
     * @param height Image height
     * @param subpixel If true, use bilinear sampling from source image
     * @param wrap If true, wrap at image boundaries
     * @param blend If true, blend with existing output
     */
    void apply(const uint32_t* input, uint32_t* output,
               int width, int height,
               bool subpixel, bool wrap, bool blend) const;

    int grid_width() const { return grid_width_; }
    int grid_height() const { return grid_height_; }
    bool is_valid() const { return !grid_.empty(); }

private:
    // Grid storage: pairs of 16.16 fixed-point source coordinates
    // Stored as (x, y) pairs: grid_[gy * grid_width_ + gx] = {x_fixed, y_fixed}
    std::vector<std::pair<int32_t, int32_t>> grid_;

    int grid_width_;
    int grid_height_;
    int output_width_;
    int output_height_;

    // Convert between double and 16.16 fixed-point
    static int32_t to_fixed(double val) { return static_cast<int32_t>(val * 65536.0); }
    static double from_fixed(int32_t val) { return val / 65536.0; }

    // Bilinear interpolation of four grid points
    std::pair<int32_t, int32_t> interpolate_grid(int gx, int gy, int fx, int fy) const;

    // Sample pixel from source image (nearest or bilinear)
    uint32_t sample_pixel(const uint32_t* input, int width, int height,
                          int32_t x_fixed, int32_t y_fixed,
                          bool subpixel, bool wrap) const;

    // Bilinear blend of four pixels
    uint32_t blend_pixels(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11,
                          int fx, int fy) const;

    // Blend two pixels (for output blending)
    uint32_t blend_max(uint32_t a, uint32_t b) const;
};

} // namespace avs
