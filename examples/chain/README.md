# ofxAVS Example

## Building

**IMPORTANT**: Always use the OF_ROOT environment variable to specify the openFrameworks path:

```bash
export OF_ROOT=/path/to/your/openFrameworks
cd /path/to/ofxAVS/example
make
```

Do NOT modify config.make to hardcode paths - use the environment variable instead to keep the repo clean.

## Running

The example demonstrates an oscilloscope effect combined with dynamic movement and feedback:

1. **Clear effect**: Provides feedback trails (only_first=true)
2. **Oscilloscope**: Draws audio waveform visualization  
3. **Dynamic Movement**: Applies grid-based transformations (default spiral effect)

### Controls

- **d**: Reload oscilloscope + dynamic movement chain
- **1-4**: Select individual effects
- **c**: Clear all effects  
- **SPACE**: Toggle auto-cycling
- **r**: Add random effect

The effect chain creates classic AVS-style visuals with stepped transformation artifacts due to the grid-based evaluation in Dynamic Movement.