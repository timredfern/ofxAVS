// avs_lib - Portable Advanced Visualization Studio library
// Extended line drawing - NOT part of original AVS
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Extensions to original AVS line drawing:
//   - Wu's anti-aliased algorithm
//   - Angle-corrected thickness
//   - Rounded endpoints
//   - Variable-size points (circles/squares)

#pragma once

#include "line_draw.h"
#include "blend_ext.h"
#include <cmath>

namespace avs {

// Blend a pixel with alpha (0-255) for anti-aliased drawing
inline void blend_pixel_alpha(uint32_t* fb, uint32_t color, int alpha) {
    if (alpha <= 0) return;
    if (alpha >= 255) {
        BLEND_LINE(fb, color);
        return;
    }
    // Blend color with existing pixel using alpha
    uint32_t blended = BLEND_ADJ(color, *fb, alpha);
    BLEND_LINE(fb, blended);
}

// Draw a filled circle (for rounded endpoints and points)
inline void draw_filled_circle(uint32_t* fb, int cx, int cy, int radius,
                                int width, int height, uint32_t color) {
    if (radius <= 0) {
        draw_pixel(fb, cx, cy, width, height, color);
        return;
    }

    for (int y = -radius; y <= radius; y++) {
        int py = cy + y;
        if (py < 0 || py >= height) continue;

        int dx = static_cast<int>(std::sqrt(radius * radius - y * y));
        int x1 = std::max(0, cx - dx);
        int x2 = std::min(width - 1, cx + dx);

        uint32_t* p = fb + py * width + x1;
        for (int x = x1; x <= x2; x++) {
            BLEND_LINE(p, color);
            p++;
        }
    }
}

// Draw a filled square (for block-style points)
inline void draw_filled_square(uint32_t* fb, int cx, int cy, int size,
                                int width, int height, uint32_t color) {
    int half = size / 2;
    int y1 = std::max(0, cy - half);
    int y2 = std::min(height - 1, cy + half);
    int x1 = std::max(0, cx - half);
    int x2 = std::min(width - 1, cx + half);

    for (int py = y1; py <= y2; py++) {
        uint32_t* p = fb + py * width + x1;
        for (int px = x1; px <= x2; px++) {
            BLEND_LINE(p, color);
            p++;
        }
    }
}

// Draw an anti-aliased filled circle
inline void draw_aa_circle(uint32_t* fb, int cx, int cy, int radius,
                           int width, int height, uint32_t color) {
    if (radius <= 0) {
        draw_pixel(fb, cx, cy, width, height, color);
        return;
    }

    float r = static_cast<float>(radius) + 0.5f;
    float r2 = r * r;

    for (int y = -radius - 1; y <= radius + 1; y++) {
        int py = cy + y;
        if (py < 0 || py >= height) continue;

        for (int x = -radius - 1; x <= radius + 1; x++) {
            int px = cx + x;
            if (px < 0 || px >= width) continue;

            float dist2 = static_cast<float>(x * x + y * y);
            if (dist2 <= r2) {
                float dist = std::sqrt(dist2);
                float edge_dist = r - dist;
                if (edge_dist >= 1.0f) {
                    // Fully inside
                    BLEND_LINE(&fb[py * width + px], color);
                } else if (edge_dist > 0.0f) {
                    // Anti-aliased edge
                    blend_pixel_alpha(&fb[py * width + px], color,
                                     static_cast<int>(edge_dist * 255.0f));
                }
            }
        }
    }
}

// Wu's anti-aliased line algorithm (single pixel width)
// Based on: Xiaolin Wu, "An Efficient Antialiasing Technique",
// Computer Graphics, July 1991
inline void draw_line_wu(uint32_t* fb, int x0, int y0, int x1, int y1,
                          int width, int height, uint32_t color) {
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);

    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    float dx = static_cast<float>(x1 - x0);
    float dy = static_cast<float>(y1 - y0);
    float gradient = (dx == 0.0f) ? 1.0f : dy / dx;

    // Handle first endpoint
    float xend = static_cast<float>(x0);
    float yend = y0 + gradient * (xend - x0);
    float xgap = 1.0f - (x0 + 0.5f - std::floor(x0 + 0.5f));
    int xpxl1 = static_cast<int>(xend);
    int ypxl1 = static_cast<int>(std::floor(yend));

    if (steep) {
        if (xpxl1 >= 0 && xpxl1 < height && ypxl1 >= 0 && ypxl1 < width)
            blend_pixel_alpha(&fb[xpxl1 * width + ypxl1], color,
                             static_cast<int>((1.0f - (yend - ypxl1)) * xgap * 255));
        if (xpxl1 >= 0 && xpxl1 < height && ypxl1 + 1 >= 0 && ypxl1 + 1 < width)
            blend_pixel_alpha(&fb[xpxl1 * width + ypxl1 + 1], color,
                             static_cast<int>((yend - ypxl1) * xgap * 255));
    } else {
        if (ypxl1 >= 0 && ypxl1 < height && xpxl1 >= 0 && xpxl1 < width)
            blend_pixel_alpha(&fb[ypxl1 * width + xpxl1], color,
                             static_cast<int>((1.0f - (yend - ypxl1)) * xgap * 255));
        if (ypxl1 + 1 >= 0 && ypxl1 + 1 < height && xpxl1 >= 0 && xpxl1 < width)
            blend_pixel_alpha(&fb[(ypxl1 + 1) * width + xpxl1], color,
                             static_cast<int>((yend - ypxl1) * xgap * 255));
    }

    float intery = yend + gradient;

    // Handle second endpoint
    xend = static_cast<float>(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = x1 + 0.5f - std::floor(x1 + 0.5f);
    int xpxl2 = static_cast<int>(xend);
    int ypxl2 = static_cast<int>(std::floor(yend));

    if (steep) {
        if (xpxl2 >= 0 && xpxl2 < height && ypxl2 >= 0 && ypxl2 < width)
            blend_pixel_alpha(&fb[xpxl2 * width + ypxl2], color,
                             static_cast<int>((1.0f - (yend - ypxl2)) * xgap * 255));
        if (xpxl2 >= 0 && xpxl2 < height && ypxl2 + 1 >= 0 && ypxl2 + 1 < width)
            blend_pixel_alpha(&fb[xpxl2 * width + ypxl2 + 1], color,
                             static_cast<int>((yend - ypxl2) * xgap * 255));
    } else {
        if (ypxl2 >= 0 && ypxl2 < height && xpxl2 >= 0 && xpxl2 < width)
            blend_pixel_alpha(&fb[ypxl2 * width + xpxl2], color,
                             static_cast<int>((1.0f - (yend - ypxl2)) * xgap * 255));
        if (ypxl2 + 1 >= 0 && ypxl2 + 1 < height && xpxl2 >= 0 && xpxl2 < width)
            blend_pixel_alpha(&fb[(ypxl2 + 1) * width + xpxl2], color,
                             static_cast<int>((yend - ypxl2) * xgap * 255));
    }

    // Main loop
    for (int x = xpxl1 + 1; x < xpxl2; x++) {
        int ipart = static_cast<int>(std::floor(intery));
        float fpart = intery - ipart;

        if (steep) {
            if (x >= 0 && x < height && ipart >= 0 && ipart < width)
                blend_pixel_alpha(&fb[x * width + ipart], color,
                                 static_cast<int>((1.0f - fpart) * 255));
            if (x >= 0 && x < height && ipart + 1 >= 0 && ipart + 1 < width)
                blend_pixel_alpha(&fb[x * width + ipart + 1], color,
                                 static_cast<int>(fpart * 255));
        } else {
            if (ipart >= 0 && ipart < height && x >= 0 && x < width)
                blend_pixel_alpha(&fb[ipart * width + x], color,
                                 static_cast<int>((1.0f - fpart) * 255));
            if (ipart + 1 >= 0 && ipart + 1 < height && x >= 0 && x < width)
                blend_pixel_alpha(&fb[(ipart + 1) * width + x], color,
                                 static_cast<int>(fpart * 255));
        }
        intery += gradient;
    }
}

// Thick anti-aliased line using Wu's algorithm with perpendicular coverage
inline void draw_line_wu_thick(uint32_t* fb, int x0, int y0, int x1, int y1,
                                int width, int height, uint32_t color, float thickness) {
    float dx = static_cast<float>(x1 - x0);
    float dy = static_cast<float>(y1 - y0);
    float len = std::sqrt(dx * dx + dy * dy);

    if (len < 0.001f) {
        draw_filled_circle(fb, x0, y0, static_cast<int>(thickness / 2), width, height, color);
        return;
    }

    // Perpendicular unit vector
    float px = -dy / len;
    float py = dx / len;
    float half_w = thickness / 2.0f;

    // Draw multiple parallel Wu lines to create thickness
    int num_lines = static_cast<int>(thickness) + 1;
    for (int i = 0; i < num_lines; i++) {
        float offset = -half_w + (thickness * i) / (num_lines - 1 + 0.001f);
        int ox = static_cast<int>(offset * px);
        int oy = static_cast<int>(offset * py);
        draw_line_wu(fb, x0 + ox, y0 + oy, x1 + ox, y1 + oy, width, height, color);
    }
}

// Extended draw_point - with style-based rendering
// Use this instead of draw_point() when style support is needed
inline void draw_point_styled(uint32_t* fb, int x, int y, int width, int height, uint32_t color) {
    int style = get_line_style();
    if (style & LINE_STYLE_POINTSIZE) {
        int size = get_line_width();
        if (size > 1) {
            bool rounded = (style & LINE_STYLE_ROUNDED) != 0;
            bool aa = (style & LINE_STYLE_AA) != 0;
            int radius = (size - 1) / 2;

            if (rounded) {
                if (aa) {
                    draw_aa_circle(fb, x, y, radius, width, height, color);
                } else {
                    draw_filled_circle(fb, x, y, radius, width, height, color);
                }
            } else {
                draw_filled_square(fb, x, y, size, width, height, color);
            }
            return;
        }
    }
    draw_pixel(fb, x, y, width, height, color);
}

// Extended draw_line - with style-based rendering
// Use this instead of draw_line() when style support is needed
inline void draw_line_styled(uint32_t* fb, int x1, int y1, int x2, int y2,
                             int width, int height, uint32_t color) {
    int lw = get_line_width();
    int style = get_line_style();

    bool aa = (style & LINE_STYLE_AA) != 0;
    bool angle_correct = (style & LINE_STYLE_ANGLE_CORRECT) != 0;
    bool rounded = (style & LINE_STYLE_ROUNDED) != 0;

    // Calculate angle-corrected thickness if needed
    float effective_width = static_cast<float>(lw);
    if (angle_correct && lw > 1) {
        float dx = static_cast<float>(x2 - x1);
        float dy = static_cast<float>(y2 - y1);
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.001f) {
            float cos_angle = std::abs(dx) / len;
            float sin_angle = std::abs(dy) / len;
            float correction = std::max(cos_angle, sin_angle);
            if (correction > 0.001f) {
                effective_width = lw / correction;
            }
        }
    }

    // Draw rounded endpoints if requested
    if (rounded && lw > 1) {
        int radius = lw / 2;
        draw_filled_circle(fb, x1, y1, radius, width, height, color);
        draw_filled_circle(fb, x2, y2, radius, width, height, color);
    }

    // Choose drawing algorithm
    if (aa) {
        if (lw <= 1) {
            draw_line_wu(fb, x1, y1, x2, y2, width, height, color);
        } else {
            draw_line_wu_thick(fb, x1, y1, x2, y2, width, height, color, effective_width);
        }
    } else {
        draw_line_bresenham(fb, x1, y1, x2, y2, width, height, color,
                            static_cast<int>(effective_width + 0.5f));
    }
}

} // namespace avs
