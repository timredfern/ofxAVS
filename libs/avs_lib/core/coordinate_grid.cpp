// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "coordinate_grid.h"
#include "script/script_engine.h"
#include <cmath>
#include <algorithm>

namespace avs {

CoordinateGrid::CoordinateGrid()
    : grid_width_(0), grid_height_(0), output_width_(0), output_height_(0)
{
}

void CoordinateGrid::resize(int grid_width, int grid_height)
{
    grid_width_ = grid_width;
    grid_height_ = grid_height;
    grid_.resize(grid_width * grid_height, {0, 0});
}

void CoordinateGrid::set(int gx, int gy, double src_x, double src_y)
{
    if (gx < 0 || gx >= grid_width_ || gy < 0 || gy >= grid_height_) {
        return;
    }
    grid_[gy * grid_width_ + gx] = {to_fixed(src_x), to_fixed(src_y)};
}

std::pair<double, double> CoordinateGrid::get(int gx, int gy) const
{
    if (gx < 0 || gx >= grid_width_ || gy < 0 || gy >= grid_height_) {
        return {0.0, 0.0};
    }
    auto [x_fixed, y_fixed] = grid_[gy * grid_width_ + gx];
    return {from_fixed(x_fixed), from_fixed(y_fixed)};
}

std::pair<double, double> CoordinateGrid::sample(double norm_x, double norm_y) const
{
    if (grid_.empty() || grid_width_ < 2 || grid_height_ < 2) {
        return {0.0, 0.0};
    }

    // Convert normalized [0,1] to grid coordinates
    double grid_x = norm_x * (grid_width_ - 1);
    double grid_y = norm_y * (grid_height_ - 1);

    // Get integer grid cell and fractional part
    int gx = static_cast<int>(grid_x);
    int gy = static_cast<int>(grid_y);

    // Handle boundary cases
    if (gx >= grid_width_ - 1) {
        gx = grid_width_ - 2;
        grid_x = grid_width_ - 1;
    }
    if (gy >= grid_height_ - 1) {
        gy = grid_height_ - 2;
        grid_y = grid_height_ - 1;
    }
    gx = std::max(0, gx);
    gy = std::max(0, gy);

    // Fixed-point fractional part (8.8 format for interpolation)
    int fx = static_cast<int>((grid_x - gx) * 256);
    int fy = static_cast<int>((grid_y - gy) * 256);

    // Bilinear interpolation of grid points
    auto [x_fixed, y_fixed] = interpolate_grid(gx, gy, fx, fy);

    return {from_fixed(x_fixed), from_fixed(y_fixed)};
}

std::pair<int32_t, int32_t> CoordinateGrid::interpolate_grid(int gx, int gy, int fx, int fy) const
{
    // Get four surrounding grid points
    auto [x00, y00] = grid_[gy * grid_width_ + gx];
    auto [x01, y01] = grid_[gy * grid_width_ + gx + 1];
    auto [x10, y10] = grid_[(gy + 1) * grid_width_ + gx];
    auto [x11, y11] = grid_[(gy + 1) * grid_width_ + gx + 1];

    int fx_inv = 256 - fx;
    int fy_inv = 256 - fy;

    // Bilinear interpolation (matching original AVS fixed-point math)
    // Top edge: interpolate between (0,0) and (1,0)
    int64_t x_top = (static_cast<int64_t>(x00) * fx_inv + static_cast<int64_t>(x01) * fx) >> 8;
    int64_t y_top = (static_cast<int64_t>(y00) * fx_inv + static_cast<int64_t>(y01) * fx) >> 8;

    // Bottom edge: interpolate between (0,1) and (1,1)
    int64_t x_bot = (static_cast<int64_t>(x10) * fx_inv + static_cast<int64_t>(x11) * fx) >> 8;
    int64_t y_bot = (static_cast<int64_t>(y10) * fx_inv + static_cast<int64_t>(y11) * fx) >> 8;

    // Final: interpolate between top and bottom
    int32_t x_result = static_cast<int32_t>((x_top * fy_inv + x_bot * fy) >> 8);
    int32_t y_result = static_cast<int32_t>((y_top * fy_inv + y_bot * fy) >> 8);

    return {x_result, y_result};
}

void CoordinateGrid::generate(int grid_width, int grid_height,
                              int output_width, int output_height,
                              const std::string& script,
                              CoordMode mode,
                              AudioData audio)
{
    resize(grid_width, grid_height);
    output_width_ = output_width;
    output_height_ = output_height;

    ScriptEngine engine;
    engine.set_audio_context(audio, false);

    // Calculate max distance for polar mode (diagonal/2, matching original AVS)
    double max_d = std::sqrt(static_cast<double>(output_width * output_width +
                                                  output_height * output_height)) * 0.5;
    double inv_max_d = 1.0 / max_d;

    double dw2 = output_width * 0.5;
    double dh2 = output_height * 0.5;

    for (int gy = 0; gy < grid_height; gy++) {
        for (int gx = 0; gx < grid_width; gx++) {
            // Convert grid coordinates to pixel coordinates
            double pixel_x = (gx * (output_width - 1.0)) / (grid_width - 1);
            double pixel_y = (gy * (output_height - 1.0)) / (grid_height - 1);

            // Set pixel context
            engine.set_pixel_context(static_cast<int>(pixel_x), static_cast<int>(pixel_y),
                                     output_width, output_height);

            double src_x, src_y;

            if (mode == CoordMode::RECTANGULAR) {
                // Rectangular mode: x, y in [-1, 1]
                double x = (pixel_x - dw2) * 2.0 / output_width;
                double y = (pixel_y - dh2) * 2.0 / output_height;

                engine.set_variable("x", x);
                engine.set_variable("y", y);

                // Execute script
                engine.evaluate(script);

                // Read back modified values
                double new_x = engine.get_variable("x");
                double new_y = engine.get_variable("y");

                // Handle NaN/inf
                if (!std::isfinite(new_x)) new_x = x;
                if (!std::isfinite(new_y)) new_y = y;

                // Convert back to pixel coordinates
                src_x = (new_x + 1.0) * dw2;
                src_y = (new_y + 1.0) * dh2;

            } else {
                // Polar mode: d (distance), r (angle)
                double centered_x = pixel_x - dw2;
                double centered_y = pixel_y - dh2;

                // Calculate polar coordinates matching original AVS
                double x = centered_x * 2.0 / output_width;
                double y = centered_y * 2.0 / output_height;
                double d = std::sqrt(centered_x * centered_x + centered_y * centered_y) * inv_max_d;
                double r = std::atan2(centered_y, centered_x) + M_PI * 0.5;

                engine.set_variable("x", x);
                engine.set_variable("y", y);
                engine.set_variable("d", d);
                engine.set_variable("r", r);

                // Execute script
                engine.evaluate(script);

                // Read back modified values
                double new_d = engine.get_variable("d");
                double new_r = engine.get_variable("r");

                // Handle NaN/inf
                if (!std::isfinite(new_d)) new_d = d;
                if (!std::isfinite(new_r)) new_r = r;

                // Convert back to cartesian: remove PI/2 offset, scale d back up
                new_r -= M_PI * 0.5;
                src_x = dw2 + std::cos(new_r) * new_d * max_d;
                src_y = dh2 + std::sin(new_r) * new_d * max_d;
            }

            // Store source coordinates
            set(gx, gy, src_x, src_y);
        }
    }
}

void CoordinateGrid::apply(const uint32_t* input, uint32_t* output,
                           int width, int height,
                           bool subpixel, bool wrap, bool blend) const
{
    if (grid_.empty() || grid_width_ < 2 || grid_height_ < 2) {
        std::copy(input, input + width * height, output);
        return;
    }

    // Fixed-point step sizes (16.16 format) matching original AVS
    // xc_dpos = (w<<16)/(XRES-1)
    int32_t xc_dpos = (width << 16) / (grid_width_ - 1);
    int32_t yc_dpos = (height << 16) / (grid_height_ - 1);

    int32_t yc_pos = 0;
    int ly_pos = 0;

    for (int gy = 0; gy < grid_height_ - 1; gy++) {
        yc_pos += yc_dpos;
        // For the last segment, extend all the way to the edge
        int end_y = (gy == grid_height_ - 2) ? height : (yc_pos >> 16);
        int y_seek = end_y - ly_pos;
        if (y_seek <= 0) continue;
        ly_pos = end_y;

        // Build interpolation table for this grid row
        // Each entry: [x_src, y_src, dx_per_row, dy_per_row]
        std::vector<int32_t> interp_x(grid_width_);
        std::vector<int32_t> interp_y(grid_width_);
        std::vector<int32_t> interp_dx(grid_width_);
        std::vector<int32_t> interp_dy(grid_width_);

        for (int gx = 0; gx < grid_width_; gx++) {
            auto [x0, y0] = grid_[gy * grid_width_ + gx];
            auto [x1, y1] = grid_[(gy + 1) * grid_width_ + gx];
            interp_x[gx] = x0;
            interp_y[gx] = y0;
            interp_dx[gx] = (x1 - x0) / y_seek;
            interp_dy[gx] = (y1 - y0) / y_seek;
        }

        // Process each scanline in this grid row segment
        for (int row = 0; row < y_seek; row++) {
            int dest_y = ly_pos - y_seek + row;
            if (dest_y < 0 || dest_y >= height) {
                // Still need to advance interpolation
                for (int gx = 0; gx < grid_width_; gx++) {
                    interp_x[gx] += interp_dx[gx];
                    interp_y[gx] += interp_dy[gx];
                }
                continue;
            }

            int32_t xc_pos = 0;
            int lx_pos = 0;

            for (int gx = 0; gx < grid_width_ - 1; gx++) {
                xc_pos += xc_dpos;
                // For the last segment, extend all the way to the edge
                int end_x = (gx == grid_width_ - 2) ? width : (xc_pos >> 16);
                int x_seek = end_x - lx_pos;
                if (x_seek <= 0) continue;
                lx_pos = end_x;

                // Get source coords at left edge of this segment
                int32_t xp = interp_x[gx];
                int32_t yp = interp_y[gx];

                // Calculate per-pixel delta across this segment
                int32_t d_x = (interp_x[gx + 1] - xp) / x_seek;
                int32_t d_y = (interp_y[gx + 1] - yp) / x_seek;

                // Process each pixel in this segment
                for (int col = 0; col < x_seek; col++) {
                    int dest_x = lx_pos - x_seek + col;
                    if (dest_x >= 0 && dest_x < width) {
                        uint32_t pixel = sample_pixel(input, width, height, xp, yp, subpixel, wrap);
                        int dest_idx = dest_y * width + dest_x;
                        if (blend) {
                            output[dest_idx] = blend_max(pixel, output[dest_idx]);
                        } else {
                            output[dest_idx] = pixel;
                        }
                    }
                    xp += d_x;
                    yp += d_y;
                }
            }

            // Advance Y interpolation for next scanline
            for (int gx = 0; gx < grid_width_; gx++) {
                interp_x[gx] += interp_dx[gx];
                interp_y[gx] += interp_dy[gx];
            }
        }
    }
}

uint32_t CoordinateGrid::sample_pixel(const uint32_t* input, int width, int height,
                                       int32_t x_fixed, int32_t y_fixed,
                                       bool subpixel, bool wrap) const
{
    // Maximum coordinate values in 16.16 fixed-point
    // Use (width-1) for subpixel mode to allow interpolation at edges
    int32_t w_max = (width - 1) << 16;
    int32_t h_max = (height - 1) << 16;

    // Handle wrap or clamp
    if (wrap) {
        // Wrap coordinates using width/height (not width-1)
        int32_t w_wrap = width << 16;
        int32_t h_wrap = height << 16;

        // Use modulo-style wrapping
        if (x_fixed < 0) {
            x_fixed = w_wrap - ((-x_fixed) % w_wrap);
            if (x_fixed >= w_wrap) x_fixed = 0;
        } else if (x_fixed >= w_wrap) {
            x_fixed = x_fixed % w_wrap;
        }

        if (y_fixed < 0) {
            y_fixed = h_wrap - ((-y_fixed) % h_wrap);
            if (y_fixed >= h_wrap) y_fixed = 0;
        } else if (y_fixed >= h_wrap) {
            y_fixed = y_fixed % h_wrap;
        }
    } else {
        // Clamp coordinates
        x_fixed = std::clamp(x_fixed, 0, w_max);
        y_fixed = std::clamp(y_fixed, 0, h_max);
    }

    if (subpixel) {
        // Bilinear sampling from source image
        int x0 = x_fixed >> 16;
        int y0 = y_fixed >> 16;
        int fx = (x_fixed >> 8) & 0xFF;  // 8-bit fractional part
        int fy = (y_fixed >> 8) & 0xFF;

        // Clamp to valid range for bilinear (need x0+1, y0+1 to be valid)
        if (x0 >= width - 1) {
            x0 = width - 2;
            fx = 255;
        }
        if (y0 >= height - 1) {
            y0 = height - 2;
            fy = 255;
        }
        if (x0 < 0) {
            x0 = 0;
            fx = 0;
        }
        if (y0 < 0) {
            y0 = 0;
            fy = 0;
        }

        uint32_t p00 = input[y0 * width + x0];
        uint32_t p01 = input[y0 * width + x0 + 1];
        uint32_t p10 = input[(y0 + 1) * width + x0];
        uint32_t p11 = input[(y0 + 1) * width + x0 + 1];

        return blend_pixels(p00, p01, p10, p11, fx, fy);
    } else {
        // Nearest neighbor sampling
        int x = (x_fixed + 0x8000) >> 16;  // Round to nearest
        int y = (y_fixed + 0x8000) >> 16;

        x = std::clamp(x, 0, width - 1);
        y = std::clamp(y, 0, height - 1);

        return input[y * width + x];
    }
}

uint32_t CoordinateGrid::blend_pixels(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11,
                                       int fx, int fy) const
{
    int fx_inv = 256 - fx;
    int fy_inv = 256 - fy;

    auto interp_channel = [fx, fy, fx_inv, fy_inv](uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11, int shift) -> uint32_t {
        int c00 = (p00 >> shift) & 0xFF;
        int c01 = (p01 >> shift) & 0xFF;
        int c10 = (p10 >> shift) & 0xFF;
        int c11 = (p11 >> shift) & 0xFF;

        int c0 = (c00 * fx_inv + c01 * fx) >> 8;
        int c1 = (c10 * fx_inv + c11 * fx) >> 8;
        int result = (c0 * fy_inv + c1 * fy) >> 8;

        return result & 0xFF;
    };

    uint32_t a = interp_channel(p00, p01, p10, p11, 24);
    uint32_t r = interp_channel(p00, p01, p10, p11, 16);
    uint32_t g = interp_channel(p00, p01, p10, p11, 8);
    uint32_t b = interp_channel(p00, p01, p10, p11, 0);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t CoordinateGrid::blend_max(uint32_t a, uint32_t b) const
{
    uint32_t r = std::max((a >> 16) & 0xFF, (b >> 16) & 0xFF);
    uint32_t g = std::max((a >> 8) & 0xFF, (b >> 8) & 0xFF);
    uint32_t blue = std::max(a & 0xFF, b & 0xFF);
    uint32_t alpha = std::max((a >> 24) & 0xFF, (b >> 24) & 0xFF);

    return (alpha << 24) | (r << 16) | (g << 8) | blue;
}

} // namespace avs
