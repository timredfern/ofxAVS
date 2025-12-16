# AVS Project Instructions for Claude

## Critical Build Requirements

**NEVER hardcode paths in config.make or other files. ALWAYS use environment variables:**

```bash
export OF_ROOT=/path/to/openFrameworks
```

This is required because:
1. It keeps the repository clean of filesystem-specific paths
2. Different developers use different openFrameworks locations  
3. Prevents accidental commits of hardcoded paths

## Building OpenFrameworks Examples

```bash
export OF_ROOT=/path/to/your/openFrameworks
cd example
make
```

## Project Context

This is an Advanced Visualization Studio (AVS) port to modern C++ and OpenFrameworks. The current focus is on implementing oscilloscope effects with dynamic movement and feedback for classic AVS-style visualizations.

Key components:
- Oscilloscope effect for audio waveform visualization
- Dynamic movement effect with grid-based transformations  
- Clear effect with feedback for trails
- Grid-based coordinate interpolation for authentic AVS stepping artifacts