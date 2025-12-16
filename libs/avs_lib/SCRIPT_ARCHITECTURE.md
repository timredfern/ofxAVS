# AVS Script Engine Architecture

## Core Components

### 1. ScriptContext
Manages variables and execution environment for a single effect instance.

```cpp
class ScriptContext {
    // Core variables available to all effects
    double w, h;          // screen dimensions  
    double t;             // time
    double b;             // beat detection (0 or 1)
    double fps;           // current framerate
    
    // Audio variables
    double *waveform;     // 576 samples
    double *spectrum;     // 576 samples
    
    // Coordinate variables (effect-specific)
    double x, y;          // rectangular coords
    double r, d;          // polar coords
    double i;             // index/counter
    
    // User-defined persistent variables
    std::map<std::string, double> user_vars;
    
    // Methods
    void reset();
    void updateAudioData(AudioData& data);
    void setCoordinates(double x, double y);
    void convertToPolar();
    void convertToRect();
};
```

### 2. ScriptPhase Enum
```cpp
enum class ScriptPhase {
    INIT,    // Run once on effect creation/parameter change
    FRAME,   // Run once per frame
    BEAT,    // Run on beat detection
    PIXEL,   // Run per coordinate (pixel or grid point)
    POINT    // Run per particle/oscilloscope point
};
```

### 3. ScriptExecutor
Core execution engine that compiles and runs EEL scripts.

```cpp
class ScriptExecutor {
private:
    AVS_EEL_IF eel_ctx;
    std::map<ScriptPhase, CompiledCode> compiled_scripts;
    ScriptContext* context;
    
public:
    // Compilation
    bool compile(ScriptPhase phase, const std::string& code);
    void clearPhase(ScriptPhase phase);
    
    // Execution
    void executeInit();
    void executeFrame();
    void executeBeat();
    void executePixel(double x, double y);  
    void executePoint(int index);
    
    // Variable binding
    void bindContext(ScriptContext* ctx);
    void registerVariable(const std::string& name, double* ptr);
};
```

### 4. Effect-Specific Script Adapters

#### TransformScriptAdapter (for Trans/Movement)
```cpp
class TransformScriptAdapter {
    ScriptExecutor executor;
    ScriptContext context;
    int* lookup_table;  // Full-resolution lookup table
    
    void generateLookupTable(int w, int h) {
        // Evaluate script for EVERY pixel
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                context.setCoordinates(x, y);
                executor.executePixel(x, y);
                // Store transformed coordinates in lookup_table
                lookup_table[y*w+x] = encodeCoords(context.x, context.y);
            }
        }
    }
};
```

#### DynamicMovementAdapter (for Trans/Dynamic Movement)
```cpp
class DynamicMovementAdapter {
    ScriptExecutor executor;
    ScriptContext context;
    CoordinateLookupTable* grid;  // Only Dynamic Movement uses this!
    
    void generateGrid(int w, int h, int gridW, int gridH) {
        grid->init(w, h, gridW, gridH);
        
        // Execute init/frame/beat phases
        executor.executeInit();
        executor.executeFrame();
        if (isBeat) executor.executeBeat();
        
        // Evaluate script only at grid points
        for (int gy = 0; gy < gridH; gy++) {
            for (int gx = 0; gx < gridW; gx++) {
                context.setCoordinates(
                    gx * w / (gridW-1), 
                    gy * h / (gridH-1)
                );
                executor.executePixel(context.x, context.y);
                grid->setGridPoint(gx, gy, context.x, context.y);
            }
        }
        
        // Grid handles interpolation internally
        grid->generateInterpolatedLookup();
    }
};
```

#### SuperScopeAdapter (for Render/SuperScope)
```cpp
class SuperScopeAdapter {
    ScriptExecutor executor;
    ScriptContext context;
    std::vector<Point> points;
    
    void render() {
        executor.executeInit();
        executor.executeFrame();
        if (isBeat) executor.executeBeat();
        
        // Execute point script for each audio sample
        for (int i = 0; i < numPoints; i++) {
            context.i = i;
            context.v = getAudioSample(i);
            executor.executePoint(i);
            points.push_back({context.x, context.y});
        }
    }
};
```

### 5. Variable Registration System

```cpp
class VariableRegistry {
    // Predefined variable sets for different effect types
    enum VariableSet {
        BASIC,      // w, h, t, b
        AUDIO,      // waveform, spectrum  
        COORDS_2D,  // x, y
        COORDS_POLAR, // r, d
        PARTICLE    // i, v, red, green, blue
    };
    
    void registerSet(ScriptContext* ctx, VariableSet set);
    void registerCustom(ScriptContext* ctx, const std::string& name);
};
```

## Key Design Principles

### 1. Phase Separation
- Each effect declares which phases it uses
- Scripts are compiled once per phase
- Execution order: init → frame → beat → pixel/point

### 2. Variable Scope Management  
- Context variables persist across phases within a frame
- User variables persist across frames
- Coordinate variables updated per pixel/point

### 3. Performance Optimization
- Compiled scripts cached per phase
- Variable pointers bound once at compilation
- Minimal overhead per execution

### 4. Coordinate System Flexibility
- Effects can work in rectangular or polar
- Automatic conversion functions available
- Normalized (-1 to 1) or pixel coordinates

### 5. CoordinateLookupTable Usage
- **ONLY** used by Trans/Dynamic Movement
- Other effects use different mechanisms:
  - Trans/Movement: Full-resolution int* lookup table
  - Render effects: Direct point generation
  - Filter effects: No coordinate transformation

## Integration Example

```cpp
// Trans/Dynamic Movement effect
class DynamicMovementEffect : public EffectBase {
    DynamicMovementAdapter script_adapter;
    CoordinateLookupTable grid;  // Only this effect needs it!
    
    void render(FrameBuffer& in, FrameBuffer& out) {
        // Generate sparse grid with interpolation
        script_adapter.generateGrid(
            in.width, in.height, 
            gridSize, gridSize
        );
        
        // Apply interpolated transformation
        grid.apply(in, out);
    }
};

// Trans/Movement effect  
class MovementEffect : public EffectBase {
    TransformScriptAdapter script_adapter;
    // No CoordinateLookupTable needed!
    
    void render(FrameBuffer& in, FrameBuffer& out) {
        // Generate full-resolution lookup
        script_adapter.generateLookupTable(in.width, in.height);
        
        // Apply transformation using lookup table
        applyLookup(in, out);
    }
};
```

## Benefits of This Architecture

1. **Clean Separation**: Each effect type has its own adapter
2. **No Waste**: CoordinateLookupTable only used where needed
3. **Flexibility**: Easy to add new effect types
4. **Performance**: Optimal path for each effect type
5. **Maintainability**: Clear which components each effect uses