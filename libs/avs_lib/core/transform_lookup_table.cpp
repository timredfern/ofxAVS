#include "transform_lookup_table.h"
#include "script/script_engine.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace avs {

TransformLookupTable::TransformLookupTable()
    : width_(0), height_(0), subpixel_(false), wrap_(false)
{
}

TransformLookupTable::~TransformLookupTable() = default;

void TransformLookupTable::generate(int width, int height,
                                  const std::string& x_expr, const std::string& y_expr,
                                  bool rectangular, bool subpixel,
                                  const AudioData& audio_data, bool wrap)
{
    width_ = width;
    height_ = height;
    subpixel_ = subpixel;
    wrap_ = wrap;
    
    // Allocate lookup table
    lookup_table_.resize(width * height);
    
    if (rectangular) {
        generate_rectangular(x_expr, y_expr, audio_data);
    } else {
        generate_polar(x_expr, y_expr, audio_data);
    }
}

void TransformLookupTable::generate_rectangular(const std::string& x_expr, const std::string& y_expr,
                                               const AudioData& audio_data)
{
    ScriptEngine x_engine, y_engine;
    
    // Set audio context for both engines
    x_engine.set_audio_context(audio_data, false); // We'll update beat per evaluation if needed
    y_engine.set_audio_context(audio_data, false);
    
    double w2 = width_ / 2.0;
    double h2 = height_ / 2.0;
    double x_scale = 1.0 / w2;
    double y_scale = 1.0 / h2;
    
    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            // Convert pixel coordinates to normalized [-1, 1] range
            double norm_x = (x - w2) * x_scale;
            double norm_y = (y - h2) * y_scale;
            
            // Set pixel context for expressions
            x_engine.set_pixel_context(x, y, width_, height_);
            y_engine.set_pixel_context(x, y, width_, height_);
            
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
            
            // Encode lookup value
            int idx = y * width_ + x;
            lookup_table_[idx] = encode_lookup(pixel_x, pixel_y);
        }
    }
}

void TransformLookupTable::generate_polar(const std::string& x_expr, const std::string& y_expr,
                                         const AudioData& audio_data)
{
    ScriptEngine x_engine, y_engine; // x_expr affects 'd', y_expr affects 'r'
    
    // Set audio context
    x_engine.set_audio_context(audio_data, false);
    y_engine.set_audio_context(audio_data, false);
    
    double max_d = std::sqrt((width_ * width_ + height_ * height_)) / 2.0;
    double inv_max_d = 1.0 / max_d;
    double w2 = width_ / 2.0;
    double h2 = height_ / 2.0;
    
    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            // Convert to polar coordinates
            double xd = x - w2;
            double yd = y - h2;
            double d = std::sqrt(xd * xd + yd * yd) * inv_max_d; // normalized distance [0,1]
            double r = std::atan2(yd, xd) + M_PI * 0.5; // angle [0, 2π]
            
            // Set context
            x_engine.set_pixel_context(x, y, width_, height_);
            y_engine.set_pixel_context(x, y, width_, height_);
            
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
            
            // Encode lookup value
            int idx = y * width_ + x;
            lookup_table_[idx] = encode_lookup(pixel_x, pixel_y);
        }
    }
}

void TransformLookupTable::clamp_or_wrap(double& x, double& y) const
{
    if (wrap_) {
        // Wrap coordinates
        if (subpixel_) {
            // For subpixel, wrap to [0, dimension-1]
            x = std::fmod(x, width_ - 1);
            if (x < 0) x += width_ - 1;
            y = std::fmod(y, height_ - 1);
            if (y < 0) y += height_ - 1;
        } else {
            // For non-subpixel, wrap to [0, dimension]
            x = std::fmod(x, width_);
            if (x < 0) x += width_;
            y = std::fmod(y, height_);
            if (y < 0) y += height_;
        }
    } else {
        // Clamp coordinates
        if (subpixel_) {
            x = std::clamp(x, 0.0, (double)(width_ - 1));
            y = std::clamp(y, 0.0, (double)(height_ - 1));
        } else {
            x = std::clamp(x, 0.0, (double)(width_ - 1));
            y = std::clamp(y, 0.0, (double)(height_ - 1));
        }
    }
}

uint32_t TransformLookupTable::encode_lookup(double x, double y) const
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
        if (base_x >= width_ - 1) { base_x = width_ - 2; x_partial = 31; }
        if (base_y >= height_ - 1) { base_y = height_ - 2; y_partial = 31; }
        
        uint32_t base_offset = base_x + base_y * width_;
        return base_offset | (y_partial << 22) | (x_partial << 27);
    } else {
        // Simple integer coordinates
        int ix = (int)(x + 0.5);
        int iy = (int)(y + 0.5);
        
        // Bounds check
        ix = std::clamp(ix, 0, width_ - 1);
        iy = std::clamp(iy, 0, height_ - 1);
        
        return ix + iy * width_;
    }
}

void TransformLookupTable::apply(const uint32_t* input, uint32_t* output,
                               int width, int height, bool blend) const
{
    if (lookup_table_.empty() || width != width_ || height != height_) {
        // Invalid table or size mismatch, copy input to output
        std::copy(input, input + width * height, output);
        return;
    }
    
    const uint32_t OFFSET_MASK = (1 << 22) - 1;
    
    for (int i = 0; i < width * height; i++) {
        uint32_t lookup = lookup_table_[i];
        uint32_t pixel;
        
        if (subpixel_) {
            // Extract subpixel interpolation data
            uint32_t base_offset = lookup & OFFSET_MASK;
            uint32_t x_partial = (lookup >> 27) & 31;
            uint32_t y_partial = (lookup >> 22) & 31;
            
            // Perform bilinear interpolation
            pixel = sample_with_interpolation(input, base_offset, x_partial, y_partial);
        } else {
            // Simple lookup
            pixel = input[lookup];
        }
        
        if (blend) {
            // Simple average blend
            uint32_t existing = output[i];
            uint32_t r = ((pixel & 0xFF0000) + (existing & 0xFF0000)) / 2;
            uint32_t g = ((pixel & 0x00FF00) + (existing & 0x00FF00)) / 2;
            uint32_t b = ((pixel & 0x0000FF) + (existing & 0x0000FF)) / 2;
            uint32_t a = ((pixel & 0xFF000000) + (existing & 0xFF000000)) / 2;
            output[i] = (a & 0xFF000000) | (r & 0xFF0000) | (g & 0x00FF00) | (b & 0x0000FF);
        } else {
            output[i] = pixel;
        }
    }
}

uint32_t TransformLookupTable::sample_with_interpolation(const uint32_t* input, 
                                                       uint32_t base_offset,
                                                       uint32_t x_partial, 
                                                       uint32_t y_partial) const
{
    // Get base coordinates
    int base_x = base_offset % width_;
    int base_y = base_offset / width_;
    
    // Sample four neighboring pixels
    uint32_t p11 = input[base_offset];
    uint32_t p21 = (base_x + 1 < width_) ? input[base_offset + 1] : p11;
    uint32_t p12 = (base_y + 1 < height_) ? input[base_offset + width_] : p11;
    uint32_t p22 = (base_x + 1 < width_ && base_y + 1 < height_) ? 
                   input[base_offset + width_ + 1] : p11;
    
    // Convert partial coordinates to [0,1] range
    double fx = x_partial / 32.0;
    double fy = y_partial / 32.0;
    
    // Interpolate each color channel
    auto interp_channel = [](uint32_t p11, uint32_t p12, uint32_t p21, uint32_t p22,
                           double fx, double fy, int shift) -> uint32_t {
        int c11 = (p11 >> shift) & 0xFF;
        int c12 = (p12 >> shift) & 0xFF;
        int c21 = (p21 >> shift) & 0xFF;
        int c22 = (p22 >> shift) & 0xFF;
        
        double c1 = c11 * (1.0 - fx) + c21 * fx;
        double c2 = c12 * (1.0 - fx) + c22 * fx;
        double result = c1 * (1.0 - fy) + c2 * fy;
        
        return (uint32_t)(result + 0.5) & 0xFF;
    };
    
    uint32_t r = interp_channel(p11, p12, p21, p22, fx, fy, 16);
    uint32_t g = interp_channel(p11, p12, p21, p22, fx, fy, 8);
    uint32_t b = interp_channel(p11, p12, p21, p22, fx, fy, 0);
    uint32_t a = interp_channel(p11, p12, p21, p22, fx, fy, 24);
    
    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t TransformLookupTable::get_lookup(int x, int y) const
{
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return 0;
    }
    return lookup_table_[y * width_ + x];
}

} // namespace avs