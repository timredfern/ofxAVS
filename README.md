# ofxAVS - Advanced Visualization Studio for OpenFrameworks

A modern C++ port of the legendary Advanced Visualization Studio (AVS) as a self-contained OpenFrameworks addon.

## What is AVS?

Advanced Visualization Studio was a groundbreaking music visualization plugin for Winamp, created by Justin Frankel. It featured:

- **Modular effect chains** - Chain together multiple visual effects
- **Real-time audio responsiveness** - Effects react to music spectrum and beats  
- **Scripting engine** - User-programmable effects with mathematical expressions
- **Preset system** - Share and load complete visualizations
- **Legendary effects** - Superscope, Dynamic Movement, and 50+ iconic effects

This project ports the complete AVS system to modern C++ while maintaining compatibility with original presets and effect behavior.

## Features

### Phase 1 Complete ✅
- **Cross-platform core library** - Pure C++17, no external dependencies
- **Original audio format** - Perfect compatibility with AVS effect algorithms  
- **Modern parameter system** - Type-safe parameters with UI integration
- **Plugin architecture** - Extensible effect system
- **3 effects ported**: Clear, Oscilloscope, Blur
- **Standalone examples** - Test library without OpenFrameworks

### In Progress 🚧
- **Script engine architecture** - Multi-phase execution system (init/frame/beat/pixel/point)
- **Transform effect refactoring** - Split into MovementEffect and DynamicMovementEffect
- **Coordinate systems** - Full-resolution vs grid-based interpolation
- **AVS compatibility system** - Exact preset loading and effect behavior

### Future Phases 📋
- **Complete effect library** - All 54+ original effects with authentic behavior
- **NS-EEL scripting engine** - Complete expression evaluator with all functions
- **OpenFrameworks integration** - Real-time GUI and audio input
- **UI architecture** - Effect chain editing and parameter controls

## Architecture

```
ofxAVS/
├── src/
│   ├── ofxAVS.h/cpp           # 🚧 Future: OF integration layer
├── libs/avs_lib/              # ✅ Complete embedded AVS library
│   ├── ARCHITECTURE.md        # 📋 Complete architecture documentation
│   ├── EFFECTS.md             # 📋 Catalog of all 54+ AVS effects
│   ├── SCRIPT_ARCHITECTURE.md # 📋 Script engine design
│   ├── core/                  # Core rendering engine
│   │   ├── script/            # 🚧 Multi-phase script execution
│   │   ├── transforms/        # 🚧 Coordinate transformation utilities
│   │   ├── renderer.h         # Main effect chain processor
│   │   ├── effect_base.h      # Base class for all effects
│   │   ├── parameter.h        # Modern parameter system
│   │   └── plugin_manager.h   # Effect registration & creation
│   ├── effects/               # 🚧 Refactored AVS effects
│   │   ├── movement_effect.h          # Trans/Movement (23 presets + custom)
│   │   ├── dynamic_movement_effect.h  # Trans/Dynamic Movement (grid-based)
│   │   ├── superscope_effect.h        # Render/SuperScope (point rendering)
│   │   ├── clear_effect.h             # ✅ Screen clearing
│   │   ├── oscilloscope_effect.h      # ✅ Waveform visualization
│   │   └── blur_effect.h              # ✅ Box blur
│   └── tests/                 # ✅ Comprehensive effect testing
└── addon_config.mk           # OpenFrameworks addon configuration
```

## Quick Start

### As OpenFrameworks Addon (Future - Phase 2)

```cpp
// ofApp.h
#include "ofxAVS.h"

class ofApp : public ofBaseApp {
    ofxAVS avs;
public:
    void setup() {
        avs.setup(ofGetWidth(), ofGetHeight());
        avs.addEffect("clear");
        avs.addEffect("oscilloscope"); 
        avs.addEffect("blur");
    }
    
    void draw() {
        avs.draw();
    }
    
    void audioIn(ofSoundBuffer& input) {
        avs.audioIn(input);
    }
};
```

### As Standalone Library (Available Now)

```cpp
#include "avs_lib/core/renderer.h"
#include "avs_lib/core/builtin_effects.h"

int main() {
    // Initialize
    avs::register_builtin_effects();
    avs::Renderer renderer(640, 480);
    
    // Build effect chain
    auto& pm = avs::PluginManager::instance();
    renderer.add_effect(pm.create_effect("clear"));
    renderer.add_effect(pm.create_effect("oscilloscope"));
    renderer.add_effect(pm.create_effect("blur"));
    
    // Render frame
    char audio_data[2][2][576]; // Fill with audio data
    uint32_t framebuffer[640 * 480];
    renderer.render(audio_data, false, framebuffer);
    
    return 0;
}
```

## Building

### Test Standalone Library
```bash
cd src/avs_lib/examples/standalone
mkdir build && cd build
cmake .. && make
./avs_example  # Generates frame_0.ppm through frame_4.ppm
```

### As OpenFrameworks Addon
```bash
# Clone into your OF addons directory
cd openFrameworks/addons
git clone [repo-url] ofxAVS

# Add to your project's addons.make
echo "ofxAVS" >> addons.make
```

## Design Philosophy

### 1. **Preserve Original Behavior**
- Keep exact audio data format: `char visdata[2][2][576]`
- Maintain effect render function signatures for easy porting
- Support original preset loading and parameter values

### 2. **Modern C++ Architecture**  
- RAII memory management, no raw pointers
- Type-safe parameter system with runtime introspection
- Cross-platform with no external dependencies
- Framework-agnostic core library

### 3. **Community-Driven Porting**
- Simple effect porting process (mostly copy-paste + parameter setup)
- Automatic registration system
- Clear separation between library and framework integration

## Effect Porting Status

| Effect | Status | Complexity | Notes |
|--------|--------|------------|-------|
| Clear | ✅ Done | Low | Screen clearing with blend modes |
| Oscilloscope | ✅ Done | Low | Audio waveform visualization |  
| Blur | ✅ Done | Medium | Box blur with configurable strength |
| Movement | 🚧 In Progress | High | 23 presets + custom scripting, full-resolution lookup |
| Dynamic Movement | 🚧 In Progress | High | Grid-based coordinate interpolation |
| SuperScope | 🚧 In Progress | High | Point-phase audio rendering with scripting |
| Simple Spectrum | 📋 Planned | Medium | Basic spectrum visualization |
| Water | 📋 Planned | High | Physics simulation with scripting |

**Current Focus**: Establish script engine architecture and coordinate transformation systems before expanding effect library.

## Contributing

The core library is designed for easy community contribution:

1. **Effect Porting**: Most original effects can be ported by copy-pasting render logic
2. **Testing**: Compare output against original Windows AVS
3. **Documentation**: Help document the extensive AVS effect ecosystem
4. **Presets**: Test with classic AVS preset collections

## Original Credits

- **Justin Frankel**: Creator of Advanced Visualization Studio
- **Nullsoft**: Original AVS development and open-source release (2005)
- **AVS Community**: 20+ years of amazing presets and effects

## License

BSD 3-Clause - matches original AVS license

## Development Status

**Phase 1 Complete** ✅ - Core library architecture and basic effects  
**Phase 2 In Progress** 🚧 - Script engine architecture and transform effect refactoring  
**Phase 3 Planned** 📋 - Complete effect library with authentic AVS behavior  
**Phase 4 Future** 🔮 - OpenFrameworks integration and UI architecture  

---

*Bringing the magic of AVS to modern creative coding platforms* ✨