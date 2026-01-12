// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Line drawing with blend mode and line width support
// Extensions (controlled by AVS_EXTENSION_ANTIALIASED_LINES):
//   - Wu's anti-aliased algorithm
//   - Angle-corrected thickness
//   - Rounded endpoints

#pragma once

#include "avs_config.h"
#include "blend.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace avs {

// Get current line width from global line blend mode (bits 16-23)
inline int get_line_width() {
    if (!(g_line_blend_mode & 0x80000000)) return 1;
    int lw = (g_line_blend_mode >> 16) & 0xFF;
    return (lw < 1) ? 1 : lw;
}

#ifdef AVS_LINE_DRAWING_EXTENSIONS
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
#endif // AVS_LINE_DRAWING_EXTENSIONS

// Draw a single point with current blend mode
inline void draw_point(uint32_t* fb, int x, int y, int width, int height, uint32_t color) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        BLEND_LINE(&fb[y * width + x], color);
    }
}

#ifdef AVS_LINE_DRAWING_EXTENSIONS
// Draw a filled circle (for rounded endpoints)
inline void draw_filled_circle(uint32_t* fb, int cx, int cy, int radius,
                                int width, int height, uint32_t color) {
    if (radius <= 0) {
        draw_point(fb, cx, cy, width, height, color);
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
#endif // AVS_LINE_DRAWING_EXTENSIONS

// Original Bresenham-style thick line (from AVS linedraw.cpp)
inline void draw_line_bresenham(uint32_t* fb, int x1, int y1, int x2, int y2,
                                 int width, int height, uint32_t color, int lw) {
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);

    if (lw < 1) lw = 1;
    else if (lw > 255) lw = 255;

    int lw2 = lw / 2;

    // Optimize vertical lines
    if (dx == 0) {
        x1 -= lw2;
        if (x1 + lw >= 0 && x1 < width) {
            int d = std::max(std::min(y1, y2), 0);
            int ye = std::min(std::max(y1, y2), height - 1);
            if (x1 < 0) { lw += x1; x1 = 0; }
            if (x1 + lw >= width) lw = width - x1;
            uint32_t* p = fb + d * width + x1;
            int stride = width - lw;
            if (lw > 0) {
                while (d++ < ye) {
                    for (int x = 0; x < lw; x++) { BLEND_LINE(p, color); p++; }
                    p += stride;
                }
            }
        }
        return;
    }

    // Optimize horizontal lines
    if (y1 == y2) {
        y1 -= lw2;
        if (y1 + lw >= 0 && y1 < height) {
            int d = std::max(std::min(x1, x2), 0);
            int xe = std::min(std::max(x1, x2), width - 1);
            if (y1 < 0) { lw += y1; y1 = 0; }
            if (y1 + lw >= height) lw = height - y1;
            uint32_t* p = fb + y1 * width + d;
            int stride = width - (xe - d);
            for (int y = 0; y < lw; y++) {
                for (int lt = d; lt < xe; lt++) { BLEND_LINE(p, color); p++; }
                p += stride;
            }
        }
        return;
    }

    // General case: Bresenham with thickness
    if (dy <= dx) {
        if (x2 < x1) { std::swap(x1, x2); std::swap(y1, y2); }
        int yincr = (y2 > y1) ? 1 : -1;
        int offsincr = (y2 > y1) ? width : -width;
        y1 -= lw2;
        int offs = y1 * width + x1;
        int d = dy + dy - dx;
        int Eincr = dy + dy;
        int NEincr = d - dx;

        if (x2 >= 0 && x1 < width) {
            if (x1 < 0) {
                int v = yincr * -x1;
                if (dx) v = (v * dy) / dx;
                y1 += v; offs += v * width - x1; x1 = 0;
            }
            if (x2 > width) x2 = width;
            while (x1 < x2) {
                int yp = y1, ype = y1 + lw;
                uint32_t* newfb = fb + offs;
                if (yp < 0) { newfb -= yp * width; yp = 0; }
                if (ype > height) ype = height;
                while (yp++ < ype) { BLEND_LINE(newfb, color); newfb += width; }
                if (d < 0) d += Eincr;
                else { d += NEincr; y1 += yincr; offs += offsincr; }
                offs++; x1++;
            }
        }
    } else {
        if (y2 < y1) { std::swap(x1, x2); std::swap(y1, y2); }
        int xincr = (x2 > x1) ? 1 : -1;
        int d = dx + dx - dy;
        int Eincr = dx + dx;
        int NEincr = d - dy;
        x1 -= lw2;
        int offs = y1 * width + x1;

        if (y2 >= 0 && y1 < height) {
            if (y1 < 0) {
                int v = xincr * -y1;
                if (dy) v = (v * dx) / dy;
                x1 += v; offs += v - y1 * width; y1 = 0;
            }
            if (y2 > height) y2 = height;
            while (y1 < y2) {
                int xp = x1, xpe = x1 + lw;
                uint32_t* newfb = fb + offs;
                if (xp < 0) { newfb -= xp; xp = 0; }
                if (xpe > width) xpe = width;
                while (xp++ < xpe) { BLEND_LINE(newfb, color); newfb++; }
                if (d < 0) d += Eincr;
                else { d += NEincr; x1 += xincr; offs += xincr; }
                offs += width; y1++;
            }
        }
    }
}

// Main line drawing function - dispatches based on style flags
inline void draw_line(uint32_t* fb, int x1, int y1, int x2, int y2,
                      int width, int height, uint32_t color) {
    int lw = get_line_width();

#ifdef AVS_LINE_DRAWING_EXTENSIONS
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
            // Angle from horizontal
            float cos_angle = std::abs(dx) / len;
            float sin_angle = std::abs(dy) / len;
            // Use the larger component to avoid division by near-zero
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
#else
    // Original AVS behavior - just Bresenham
    draw_line_bresenham(fb, x1, y1, x2, y2, width, height, color, lw);
#endif
}

} // namespace avs
