# AVS Effects Catalog

Based on analysis of the original Winamp AVS and reference screenshots, this document catalogs the different effect types and their architectural requirements.

## Core Effect Categories

### 1. Render Effects (Generate pixels)
**Purpose**: Create new visual content from audio data  
**Data Flow**: Audio → Pixels  
**Examples**: 
- **SuperScope**: Multi-phase script execution (init/frame/beat/point) for drawing oscilloscope-like visualizations
  - Has separate init, frame, beat, and point script phases
  - Draws dots or lines based on audio waveform/spectrum data
  - Variables: x, y coordinates for drawing points

### 2. Transform Effects (Move/distort pixels) 
**Purpose**: Transform existing pixel data using coordinate mapping  
**Data Flow**: Pixels + Audio → Transformed Pixels

#### 2a. Trans/Movement (Full Resolution with Lookup Table)
- **Script**: Single expression or 23 built-in presets
- **Coordinate System**: Rectangular (x,y) or Polar (r,d), user selectable
- **Resolution**: Full pixel resolution - evaluates expression for EVERY pixel, stores in lookup table
- **Options**: Source map, Wrap, Blend, Bilinear filtering
- **Presets**: 23 built-in effects like "swirl", "tunneling", "bubbling outward"
- **Custom Script**: Modifies r,d (polar) or x,y (rectangular) coordinates per pixel
- **Example**: `d = d * (0.99 * (1.0 - sin(r-$PI*0.5) / 32.0)); r = r + (0.03 * sin(d * $PI * 4));`

#### 2b. Trans/Dynamic Movement (Grid-based with coordinate interpolation)
- **Script**: Multi-phase execution (init/frame/beat/pixel)
- **Coordinate System**: Rectangular or Polar, script controlled
- **Resolution**: Sparse grid evaluation (configurable 2x2 to 256x256, default 16x16)
- **Performance**: Much faster - evaluates script only at grid points, interpolates coordinates for all other pixels
- **Options**: Grid size selector, Blend, Wrap, Bilinear filtering, "No movement" mode
- **Key Feature**: This is the effect with the low-resolution grid and coordinate interpolation artifacts!

### 3. Filter Effects (Modify pixels in-place)
**Purpose**: Process existing pixels without coordinate transformation  
**Examples**: Blur, Water, Color Map, Unique Tone, Fadeout

### 4. Misc Effects (Utility/Control)
**Purpose**: Buffer management, rendering control  
**Examples**: Buffer Save, Set Render Mode, Comment

## Script Engine Architecture Requirements

Based on the effect analysis, we need:

### 1. Multi-Phase Script Execution
```
init    - Run once when effect is added/parameters change
frame   - Run once per frame 
beat    - Run when beat is detected
pixel   - Run per coordinate (full-res) or per grid point (dynamic)
point   - Run per audio sample (for render effects)
```

### 2. Coordinate Buffer System
For Dynamic Movement effects:
- **Grid Resolution**: Configurable (2x2 to 64x64 typical)
- **Coordinate Storage**: Store transformed x,y coordinates at grid points
- **Interpolation**: Bilinear interpolation between grid coordinates
- **Performance**: ~100x faster than full-resolution for complex scripts

### 3. Variable Context Management
Different effects need different variable sets:
- **Audio**: Spectrum data, waveform, beat detection, time
- **Geometry**: Screen dimensions (sw, sh), coordinate ranges
- **Transform**: Current pixel coordinates (x, y) or (r, d)
- **Custom**: User-defined persistent variables across frames

## Recommended Implementation Strategy

### Phase 1: Core Framework
1. **Script Engine**: Multi-phase execution system
2. **Variable System**: Audio + geometry context
3. **Coordinate Buffer**: Grid-based storage with interpolation

### Phase 2: Basic Effects  
1. **Trans/Movement**: Full-resolution coordinate transforms
2. **Trans/Dynamic Movement**: Grid-based coordinate transforms with interpolation
3. **Render/SuperScope**: Multi-phase rendering system

### Phase 3: Advanced Effects
1. Filter effects (Blur, Water, etc.)
2. Buffer management effects
3. Optimization and performance tuning

## Key Architectural Insights

1. **Two Transform Approaches**: Full-resolution vs grid-based with interpolation
2. **Script Phases**: Different effects use different execution phases
3. **Coordinate Interpolation**: Critical for performance in dynamic effects
4. **Variable Contexts**: Each effect type has specific variable requirements
5. **Buffer Management**: Effects can read from/write to different buffers

This architecture allows authentic AVS behavior while maintaining performance and extensibility.