// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "coordinate_lookup_table.h"
#include "script/script_engine.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace avs {

CoordinateLookupTable::CoordinateLookupTable()
    : output_width_(0), output_height_(0), grid_width_(0), grid_height_(0),
      subpixel_(false), wrap_(false), interp_mode_(InterpolationMode::LINEAR)
{
}

CoordinateLookupTable::~CoordinateLookupTable() = default;

void CoordinateLookupTable::generate(int width, int height, int grid_width, int grid_height,
                                  const std::string& x_expr, const std::string& y_expr,
                                  bool rectangular, bool subpixel,
                                  AudioData audio_data, bool wrap,
                                  InterpolationMode interp_mode)
{
    output_width_ = width;
    output_height_ = height;
    grid_width_ = grid_width;
    grid_height_ = grid_height;
    subpixel_ = subpixel;
    wrap_ = wrap;
    interp_mode_ = interp_mode;
    
    // Allocate grid to store coordinate pairs
    coordinate_grid_.resize(grid_width * grid_height);
    
    if (rectangular) {
        generate_rectangular(x_expr, y_expr, audio_data);
    } else {
        generate_polar(x_expr, y_expr, audio_data);
    }
}

void CoordinateLookupTable::generate_rectangular(const std::string& x_expr, const std::string& y_expr,
                                               AudioData audio_data)
{
    ScriptEngine engine;
    
    // Set audio context
    engine.set_audio_context(audio_data, false);
    
    // Check if both expressions are the same (indicating a multi-statement script)
    bool is_multi_statement = (x_expr == y_expr);
    
    // Generate coordinate transformations for each grid point
    for (int gy = 0; gy < grid_height_; gy++) {
        for (int gx = 0; gx < grid_width_; gx++) {
            // Convert grid coordinates to normalized coordinates [0, 1]
            auto norm_coords = normalize_coordinates(gx, gy);
            double norm_x = norm_coords.first;
            double norm_y = norm_coords.second;
            
            // Set pixel context (convert grid to pixel coordinates for context)
            int pixel_x = (gx * output_width_) / grid_width_;
            int pixel_y = (gy * output_height_) / grid_height_;
            engine.set_pixel_context(pixel_x, pixel_y, output_width_, output_height_);
            
            // Set normalized coordinates as variables
            engine.set_variable("x", norm_x);
            engine.set_variable("y", norm_y);
            
            double dest_norm_x, dest_norm_y;
            
            if (is_multi_statement) {
                // Execute the script once (it should contain assignments like "x=x; y=y-0.01")
                engine.evaluate(x_expr);
                
                // Read back the modified x and y values
                dest_norm_x = engine.get_variable("x");
                dest_norm_y = engine.get_variable("y");
            } else {
                // Evaluate separate expressions for x and y
                dest_norm_x = engine.evaluate(x_expr);
                dest_norm_y = engine.evaluate(y_expr);
            }
            
            // Handle invalid results (NaN, inf)
            if (!std::isfinite(dest_norm_x)) dest_norm_x = norm_x;
            if (!std::isfinite(dest_norm_y)) dest_norm_y = norm_y;
            
            // Store the transformed coordinates (still in normalized space)
            coordinate_grid_[gy * grid_width_ + gx] = {dest_norm_x, dest_norm_y};
        }
    }
}

void CoordinateLookupTable::generate_polar(const std::string& x_expr, const std::string& y_expr,
                                         AudioData audio_data)
{
    ScriptEngine engine;  // Single engine like rectangular mode

    // Set audio context
    engine.set_audio_context(audio_data, false);

    // Calculate max distance for normalization (diagonal/2, matching original AVS)
    double max_d = std::sqrt((double)(output_width_ * output_width_ +
                                       output_height_ * output_height_)) * 0.5;
    double inv_max_d = 1.0 / max_d;

    // Generate coordinate transformations for each grid point
    for (int gy = 0; gy < grid_height_; gy++) {
        for (int gx = 0; gx < grid_width_; gx++) {
            // Convert grid coordinates to pixel coordinates
            double pixel_x = (gx * (output_width_ - 1.0)) / (grid_width_ - 1);
            double pixel_y = (gy * (output_height_ - 1.0)) / (grid_height_ - 1);

            // Convert to centered coordinates (center of screen = 0,0)
            double centered_x = pixel_x - output_width_ * 0.5;
            double centered_y = pixel_y - output_height_ * 0.5;

            // Calculate polar coordinates matching original AVS:
            // x, y in range [-1, 1]
            // d normalized so corners ≈ 1
            // r with PI/2 offset so 0 = "up"
            double x = centered_x * 2.0 / output_width_;
            double y = centered_y * 2.0 / output_height_;
            double d = std::sqrt(centered_x * centered_x + centered_y * centered_y) * inv_max_d;
            double r = std::atan2(centered_y, centered_x) + M_PI * 0.5;  // Offset like original

            // Set pixel context
            engine.set_pixel_context((int)pixel_x, (int)pixel_y, output_width_, output_height_);

            // Set ALL variables (x, y, d, r) like original AVS does
            engine.set_variable("x", x);
            engine.set_variable("y", y);
            engine.set_variable("d", d);
            engine.set_variable("r", r);

            // Execute the script ONCE (same pattern as rectangular mode)
            engine.evaluate(x_expr);

            // Read back the modified d and r values
            double new_d = engine.get_variable("d");
            double new_r = engine.get_variable("r");

            // Handle invalid results
            if (!std::isfinite(new_d)) new_d = d;
            if (!std::isfinite(new_r)) new_r = r;

            // Convert back to cartesian: remove PI/2 offset, scale d back up
            new_r -= M_PI * 0.5;
            double dest_pixel_x = output_width_ * 0.5 + std::cos(new_r) * new_d * max_d;
            double dest_pixel_y = output_height_ * 0.5 + std::sin(new_r) * new_d * max_d;

            // Convert to normalized [0, 1] for storage
            double dest_norm_x = dest_pixel_x / (output_width_ - 1);
            double dest_norm_y = dest_pixel_y / (output_height_ - 1);

            // Store the transformed coordinates
            coordinate_grid_[gy * grid_width_ + gx] = {dest_norm_x, dest_norm_y};
        }
    }
}

std::pair<double, double> CoordinateLookupTable::normalize_coordinates(int pixel_x, int pixel_y) const
{
    // Convert grid coordinates to normalized [0, 1] range
    double norm_x = (double)pixel_x / (grid_width_ - 1);
    double norm_y = (double)pixel_y / (grid_height_ - 1);
    return {norm_x, norm_y};
}

std::pair<double, double> CoordinateLookupTable::denormalize_coordinates(double norm_x, double norm_y) const
{
    // Convert normalized coordinates back to pixel coordinates
    double pixel_x = norm_x * (output_width_ - 1);
    double pixel_y = norm_y * (output_height_ - 1);
    return {pixel_x, pixel_y};
}

std::pair<double, double> CoordinateLookupTable::get_grid_coordinates(int gx, int gy) const
{
    if (gx < 0 || gx >= grid_width_ || gy < 0 || gy >= grid_height_) {
        return {0.0, 0.0};
    }
    return coordinate_grid_[gy * grid_width_ + gx];
}

std::pair<double, double> CoordinateLookupTable::get_interpolated_coordinates(double grid_x, double grid_y) const
{
    if (coordinate_grid_.empty()) {
        return {0.0, 0.0};
    }
    
    return interpolate_coordinates(grid_x, grid_y);
}

std::pair<double, double> CoordinateLookupTable::interpolate_coordinates(double grid_x, double grid_y) const
{
    if (interp_mode_ == InterpolationMode::NONE) {
        // Nearest neighbor - just round to nearest grid point
        int gx = (int)(grid_x + 0.5);
        int gy = (int)(grid_y + 0.5);
        gx = std::clamp(gx, 0, grid_width_ - 1);
        gy = std::clamp(gy, 0, grid_height_ - 1);
        return get_grid_coordinates(gx, gy);
    } else {
        // Bilinear interpolation between grid points
        int gx = (int)grid_x;
        int gy = (int)grid_y;
        double fx = grid_x - gx;
        double fy = grid_y - gy;
        
        // Handle edge case where grid_x/grid_y are exactly at the boundary
        if (gx >= grid_width_ - 1) {
            gx = grid_width_ - 2;
            fx = 1.0;
        }
        if (gy >= grid_height_ - 1) {
            gy = grid_height_ - 2;
            fy = 1.0;
        }
        
        // Clamp to valid grid range
        gx = std::clamp(gx, 0, grid_width_ - 2);
        gy = std::clamp(gy, 0, grid_height_ - 2);
        
        // Get four surrounding grid points
        auto tl = get_grid_coordinates(gx, gy);       // top-left
        auto tr = get_grid_coordinates(gx + 1, gy);   // top-right
        auto bl = get_grid_coordinates(gx, gy + 1);   // bottom-left
        auto br = get_grid_coordinates(gx + 1, gy + 1); // bottom-right
        
        // Bilinear interpolation
        double interp_x = tl.first * (1.0 - fx) * (1.0 - fy) +
                         tr.first * fx * (1.0 - fy) +
                         bl.first * (1.0 - fx) * fy +
                         br.first * fx * fy;
                         
        double interp_y = tl.second * (1.0 - fx) * (1.0 - fy) +
                         tr.second * fx * (1.0 - fy) +
                         bl.second * (1.0 - fx) * fy +
                         br.second * fx * fy;
        
        return {interp_x, interp_y};
    }
}

void CoordinateLookupTable::apply(const uint32_t* input, uint32_t* output,
                               int width, int height, bool blend) const
{
    if (coordinate_grid_.empty() || width != output_width_ || height != output_height_) {
        // Invalid table or size mismatch, copy input to output
        std::copy(input, input + width * height, output);
        return;
    }
    
    // For each output pixel, map to grid coordinates and interpolate
    for (int dest_y = 0; dest_y < height; dest_y++) {
        for (int dest_x = 0; dest_x < width; dest_x++) {
            int dest_idx = dest_y * width + dest_x;
            
            // Calculate grid coordinates for this output pixel
            double grid_x = (dest_x * (grid_width_ - 1.0)) / (width - 1.0);
            double grid_y = (dest_y * (grid_height_ - 1.0)) / (height - 1.0);
            
            // Get interpolated source coordinates (in normalized space)
            auto source_coords = interpolate_coordinates(grid_x, grid_y);
            
            // Convert from normalized space to pixel coordinates
            auto pixel_coords = denormalize_coordinates(source_coords.first, source_coords.second);
            double src_x = pixel_coords.first;
            double src_y = pixel_coords.second;
            
            // Apply clamping or wrapping
            clamp_or_wrap(src_x, src_y);
            
            // Sample from the source pixel
            uint32_t sampled_pixel = sample_pixel(input, src_x, src_y);
            
            if (blend) {
                output[dest_idx] = blend_max(sampled_pixel, output[dest_idx]);
            } else {
                output[dest_idx] = sampled_pixel;
            }
        }
    }
}

uint32_t CoordinateLookupTable::sample_pixel(const uint32_t* input, double x, double y) const
{
    if (subpixel_) {
        // Bilinear interpolation sampling
        int x0 = (int)x;
        int y0 = (int)y;
        double fx = x - x0;
        double fy = y - y0;
        
        // Handle edge case where coordinates are exactly at the boundary
        if (x0 >= output_width_ - 1) {
            x0 = output_width_ - 2;
            fx = 1.0;
        }
        if (y0 >= output_height_ - 1) {
            y0 = output_height_ - 2;
            fy = 1.0;
        }
        
        // Bounds checking
        x0 = std::clamp(x0, 0, output_width_ - 2);
        y0 = std::clamp(y0, 0, output_height_ - 2);
        
        // Sample four neighboring pixels
        uint32_t p00 = input[y0 * output_width_ + x0];
        uint32_t p01 = input[y0 * output_width_ + x0 + 1];
        uint32_t p10 = input[(y0 + 1) * output_width_ + x0];
        uint32_t p11 = input[(y0 + 1) * output_width_ + x0 + 1];
        
        return interpolate_pixels(p00, p01, p10, p11, fx, fy);
    } else {
        // Nearest neighbor sampling
        int ix = (int)(x + 0.5);
        int iy = (int)(y + 0.5);
        
        // Bounds check
        ix = std::clamp(ix, 0, output_width_ - 1);
        iy = std::clamp(iy, 0, output_height_ - 1);
        
        return input[iy * output_width_ + ix];
    }
}

void CoordinateLookupTable::clamp_or_wrap(double& x, double& y) const
{
    if (wrap_) {
        // Wrap coordinates
        x = std::fmod(x, output_width_ - 1);
        if (x < 0) x += output_width_ - 1;
        y = std::fmod(y, output_height_ - 1);
        if (y < 0) y += output_height_ - 1;
    } else {
        // Clamp coordinates
        x = std::clamp(x, 0.0, (double)(output_width_ - 1));
        y = std::clamp(y, 0.0, (double)(output_height_ - 1));
    }
}

uint32_t CoordinateLookupTable::interpolate_pixels(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11, 
                                                  double fx, double fy) const
{
    // Convert to 8.8 fixed point (8 bits integer, 8 bits fraction)
    int ifx = (int)(fx * 256.0);
    int ify = (int)(fy * 256.0);
    int ifx_inv = 256 - ifx;
    int ify_inv = 256 - ify;
    
    // Interpolate each color channel using integer math
    auto interp_channel = [](uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11,
                           int ifx, int ify, int ifx_inv, int ify_inv, int shift) -> uint32_t {
        int c00 = (p00 >> shift) & 0xFF;
        int c01 = (p01 >> shift) & 0xFF;
        int c10 = (p10 >> shift) & 0xFF;
        int c11 = (p11 >> shift) & 0xFF;
        
        // Bilinear interpolation with integer math
        int c0 = (c00 * ifx_inv + c01 * ifx) >> 8;  // Top edge
        int c1 = (c10 * ifx_inv + c11 * ifx) >> 8;  // Bottom edge
        int result = (c0 * ify_inv + c1 * ify) >> 8; // Final
        
        return result & 0xFF;
    };
    
    uint32_t r = interp_channel(p00, p01, p10, p11, ifx, ify, ifx_inv, ify_inv, 16);
    uint32_t g = interp_channel(p00, p01, p10, p11, ifx, ify, ifx_inv, ify_inv, 8);
    uint32_t b = interp_channel(p00, p01, p10, p11, ifx, ify, ifx_inv, ify_inv, 0);
    uint32_t a = interp_channel(p00, p01, p10, p11, ifx, ify, ifx_inv, ify_inv, 24);
    
    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t CoordinateLookupTable::blend_max(uint32_t a, uint32_t b) const
{
    uint32_t r = std::max((a >> 16) & 0xFF, (b >> 16) & 0xFF);
    uint32_t g = std::max((a >> 8) & 0xFF, (b >> 8) & 0xFF);
    uint32_t b_val = std::max(a & 0xFF, b & 0xFF);
    uint32_t alpha = std::max((a >> 24) & 0xFF, (b >> 24) & 0xFF);
    
    return (alpha << 24) | (r << 16) | (g << 8) | b_val;
}

} // namespace avs