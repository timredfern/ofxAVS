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
- `SuperScopeEffect` → `r_sscope.cpp` (Render/SuperScope)

### 2. Shared Script Infrastructure
Common scripting components are abstracted for reuse while maintaining compatibility:
- `EELExecutor` - NS-EEL compilation and execution
- `ScriptContext` - Variable management and execution state
- `ScriptVariables` - Standard variable registration (w, h, x, y, r, d, etc.)

### 3. Coordinate System Separation
Different transformation approaches use appropriate data structures:
- **MovementEffect**: `FullResolutionTable` (int* lookup, one entry per pixel)
- **DynamicMovementEffect**: `CoordinateLookupTable` (sparse grid with interpolation)
- **Render Effects**: Direct point generation (no coordinate transformation)

## Directory Structure

```
libs/avs_lib/
├── ARCHITECTURE.md              # This file
├── core/
│   ├── script/                  # Shared script execution
│   │   ├── eel_executor.cpp     # NS-EEL compilation/execution
│   │   ├── script_context.cpp   # Variable management & state
│   │   └── script_variables.cpp # Standard variable registration
│   ├── transforms/              # Coordinate transformation utilities
│   │   ├── full_resolution_table.cpp    # For MovementEffect
│   │   └── coordinate_lookup_table.cpp  # For DynamicMovementEffect
│   ├── parameter.cpp            # Parameter system (existing)
│   └── effect_base.h           # Base effect interface (existing)
├── effects/
│   ├── movement_effect.cpp          # Trans/Movement (r_trans.cpp)
│   ├── dynamic_movement_effect.cpp  # Trans/Dynamic Movement (r_dmove.cpp)
│   ├── superscope_effect.cpp       # Render/SuperScope (r_sscope.cpp)
│   └── blur_effect.cpp             # Filter/Blur (existing)
└── tests/
    └── [effect-specific tests]
```

## Script Execution System

### Script Phases
Effects use different combinations of script execution phases:

```cpp
enum class ScriptPhase {
    INIT,    // Run once on effect creation/parameter change
    FRAME,   // Run once per frame  
    BEAT,    // Run on beat detection
    PIXEL,   // Run per coordinate (screen pixel or grid point)
    POINT    // Run per data sample (audio/particles)
};
```

### Phase Usage by Effect Type
- **MovementEffect**: PIXEL only (evaluates expression per pixel)
- **DynamicMovementEffect**: INIT + FRAME + BEAT + PIXEL (full multi-phase)
- **SuperScopeEffect**: INIT + FRAME + BEAT + POINT (audio sample processing)

### Variable Context
Each effect manages variables through `ScriptContext`:

```cpp
class ScriptContext {
    // Core variables (all effects)
    double w, h;              // Screen dimensions
    double t;                 // Time
    double b;                 // Beat detection
    
    // Audio variables  
    double *waveform;         // 576 audio samples
    double *spectrum;         // 576 spectrum samples
    
    // Coordinate variables (transform effects)
    double x, y;              // Rectangular coordinates
    double r, d;              // Polar coordinates
    
    // Index variables (render effects)
    double i;                 // Sample/particle index
    double v;                 // Audio value
    
    // User-defined persistent variables
    std::map<std::string, double> user_vars;
};
```

## Effect Implementation Patterns

### Transform Effects (MovementEffect)
```cpp
class MovementEffect : public EffectBase {
    EELExecutor script_executor;
    ScriptContext script_context;
    FullResolutionTable lookup_table;  // int* table, one per pixel
    
    void render(FrameBuffer& in, FrameBuffer& out) {
        // Generate full-resolution lookup table
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                script_context.setCoordinates(x, y);
                script_executor.executePixel();
                lookup_table.store(x, y, script_context.x, script_context.y);
            }
        }
        
        // Apply transformation using lookup
        lookup_table.apply(in, out);
    }
};
```

### Dynamic Transform Effects (DynamicMovementEffect)
```cpp
class DynamicMovementEffect : public EffectBase {
    EELExecutor script_executor;
    ScriptContext script_context;
    CoordinateLookupTable grid_table;  // Sparse grid with interpolation
    
    void render(FrameBuffer& in, FrameBuffer& out) {
        // Execute frame-level scripts
        script_executor.executeInit();
        script_executor.executeFrame();
        if (isBeat) script_executor.executeBeat();
        
        // Generate sparse grid (e.g., 16x16)
        for (int gy = 0; gy < gridHeight; gy++) {
            for (int gx = 0; gx < gridWidth; gx++) {
                script_context.setGridCoordinates(gx, gy, gridWidth, gridHeight);
                script_executor.executePixel();
                grid_table.setGridPoint(gx, gy, script_context.x, script_context.y);
            }
        }
        
        // Apply with interpolation
        grid_table.applyWithInterpolation(in, out);
    }
};
```

### Render Effects (SuperScopeEffect)  
```cpp
class SuperScopeEffect : public EffectBase {
    EELExecutor script_executor;
    ScriptContext script_context;
    std::vector<Point> render_points;
    
    void render(AudioData& audio, FrameBuffer& out) {
        // Execute frame-level scripts
        script_executor.executeInit();
        script_executor.executeFrame();
        if (isBeat) script_executor.executeBeat();
        
        // Process audio samples
        render_points.clear();
        for (int i = 0; i < 576; i++) {
            script_context.i = i;
            script_context.v = audio.waveform[i];
            script_executor.executePoint();
            render_points.push_back({script_context.x, script_context.y});
        }
        
        // Draw points/lines
        drawPoints(render_points, out);
    }
};
```

## Compatibility Strategy

### Preset Loading
Each effect maintains the exact parameter format of its original AVS counterpart:

```cpp
// MovementEffect preset format (matches r_trans.cpp)
struct MovementPreset {
    int effect_index;        // 0-23 for built-ins, 32767 for custom
    std::string custom_code; // Custom expression if effect_index == 32767
    bool rectangular;        // Coordinate system flag
    bool source_mapped;      // Rendering option
    // ... other r_trans.cpp options
};

// DynamicMovementPreset format (matches r_dmove.cpp)  
struct DynamicMovementPreset {
    std::string init_code;   // Init phase script
    std::string frame_code;  // Frame phase script  
    std::string beat_code;   // Beat phase script
    std::string pixel_code;  // Pixel phase script
    int grid_width, grid_height;  // Grid resolution
    // ... other r_dmove.cpp options
};
```

### Visual Fidelity
Key requirements for authentic AVS behavior:
- **Movement effects** must evaluate scripts at full resolution
- **Dynamic Movement** must use sparse grid evaluation with coordinate interpolation
- **Rendering artifacts** (stepping, aliasing) must be preserved
- **Coordinate systems** (polar vs rectangular) must behave identically
- **Beat detection** and **variable persistence** must match original timing

## Implementation Phases

### Phase 1: Core Infrastructure
1. Split existing `TransformEffect` into `MovementEffect` and `DynamicMovementEffect`
2. Create shared script components (`EELExecutor`, `ScriptContext`, `ScriptVariables`)
3. Create `FullResolutionTable` for `MovementEffect`

### Phase 2: Minimal Effect Implementation
1. Implement `MovementEffect` with 2-3 presets + custom scripting
2. Implement `DynamicMovementEffect` with basic 4-phase execution
3. Implement `SuperScopeEffect` with point-phase rendering

### Phase 3: Validation
1. Load and test existing AVS presets
2. Verify visual output matches original AVS
3. Benchmark performance against requirements

### Phase 4: UI Integration
1. Design effect chain management system
2. Implement real-time parameter editing
3. Create preset browser and save/load functionality

## Performance Considerations

- **MovementEffect**: O(width × height) script evaluations per frame
- **DynamicMovementEffect**: O(gridWidth × gridHeight) script evaluations + interpolation
- **Script compilation**: Cache compiled scripts until parameters change
- **Memory usage**: Pre-allocate lookup tables, reuse when possible
- **Threading**: Most effects support multi-threading via original AVS SMP interface

## Testing Strategy

Each effect should have:
1. **Unit tests** for parameter handling and script compilation
2. **Integration tests** for preset loading and visual output verification
3. **Performance tests** to ensure real-time capability
4. **Compatibility tests** using known AVS presets with expected output

This architecture ensures we can recreate the full AVS experience while maintaining clean, maintainable code that developers can understand and extend.