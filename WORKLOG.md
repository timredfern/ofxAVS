# AVS Work Log

**Purpose:** Persistent state for Claude Code sessions. Check this file at session start.

## Current Task

Color audit COMPLETE - all effects now use color.h utilities.

## Completed

- [x] starfield.cpp - Updated to use color.h + tests added
- [x] starfield_ext.cpp - Updated to use color.h
- [x] ddm.cpp - Updated bilinear interpolation to use color.h
- [x] ring.cpp - Updated color interpolation to use color.h
- [x] oscstar.cpp - Updated color interpolation to use color.h
- [x] rotstar.cpp - Updated color interpolation to use color.h
- [x] dot_grid.cpp - Updated color interpolation to use color.h
- [x] picture.cpp - Fixed ABGR bug, now uses color::make() for ARGB
- [x] Keyboard navigation fix (Parameters panel stable window ID)
- [x] WORKLOG.md created for session persistence

## COMPLETE COLOR AUDIT - ALL 53 EFFECTS

### CRITICAL R↔B SWAP BUGS (8 files) - ALL FIXED

| File              | Lines   | Issue                      | Status    |
|-------------------|---------|----------------------------|-----------|
| starfield.cpp     | 90-95   | cr=Blue, cb=Red, swapped   | FIXED     |
| starfield_ext.cpp | 89-96   | Same pattern               | FIXED     |
| ddm.cpp           | 258-265 | r=Blue, b=Red in bilinear  | FIXED     |
| ring.cpp          | 50-54   | r1=Blue, r3=Red            | FIXED     |
| oscstar.cpp       | ~50     | Same interpolation pattern | FIXED     |
| rotstar.cpp       | 49-53   | r1=Blue, r3=Red            | FIXED     |
| dot_grid.cpp      | ~50     | Same interpolation pattern | FIXED     |
| picture.cpp       | 57      | Constructs ABGR not ARGB!  | FIXED     |

### MAGIC NUMBERS - ALL CONVERTED TO color.h

| File                 | Issue                           | Status |
|----------------------|---------------------------------|--------|
| oscilloscope.cpp     | & 0xff, >> 8, >> 16             | DONE   |
| superscope.cpp       | color interpolation             | DONE   |
| color_clip.cpp       | channel extraction              | DONE   |
| color_fade.cpp       | channel extraction              | DONE   |
| color_modifier.cpp   | byte-level access (intentional) | N/A    |
| interferences.cpp    | channel extraction              | DONE   |
| brightness.cpp       | lookup table                    | DONE   |
| bump.cpp             | setdepth functions              | DONE   |
| fadeout.cpp          | color table build               | DONE   |
| fast_brightness.cpp  | channel math                    | DONE   |
| grain.cpp            | channel extraction/construction | DONE   |
| multiplier.cpp       | >> 16, >> 8, & 0xff, masks      | DONE   |
| movement.cpp         | blend_max, blend4               | DONE   |
| dynamic_movement.cpp | uses CoordinateGrid (no direct) | N/A    |
| effect_list.cpp      | depthof() function              | DONE   |
| unique_tone.cpp      | lookup table                    | DONE   |
| dot_fountain.cpp     | color table                     | DONE   |

### NOW USING color.h (27 files)

| File              | Status |
|-------------------|--------|
| timescope.cpp     | OK     |
| dot_plane.cpp     | OK     |
| water.cpp         | OK     |
| channel_shift.cpp | OK     |
| starfield.cpp     | OK     |
| starfield_ext.cpp | OK     |
| ddm.cpp           | OK     |
| ring.cpp          | OK     |
| oscstar.cpp       | OK     |
| rotstar.cpp       | OK     |
| dot_grid.cpp      | OK     |
| picture.cpp       | OK     |
| oscilloscope.cpp  | OK     |
| superscope.cpp    | OK     |
| color_clip.cpp    | OK     |
| color_fade.cpp    | OK     |
| interferences.cpp | OK     |
| brightness.cpp    | OK     |
| bump.cpp          | OK     |
| fadeout.cpp       | OK     |
| fast_brightness.cpp| OK    |
| grain.cpp         | OK     |
| multiplier.cpp    | OK     |
| movement.cpp      | OK     |
| effect_list.cpp   | OK     |
| unique_tone.cpp   | OK     |
| dot_fountain.cpp  | OK     |

### NO COLOR WORK (28 files)

bass_spin.cpp, blitter_feedback.cpp, blur.cpp, buffer_save.cpp, clear.cpp,
color_reduction.cpp, comment.cpp, custom_bpm.cpp, dynamic_movement_ext.cpp,
interleave.cpp, invert.cpp, mirror.cpp, mosaic.cpp, multi_delay.cpp,
moving_particle.cpp, onbeat_clear.cpp, rotoblitter.cpp, scatter.cpp,
set_render_mode.cpp, set_render_mode_ext.cpp, shift.cpp, video_delay.cpp,
water_bump.cpp, unsupported.cpp

## Summary

- 8 CRITICAL bugs - ALL FIXED
- 15 files with magic numbers - ALL CONVERTED to color.h
- 27 files now using color.h
- 2 files intentionally use different approach (color_modifier.cpp: byte access, dynamic_movement.cpp: uses CoordinateGrid)
- 26 files have no color channel work
- Total: 53 effect files

## Anti-Patterns (Hall of Shame)

These patterns were found throughout the codebase and systematically removed. This section documents them so they never return.

### `scripts_need_compile_` flag
- Set `true` in `on_parameter_changed()`, checked in `render()`
- Caused: UI lag when editing scripts (compilation blocked rendering)
- Found in: superscope, color_modifier, ddm, shift, bump, set_render_mode_ext, effect_list
- Fix: Compile immediately in `on_parameter_changed()`

### `table_valid_` flag
- Set `false` when parameters change, regenerate table in `render()`
- Caused: 2-second freezes when editing movement scripts
- Found in: movement, color_modifier
- Fix: Regenerate table immediately in `on_parameter_changed()` using cached dimensions

### Preset dropdown polling
- Check `parameters().get_int("example_preset")` every frame in `render()`
- Found in: superscope, color_modifier
- Fix: Handle in `on_parameter_changed("example_preset")`

### `w_mul_`/`wmul_` lookup tables
- Precomputed `y * w` to "avoid multiplication"
- Cargo-culted from 1990s when multiply was 10+ cycles
- Modern CPUs: multiply is 1-3 cycles, memory lookup is slower
- Found in: rotoblitter, ddm
- Fix: Delete the tables, just use `y * w`

### Per-pixel `evaluate()` instead of `compile()`/`execute()`
- Parsed script text for every pixel (millions of times)
- Caused: Movement effect table generation took seconds
- Found in: movement
- Fix: Use `CompiledScript` API

### `evaluate()` instead of compiled scripts for frame scripts
- Re-parsed script every frame instead of once on change
- Found in: dynamic_movement, dynamic_movement_ext
- Fix: Use `compile()` in `on_parameter_changed()`, `execute()` in `render()`

### ABGR channel extraction in ARGB codebase
- Original Windows AVS used ABGR (COLORREF): `pix & 0xff` = Red
- Our codebase uses ARGB: `pix & 0xff` = Blue, `(pix >> 16) & 0xff` = Red
- Blindly porting `pix & 0xff` as "red" produces swapped R/B channels
- Found in: interferences (RGB separation mode)
- Fix: Audit all per-channel operations. Red = bits 16-23, Green = bits 8-15, Blue = bits 0-7

### Animated state stored in parameters
- Writing animated values to `parameters().set_int()` every frame
- Causes: UI slider jitter, potential feedback loops with on_parameter_changed
- Found in: interferences (rotation animation)
- Fix: Use internal member variables for animated runtime state, separate from UI-bound parameters

### Magic values instead of color.h utilities
- Using `pix & 0xff`, `(pix >> 16) & 0xff`, `0xFF0000`, etc. directly
- Error-prone: easy to confuse ARGB bit positions, no compile-time checks
- Found in: many effects during initial port
- Fix: Use `avs::color::red(pix)`, `avs::color::blue(pix)` for normalized 0-255 values
- Fix: Use `avs::color::make(r, g, b)` to construct colors

### UI-to-channel mapping not matching labels
- "Red" slider affecting blue channel, "Blue" slider affecting red channel
- Caused by ABGR logic ported without adaptation to ARGB framebuffer
- User moves "Red" slider expecting red to change, but blue changes instead
- Found in: brightness (originally), channel_shift, dot_plane, dot_fountain
- Fix: Trace from UI parameter → multiplier/table → extraction → output to verify correct channel

## Last Updated

2026-01-28 - Converted all magic number files to use color.h
