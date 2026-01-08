# AVS Project Instructions

## CRITICAL RULES - READ FIRST

These rules are non-negotiable. Violating them wastes time and frustrates the user.

### 1. Never Claim "Fixed" Until User Confirms
The user builds and tests. You make changes, then WAIT for confirmation. Do not say "fixed" or "should work now."

### 2. Never Add Features That Don't Exist in Original AVS
This is a PORT of the original AVS. Match the original behavior exactly. No improvements, no enhancements, no "better" ways. If it's not in the original, don't add it.

### 3. Render Loop is Sacred - Use Events, Not Polling
**DO NOT check parameters every frame. Use `on_parameter_changed()` callbacks.**

WRONG:
```cpp
int render(...) {
    // Comparing parameters 60+ times per second - STUPID
    if (parameters().get_int("width") != last_width_ || ...) {
        regenerate();
    }
}
```

RIGHT:
```cpp
void on_parameter_changed(const std::string& param_name) override {
    if (param_name == "width") regenerate();  // Only when it actually changes
}

int render(...) {
    // Just render. Nothing else.
    apply(framebuffer, fbout, w, h);
}
```

Rules:
- Generate grids/tables in constructor
- Regenerate in `on_parameter_changed()`
- Render loop does ONLY rendering
- No `last_*` tracking variables
- No `needs_regeneration()` functions

### 4. Alpha Must Be 0xFF
Pixel format: `0xAABBGGRR`. Alpha in high byte MUST be 0xFF or pixels are invisible.

```cpp
// Modifying pixels - preserve alpha
p[i] = (pix & 0xFF000000) | new_rgb;

// Creating colors - set alpha
uint32_t color = r | (g << 8) | (b << 16) | 0xFF000000;
```

Symptom of missing alpha: effect "does nothing."

### 5. Consult EFFECTS.md Before Implementing
`libs/avs_lib/EFFECTS.md` documents original AVS controls, positions, ranges, defaults. Read it first. Update it if information is missing.

---

## Project Overview

AVS (Advanced Visualization Studio) was the Winamp visualizer plugin. This project ports it to modern C++ with OpenFrameworks.

### Source Locations
| Location | Contents |
|----------|----------|
| `libs/avs_lib/effects/` | Modern C++ effect implementations |
| `../vis_avs/avs/vis_avs/` | Original AVS source (reference) |
| `../vis_avs/avs/vis_avs/res.rc` | Original dialog layouts |
| `libs/avs_lib/EFFECTS.md` | Documented effect specifications |

Original files: `r_bright.cpp` (brightness), `r_dmove.cpp` (dynamic movement), etc.

---

## Architecture

### Two Layers
1. **avs_lib** (`libs/avs_lib/`) - Standalone C++ library. NO external dependencies. NO OpenFrameworks. NO ImGui.
2. **ofxAVS** (`src/`) - OpenFrameworks addon. Handles UI, textures, ImGui integration.

### Include Style
```cpp
// CORRECT - flat from avs_lib root
#include "core/blend.h"
#include "core/effect_base.h"

// WRONG - relative paths
#include "../core/blend.h"
```

### Shared Code
Use `core/blend.h` for blend operations. Never duplicate blend functions in effect files.

---

## Effect Implementation

Before writing a new effect:
1. Read EFFECTS.md for the effect specification
2. Read an existing effect file to understand patterns
3. Match original AVS behavior exactly

### Parameter Names Must Match Control IDs
```cpp
// UI layout defines control ID
.id = "grid_width"

// Parameter must use same name
parameters().get_int("grid_width", 16)
```

Mismatch = "unsupported control type" errors.

### Constructor Pattern
```cpp
MyEffect::MyEffect() : some_state_(false) {
    init_parameters_from_layout(effect_info.ui_layout);
    // Add any STRING parameters manually
    parameters().add_parameter(std::make_shared<Parameter>("script",
        ParameterType::STRING, std::string("default")));
    // Generate initial state
    regenerate();
}
```

---

## Pixel Format

Format: `0xAABBGGRR` (little-endian BGRA)
- Bits 0-7: Red
- Bits 8-15: Green
- Bits 16-23: Blue
- Bits 24-31: Alpha (MUST be 0xFF)

### Color Conversion
ONE conversion at UI boundary only (`src/AVSui.cpp`). Inside avs_lib, everything is `0xAABBGGRR`.

Never:
- Add per-pixel conversions in render loops
- Swap UI labels as workaround
- Add swap helper functions

---

## Building and Testing

### User Builds the App
Do not run make/cmake for the main application. User handles this.

### You Run Automated Tests
Before every commit:
```bash
cd libs/avs_lib/tests/build
make && ./avs_tests
```
All tests must pass.

### Standalone Example (no OpenFrameworks)
```bash
cd libs/avs_lib/example
mkdir build && cd build
cmake .. && make
./avs_example
```

---

## Git

Repository root: `/Users/tim/workspace/avs/ofxAVS`

Do NOT run `git init` - repo exists.

Never hardcode paths. Use environment variables:
```bash
export OF_ROOT=~/workspace/openFrameworks
```

---

## Copyright Headers

### AVS-derived files (libs/avs_lib/)
```cpp
// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License
```

### Original files (src/)
```cpp
// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License
```

---

## UI Layout

Do not change UI sizes or positions without testing. The 2x scaling is intentional. Match original `res.rc` coordinates.
