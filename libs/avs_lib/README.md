# AVS Library - Self-Contained Audio Visualization Engine

A modern C++ port of the Advanced Visualization Studio (AVS) core library. This is a **framework-agnostic** library that can be used standalone or embedded in any application framework.

## Overview

This library provides the complete AVS visualization engine with:

- **Original AVS compatibility** - preserves exact audio data format and effect behavior
- **Modern C++ design** - type-safe parameters, RAII memory management, clean interfaces
- **Zero external dependencies** - pure C++17 standard library only
- **Cross-platform** - works on Windows, macOS, Linux
- **Framework agnostic** - can be integrated with OpenFrameworks, Qt, JUCE, game engines, web, etc.

## Architecture

```
avs_lib/
├── core/                    # Core rendering engine
│   ├── renderer.h/cpp       # Main effect chain processor
│   ├── effect_base.h        # Base class for all effects
│   ├── parameter.h/cpp      # Type-safe parameter system
│   ├── plugin_manager.h/cpp # Effect registration and creation
│   └── builtin_effects.h/cpp # Built-in effect registration
├── effects/                 # Ported AVS effects
│   ├── clear_effect.h/cpp   # Screen clearing with blend modes
│   ├── oscilloscope_effect.h/cpp # Audio waveform visualization
│   └── blur_effect.h/cpp    # Configurable blur effect
└── examples/                # Standalone examples and tests
    └── standalone/          # Command-line demo application
```

## Quick Start

### Basic Usage

```cpp
#include "core/renderer.h"
#include "core/builtin_effects.h"

int main() {
    // Initialize library
    avs::register_builtin_effects();
    
    // Create renderer
    avs::Renderer renderer(640, 480);
    
    // Add effects to chain
    auto& pm = avs::PluginManager::instance();
    renderer.add_effect(pm.create_effect("clear"));
    renderer.add_effect(pm.create_effect("oscilloscope"));
    renderer.add_effect(pm.create_effect("blur"));
    
    // Render frame
    char audio_data[2][2][576];  // Fill with audio data
    uint32_t framebuffer[640 * 480];
    bool is_beat = false;
    
    renderer.render(audio_data, is_beat, framebuffer);
    
    // framebuffer now contains RGBA visualization
    return 0;
}
```

### Audio Data Format

AVS uses a specific audio format that's preserved for compatibility:

```cpp
// audio_data[channel][type][sample]
char audio_data[2][2][576];

// Channels: 0 = left, 1 = right
// Types: 0 = spectrum (frequency domain), 1 = waveform (time domain) 
// Samples: 576 data points

// Example: Set spectrum data for bass frequencies
audio_data[0][0][0] = 128;  // Left channel, spectrum, first frequency bin
audio_data[1][0][0] = 128;  // Right channel, spectrum, first frequency bin

// Example: Set waveform data
audio_data[0][1][0] = 200;  // Left channel, waveform, first sample
audio_data[1][1][0] = 100;  // Right channel, waveform, first sample
```

**Spectrum data (type=0)**: FFT output, 0-255 representing magnitude  
**Waveform data (type=1)**: Raw audio samples, 0-255 representing amplitude

## Working with Effects

### Available Effects

| Effect | ID | Description |
|--------|----|-----------| 
| Clear | `"clear"` | Fills screen with solid color, supports blend modes |
| Oscilloscope | `"oscilloscope"` | Draws audio waveforms as dots or connected lines |
| Blur | `"blur"` | Box blur with configurable strength and radius |

### Effect Parameters

Effects expose configurable parameters through a modern type-safe system:

```cpp
auto effect = pm.create_effect("blur");

// Get parameter info
auto& params = effect->parameters();
for (auto& [name, param] : params.all_parameters()) {
    std::cout << "Parameter: " << name 
              << " Type: " << (int)param->type()
              << " Value: " << param->as_float() << std::endl;
}

// Set parameters
params.set_float("strength", 0.8);
params.set_int("radius", 5);
params.set_bool("enabled", true);
params.set_color("color", 0xFF0000);  // Red

// Get parameters
double strength = params.get_float("strength");
bool enabled = params.get_bool("enabled");
```

### Creating Custom Effects

```cpp
#include "core/effect_base.h"

class MyEffect : public avs::EffectBase {
public:
    MyEffect() {
        // Set up parameters
        auto& params = parameters();
        params.add_parameter(std::make_shared<avs::Parameter>(
            "intensity", avs::ParameterType::FLOAT, 0.5, 0.0, 1.0));
    }
    
    int render(avs::AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override {
        
        double intensity = parameters().get_float("intensity");
        
        // Your effect logic here...
        // Return 0 to use framebuffer, 1 to use fbout
        
        return 0;
    }
    
    std::string get_name() const override { return "My Effect"; }
    std::string get_description() const override { return "Custom effect"; }
};

// Register effect
auto& pm = avs::PluginManager::instance();
avs::PluginInfo info;
info.name = "My Effect";
info.factory = []() { return std::make_unique<MyEffect>(); };
pm.register_effect("my_effect", info);
```

## Building Examples

### Standalone Example

The standalone example demonstrates the library without any framework dependencies:

```bash
# From the library directory (avs_lib/)
mkdir build && cd build
cmake ..
make
./avs_example
```

This will:
1. Initialize the AVS library
2. Create a 400x300 renderer  
3. Add Clear → Oscilloscope → Blur effect chain
4. Generate synthetic audio data (440Hz sine wave)
5. Render 5 frames to `frame_0.ppm` through `frame_4.ppm`

### View Generated Images

The example outputs PPM image files. To view them:

```bash
# Convert to PNG (requires ImageMagick)
convert frame_0.ppm frame_0.png

# Or view directly (macOS)
open frame_0.ppm

# Or view directly (Linux)  
eog frame_0.ppm
```

You should see:
- Black background (Clear effect)
- Green sine wave waveform (Oscilloscope effect) 
- Slight softening (Blur effect)

### Build Configuration

The library supports several build configurations:

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Release build (default)
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Disable examples
cmake -DBUILD_EXAMPLES=OFF ..
make
```

## Integration Examples

### With Custom Graphics

```cpp
// Custom graphics backend
class MyGraphics {
public:
    void upload_texture(uint32_t* pixels, int w, int h) {
        // Upload to GPU, display on screen, save to file, etc.
    }
};

// Usage
avs::Renderer renderer(1920, 1080);
MyGraphics graphics;
uint32_t framebuffer[1920 * 1080];

while (running) {
    char audio[2][2][576];
    get_audio_data(audio);  // Your audio input
    
    renderer.render(audio, detect_beat(), framebuffer);
    graphics.upload_texture(framebuffer, 1920, 1080);
}
```

### Headless Server

```cpp
// Render visualizations on server
#include "core/renderer.h"

class VisualizationServer {
    avs::Renderer renderer_{1280, 720};
    std::vector<uint32_t> buffer_;
    
public:
    std::vector<uint8_t> render_frame(const AudioData& audio) {
        renderer_.render(audio, false, buffer_.data());
        
        // Convert to PNG/JPEG and return as bytes
        return encode_image(buffer_, 1280, 720);
    }
};
```

## Performance Notes

- **Memory**: Pre-allocates framebuffers to avoid runtime allocation
- **Parameters**: String-based parameter lookup - cache pointers for hot paths
- **Effects**: Designed for 60fps real-time rendering
- **SIMD**: Future optimization target for blur and other pixel operations

## Compatibility

- **C++ Standard**: Requires C++17 or later
- **Compilers**: GCC 7+, Clang 5+, MSVC 2017+
- **Platforms**: Windows, macOS, Linux (and potentially mobile/web with minor adjustments)
- **Dependencies**: None beyond C++17 standard library

## Development Status

### Completed ✅
- Core rendering pipeline
- Parameter system  
- Plugin architecture
- 3 basic effects (Clear, Oscilloscope, Blur)
- Standalone example and build system

### In Progress 🚧
- Script engine refactoring for multi-phase execution
- Transform effect split (Movement vs Dynamic Movement)
- Coordinate lookup table optimization

### Planned 📋
- MovementEffect with 23 built-in presets + custom scripting
- DynamicMovementEffect with grid-based coordinate interpolation
- SuperScopeEffect with point-phase audio rendering
- Complete NS-EEL scripting system
- Original .avs preset loading and compatibility
- Complete effect library (54+ effects matching original AVS)
- UI architecture for effect chain editing

---

This library forms the foundation for bringing the complete AVS ecosystem to modern platforms while maintaining compatibility with 20+ years of community-created content.