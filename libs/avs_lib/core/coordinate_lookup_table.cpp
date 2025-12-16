#include "coordinate_lookup_table.h"
#include "script/script_engine.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace avs {

CoordinateLookupTable::CoordinateLookupTable()
    : output_width_(0), output_height_(0), subpixel_(false), wrap_(false)
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
    subpixel_ = subpixel;
    wrap_ = wrap;
    
    // Allocate full-resolution lookup table like original AVS
    lookup_table_.resize(width * height);
    
    if (rectangular) {
        generate_rectangular(x_expr, y_expr, audio_data);
    } else {
        generate_polar(x_expr, y_expr, audio_data);
    }
}

void CoordinateLookupTable::generate_rectangular(const std::string& x_expr, const std::string& y_expr,
                                               AudioData audio_data)
{
    ScriptEngine x_engine, y_engine;
    
    // Set audio context for both engines
    x_engine.set_audio_context(audio_data, false);
    y_engine.set_audio_context(audio_data, false);
    
    double w2 = output_width_ / 2.0;
    double h2 = output_height_ / 2.0;
    double x_scale = 1.0 / w2;
    double y_scale = 1.0 / h2;
    
    // Generate lookup table for every pixel (original AVS approach)
    for (int y = 0; y < output_height_; y++) {
        for (int x = 0; x < output_width_; x++) {
            // Convert source pixel coordinates to normalized [-1, 1] range
            double xd = x - w2;
            double yd = y - h2;
            double norm_x = xd * x_scale;
            double norm_y = yd * y_scale;
            
            // Set pixel context for expressions
            x_engine.set_pixel_context(x, y, output_width_, output_height_);
            y_engine.set_pixel_context(x, y, output_width_, output_height_);
            
            // Set normalized coordinates as variables
            x_engine.set_variable("x", norm_x);
            x_engine.set_variable("y", norm_y);
            y_engine.set_variable("x", norm_x);
            y_engine.set_variable("y", norm_y);
            
            // Evaluate transformation expressions to get destination coordinates
            double dest_norm_x = x_engine.evaluate(x_expr);
            double dest_norm_y = y_engine.evaluate(y_expr);
            
            // Handle invalid results (NaN, inf)
            if (!std::isfinite(dest_norm_x)) dest_norm_x = norm_x;
            if (!std::isfinite(dest_norm_y)) dest_norm_y = norm_y;
            
            // Convert back to pixel coordinates - this is the DESTINATION pixel
            double dest_pixel_x = (dest_norm_x + 1.0) * w2;
            double dest_pixel_y = (dest_norm_y + 1.0) * h2;
            
            // Apply clamping or wrapping to destination coordinates
            clamp_or_wrap(dest_pixel_x, dest_pixel_y);
            
            // Store destination offset in lookup table
            int src_idx = y * output_width_ + x;
            lookup_table_[src_idx] = encode_lookup(dest_pixel_x, dest_pixel_y);
        }
    }
}

void CoordinateLookupTable::generate_polar(const std::string& x_expr, const std::string& y_expr,
                                         AudioData audio_data)
{
    ScriptEngine x_engine, y_engine; // x_expr affects 'd', y_expr affects 'r'
    
    // Set audio context
    x_engine.set_audio_context(audio_data, false);
    y_engine.set_audio_context(audio_data, false);
    
    double max_d = std::sqrt((output_width_ * output_width_ + output_height_ * output_height_)) / 2.0;
    double inv_max_d = 1.0 / max_d;
    double w2 = output_width_ / 2.0;
    double h2 = output_height_ / 2.0;
    
    // Generate lookup table for every pixel
    for (int y = 0; y < output_height_; y++) {
        for (int x = 0; x < output_width_; x++) {
            // Convert to polar coordinates
            double xd = x - w2;
            double yd = y - h2;
            double d = std::sqrt(xd * xd + yd * yd) * inv_max_d; // normalized distance [0,1]
            double r = std::atan2(yd, xd) + M_PI * 0.5; // angle [0, 2π]
            
            // Set context
            x_engine.set_pixel_context(x, y, output_width_, output_height_);
            y_engine.set_pixel_context(x, y, output_width_, output_height_);
            
            // Set polar variables
            x_engine.set_variable("d", d);
            x_engine.set_variable("r", r);
            y_engine.set_variable("d", d);
            y_engine.set_variable("r", r);
            
            // Evaluate expressions (x_expr modifies d, y_expr modifies r)
            double new_d = x_engine.evaluate(x_expr);
            double new_r = y_engine.evaluate(y_expr);
            
            // Handle invalid results
            if (!std::isfinite(new_d)) new_d = d;
            if (!std::isfinite(new_r)) new_r = r;
            
            // Convert back to cartesian pixel coordinates
            new_d *= max_d; // denormalize distance
            new_r -= M_PI * 0.5; // adjust angle offset
            
            double dest_pixel_x = w2 + std::cos(new_r) * new_d;
            double dest_pixel_y = h2 + std::sin(new_r) * new_d;
            
            // Apply clamping or wrapping
            clamp_or_wrap(dest_pixel_x, dest_pixel_y);
            
            // Store in lookup table
            int src_idx = y * output_width_ + x;
            lookup_table_[src_idx] = encode_lookup(dest_pixel_x, dest_pixel_y);
        }
    }
}

void CoordinateLookupTable::clamp_or_wrap(double& x, double& y) const
{
    if (wrap_) {
        // Wrap coordinates
        if (subpixel_) {
            // For subpixel, wrap to [0, dimension-1]
            x = std::fmod(x, output_width_ - 1);
            if (x < 0) x += output_width_ - 1;
            y = std::fmod(y, output_height_ - 1);
            if (y < 0) y += output_height_ - 1;
        } else {
            // For non-subpixel, wrap to [0, dimension]
            x = std::fmod(x, output_width_);
            if (x < 0) x += output_width_;
            y = std::fmod(y, output_height_);
            if (y < 0) y += output_height_;
        }
    } else {
        // Clamp coordinates
        if (subpixel_) {
            x = std::clamp(x, 0.0, (double)(output_width_ - 1));
            y = std::clamp(y, 0.0, (double)(output_height_ - 1));
        } else {
            x = std::clamp(x, 0.0, (double)(output_width_ - 1));
            y = std::clamp(y, 0.0, (double)(output_height_ - 1));
        }
    }
}

uint32_t CoordinateLookupTable::encode_lookup(double x, double y) const
{
    if (subpixel_) {
        // Encode with subpixel interpolation data (like original AVS)
        int base_x = (int)x;
        int base_y = (int)y;
        
        // Calculate fractional parts
        double frac_x = x - base_x;
        double frac_y = y - base_y;
        
        // Encode fractional parts as 5-bit integers [0,31]
        uint32_t x_partial = (uint32_t)(frac_x * 32.0);
        uint32_t y_partial = (uint32_t)(frac_y * 32.0);
        
        // Clamp partials to valid range
        x_partial = std::min(x_partial, 31u);
        y_partial = std::min(y_partial, 31u);
        
        // Bounds check for base coordinates
        if (base_x < 0) { base_x = 0; x_partial = 0; }
        if (base_y < 0) { base_y = 0; y_partial = 0; }
        if (base_x >= output_width_ - 1) { base_x = output_width_ - 2; x_partial = 31; }
        if (base_y >= output_height_ - 1) { base_y = output_height_ - 2; y_partial = 31; }
        
        uint32_t base_offset = base_x + base_y * output_width_;
        return base_offset | (y_partial << 22) | (x_partial << 27);
    } else {
        // Simple integer coordinates
        int ix = (int)(x + 0.5);
        int iy = (int)(y + 0.5);
        
        // Bounds check
        ix = std::clamp(ix, 0, output_width_ - 1);
        iy = std::clamp(iy, 0, output_height_ - 1);
        
        return ix + iy * output_width_;
    }
}

void CoordinateLookupTable::apply(const uint32_t* input, uint32_t* output,
                               int width, int height, bool blend) const
{
    if (lookup_table_.empty() || width != output_width_ || height != output_height_) {
        // Invalid table or size mismatch, copy input to output
        std::copy(input, input + width * height, output);;
        return;
    }
    
    const uint32_t OFFSET_MASK = (1 << 22) - 1;
    
    // Inverse mapping like original AVS: for each output pixel, find which input pixel to read
    for (int dest_y = 0; dest_y < height; dest_y++) {
        for (int dest_x = 0; dest_x < width; dest_x++) {
            int dest_idx = dest_y * width + dest_x;
            
            // Calculate grid coordinates for this output pixel
            double grid_x = (dest_x * grid_width_) / (double)width;
            double grid_y = (dest_y * grid_height_) / (double)height;
            
            int gx = (int)grid_x;
            int gy = (int)grid_y;
            
            // Clamp to valid grid range
            gx = std::min(gx, grid_width_ - 1);
            gy = std::min(gy, grid_height_ - 1);
            
            if (interp_mode_ == InterpolationMode::NONE) {
                // No interpolation - use nearest grid point
                uint32_t lookup = lookup_table_[gy * grid_width_ + gx];
                uint32_t src_offset = subpixel_ ? (lookup & OFFSET_MASK) : lookup;
                if (src_offset < width * height) {
                    output[dest_idx] = input[src_offset];
                } else {
                    output[dest_idx] = 0;
                }
            } else {
                // Interpolate between coordinate transformations
                int gx1 = std::min(gx + 1, grid_width_ - 1);
                int gy1 = std::min(gy + 1, grid_height_ - 1);
                
                // Calculate fractional position within grid cell
                double fx = grid_x - gx;
                double fy = grid_y - gy;
                
                // Get lookup coordinates from grid corners
                uint32_t lookup00 = lookup_table_[gy * grid_width_ + gx];
                uint32_t lookup01 = lookup_table_[gy * grid_width_ + gx1]; 
                uint32_t lookup10 = lookup_table_[gy1 * grid_width_ + gx];
                uint32_t lookup11 = lookup_table_[gy1 * grid_width_ + gx1];
                
                // Decode coordinates from lookups
                double x00 = (lookup00 % width);
                double y00 = (lookup00 / width);
                double x01 = (lookup01 % width);
                double y01 = (lookup01 / width);
                double x10 = (lookup10 % width);
                double y10 = (lookup10 / width);
                double x11 = (lookup11 % width);
                double y11 = (lookup11 / width);
                
                // Interpolate coordinates
                double interp_x = (x00 * (1.0 - fx) + x01 * fx) * (1.0 - fy) + 
                                 (x10 * (1.0 - fx) + x11 * fx) * fy;
                double interp_y = (y00 * (1.0 - fx) + y01 * fx) * (1.0 - fy) + 
                                 (y10 * (1.0 - fx) + y11 * fx) * fy;
                
                // Sample from interpolated coordinate
                int src_x = (int)(interp_x + 0.5);
                int src_y = (int)(interp_y + 0.5);
                
                if (src_x >= 0 && src_x < width && src_y >= 0 && src_y < height) {
                    output[dest_idx] = input[src_y * width + src_x];
                } else {
                    output[dest_idx] = 0;
                }
            }
        }
    }
}

void CoordinateLookupTable::apply_subpixel_write(uint32_t pixel, uint32_t* output, 
                                               uint32_t base_offset, uint32_t x_partial, uint32_t y_partial,
                                               bool blend) const
{
    // Get base coordinates
    int base_x = base_offset % output_width_;
    int base_y = base_offset / output_width_;
    
    // Convert partials to [0,1] range
    double fx = x_partial / 32.0;
    double fy = y_partial / 32.0;
    
    // Distribute pixel to four corners based on fractional position
    if (base_x < output_width_ - 1 && base_y < output_height_ - 1) {
        uint32_t weights[4] = {
            (uint32_t)((1.0 - fx) * (1.0 - fy) * 256),
            (uint32_t)(fx * (1.0 - fy) * 256),
            (uint32_t)((1.0 - fx) * fy * 256),
            (uint32_t)(fx * fy * 256)
        };
        
        uint32_t offsets[4] = {
            base_offset,
            base_offset + 1,
            base_offset + output_width_,
            base_offset + output_width_ + 1
        };
        
        for (int i = 0; i < 4; i++) {
            if (weights[i] > 0 && offsets[i] < output_width_ * output_height_) {
                uint32_t weighted_pixel = apply_weight(pixel, weights[i]);
                if (blend) {
                    output[offsets[i]] = blend_max(weighted_pixel, output[offsets[i]]);
                } else {
                    output[offsets[i]] = weighted_pixel;
                }
            }
        }
    }
}

uint32_t CoordinateLookupTable::sample_with_interpolation(const uint32_t* input, 
                                                       uint32_t base_offset,
                                                       uint32_t x_partial, 
                                                       uint32_t y_partial) const
{
    // Get base coordinates
    int base_x = base_offset % output_width_;
    int base_y = base_offset / output_width_;
    
    // Sample four neighboring pixels with bounds checking
    uint32_t p11 = input[base_offset];
    uint32_t p21 = (base_x + 1 < output_width_ && base_offset + 1 < output_width_ * output_height_) ? 
                   input[base_offset + 1] : p11;
    uint32_t p12 = (base_y + 1 < output_height_ && base_offset + output_width_ < output_width_ * output_height_) ? 
                   input[base_offset + output_width_] : p11;
    uint32_t p22 = (base_x + 1 < output_width_ && base_y + 1 < output_height_ && 
                   base_offset + output_width_ + 1 < output_width_ * output_height_) ? 
                   input[base_offset + output_width_ + 1] : p11;
    
    // Convert partial coordinates to [0,1] range
    double fx = x_partial / 32.0;
    double fy = y_partial / 32.0;
    
    return interpolate_pixels(p11, p21, p12, p22, fx, fy);
}

uint32_t CoordinateLookupTable::interpolate_pixels(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11, 
                                                  double fx, double fy) const
{
    // Interpolate each color channel
    auto interp_channel = [](uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11,
                           double fx, double fy, int shift) -> uint32_t {
        int c00 = (p00 >> shift) & 0xFF;
        int c01 = (p01 >> shift) & 0xFF;
        int c10 = (p10 >> shift) & 0xFF;
        int c11 = (p11 >> shift) & 0xFF;
        
        double c0 = c00 * (1.0 - fx) + c01 * fx;
        double c1 = c10 * (1.0 - fx) + c11 * fx;
        double result = c0 * (1.0 - fy) + c1 * fy;
        
        return (uint32_t)(result + 0.5) & 0xFF;
    };
    
    uint32_t r = interp_channel(p00, p01, p10, p11, fx, fy, 16);
    uint32_t g = interp_channel(p00, p01, p10, p11, fx, fy, 8);
    uint32_t b = interp_channel(p00, p01, p10, p11, fx, fy, 0);
    uint32_t a = interp_channel(p00, p01, p10, p11, fx, fy, 24);
    
    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t CoordinateLookupTable::apply_weight(uint32_t pixel, uint32_t weight) const
{
    uint32_t r = ((pixel >> 16) & 0xFF) * weight / 256;
    uint32_t g = ((pixel >> 8) & 0xFF) * weight / 256;
    uint32_t b = (pixel & 0xFF) * weight / 256;
    uint32_t a = ((pixel >> 24) & 0xFF) * weight / 256;
    
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

uint32_t CoordinateLookupTable::get_lookup(int x, int y) const
{
    if (x < 0 || x >= output_width_ || y < 0 || y >= output_height_) {
        return 0;
    }
    return lookup_table_[y * output_width_ + x];
}

} // namespace avs