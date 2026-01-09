# AVS Project Instructions

## 1. Project Context

**What is AVS?**
AVS (Advanced Visualization Studio) was Winamp's visualizer plugin from the early 2000s. This project ports it to modern C++ with OpenFrameworks.

**Source Locations**
| Location | Contents |
|----------|----------|
| `libs/avs_lib/effects/` | Modern C++ effect implementations |
| `../vis_avs/avs/vis_avs/` | Original AVS source (reference) |
| `../vis_avs/avs/vis_avs/res.rc` | Original dialog layouts |
| `libs/avs_lib/EFFECTS.md` | Documented effect specifications |

Original files use naming like `r_bright.cpp` (brightness), `r_dmove.cpp` (dynamic movement).

**Architecture**
- **avs_lib** (`libs/avs_lib/`) - Standalone C++ library. Zero external dependencies. No OpenFrameworks, no ImGui.
- **ofxAVS** (`src/`) - OpenFrameworks addon layer. Handles UI rendering, textures, ImGui integration.

---

## 2. Design Philosophy

**Exact Port, No Enhancements**
This is a port, not a rewrite. Match original AVS behavior exactly. If the original has a bug, keep the bug. If the original lacks a feature, don't add it. Never "improve" or "modernize" behavior.

**EFFECTS.md is the Source of Truth**
Before implementing any effect, read `libs/avs_lib/EFFECTS.md`. It documents original control types, positions, ranges, and defaults from the Windows AVS source. If information is missing, research the original source and update EFFECTS.md first.

---

## 3. Workflow

**Never Claim "Fixed" Until User Confirms**
You write code. The user builds and tests. After making changes, STOP and WAIT for the user to report results. Never say "fixed", "should work", or "this will work" - you don't know until tested.

**User Builds the App**
Do not run make/cmake for the main OpenFrameworks application. The user handles this.

**Run Automated Tests Before Committing**
```bash
cd libs/avs_lib/tests/build
make && ./avs_tests
```
All tests must pass before any commit. No exceptions.

---

## 4. Code Patterns

**Events, Not Polling**
Effects have parameters (grid size, scripts, blend modes). When these change, you need to regenerate lookup tables or grids.

WRONG: Check every parameter every frame to see if it changed. This wastes CPU comparing strings 60 times per second and requires `last_*` variables for everything.

RIGHT: The UI calls `on_parameter_changed(param_name)` when the user changes something. Regenerate there. The render loop just renders.

```cpp
// Regenerate when parameter changes
void on_parameter_changed(const std::string& param_name) override {
    if (param_name == "grid_width" || param_name == "script") {
        regenerate();
    }
}

// Render loop does ONLY rendering
int render(...) {
    grid_.apply(framebuffer, fbout, w, h);
    return 1;
}
```

**Anti-Pattern Examples (DO NOT DO THESE):**

1. **Comparing strings every frame:**
   ```cpp
   // WRONG - comparing script text 60 times per second
   if (parameters().get_string("init_script") != last_init_script_) {
       recompile();
       last_init_script_ = parameters().get_string("init_script");
   }
   ```

2. **Passing screen dimensions to generation functions:**
   ```cpp
   // WRONG - why does generate_grid need screen dimensions?
   void generate_grid(int w, int h, AudioData visdata) {
       for (int y = 0; y < h; y++) {  // NO! Grid has its own resolution!
           for (int x = 0; x < w; x++) {
   ```

   ```cpp
   // RIGHT - grid uses its own parameters for generation
   void regenerate_grid() {
       int gw = parameters().get_int("grid_width");
       int gh = parameters().get_int("grid_height");
       grid_.generate(gw, gh, script);  // Grid's own resolution
   }

   // Screen dimensions only used when APPLYING the grid to pixels
   int render(..., int w, int h) {
       grid_.apply(framebuffer, fbout, w, h);  // Map grid to screen here
   }
   ```

   Generation uses PARAMETER dimensions. Rendering uses SCREEN dimensions. Don't mix them.

   When your understanding changes, update the code. We added w,h to generate() before understanding grids. After learning the grid has its own resolution, we should have immediately removed them. We didn't. The stale parameters led to tracking `last_w_`, `last_h_` and regenerating on resize - all unnecessary complexity from not cleaning up after ourselves.

3. **The `needs_regeneration()` function:**
   ```cpp
   // WRONG - if you have this function, you're doing it wrong
   bool needs_regeneration() {
       return param1 != last1_ || param2 != last2_ || ...;
   }
   ```
   This function should not exist. Use `on_parameter_changed()` instead.

**Parameter Names Must Match Control IDs**
The UI layout defines control IDs. Your parameter names must be identical.
```cpp
.id = "grid_width"  // in UI layout
parameters().get_int("grid_width")  // must match exactly
```
Mismatch causes "unsupported control type" errors.

**Include Style**
```cpp
#include "core/blend.h"      // CORRECT - flat from avs_lib root
#include "../core/blend.h"   // WRONG - relative paths
```

**Use Shared Code**
Blend operations are in `core/blend.h`. Never duplicate them in effect files.

---

## 5. Technical Reference

**Pixel Format**
Each pixel is a 32-bit integer: `0xAARRGGBB` stored as `0xAABBGGRR` in memory (little-endian).
- Bits 0-7: Red
- Bits 8-15: Green
- Bits 16-23: Blue
- Bits 24-31: Alpha

**Alpha Channel Handling**

Alpha propagates through the effect chain and can be used by effects for internal compositing:

- **Effects CAN read alpha** from the framebuffer (set by previous effects)
- **Effects CAN write alpha** for subsequent effects to use
- **BLEND macro preserves alpha** - see `r_defs.h` lines 100-111
- **At final display**: Renderer forces alpha to 0xFF when copying to the output buffer

Original AVS on Windows displayed with `BitBlt(SRCCOPY)` which ignores alpha. Our port uses OpenFrameworks which respects alpha, so we force 0xFF at the output stage only.

Example - effect that uses alpha for compositing:
```cpp
// Read alpha from previous effect
uint8_t alpha = (framebuffer[i] >> 24) & 0xFF;

// Use alpha for blending
if (alpha > 128) {
    framebuffer[i] = blend_pixels(framebuffer[i], new_color);
}

// Write alpha for next effect
framebuffer[i] = (my_alpha << 24) | (r << 16) | (g << 8) | b;
```

The alpha is only forced opaque at the final output stage in `Renderer::render()`:
```cpp
output_buffer[i] = static_cast<uint32_t>(temp_buffer[i]) | 0xFF000000;
```

**Do NOT** add `| 0xFF000000` inside effect code - alpha should propagate through the chain for effects that use it.

**Color Conversion**
Colors are stored as `0xAABBGGRR` everywhere in avs_lib. The only format conversion happens in `src/AVSui.cpp` when interfacing with ImGui color pickers. Never add conversions inside render loops or effect code.

**UI Layout**
Dialog coordinates come from original `res.rc`. The 2x position scaling is intentional. Don't adjust sizes or positions without testing.

---

## 6. Housekeeping

**Git**
Repository: `/Users/tim/workspace/avs/ofxAVS` (already initialized, don't run `git init`)

Never hardcode filesystem paths. Use environment variables:
```bash
export OF_ROOT=~/workspace/openFrameworks
```

**Copyright Headers**

For avs_lib files:
```cpp
// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License
```

For ofxAVS files:
```cpp
// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License
```
