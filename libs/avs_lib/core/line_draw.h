// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Line drawing with blend mode and line width support
// Port of linedraw.cpp from original AVS
//
// This header provides the core drawing primitives.
// For the main draw_line and draw_point API, include one of:
//   - line_draw_original.h  (original AVS behavior)
//   - line_draw_ext.h       (extended with AA, styles, etc.)

#pragma once

#include "blend.h"
#include <algorithm>
#include <cstdint>

namespace avs {

// Get current line width from global line blend mode (bits 16-23)
inline int get_line_width() {
    if (!(g_line_blend_mode & 0x80000000)) return 1;
    int lw = (g_line_blend_mode >> 16) & 0xFF;
    return (lw < 1) ? 1 : lw;
}

// Draw a single pixel with current blend mode
inline void draw_pixel(uint32_t* fb, int x, int y, int width, int height, uint32_t color) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        BLEND_LINE(&fb[y * width + x], color);
    }
}

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

// Original AVS draw_point - single pixel
inline void draw_point(uint32_t* fb, int x, int y, int width, int height, uint32_t color) {
    draw_pixel(fb, x, y, width, height, color);
}

// Original AVS draw_line - Bresenham algorithm
inline void draw_line(uint32_t* fb, int x1, int y1, int x2, int y2,
                      int width, int height, uint32_t color) {
    int lw = get_line_width();
    draw_line_bresenham(fb, x1, y1, x2, y2, width, height, color, lw);
}

} // namespace avs
