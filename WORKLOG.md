# AVS Work Log

**Purpose:** Persistent state for Claude Code sessions. Check this file at session start.

## Current Task

Ready for user testing on macOS and Linux/Pi4.

## Recent Work

### MIDI/EventBus Support - COMPLETE

Full MIDI support implemented:
- `libs/avs_lib/core/event_bus.h/cpp` - Lock-free SPSC ring buffer, EventBus singleton
- `libs/avs_lib/MIDI.md` - Design documentation
- Script arrays: `midi_cc[0..127]`, `midi_note[0..127]`, `midi_note_index[i]`
- Script scalars: `midi_note_count`, `midi_pitch_bend`, `midi_any_note`
- Renderer calls `EventBus::instance().process_frame()` before effects
- Tests in `test_midi_eventbus.cpp`

### MIDI File Playback - COMPLETE

- `src/MidiFile.h/cpp` - SMF parser with tempo/timing support
- Synced with audio playback position
- Events pushed to EventBus during playback
- Debug toggle with backtick key

### Script Engine Extensions - COMPLETE

- Comparison operators: `<` `>` `<=` `>=` `==` `!=`
- Line comments: `// comment`
- User-defined arrays: `arr[i] = value`
- `while(condition, body)` and `loop(count, body)`
- Fixed `%` modulo to use integer math
- Documentation in `SCRIPT_ARCHITECTURE.md`

### Cross-Platform Audio - COMPLETE

- Replaced macOS-only ofxAudioDecoder with miniaudio
- `libs/miniaudio/miniaudio.h` - Single-header library
- `src/AudioFileLoader.h/cpp` - Cross-platform audio decoder
- Supports macOS, Linux, Windows

### C++20 Compatibility - COMPLETE

- Fixed designated initializer order for GCC 13+ (Debian Trixie/Pi4)
- Upgraded to C++20 with -pedantic to catch issues on macOS
- Files fixed: color_clip, starfield, water_bump, fast_brightness, channel_shift, multiplier, dot_grid, multi_delay, video_delay, clear, bump, bass_spin, blur, brightness, oscilloscope, superscope, beat_detector

### Color Audit - COMPLETE

All 53 effects audited. 8 critical R↔B bugs fixed. All magic numbers converted to color.h.

---

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

2026-02-06 - MIDI/EventBus complete, cross-platform audio, C++20 fixes
