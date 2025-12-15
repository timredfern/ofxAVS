#include "coordinate_lookup_table.h"
#include "script/script_engine.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace avs {

CoordinateLookupTable::CoordinateLookupTable()
    : output_width_(0), output_height_(0), grid_width_(0), grid_height_(0), subpixel_(false), wrap_(false), interp_mode_(InterpolationMode::LINEAR)
{
}

CoordinateLookupTable::~CoordinateLookupTable() = default;

void CoordinateLookupTable::generate(int width, int height, int grid_width, int grid_height,
                                  const std::string& x_expr, const std::string& y_expr,
                                  bool rectangular, bool subpixel,
                                  const AudioData& audio_data, bool wrap,
                                  InterpolationMode interp_mode)
{
    output_width_ = width;
    output_height_ = height;
    grid_width_ = grid_width;
    grid_height_ = grid_height;
    subpixel_ = subpixel;
    wrap_ = wrap;
    interp_mode_ = interp_mode;
    
    // Allocate lookup table for grid (not full resolution)
    lookup_table_.resize(grid_width * grid_height);
    
    if (rectangular) {
        generate_rectangular(x_expr, y_expr, audio_data);
    } else {
        generate_polar(x_expr, y_expr, audio_data);
    }
}

void CoordinateLookupTable::generate_rectangular(const std::string& x_expr, const std::string& y_expr,
                                               const AudioData& audio_data)
{
    ScriptEngine x_engine, y_engine;
    
    // Set audio context for both engines
    x_engine.set_audio_context(audio_data, false);
    y_engine.set_audio_context(audio_data, false);
    
    double w2 = output_width_ / 2.0;
    double h2 = output_height_ / 2.0;
    double x_scale = 1.0 / w2;
    double y_scale = 1.0 / h2;
    
    // Generate lookup table at grid points, not every pixel
    for (int gy = 0; gy < grid_height_; gy++) {
        for (int gx = 0; gx < grid_width_; gx++) {
            // Map grid position to output pixel position
            double px = (gx * output_width_) / (double)grid_width_;
            double py = (gy * output_height_) / (double)grid_height_;
            
            // Convert pixel coordinates to normalized [-1, 1] range
            double norm_x = (px - w2) * x_scale;
            double norm_y = (py - h2) * y_scale;
            
            // Set pixel context for expressions (using actual pixel position)
            x_engine.set_pixel_context((int)px, (int)py, output_width_, output_height_);
            y_engine.set_pixel_context((int)px, (int)py, output_width_, output_height_);
            
            // Also set normalized coordinates as variables
            x_engine.set_variable("x", norm_x);
            x_engine.set_variable("y", norm_y);
            y_engine.set_variable("x", norm_x);
            y_engine.set_variable("y", norm_y);
            
            // Evaluate transformation expressions
            double new_x = x_engine.evaluate(x_expr);
            double new_y = y_engine.evaluate(y_expr);
            
            // Handle invalid results (NaN, inf)
            if (!std::isfinite(new_x)) new_x = norm_x;
            if (!std::isfinite(new_y)) new_y = norm_y;
            
            // Convert back to pixel coordinates
            double pixel_x = (new_x + 1.0) * w2;
            double pixel_y = (new_y + 1.0) * h2;
            
            // Apply clamping or wrapping
            clamp_or_wrap(pixel_x, pixel_y);
            
            // Store in grid lookup table
            int grid_idx = gy * grid_width_ + gx;
            lookup_table_[grid_idx] = encode_lookup(pixel_x, pixel_y);
        }
    }
}

void CoordinateLookupTable::generate_polar(const std::string& x_expr, const std::string& y_expr,
                                         const AudioData& audio_data)
{
    ScriptEngine x_engine, y_engine; // x_expr affects 'd', y_expr affects 'r'
    
    // Set audio context
    x_engine.set_audio_context(audio_data, false);
    y_engine.set_audio_context(audio_data, false);
    
    double max_d = std::sqrt((output_width_ * output_width_ + output_height_ * output_height_)) / 2.0;
    double inv_max_d = 1.0 / max_d;
    double w2 = output_width_ / 2.0;
    double h2 = output_height_ / 2.0;
    
    // Generate lookup table at grid points
    for (int gy = 0; gy < grid_height_; gy++) {
        for (int gx = 0; gx < grid_width_; gx++) {
            // Map grid position to output pixel position
            double px = (gx * output_width_) / (double)grid_width_;
            double py = (gy * output_height_) / (double)grid_height_;
            // Convert to polar coordinates
            double xd = px - w2;
            double yd = py - h2;
            double d = std::sqrt(xd * xd + yd * yd) * inv_max_d; // normalized distance [0,1]
            double r = std::atan2(yd, xd) + M_PI * 0.5; // angle [0, 2π]
            
            // Set context
            x_engine.set_pixel_context((int)px, (int)py, output_width_, output_height_);
            y_engine.set_pixel_context((int)px, (int)py, output_width_, output_height_);
            
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
            
            double pixel_x = w2 + std::cos(new_r) * new_d;
            double pixel_y = h2 + std::sin(new_r) * new_d;
            
            // Apply clamping or wrapping
            clamp_or_wrap(pixel_x, pixel_y);
            
            // Store in grid lookup table
            int grid_idx = gy * grid_width_ + gx;
            lookup_table_[grid_idx] = encode_lookup(pixel_x, pixel_y);
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
        // Encode with subpixel interpolation data
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
        std::copy(input, input + width * height, output);
        return;
    }
    
    const uint32_t OFFSET_MASK = (1 << 22) - 1;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pixel_idx = y * width + x;
            
            // Map pixel position to grid position
            double grid_x = (x * grid_width_) / (double)width;
            double grid_y = (y * grid_height_) / (double)height;
            
            // Get grid indices and fractional parts
            int gx0 = (int)grid_x;
            int gy0 = (int)grid_y;
            int gx1 = std::min(gx0 + 1, grid_width_ - 1);
            int gy1 = std::min(gy0 + 1, grid_height_ - 1);
            
            double fx = grid_x - gx0;
            double fy = grid_y - gy0;
            
            // Get lookup values from grid corners
            uint32_t lookup00 = lookup_table_[gy0 * grid_width_ + gx0];
            uint32_t lookup01 = lookup_table_[gy0 * grid_width_ + gx1];
            uint32_t lookup10 = lookup_table_[gy1 * grid_width_ + gx0];
            uint32_t lookup11 = lookup_table_[gy1 * grid_width_ + gx1];
            
            uint32_t pixel;
            
            if (subpixel_) {
                // For subpixel mode, each lookup contains interpolation data
                uint32_t base_offset = lookup00 & OFFSET_MASK;
                uint32_t x_partial = (lookup00 >> 27) & 31;
                uint32_t y_partial = (lookup00 >> 22) & 31;
                pixel = sample_with_interpolation(input, base_offset, x_partial, y_partial);
            } else {
                // Apply grid interpolation based on mode
                switch (interp_mode_) {
                    case InterpolationMode::NONE:
                        // No interpolation - use nearest grid point (classic stepped look)
                        pixel = input[lookup00];
                        break;
                        
                    case InterpolationMode::NEAREST:
                        // Nearest neighbor within grid
                        {
                            uint32_t lookup = (fx < 0.5 && fy < 0.5) ? lookup00 :
                                            (fx >= 0.5 && fy < 0.5) ? lookup01 :
                                            (fx < 0.5 && fy >= 0.5) ? lookup10 : lookup11;
                            pixel = input[lookup];
                        }
                        break;
                        
                    case InterpolationMode::LINEAR:
                    default:
                        // Bilinear interpolation between grid lookups for smooth results
                        {
                            uint32_t p00 = input[lookup00];
                            uint32_t p01 = input[lookup01]; 
                            uint32_t p10 = input[lookup10];
                            uint32_t p11 = input[lookup11];
                            pixel = interpolate_pixels(p00, p01, p10, p11, fx, fy);
                        }
                        break;
                }
            }
            
            if (blend) {
                // Simple average blend
                uint32_t existing = output[pixel_idx];
                uint32_t r = ((pixel & 0xFF0000) + (existing & 0xFF0000)) / 2;
                uint32_t g = ((pixel & 0x00FF00) + (existing & 0x00FF00)) / 2;
                uint32_t b = ((pixel & 0x0000FF) + (existing & 0x0000FF)) / 2;
                uint32_t a = ((pixel & 0xFF000000) + (existing & 0xFF000000)) / 2;
                output[pixel_idx] = (a & 0xFF000000) | (r & 0xFF0000) | (g & 0x00FF00) | (b & 0x0000FF);
            } else {
                output[pixel_idx] = pixel;
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
    
    // Sample four neighboring pixels
    uint32_t p11 = input[base_offset];
    uint32_t p21 = (base_x + 1 < output_width_) ? input[base_offset + 1] : p11;
    uint32_t p12 = (base_y + 1 < output_height_) ? input[base_offset + output_width_] : p11;
    uint32_t p22 = (base_x + 1 < output_width_ && base_y + 1 < output_height_) ? 
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

uint32_t CoordinateLookupTable::get_lookup(int x, int y) const
{
    if (x < 0 || x >= grid_width_ || y < 0 || y >= grid_height_) {
        return 0;
    }
    return lookup_table_[y * grid_width_ + x];
}

} // namespace avs