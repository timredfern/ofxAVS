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

**Read APIs Before Using Them**
Before writing code that uses a class or function, READ THE HEADER FILE. Do not assume what methods exist based on common patterns or what "makes sense". Every API assumption must be verified by reading actual source code.

WRONG:
```cpp
// Assumed ScriptEngine had these methods without checking
script_engine_.compile(script, "pixel");
script_engine_.execute("pixel");
var_d_ = script_engine_.register_variable("d");
```

RIGHT:
```cpp
// First read script_engine.h, found actual API:
script_engine_.set_variable("d", value);
script_engine_.evaluate(script);
double d = script_engine_.get_variable("d");
```

This applies to ALL code - effects, core classes, utilities. If you haven't read the header, you don't know the API.

**User Builds the App**
Do not run make/cmake for the main OpenFrameworks application. The user handles this.

**Run Automated Tests Before Committing**
```bash
cd libs/avs_lib/tests/build
make && ./avs_tests
```
All tests must pass before any commit. No exceptions.

**Wait for User Testing Before Committing**
After implementing a feature or fix:
1. Run automated tests (above)
2. STOP and wait for the user to build and test the app
3. Only commit after the user confirms it works

Do not commit immediately after writing code. The user needs to verify changes work in the actual application before they go into version control.

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

4. **Validity flags checked in render():**
   ```cpp
   // WRONG - flag set elsewhere, work done in render
   void on_parameter_changed(...) {
       scripts_need_compile_ = true;  // Just set a flag
       table_valid_ = false;
   }
   int render(...) {
       if (scripts_need_compile_) {   // Check flag, do expensive work
           compile_all_scripts();     // This blocks rendering!
           scripts_need_compile_ = false;
       }
       if (!table_valid_) {
           regenerate_lookup_table(); // 2 million iterations!
       }
   }
   ```
   ```cpp
   // RIGHT - do work immediately when parameter changes
   void on_parameter_changed(...) {
       compile_all_scripts();         // Do it now
       regenerate_lookup_table();     // Do it now
   }
   int render(...) {
       // Just render. No flag checks. No compilation.
       apply_transformation(...);
   }
   ```

5. **Polling UI state in render():**
   ```cpp
   // WRONG - checking dropdown value every frame
   int render(...) {
       int preset_idx = parameters().get_int("example_preset");
       if (preset_idx > 0) {
           load_preset(preset_idx);
           parameters().set_int("example_preset", 0);  // Reset
       }
   }
   ```
   ```cpp
   // RIGHT - respond to UI events
   void on_parameter_changed(const std::string& param_name) {
       if (param_name == "example_preset") {
           int preset_idx = parameters().get_int("example_preset");
           if (preset_idx > 0) {
               load_preset(preset_idx);
               parameters().set_int("example_preset", 0);
           }
       }
   }
   ```

6. **Width multiplier lookup tables:**
   ```cpp
   // WRONG - 1990s "optimization" that hurts modern CPUs
   std::vector<int> w_mul_;
   w_mul_[y] = y * w;  // Precompute to "avoid multiplication"
   int idx = x + w_mul_[y];  // Memory lookup
   ```
   ```cpp
   // RIGHT - just multiply (1-3 cycles on modern CPUs)
   int idx = x + y * w;  // Faster than cache-missing memory lookup
   ```

7. **Per-pixel script parsing:**
   ```cpp
   // CATASTROPHICALLY WRONG - parses script 2 million times at 1080p
   for (int py = 0; py < h; py++) {
       for (int px = 0; px < w; px++) {
           engine.evaluate(script);  // PARSES TEXT EVERY PIXEL
       }
   }
   ```
   ```cpp
   // RIGHT - compile once, execute many
   void on_parameter_changed(...) {
       compiled_script_ = engine.compile(script);  // Parse once
   }
   int render(...) {
       for (int py = 0; py < h; py++) {
           for (int px = 0; px < w; px++) {
               engine.execute(compiled_script_);  // Fast execution
           }
       }
   }
   ```

---

## Hall of Shame (Historical Anti-Patterns)

These patterns were found throughout the codebase and systematically removed. This section documents them so they never return.

**Pattern: `scripts_need_compile_` flag**
- Set `true` in `on_parameter_changed()`, checked in `render()`
- Caused: UI lag when editing scripts (compilation blocked rendering)
- Found in: superscope, color_modifier, ddm, shift, bump, set_render_mode_ext, effect_list
- Fix: Compile immediately in `on_parameter_changed()`

**Pattern: `table_valid_` flag**
- Set `false` when parameters change, regenerate table in `render()`
- Caused: 2-second freezes when editing movement scripts
- Found in: movement, color_modifier
- Fix: Regenerate table immediately in `on_parameter_changed()` using cached dimensions

**Pattern: Preset dropdown polling**
- Check `parameters().get_int("example_preset")` every frame in `render()`
- Found in: superscope, color_modifier
- Fix: Handle in `on_parameter_changed("example_preset")`

**Pattern: `w_mul_`/`wmul_` lookup tables**
- Precomputed `y * w` to "avoid multiplication"
- Cargo-culted from 1990s when multiply was 10+ cycles
- Modern CPUs: multiply is 1-3 cycles, memory lookup is slower
- Found in: rotoblitter, ddm
- Fix: Delete the tables, just use `y * w`

**Pattern: Per-pixel `evaluate()` instead of `compile()`/`execute()`**
- Parsed script text for every pixel (millions of times)
- Caused: Movement effect table generation took seconds
- Found in: movement
- Fix: Use `CompiledScript` API

**Pattern: `evaluate()` instead of compiled scripts for frame scripts**
- Re-parsed script every frame instead of once on change
- Found in: dynamic_movement, dynamic_movement_ext
- Fix: Use `compile()` in `on_parameter_changed()`, `execute()` in `render()`

**Pattern: ABGR channel extraction in ARGB codebase**
- Original Windows AVS used ABGR (COLORREF): `pix & 0xff` = Red
- Our codebase uses ARGB: `pix & 0xff` = Blue, `(pix >> 16) & 0xff` = Red
- Blindly porting `pix & 0xff` as "red" produces swapped R/B channels
- Found in: interferences (RGB separation mode)
- Fix: Audit all per-channel operations. Red = bits 16-23, Green = bits 8-15, Blue = bits 0-7

**Pattern: Animated state stored in parameters**
- Writing animated values to `parameters().set_int()` every frame
- Causes: UI slider jitter, potential feedback loops with on_parameter_changed
- Found in: interferences (rotation animation)
- Fix: Use internal member variables for animated runtime state, separate from UI-bound parameters

---

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

**Implementing load_parameters()**
Every effect needs `load_parameters()` to load binary AVS preset data. Without it, presets won't restore effect settings.

1. **Research the binary format** - Read the original `r_*.cpp` file's `load_config()` function. Note the order and types of fields.

2. **Implement the override**:
```cpp
// In header:
void load_parameters(const std::vector<uint8_t>& data) override;

// In cpp:
void MyEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;
    BinaryReader reader(data);

    // Read fields in same order as original load_config()
    int some_value = reader.read_u32();
    parameters().set_int("some_param", some_value);

    // For colors: swap R↔B to convert legacy ABGR to internal ARGB
    uint32_t color = avs::color::swap_rb(reader.read_u32()) | 0xFF000000;
    parameters().set_color("color", color);

    // For scripts: length-prefixed strings
    std::string script = reader.read_length_prefixed_string();
    parameters().set_string("script", script);
}
```

3. **Color conversion** - Binary AVS presets store colors in legacy ABGR format (`0x00BBGGRR`). Use `swap_rb()` to convert to internal ARGB format, then add alpha. This is the ONE place where R↔B conversion happens.

4. **Write tests** - Add tests in `test_load_parameters_colors.cpp` to verify color loading works correctly.

---

## 5. Technical Reference

**Pixel Format: ARGB**

All colors in avs_lib use **ARGB format**: `0xAARRGGBB`

| Bits | Component | Mask |
|------|-----------|------|
| 24-31 | Alpha | `0xFF000000` |
| 16-23 | Red | `0x00FF0000` |
| 8-15 | Green | `0x0000FF00` |
| 0-7 | Blue | `0x000000FF` |

On little-endian systems, this is stored in memory as bytes `[B, G, R, A]`.

**Why ARGB?** OpenFrameworks/OpenGL require ARGB for texture uploads. This is the native format throughout avs_lib.

**AudioData Format**

Audio data uses a fixed format matching original AVS and grandchild:
```cpp
typedef char AudioData[2][2][576];
// AudioData[type][channel][sample]

// ALWAYS use constants instead of magic numbers:
constexpr int AUDIO_SPECTRUM = 0;  // Frequency domain (unsigned 0-255)
constexpr int AUDIO_WAVEFORM = 1;  // Time domain (signed, XOR 128 for unsigned)
constexpr int AUDIO_LEFT = 0;      // Left channel
constexpr int AUDIO_RIGHT = 1;     // Right channel

// Example: visdata[AUDIO_WAVEFORM][AUDIO_LEFT][i] for left channel waveform sample i
```

**NEVER use magic numbers 0 and 1 for audio indices.** Always use the constants. This prevents the widespread convention confusion that previously existed in the codebase.

**Legacy Format: Windows AVS uses ABGR**

Original Windows AVS used **ABGR format**: `0xAABBGGRR` (also known as Windows COLORREF with alpha). Binary `.avs` preset files store colors in this format.

| Format | Hex Layout | Bits 0-7 | Bits 16-23 |
|--------|------------|----------|------------|
| ARGB (avs_lib) | `0xAARRGGBB` | Blue | Red |
| ABGR (legacy) | `0xAABBGGRR` | Red | Blue |

**The One Color Conversion**

When loading legacy presets, we swap Red and Blue **once**:
```cpp
// In load_parameters() - convert legacy ABGR to internal ARGB
uint32_t color = swap_rb(reader.read_u32());
```

This is the **only** R↔B swap in the entire codebase. After loading, all colors are ARGB. Effects, blend operations, line drawing - everything uses ARGB. No further conversions needed.

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

// Write alpha for next effect (ARGB format)
framebuffer[i] = (my_alpha << 24) | (r << 16) | (g << 8) | b;
```

The alpha is only forced opaque at the final output stage in `Renderer::render()`:
```cpp
output_buffer[i] = static_cast<uint32_t>(temp_buffer[i]) | 0xFF000000;
```

**Do NOT** add `| 0xFF000000` inside effect code - alpha should propagate through the chain for effects that use it.

**Color Utilities**

Use `core/color.h` for color manipulation:

```cpp
#include "core/color.h"

// Extract components (ARGB format)
uint8_t r = avs::color::red(color);    // bits 16-23
uint8_t g = avs::color::green(color);  // bits 8-15
uint8_t b = avs::color::blue(color);   // bits 0-7

// Build ARGB color
uint32_t c = avs::color::make(r, g, b);  // alpha defaults to 0xFF

// Legacy preset loading (ABGR → ARGB)
uint32_t color = avs::color::swap_rb(reader.read_u32());
```

**UI Layout**
Dialog coordinates come from original `res.rc`. The 2x position scaling is intentional. Don't adjust sizes or positions without testing.

When implementing effects with script edit boxes (EDITTEXT), always check `res.rc` for accompanying LTEXT labels. Script dialogs typically have labels like "init", "frame", "beat", "point" next to their edit boxes:
```cpp
// From res.rc - note the LTEXT labels
LTEXT           "init",IDC_STATIC,0,20,10,8
EDITTEXT        IDC_EDIT1,29,0,204,52,...

// Must include both in ui_layout:
{.id = "init_label", .text = "init", .type = ControlType::LABEL, .x = 0, .y = 20, ...},
{.id = "init_script", .text = "", .type = ControlType::EDITTEXT, .x = 29, .y = 0, ...},
```

**Expression Help Text**
Scripted effects often have a help button (IDC_HELPBTN) that shows variable documentation. When implementing such effects:

1. Check `g_DlgProc` for `IDC_HELPBTN` handler - it calls `compilerfunctionlist(hwndDlg, text)` with effect-specific help
2. Extract the help text (null-separated: title + content) and add to PluginInfo:
```cpp
.help_text =
    "Effect Name\n"
    "\n"
    "Variables:\n"
    "x, y = position (read/write)\n"
    "w, h = dimensions (read-only)\n"
```
3. The shared expression help (General, Operators, Functions, Constants) is in `core/expression_help.h`
4. Document the help text in EFFECTS.md under a **Help Text** section

---

## 6. Cross-Implementation Testing

**Reference Images**

Test output images are stored in `~/workspace/avs/ref/`:
- `test_avslib_{frame}_{preset}.ppm` - Output from our avs_lib
- `test_grandchild_{frame}_{preset}.ppm` - Output from grandchild (reference)

**Grandchild Reference Implementation**

There is a branch of grandchild's `avs-render-tool` with a command line tool that uses the **same synthetic audio data** as our `avs_render_test`. This allows direct pixel-by-pixel comparison between implementations.

**Test Script**

Use `tools/avs_test` to generate test images:
```bash
avs_test "preset name.avs" <frame> ~/workspace/avs/ref
```

---

## 7. Housekeeping

**Git**
Repository: `/Users/tim/workspace/avs/ofxAVS` (already initialized, don't run `git init`)

Never hardcode filesystem paths. Use environment variables:
```bash
export OF_ROOT=~/workspace/openFrameworks
```

**Tools Location**
Always put new scripts and tools in the `tools/` directory. The canonical location is in the repository, not in ~/bin. User may symlink or copy to ~/bin for convenience, but the source of truth is tools/.

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
