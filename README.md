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

### Future Phases 🚧
- **NS-EEL scripting engine** - Complete expression evaluator port
- **Original preset loading** - Load classic .avs files
- **Complete effect library** - All 50+ original effects
- **OpenFrameworks integration** - Real-time GUI and audio

## Architecture

```
ofxAVS/
├── src/
│   ├── ofxAVS.h/cpp           # 🚧 Future: OF integration layer
│   └── avs_lib/               # ✅ Complete embedded AVS library
│       ├── core/              # Core rendering engine
│       │   ├── renderer.h     # Main effect chain processor
│       │   ├── effect_base.h  # Base class for all effects
│       │   ├── parameter.h    # Modern parameter system
│       │   └── plugin_manager.h # Effect registration & creation
│       ├── effects/           # Ported AVS effects
│       │   ├── clear_effect.h
│       │   ├── oscilloscope_effect.h  
│       │   └── blur_effect.h
│       └── examples/          # Standalone library examples
│           └── standalone/    # PPM image output demo
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
| Brightness | 🔧 Next | Low | Simple pixel intensity adjustment |
| Movement | 🔧 Next | Medium | Coordinate transformations |
| Superscope | 🚧 Later | High | Requires NS-EEL scripting engine |

**Goal**: Port the 20 most popular effects that cover 90% of preset usage.

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
**Phase 2 In Progress** 🚧 - OpenFrameworks integration  
**Phase 3 Planned** 📋 - NS-EEL scripting and preset loading  
**Phase 4 Future** 🔮 - Complete effect library and community tools  

---

*Bringing the magic of AVS to modern creative coding platforms* ✨