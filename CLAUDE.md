# AVS Project Instructions for Claude

## Critical Development Rules

**NEVER claim something is "fixed" until the user has tested and verified it working.**

The user handles all building and testing. After making code changes, wait for the user to confirm whether the fix works before claiming success.

## EVENTS vs RENDERING - OPTIMIZE THE RENDER LOOP

**THIS IS CRITICAL. DO NOT POLLUTE THE RENDER LOOP WITH PARAMETER CHECKS.**

### The Anti-Pattern (NEVER DO THIS):
```cpp
// WRONG - checking parameters every single frame
int render(...) {
    if (parameters().get_int("grid_width") != last_grid_width_ ||
        parameters().get_int("grid_height") != last_grid_height_ ||
        parameters().get_string("script") != last_script_ ||
        ...) {  // COMPARING 8 PARAMETERS EVERY FRAME
        regenerate_grid();
        last_grid_width_ = parameters().get_int("grid_width");
        // etc
    }
}
```

This pattern:
- Wastes CPU on redundant string/int comparisons 60+ times per second
- Requires tracking `last_*` variables for every parameter
- Makes render() bloated with housekeeping code
- Is fundamentally backwards - the UI TELLS you when parameters change!

### The Correct Pattern (ALWAYS DO THIS):
```cpp
// RIGHT - event-driven: respond when parameters actually change
void on_parameter_changed(const std::string& param_name) override {
    if (param_name == "grid_width" || param_name == "grid_height" ||
        param_name == "script") {
        regenerate_grid();  // Only runs when something actually changes
    }
}

int render(...) {
    // render() is ONLY for rendering. No parameter checks.
    grid_.apply(framebuffer, fbout, w, h, ...);
    return 1;
}
```

### Key Principles:
1. **Initialize on construction** - Generate grids, lookup tables, etc. in the constructor
2. **Regenerate on parameter change** - Use `on_parameter_changed()` callback
3. **Render loop is sacred** - Only do actual rendering work there
4. **No `last_*` tracking variables** - The event callback eliminates the need
5. **No `needs_regeneration()` functions** - These are symptoms of the anti-pattern

### Separate Concerns:
- **Grid/table GENERATION**: Depends on parameters (grid_width, script, etc.) - do once on init, redo on param change
- **Grid/table APPLICATION**: Depends on runtime dimensions (w, h) - do every frame in render()

Example: DynamicMovement grid uses its own `grid_width × grid_height` resolution for script evaluation.
The actual framebuffer dimensions (w, h) are only used at runtime to MAP the grid to pixels.

# Project Context
This is the ORIGINAL AVS (Advanced Visualization Studio) source code being ported to modern C++/OpenFrameworks.
It contains the original Windows dialog procedures and UI layout data from the Winamp plugin.
## Source Code Locations
- **Modern C++ port**: `./libs/avs_lib/effects/` - New effect implementations
- **Original AVS source**: `../vis_avs/avs/vis_avs/` - Contains g_DlgProc functions and Windows UI layouts
- **Resource layouts**: Look in `../vis_avs/avs/vis_avs/res.rc` for dialog coordinates

The original source files use naming like `r_bright.cpp` for brightness effect, `r_dmove.cpp` for movement, etc.
Each contains the `g_DlgProc` function with the original Windows dialog control layouts and IDs.

## Pixel Format

**Framebuffer format is 0xAABBGGRR** (R in bits 0-7, G in bits 8-15, B in bits 16-23, A in bits 24-31).

This matches OF_PIXELS_BGRA on little-endian systems.

**CRITICAL: Alpha MUST always be 0xFF (opaque). Forgetting alpha = invisible pixels.**

### Rule: ALWAYS set or preserve alpha

1. **When modifying existing pixels** - preserve alpha from original:
   ```cpp
   p[i] = (pix & 0xFF000000) | new_rgb_value;
   ```

2. **When creating new colors** (interpolation, calculations, constants) - SET alpha to 0xFF:
   ```cpp
   // Color interpolation - MUST add alpha
   current_color = r | (g << 8) | (b << 16) | 0xFF000000;

   // Color constants - MUST include alpha
   uint32_t white = 0xFFFFFFFF;  // NOT 0x00FFFFFF
   ```

3. **When writing directly to framebuffer** (replace mode) - ensure color has alpha:
   ```cpp
   p[x] = current_color;  // current_color MUST have 0xFF in high byte
   ```

**Symptom of missing alpha**: Effect "does nothing" or pixels are invisible/transparent.

## Color Format Conversion Rules

**ONE conversion at the UI boundary. NO conversions inside avs_lib.**

- All color values in avs_lib use 0xAABBGGRR format consistently
- The ONLY place format conversion happens is in `src/AVSui.cpp` when converting between ImGui RGBA floats and parameter storage
- NEVER add per-pixel conversions in render loops
- NEVER add swap functions or label swapping as workarounds
- If colors appear wrong, fix the actual lookup table or algorithm, not the UI labels

### Common Mistakes to AVOID:
1. **Per-pixel conversion in framebuffer** - WRONG. Conversion is only at UI boundary when picking colors
2. **Swapping UI labels** - WRONG. Fix the underlying code, not the labels
3. **Adding swap helper functions** - WRONG. The format should be correct throughout
4. **Masking before clamping in lookup tables** - WRONG. Clamp first, then mask if needed:
   ```cpp
   // WRONG - mask truncates before clamp can work
   red_tab[n] = ((n * rm) >> 16) & 0xff;
   if (red_tab[n] > 0xff) red_tab[n] = 0xff;  // Never triggers!

   // CORRECT - clamp first, then mask if needed
   red_tab[n] = (n * rm) >> 16;
   if (red_tab[n] > 0xff) red_tab[n] = 0xff;
   ```

## UI Layout Rules

**DO NOT arbitrarily change UI sizes, positions, or scaling factors.**

- UI element positions and sizes are documented in EFFECTS.md from the original AVS
- The 2x position scaling is intentional and built into the rendering code
- If you think something needs resizing, **test first** and let the user decide
- Never "preemptively" adjust sizes based on guesses about what might fit

## Critical Build Requirements

**NEVER hardcode paths in config.make or other files. ALWAYS use environment variables:**

```bash
export OF_ROOT=~/workspace/openFrameworks
```

This is required because:
1. It keeps the repository clean of filesystem-specific paths
2. Different developers use different openFrameworks locations  
3. Prevents accidental commits of hardcoded paths

## Git Repository

The git repository root is `/Users/tim/workspace/avs/ofxAVS`. Do NOT run `git init` - the repo already exists.

## Building and Testing

**The user handles ALL application builds and manual testing.** Do not attempt to run make, cmake, or any build commands for the main application. Do not add "Build and test" as a todo item - the user will do this.

**ALWAYS run automated tests before committing.** The test suite in `libs/avs_lib/tests/` can and should be run before any commit:
```bash
cd libs/avs_lib/tests/build
make && ./avs_tests
```
All tests must pass before committing. If tests fail, fix them or discuss with the user before proceeding.

### Standalone avs_lib example (no OpenFrameworks)
```bash
cd libs/avs_lib/example
mkdir build && cd build
cmake ..
make
./avs_example
```

### OpenFrameworks projects
The user will build these manually via Xcode or make.

## Library Architecture

**avs_lib** (`libs/avs_lib/`) is a standalone, framework-agnostic C++ library with zero external dependencies. It must remain portable and not include any OpenFrameworks or ImGui code.

**ofxAVS** (`src/`) is the OpenFrameworks addon layer that provides:
- ImGui-based UI rendering (`src/ui.h`, `src/ui.cpp` using `avs_ui::renderImGui()`)
- OpenFrameworks texture/pixel handling
- Integration with ofxImGui

Include paths: `addon_config.mk` adds `libs/avs_lib` to include paths, so use `#include "core/ui.h"` not `#include "../libs/avs_lib/core/ui.h"`.

## Project Context

This is an Advanced Visualization Studio (AVS) port to modern C++ and OpenFrameworks. The current focus is on implementing oscilloscope effects with dynamic movement and feedback for classic AVS-style visualizations.

Key components:
- Oscilloscope effect for audio waveform visualization
- Dynamic movement effect with grid-based transformations  
- Clear effect with feedback for trails
- Grid-based coordinate interpolation for authentic AVS stepping artifacts


## Fidelity to Original AVS

**CRITICAL RULE: ALWAYS stay true to the original AVS implementation. NEVER add your own enhancements or improvements.**

- **ALWAYS consult `libs/avs_lib/EFFECTS.md` first** - it is a comprehensive catalogue of research into the original Windows AVS code, documenting all control types, positions, IDs, blend modes, and effect behaviors. This should be the primary reference and may avoid the need to dig into the original source files.
- **SLIDERS MUST have Range and Default documented in EFFECTS.md** - When researching an effect, always document slider ranges as `Range(min, max), Default(value)`. If this information is missing from EFFECTS.md, research it from the original source and add it to EFFECTS.md BEFORE implementing.
- If EFFECTS.md doesn't have the needed information, research the original Windows AVS code in `../vis_avs/avs/vis_avs/` and UPDATE EFFECTS.md with the findings
- If a feature doesn't exist in the original, don't add it
- If behavior differs from original, change it to match
- Individual effects may have their own enable checkboxes, but there's no universal enable/disable for effects in the main chain
- The main effect chain is add/remove only, not enable/disable
- Dialog layouts should match the original coordinates in `res.rc`
- Control types should match original Windows controls (radio buttons, checkboxes, sliders)

## Effect Implementation

**Before implementing a new effect, READ at least one existing effect file to understand current patterns.** Check constructor pattern, PluginInfo structure, include style, and how parameters are set up.

### Include Style
Use flat includes from the avs_lib root:
- CORRECT: `#include "core/blend.h"`
- WRONG: `#include "../core/blend.h"`

### Use Shared Code
- Use `core/blend.h` for BLEND, BLEND_AVG, BLEND_MAX, BLEND_MIN, BLEND_SUB
- NEVER duplicate blend functions locally in effect files
- If you need a blend operation, include the shared header

## Copyright Headers for Source Files

**All new source files MUST include appropriate copyright headers:**

### For AVS-derived files (libs/avs_lib/)
Files that implement AVS functionality or are based on original AVS algorithms:

```cpp
// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root
```

### For ofxAVS and original files
Files that are OpenFrameworks-specific or original implementations:

```cpp
// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root
```

### Requirements
- **ALL** new .cpp and .h files must include appropriate header
- Choose the correct header based on whether the file is AVS-derived or original
- Place header at the very top of the file, before any includes
- Update year if adding to files in future years