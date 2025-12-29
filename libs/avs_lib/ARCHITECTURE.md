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
- `OscilloscopeEffect` → `r_oscstar.cpp` (Render/Oscilloscope)
- `BlurEffect` → `r_blur.cpp` (Trans/Blur)
- `BrightnessEffect` → `r_bright.cpp` (Trans/Brightness)
- `ClearEffect` → `r_clear.cpp` (Render/Clear Screen)

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
- `CoordinateLookupTable` class (sparse grid with interpolation) for DynamicMovementEffect

**Current Implementation:**
- `MovementEffect`: Internal `std::vector<int> lookup_table_` (full resolution)
- `DynamicMovementEffect`: Uses `CoordinateLookupTable` class (sparse grid)

**Rationale for change:** The full-resolution table was inlined into `MovementEffect` rather than extracted to a separate class. This keeps the simpler effect self-contained while the more complex grid-based interpolation in `DynamicMovementEffect` benefits from the separate `CoordinateLookupTable` utility class.

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
│       └── coordinate_lookup_table.cpp
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
│   ├── coordinate_lookup_table.cpp  # Grid-based transforms with interpolation
│   ├── builtin_effects.cpp     # Built-in effect presets
│   ├── parameter.cpp           # Parameter system
│   ├── plugin_manager.cpp      # Effect registration and factory
│   ├── effect_base.h           # Base effect interface
│   └── ui.h                    # UI layout definitions
├── effects/
│   ├── movement_effect.cpp         # Trans/Movement (r_trans.cpp)
│   ├── dynamic_movement_effect.cpp # Trans/Dynamic Movement (r_dmove.cpp)
│   ├── oscilloscope_effect.cpp     # Render/Oscilloscope (r_oscstar.cpp)
│   ├── blur_effect.cpp             # Trans/Blur (r_blur.cpp)
│   ├── brightness_effect.cpp       # Trans/Brightness (r_bright.cpp)
│   └── clear_effect.cpp            # Render/Clear Screen (r_clear.cpp)
├── example/                     # Standalone example (no OpenFrameworks)
└── tests/                       # Catch2 unit tests
```

**Rationale for changes:**
- `transforms/` subdirectory was not created; `coordinate_lookup_table.cpp` lives directly in `core/`
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
- Control types: CHECKBOX, SLIDER, BUTTON, RADIO_GROUP, TEXT_INPUT, EDITTEXT, COLOR_BUTTON, DROPDOWN
- Parameter binding via matching control ID to parameter name

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
- `BlurLevel` - NONE, LIGHT, MEDIUM, HEAVY
- `RoundMode` - DOWN, UP

## Script Execution System

### Script Phases

**Original Plan:**
```cpp
enum class ScriptPhase {
    INIT,    // Run once on effect creation/parameter change
    FRAME,   // Run once per frame
    BEAT,    // Run on beat detection
    PIXEL,   // Run per coordinate (screen pixel or grid point)
    POINT    // Run per data sample (audio/particles)
};
```

**Current Implementation:**
DynamicMovementEffect implements multi-phase execution internally:
- `execute_init_script()` - Run once when effect created/parameters change
- `execute_frame_script()` - Run once per frame
- `execute_beat_script()` - Run on beat detection
- `execute_pixel_script()` - Run per grid point

The `ScriptPhase` enum was not extracted as a shared abstraction. Each effect manages its own script phases as needed.

### Phase Usage by Effect Type
- **MovementEffect**: Single expression evaluation per pixel (no multi-phase)
- **DynamicMovementEffect**: INIT + FRAME + BEAT + PIXEL (full multi-phase)
- **OscilloscopeEffect**: No scripting (hardcoded rendering)

**Note:** SuperScopeEffect (with POINT phase for audio sample processing) is not yet implemented. OscilloscopeEffect is a simpler non-scriptable version.

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
    CoordinateLookupTable grid_table_;  // Sparse grid with interpolation

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

**Note:** The planned `SuperScopeEffect` with full POINT-phase scripting is not yet implemented.

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

### Interpolation Modes
The `CoordinateLookupTable` supports three modes for authentic AVS reproduction:
- `NONE` - No interpolation, creates classic stepped/blocky artifacts
- `LINEAR` - Bilinear interpolation for smooth transforms
- `NEAREST` - Nearest neighbor, sharp but less blocky than none

## Implementation Status

### Completed
- Core parameter system with typed values
- Plugin registration and factory pattern
- UI layout system with data-driven controls
- RADIO_GROUP with typed enums
- BlurEffect (bit-shift optimized, matches original r_blur.cpp)
- BrightnessEffect (lookup table based, matches r_bright.cpp)
- ClearEffect (with blend modes)
- OscilloscopeEffect (basic non-scriptable version)
- CoordinateLookupTable with interpolation modes
- MovementEffect (23 presets + custom scripting)
- DynamicMovementEffect (multi-phase scripting)

### Planned / Not Yet Implemented
- SuperScopeEffect with POINT-phase scripting
- Preset file loading (.avs format)
- Full NS-EEL compatibility (current parser covers common subset)
- Remaining ~40 AVS effects

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

See **[OPTIMISATION.md](OPTIMISATION.md)** for detailed analysis including bit-shift division masks, sample assembly output, and NEON intrinsics reference.

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
- Effect index (4 bytes) maps to effect type
- Nested effect lists serialize recursively
- APE (third-party) effects use 32-byte string identifiers

**Status:** Not implemented. Required for loading existing .avs presets.

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
