# AVS Library Architecture

## Overview

This library recreates the Advanced Visualization Studio (AVS) effect system with a focus on:
- **100% compatibility** with existing AVS presets
- **Authentic visual output** matching the original quirks and artifacts
- **Clean, maintainable code** with reusable components
- **Performance** suitable for real-time audio visualization

## Core Architecture Principles

### 1. Effect-Specific Implementation
Each AVS effect is implemented as a separate class that exactly matches the original behavior:
- `MovementEffect` → `r_trans.cpp` (Trans/Movement)
- `DynamicMovementEffect` → `r_dmove.cpp` (Trans/Dynamic Movement)
- `OscilloscopeEffect` → `r_simple.cpp` (Render/Simple)
- `SuperScopeEffect` → `r_sscope.cpp` (Render/SuperScope)
- `BlurEffect` → `r_blur.cpp` (Trans/Blur)
- `BrightnessEffect` → `r_bright.cpp` (Trans/Brightness)
- `ClearEffect` → `r_clear.cpp` (Render/Clear Screen)
- `OnBeatClearEffect` → `r_onetone.cpp` (Render/OnBeat Clear)
- `DotGridEffect` → `r_dotgrid.cpp` (Render/Dot Grid)
- `DotFountainEffect` → `r_fountain.cpp` (Render/Dot Fountain)
- `WaterEffect` → `r_water.cpp` (Trans/Water)
- `SetRenderModeEffect` → `r_linemode.cpp` (Misc/Set Render Mode)
- `EffectList` → `r_list.cpp` (Container/Effect List)

### 2. Script Infrastructure

**Original Plan:** Shared scripting components using NS-EEL:
- `EELExecutor` - NS-EEL compilation and execution
- `ScriptContext` - Variable management and execution state
- `ScriptVariables` - Standard variable registration (w, h, x, y, r, d, etc.)

**Current Implementation:** Custom expression parser with lexer/parser architecture:
- `ScriptEngine` - Expression evaluation with variable support
- `lexer.cpp` / `parser.cpp` - Custom expression parsing

**Rationale for change:** A custom parser was implemented instead of NS-EEL integration to:
1. Avoid external dependencies (NS-EEL requires NASM assembler)
2. Maintain portability across platforms (ARM, x86, WebAssembly)
3. Allow easier debugging and modification of expression handling

The custom parser supports the subset of EEL syntax used by AVS effects (arithmetic, trigonometry, conditionals, variable assignment).

### 3. Coordinate System Separation

**Original Plan:**
- `FullResolutionTable` class (int* lookup, one entry per pixel) for MovementEffect
- `CoordinateGrid` class (sparse grid with interpolation) for DynamicMovementEffect

**Current Implementation:**
- `MovementEffect`: Internal `std::vector<int> lookup_table_` (full resolution)
- `DynamicMovementEffect`: Uses `CoordinateGrid` class (sparse grid)

**Rationale for change:** The full-resolution table was inlined into `MovementEffect` rather than extracted to a separate class. This keeps the simpler effect self-contained while the more complex grid-based interpolation in `DynamicMovementEffect` benefits from the separate `CoordinateGrid` utility class.

## Directory Structure

**Original Plan:**
```
libs/avs_lib/
├── core/
│   ├── script/
│   │   ├── eel_executor.cpp
│   │   ├── script_context.cpp
│   │   └── script_variables.cpp
│   └── transforms/
│       ├── full_resolution_table.cpp
│       └── coordinate_grid.cpp
```

**Current Structure:**
```
libs/avs_lib/
├── ARCHITECTURE.md              # This file
├── EFFECTS.md                   # Effect catalog with UI layouts from original AVS
├── OPTIMISATION.md              # Performance notes (bit-shift blur, auto-vectorization)
├── core/
│   ├── script/                  # Expression evaluation
│   │   ├── lexer.cpp           # Token scanning
│   │   ├── parser.cpp          # Expression parsing
│   │   └── script_engine.cpp   # Variable context and evaluation
│   ├── coordinate_grid.cpp     # Grid-based transforms with interpolation
│   ├── builtin_effects.cpp     # Built-in effect registration
│   ├── parameter.cpp           # Parameter system
│   ├── plugin_manager.cpp      # Effect registration and factory
│   ├── preset.cpp              # Binary .avs preset loading
│   ├── blend.cpp               # Blend mode implementations
│   ├── line_draw.h             # Line drawing (Bresenham, Wu anti-aliasing)
│   ├── effect_base.h           # Base effect interface
│   └── ui.h                    # UI layout definitions
├── effects/                     # 23+ ported AVS effects
│   ├── movement_effect.cpp         # Trans/Movement (23 presets + custom)
│   ├── dynamic_movement_effect.cpp # Trans/Dynamic Movement (grid-based)
│   ├── superscope_effect.cpp       # Render/SuperScope (scriptable)
│   ├── set_render_mode_effect.cpp  # Misc/Set Render Mode (line control)
│   ├── dot_fountain_effect.cpp     # Render/Dot Fountain (3D particles)
│   ├── water_effect.cpp            # Trans/Water (ripple effect)
│   └── ...                         # See EFFECTS.md for full list
├── example/                     # Standalone example (no OpenFrameworks)
├── tools/                       # CLI utilities
│   └── avs2json.cpp            # Convert .avs presets to JSON for debugging
└── tests/                       # Catch2 unit tests (500+ assertions)
```

**Rationale for changes:**
- `transforms/` subdirectory was not created; `coordinate_grid.cpp` lives directly in `core/`
- `full_resolution_table` was not extracted as separate class
- Added `EFFECTS.md` for documenting original AVS UI layouts
- Added `OPTIMISATION.md` for performance findings

## UI Architecture

### Separation of Concerns

The library is designed with a clear separation between core logic and UI:

- **avs_lib** (`libs/avs_lib/`) - Pure C++ library with zero external dependencies. Contains all effect logic, parameter systems, and UI layout *definitions* (not rendering).

- **ofxAVS** (`src/`) - OpenFrameworks addon that provides the proof-of-concept UI implementation using ImGui. Renders the UI layouts defined in avs_lib.

This separation allows avs_lib to be embedded in any host application (DAW plugins, standalone apps, web via WebAssembly) while ofxAVS demonstrates one possible integration.

### Data-Driven UI Layouts

Each effect defines its UI through `PluginInfo::ui_layout`, containing:
- Control positions and sizes from original AVS dialogs (all 137x137 pixels)
- Control types: CHECKBOX, SLIDER, BUTTON, RADIO_GROUP, TEXT_INPUT, EDITTEXT, COLOR_BUTTON, COLOR_ARRAY, DROPDOWN, LABEL, GROUPBOX
- Parameter binding via matching control ID to parameter name

### Capturing Original Windows Dialog Layouts

The original AVS used Windows dialog resources (`.rc` files) with absolute pixel positioning. Each effect had a 137×137 pixel dialog with controls at explicit x,y coordinates. avs_lib preserves these layouts exactly so any renderer can faithfully recreate the original UI.

**Documentation Process:**
1. Examine original AVS source in `../vis_avs/avs/vis_avs/` (e.g., `r_bright.cpp`)
2. Find the `g_DlgProc` function which handles the Windows dialog
3. Cross-reference with `res.rc` for exact control coordinates and types
4. Document findings in `EFFECTS.md` with control IDs, positions, sizes, and types
5. Implement effect UI using the documented layout in `PluginInfo::ui_layout`

**Windows Control Types → avs_lib Types:**
| Windows Control | avs_lib ControlType | Notes |
|-----------------|---------------------|-------|
| LTEXT | LABEL | Static text at position |
| GROUPBOX | GROUPBOX | Visual frame with title |
| AUTOCHECKBOX | CHECKBOX | Toggle checkbox |
| AUTORADIOBUTTON | RADIO_GROUP | Mutually exclusive options |
| CONTROL "msctls_trackbar32" | SLIDER | Horizontal slider |
| PUSHBUTTON | BUTTON | Clickable button |
| EDITTEXT | TEXT_INPUT / EDITTEXT | Single/multi-line text |

**Key design principle:** Windows separates controls that some UI frameworks combine. Labels are always independent LTEXT controls at explicit positions, not embedded in other controls. avs_lib stores them as separate LABEL controls so renderers can position them correctly.

```cpp
// Effect layout preserves Windows dialog structure
// Labels are SEPARATE controls, not embedded in EDITTEXT/SLIDER
{.id = "init_label", .type = ControlType::LABEL, .x = 0, .y = 3, .text = "init"},
{.id = "init_script", .type = ControlType::EDITTEXT, .x = 25, .y = 0, .text = "", ...}

{.id = "red_label", .type = ControlType::LABEL, .x = 0, .y = 15, .text = "Red"},
{.id = "red_adjust", .type = ControlType::SLIDER, .x = 25, .y = 13, .w = 97, ...}
```

**EDITTEXT `.text` field:** For EDITTEXT controls, the `.text` field should be empty (`""`). The label is a separate LABEL control. Do NOT put the label text in the EDITTEXT's `.text` field.

See `src/ARCHITECTURE.md` in ofxAVS for the reference ImGui renderer implementation.

### Effect Help Text

Scripted effects can provide expression help documentation via `EffectUILayout::help_text`. This is displayed in the UI to assist users writing expressions:

```cpp
.ui_layout = EffectUILayout({
    // controls...
}, R"(Variables:
  n = number of points
  i = current point index
  v = audio value at point
  ...)")
```

The help text uses the original AVS format with variable names and descriptions.

### RADIO_GROUP Control
Radio button groups use explicit coordinates per option rather than computed layouts:
```cpp
{
    .id = "blur_level",
    .type = ControlType::RADIO_GROUP,
    .radio_options = {
        {"No blur", 2, 1, 39, 10},
        {"Light blur", 2, 12, 45, 10},
        {"Medium blur", 2, 23, 54, 10},
        {"Heavy blur", 2, 34, 50, 10}
    },
    .default_val = static_cast<int>(BlurLevel::MEDIUM)
}
```

### Typed Enums
Common radio group values use typed enums for clarity:
- `BlendMode` - REPLACE, ADDITIVE, BLEND_5050, DEFAULT
- `DrawStyle` - LINES, SOLID, DOTS
- `AudioChannel` - LEFT, RIGHT, CENTER
- `VerticalPosition` - TOP, BOTTOM, CENTER
- `RenderMode` - SPECTRUM, OSCILLOSCOPE
- `BlurLevel` - NONE, LIGHT, MEDIUM, HEAVY (effect-specific)
- `RoundMode` - DOWN, UP (effect-specific)

## Script Execution System

### Script Phases and Execution Order

Effects with scripting support use a persistent `ScriptEngine` that preserves variables across frames. Scripts execute in a specific order:

1. **INIT** - Runs once via `on_parameter_changed("init_script")` when the init script is modified. Sets up initial variable values.

2. **FRAME** - Runs every frame in `render()`. Used for time-based animation (e.g., `counter = counter + 1`).

3. **BEAT** - Runs on beats, **AFTER** frame script. This order is critical: beat can override frame's changes.
   ```
   // Example: beat snaps value that frame is smoothly animating
   frame: alpha = alpha * 0.99    // smooth decay
   beat:  alpha = 1.0             // snap to full on beat
   ```
   If beat ran before frame, frame would immediately decay the beat's effect.

4. **PIXEL/POINT** - Runs per coordinate (DynamicMovement) or per audio sample (SuperScope). Uses variables set by frame/beat.

### Event-Driven Init Script

The init script runs in `on_parameter_changed()`, NOT in `render()`. This avoids:
- Checking a flag every frame (`if (!initialized_)`)
- Per-frame string comparisons
- The anti-pattern of polling state that should be event-driven

### Phase Usage by Effect Type
- **MovementEffect**: Single expression evaluation per pixel (no multi-phase)
- **DynamicMovementEffect**: INIT + FRAME + BEAT + PIXEL (full multi-phase)
- **SuperScopeEffect**: INIT + FRAME + BEAT + POINT (per audio sample)
- **OscilloscopeEffect**: No scripting (hardcoded rendering)
- **SetRenderModeEffect**: INIT + FRAME + BEAT (controls global line drawing state)

## Effect Implementation Patterns

### Transform Effects (MovementEffect)
```cpp
class MovementEffect : public EffectBase {
    std::vector<int> lookup_table_;  // Full resolution lookup

    void render(AudioData visdata, int isBeat,
                uint32_t* framebuffer, uint32_t* fbout, int w, int h) {
        // Generate full-resolution lookup table
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                // Evaluate expression for this pixel
                double px = ..., py = ...;
                evaluate_movement_script(script, px, py, r, d, visdata, w, h);
                lookup_table_[y * w + x] = source_index;
            }
        }

        // Apply transformation
        for (int i = 0; i < w * h; i++) {
            fbout[i] = framebuffer[lookup_table_[i]];
        }
    }
};
```

### Dynamic Transform Effects (DynamicMovementEffect)
```cpp
class DynamicMovementEffect : public EffectBase {
    CoordinateGrid grid_;  // Sparse grid with interpolation

    void render(...) {
        // Execute frame-level scripts
        execute_init_script(visdata, w, h);
        execute_frame_script(visdata, w, h);
        if (isBeat) execute_beat_script(visdata, w, h);

        // Generate sparse grid (e.g., 16x16)
        grid_table_.generate(w, h, grid_w, grid_h,
                            x_expr, y_expr, rectangular, subpixel,
                            visdata, wrap, interp_mode);

        // Apply with interpolation
        grid_table_.apply(framebuffer, fbout, w, h, blend);
    }
};
```

### Render Effects (OscilloscopeEffect)
```cpp
class OscilloscopeEffect : public EffectBase {
    void render(AudioData visdata, int isBeat,
                uint32_t* framebuffer, uint32_t* fbout, int w, int h) {
        // Process audio samples directly (no scripting)
        for (int i = 0; i < 576; i++) {
            int x = i * w / 576;
            int y = h/2 + (visdata[0][0][i] * h / 256);
            // Draw line/dot at (x, y)
        }
    }
};
```

**Note:** `SuperScopeEffect` with full POINT-phase scripting is implemented in `superscope_effect.cpp`.

### Line Drawing Infrastructure

AVS uses a global line drawing mode that affects how render effects (Oscilloscope, SuperScope) draw lines. This is controlled by the `Set Render Mode` effect and stored in a global variable `g_line_blend_mode`.

**Line Drawing Module** (`core/line_draw.h`):
- `line()` - Bresenham's algorithm for single-pixel lines
- `line_wu()` - Wu's anti-aliased line algorithm (1991, public domain)
- Support for variable line width (1-256 pixels)
- Blend modes: Replace, Additive, Maximum, 50/50, Subtractive, XOR, Minimum

**Global Line Mode** (`g_line_blend_mode`):
The 32-bit packed value contains:
- Bit 31: Enabled flag
- Bits 24-26: Line style (0=solid, 1=dotted, 2=anti-aliased)
- Bits 16-23: Line width (1-256)
- Bits 8-15: Alpha value (for future use)
- Bits 0-7: Blend mode

**Set Render Mode Effect**:
Controls the global line drawing state with optional scripting:
- Script variables: `lw` (line width), `bm` (blend mode), `a` (alpha), `aa` (anti-alias), `ac` (additive color), `rd` (random dots)
- Init/Frame/Beat scripts allow dynamic control (e.g., thicker lines on beat)
- Backwards compatible with presets that don't use scripting

```cpp
// Example: Pulse line width on beat
// Init: lw=2
// Frame: lw=max(1,lw-0.5)
// Beat: lw=10
```

## Compatibility Strategy

### Preset Loading
Each effect maintains the exact parameter format of its original AVS counterpart.

### Visual Fidelity
Key requirements for authentic AVS behavior:
- **Movement effects** must evaluate scripts at full resolution
- **Dynamic Movement** must use sparse grid evaluation with coordinate interpolation
- **Rendering artifacts** (stepping, aliasing) must be preserved
- **Coordinate systems** (polar vs rectangular) must behave identically
- **Beat detection** and **variable persistence** must match original timing

### Interpolation in CoordinateGrid

The `CoordinateGrid` class uses a two-stage transformation matching original AVS:

**Stage 1 - Grid interpolation** (always bilinear):
- Script runs at sparse grid points (e.g., 16x16)
- For each output pixel, bilinearly interpolates between grid points
- Small grids create visible stepping artifacts

**Stage 2 - Pixel sampling** (controlled by `subpixel` flag):
- `subpixel=true`: Bilinear sample from source image (smooth)
- `subpixel=false`: Nearest neighbor from source (sharp/blocky)

## Implementation Status

### Completed
- Core parameter system with typed values
- Plugin registration and factory pattern
- UI layout system with data-driven controls (Windows-to-ImGui translation)
- RADIO_GROUP with typed enums
- Binary .avs preset loading with legacy index mapping
- BlurEffect (bit-shift optimized, matches original r_blur.cpp)
- BrightnessEffect (lookup table based, matches r_bright.cpp)
- ClearEffect (with blend modes)
- OnBeatClearEffect (beat-triggered clear with blend)
- DotGridEffect (animated color-cycling dot grid)
- DotFountainEffect (3D particle fountain with matrix transforms)
- WaterEffect (ripple distortion with neighbor averaging)
- OscilloscopeEffect (basic non-scriptable version)
- SuperScopeEffect (with POINT-phase scripting)
- CoordinateGrid with two-stage transformation
- MovementEffect (23 presets + custom scripting)
- DynamicMovementEffect (multi-phase scripting)
- EffectList (container for nested effects)
- SetRenderModeEffect (line drawing control with scripting)
- Line drawing module with Wu's anti-aliased algorithm
- MirrorEffect, ShiftEffect, ScatterEffect
- RotoBlitterEffect (rotation/zoom transforms)
- InterferencesEffect, MovingParticleEffect
- CustomBpmEffect, ColorFadeEffect, FadeoutEffect
- DDMEffect (Dynamic Distance Modifier)
- BumpEffect (bump mapping)

### Planned / Not Yet Implemented
- Full NS-EEL compatibility (current parser covers common subset)
- Remaining ~25 AVS effects
- Global frame buffer system (Buffer Save effect)
- Preset transitions

## Performance Considerations

- **MovementEffect**: O(width × height) script evaluations per frame
- **DynamicMovementEffect**: O(gridWidth × gridHeight) script evaluations + interpolation
- **BlurEffect**: O(n) single-pass with bit-shift division, auto-vectorizes to NEON/SSE
- **Script compilation**: Cache compiled scripts until parameters change
- **Memory usage**: Pre-allocate lookup tables, reuse when possible

### Optimisation Strategy

The original AVS used hand-written x86 MMX assembly for performance-critical effects. Our approach instead relies on:

1. **Bit-manipulation techniques** from original AVS (e.g., channel-isolated division via masks)
2. **Compiler auto-vectorization** with `-O3 -mtune=native`
3. **NEON intrinsics** as a fallback if auto-vectorization proves insufficient

This achieves comparable performance without platform-specific assembly, confirmed by inspecting compiler output which shows ARM NEON instructions (`ld4.4s`, `ushr.4s`, `add.4s`) being generated automatically.

See **[OPTIMISATION.md](OPTIMISATION.md)** for detailed analysis including bit-shift division masks, sample assembly output, NEON intrinsics reference, and SMP/parallel rendering strategy.

## Testing Strategy

### What We Test

Tests focus on **deterministic, verifiable components**:
- **Coordinate transformations** - polar/rectangular conversion accuracy
- **Interpolation algorithms** - bilinear sampling correctness
- **Script parsing** - expression evaluation, variable handling
- **Parameter systems** - value storage, range clamping, type conversion
- **Lookup table generation** - grid coordinate calculations

### What We Don't Test

**Pixel-level effect output is not tested directly.** The final rendered output of effects depends on:
- Floating-point accumulation across many operations
- Interpolation edge cases at image boundaries
- Interaction between multiple effects in a chain
- Audio input values that vary per frame

Predicting exact pixel values would require duplicating the entire rendering logic in tests, which provides little value. Instead, visual verification is done manually by comparing output against original AVS.

### Test Types

1. **Unit tests** - Parameter handling, script compilation, coordinate math
2. **Visual verification** - Manual comparison with original AVS output
3. **Performance tests** - Ensure real-time capability (not yet automated)

## Audio Data Format

### AudioData Structure

AVS uses a fixed audio data format:
```cpp
typedef char AudioData[2][2][576];
// [spectrum/waveform][left/right][samples]
// visdata[0] = spectrum (unsigned 0-255)
// visdata[1] = waveform (signed, XOR 128 for unsigned)
```

### Original Winamp/AVS FFT Pipeline

Investigation of the original Winamp source code revealed a two-stage pipeline:

#### Stage 1: Winamp FFT Generation (`Winamp/VIS.cpp`)

**FFT Parameters:**
| Parameter | Value |
|-----------|-------|
| Input samples | 512 |
| FFT order | 9 (2^9 = 512) |
| Window function | Hann |
| DC filter | High-pass (y = x - x1 + 0.99 * y1) |
| FFT output bins | 256 complex |

**Spectrum Expansion (256 → 576 bins):**
```cpp
// VIS.cpp lines 723-742
for (x = 0; x < 256; x++) {
    float sinT = wavetrum[x*2];
    float cosT = wavetrum[x*2+1];
    float thisValue = sqrt(sinT*sinT + cosT*cosT) / 16.0f;

    FASTMIN(thisValue, 255.f);
    data[data_offs++] = lrint((thisValue + la)/2.f);  // smoothed (avg with prev)
    data[data_offs++] = lrint(thisValue);              // raw value
    la = thisValue;
}
// Fill remaining 64 slots (576-512) with decaying values
while ((data_offs % 576) != 0) {
    la /= 2;
    data[data_offs++] = lrint(la);
}
```

The 576 bins come from:
1. Each of 256 FFT bins outputs 2 values (smoothed + raw) = 512 values
2. Remaining 64 high-frequency slots filled with exponentially decaying values

#### Stage 2: AVS Log Table Processing (`vis_avs/main.cpp`)

AVS applied a logarithmic lookup table to compress dynamic range:
```cpp
// Log table creation (base ~60)
for (x = 0; x < 256; x++) {
    double a = log(x * 60.0 / 255.0 + 1.0) / log(60.0);
    int t = (int)(a * 255.0);
    g_logtab[x] = (unsigned char)t;
}

// Application to spectrum data from Winamp
g_visdata[0][0][x] = g_logtab[(unsigned char)this_mod->spectrumData[0][x]];
```

**Optional Peak Hold**: When `g_visdata_pstat` was false, spectrum values only increased:
```cpp
int t = g_logtab[(unsigned char)this_mod->spectrumData[0][x]];
if (g_visdata[0][0][x] < t)
    g_visdata[0][0][x] = t;
```

### Current Implementation (ofxAVS)

The ofxAVS addon supports two modes controlled by `#define AVS_ENHANCED_FFT`:

#### Enhanced Mode (default, `AVS_ENHANCED_FFT` defined)
- FFT size: 2048 samples (higher resolution)
- Linear interpolation for bin mapping to 576 output values
- Temporal smoothing with attack/decay envelope
- dB scale normalization with 80dB range

#### Original Mode (`AVS_ENHANCED_FFT` not defined)
- FFT size: 512 samples (matching original Winamp)
- 256 FFT bins expanded to 576 using Winamp's algorithm
- Log table compression (base ~60) matching original AVS
- No temporal smoothing

### Source Code References

- **Winamp FFT generation**: `winamp/Src/Winamp/VIS.cpp` lines 679-790
- **Winamp FFT function**: `winamp/Src/Winamp/fft.h` - fft_9() for 512-sample FFT
- **AVS log table**: `vis_avs/avs/vis_avs/main.cpp` lines 243-287
- **ofxAVS implementation**: `src/ofxAVS.cpp` `audioIn()` method

## Rendering Chain and Pixel Format

### Pixel Format

Each pixel is a 32-bit integer in ARGB format:
- Bits 0-7: Blue
- Bits 8-15: Green
- Bits 16-23: Red
- Bits 24-31: Alpha

In memory (little-endian): `0xAARRGGBB` stored as bytes `[B, G, R, A]`.

### Alpha Channel Handling

Alpha propagates through the effect chain and can be used by effects for internal compositing:

1. **Effects CAN read alpha** from the framebuffer (set by previous effects)
2. **Effects CAN write alpha** for subsequent effects to use
3. **BLEND macro preserves alpha** - original `r_defs.h` explicitly handles the alpha byte
4. **At final display**: `Renderer::render()` forces alpha to 0xFF when copying to the output buffer

**Original AVS Behavior:**
- Cleared framebuffer with `memset(framebuffer, 0, ...)` - alpha becomes 0x00
- Displayed with `BitBlt(SRCCOPY)` which ignores alpha entirely
- Effects could use alpha internally, but it had no effect on final display

**Our Port:**
- Matches original AVS behavior during the effect chain
- Forces alpha to 0xFF only at the final output stage for OpenFrameworks compatibility
- Effects can use alpha for compositing/blending with subsequent effects

```cpp
// In Renderer::render() - final output stage only
for (size_t i = 0; i < temp_buffer.size(); ++i) {
    output_buffer[i] = static_cast<uint32_t>(temp_buffer[i]) | 0xFF000000;
}
```

### Effect Return Values

Each effect's `render()` function returns an integer indicating where the result was written:
- **Return 0**: Result is in `framebuffer` (input buffer, modified in place)
- **Return 1**: Result is in `fbout` (output buffer, requires swap)

The container (`EffectList` or `EffectListRoot`) handles buffer swapping:
```cpp
int result = child->render(visdata, isBeat, current_in, current_out, w, h);
if (result == 1) {
    std::swap(current_in, current_out);
}
```

At the end, if `current_in != framebuffer`, the result is copied back to `framebuffer`.

### Buffer Clearing

When "Clear every frame" is enabled on an EffectList, it clears to transparent black (0x00000000), matching original AVS:
```cpp
memset(framebuffer, 0, w * h * sizeof(uint32_t));
```

This is intentional - alpha is not forced here. Effects in the chain may use or ignore alpha as needed.

## Key Implementation Notes

Research into the original AVS codebase revealed several important behaviors that affect authenticity. See **[AVS_PARAMS.md](AVS_PARAMS.md)** for complete documentation of global parameters and UI.

### Beat Detection Algorithm

The original beat detection is surprisingly simple - no FFT-based onset detection or tempo tracking:

```cpp
// Sum absolute waveform values for energy
for (x = 0; x < 576; x++) {
    int r = *f++ ^ 128;  // Convert unsigned to signed
    r -= 128;
    if (r < 0) r = -r;
    lt[ch] += r;
}

// Beat triggers when energy exceeds decaying threshold
if (lt[0] >= (beat_peak1 * 34) / 32 && lt[0] > (576 * 16)) {
    avs_beat = 1;
}
```

The "advanced" BPM mode (`cfg_smartbeat`) adds prediction and sticky beat locking, but the fundamental trigger is energy threshold comparison.

**Status:** Not yet implemented in avs_lib. Would be straightforward to add.

### Peak Hold Mode

The `g_visdata_pstat` flag controls spectrum decay behavior:
- **Normal mode** (`pstat=1`): Spectrum values update freely each frame
- **Peak hold mode** (`pstat=0`): Values only increase, never decrease

This creates the "sticky" spectrum appearance where peaks linger. The flag toggles each frame after the render thread copies the data.

**Status:** Not implemented. Our spectrum always updates freely.

### Global Frame Buffers

AVS provides 8 global frame buffers (`NBUF=8` in r_defs.h) that effects can use for storing and retrieving frames:

```cpp
// From rlib.cpp
void *g_n_buffers[NBUF];          // Buffer pointers
int g_n_buffers_w[NBUF];          // Buffer widths
int g_n_buffers_h[NBUF];          // Buffer heights

void *getGlobalBuffer(int w, int h, int n, int do_alloc);
```

Used by:
- **Buffer Save effect** (`r_stack.cpp`): Save/restore framebuffer with blend modes
- **Effect Lists** (`r_list.cpp`): Save/restore buffer state when entering/exiting nested lists
- **Dynamic Movement** (`r_dmove.cpp`): For coordinate caching
- **Bump effect** (`r_bump.cpp`): For depth buffer

The Buffer Save effect exposes these to users with operations (save/restore/alternate) and blend modes (replace, 50/50, additive, every other line, subtractive, XOR, max, min, multiply, adjustable).

**Status:** Not implemented. Required for complex layered presets.

### Effect List Code

Effect lists (containers) can have their own init/frame expressions, not just the effects inside them:

```cpp
// From r_list.cpp - effect lists have code too
use_code;           // Whether list code is enabled
effect_exp[0];      // Init expression
effect_exp[1];      // Frame expression
```

This adds a scripting layer at the container level that runs before child effects.

**Status:** Not implemented. Our effect lists are pure containers.

### Preset File Format

Binary format starting with signature `"Nullsoft AVS Preset 0.2\x1a"`:
- Each effect serializes its own config blob via `save_config()`/`load_config()`
- Effect index (4 bytes) maps to effect type via `legacy_index`
- Nested effect lists serialize recursively
- APE (third-party) effects use 32-byte string identifiers
- Unimplemented effects are loaded as `UnsupportedEffect` placeholders

**Status:** ✅ Implemented in `core/preset.cpp`. The `avs2json` tool can dump presets to JSON for debugging.

### Transitions System

More sophisticated than simple crossfades:
1. **Pre-initialization**: Next preset renders in background before transition starts
2. **Transition animation**: Configurable duration (250ms - 8s) and style
3. **Keep rendering**: Option to continue old effect during transition

Controlled by `cfg_transitions`, `cfg_transitions2`, `cfg_transitions_speed`.

**Status:** Not implemented.

### Thread Priority

Original AVS had explicit render thread priority control (`cfg_render_prio`) to prevent visualization from interfering with audio playback. Options: Same as Winamp, Idle, Lowest, Normal, Highest.

**Status:** Not applicable - modern systems handle this differently.

### The 576 Constant

The magic number 576 appears throughout AVS. It originates from Winamp's VIS API:
- 256 FFT bins × 2 (smoothed + raw) = 512 values
- Plus 64 high-frequency padding with exponential decay
- Total: 576 samples per channel

This is Winamp-specific, not a standard audio buffer size.

## Tools

### avs2json

Command-line tool for converting binary `.avs` presets to JSON format:

```bash
cd libs/avs_lib/tools/build
cmake .. && make
./avs2json path/to/preset.avs
```

Output includes:
- Effect chain hierarchy with nested effect lists
- All parameter values for each effect
- Unsupported effects are marked with their original type name

Useful for:
- Debugging preset loading issues
- Understanding preset structure
- Comparing original vs loaded parameters
