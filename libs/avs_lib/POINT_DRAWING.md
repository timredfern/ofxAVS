# Point Drawing

## AVS Extension

**This is an extension to AVS, not part of the original Winamp AVS.**

The original AVS only supported single-pixel points. Points were drawn as individual pixels with no size, anti-aliasing, or shape options.

This extension adds:
- Variable point sizes via the `LINE_STYLE_POINTSIZE` flag
- Anti-aliased circles when `LINE_STYLE_AA` and `LINE_STYLE_ROUNDED` are set
- Square points when `LINE_STYLE_ROUNDED` is not set
- Controlled via Set Render Mode effect

The feature is gated behind `AVS_LINE_DRAWING_EXTENSIONS` compile flag to maintain compatibility with original AVS behavior when disabled.

---

## Current Implementation (Implemented)

Located in `core/line_draw.h`:

### `draw_point()`
Entry point that checks style flags and dispatches to appropriate drawing function:
- No flags or size=1: single pixel via `draw_pixel()`
- `LINE_STYLE_POINTSIZE` + `LINE_STYLE_ROUNDED`: circle via `draw_filled_circle()` or `draw_aa_circle()`
- `LINE_STYLE_POINTSIZE` without rounded: square via `draw_filled_square()`

### `draw_filled_circle()`
Simple filled circle using horizontal scanlines. No anti-aliasing.

### `draw_filled_square()`
Axis-aligned filled rectangle centered on the point.

### `draw_aa_circle()`
Anti-aliased circle using naive per-pixel distance calculation:
- Iterates every pixel in the bounding box
- Calculates distance from center using `sqrt()`
- Applies alpha based on distance from edge

**Limitation:** This is slow for large numbers of points due to per-pixel sqrt and iteration over entire bounding box.

---

## Future Plan: Alpha Mask Lookup Tables

Replace runtime circle calculation with pre-computed alpha masks for faster rendering.

### Concept

Pre-compute AA circle patterns as alpha masks. Each pixel stores an alpha value (0-255):
- Center pixels: 255 (fully opaque)
- Edge pixels: partial alpha (the AA fringe)
- Outside: 0 (transparent)

### Data Structure

```cpp
struct PointSprite {
    int width, height;
    int origin_x, origin_y;  // hotspot offset (center of circle)
    std::vector<uint8_t> alpha;  // width * height alpha values
};
```

### Rendering

```cpp
for each pixel in sprite:
    if (alpha > 0)
        blend_pixel_alpha(fb, color, alpha);
```

### Storage Strategy

**Option 1: Pre-compute all sizes at startup**
- Generate sprites for sizes 1 to max (e.g., 64) at initialization
- Memory: 64x64 alpha mask = 4KB, so 64 sizes = ~256KB total
- Simple, predictable, fast lookup by size

**Option 2: Cache on demand**
- Compute when first requested, store in a map
- Handles arbitrary sizes
- Slightly more complex

Recommendation: Option 1 - pre-compute at startup. Memory is trivial and avoids runtime computation.

### Why Pre-computation is Necessary

Scripts can change point size per-point (`linesize=rand(20)`), so sprites must be pre-computed or cached - can't recompute per point.

---

## Future Plan: Custom Sprites

The alpha mask approach generalizes to arbitrary sprites:
- Pre-computed AA circles (various sizes)
- Pre-computed AA squares
- Custom shapes (stars, rings, hearts, etc.)
- User-loadable sprites from image files

This would allow effects to use custom particle shapes beyond circles and squares.

---

## Alternative Algorithms Considered

1. **Wu's circle algorithm** - Only touches edge pixels, skips interior. Good for larger circles.

2. **Midpoint circle with AA** - Bresenham-style integer math, exploits 8-way symmetry.

3. **Hybrid** - Lookup table for small radii, algorithmic for larger.

For visualizers where most points are small, lookup tables are likely fastest.

---

## Implementation Notes

- Sprites should be generated with the circle centered at (origin_x, origin_y)
- For odd sizes, center is at (size/2, size/2)
- For even sizes, may need to offset by 0.5 for proper centering
- Consider generating at compile time with constexpr for zero startup cost
