# ofxAVS

OpenFrameworks addon for AVS (Advanced Visualization Studio) - bringing Nullsoft's legendary Winamp visualizer to creative coding.

## What is AVS?

Advanced Visualization Studio was the iconic music visualization plugin for Winamp, created by Justin Frankel in the late 1990s. It featured modular effect chains, real-time audio reactivity, and a scripting engine that spawned thousands of community-created presets.

This project is a modern C++ port that maintains compatibility with original AVS presets while bringing the visualizer to macOS and creative coding platforms.

## Features

- **46+ effects ported** from the original AVS
- **Full preset compatibility** - Load original .avs preset files
- **Real-time audio input** - Microphone or audio file playback
- **ImGui-based UI** - Effect chain editing with parameter controls
- **Beat detection** - Automatic BPM detection and beat-triggered effects
- **Session persistence** - Effect chains saved/restored between sessions
- **Audio device selection** - Choose input/output devices at runtime

## Screenshots

The addon provides a complete visualization environment:
- Effect chain panel with drag-drop reordering
- Parameter controls matching original AVS dialogs
- Audio controls with device selection
- Real-time 600x600 visualization output

## Quick Start

### Prerequisites

- OpenFrameworks 0.12+
- C++17 compiler
- Required addons:
  - [ofxImGui](https://github.com/jvcleave/ofxImGui) - UI rendering
  - [ofxFft](https://github.com/kylemcdonald/ofxFft) - FFT audio processing
  - [ofxAudioDecoder](https://github.com/kylemcdonald/ofxAudioDecoder) - Audio file playback

### Building

```bash
# Set OpenFrameworks path
export OF_ROOT=/path/to/openFrameworks

# Clone into addons directory
cd $OF_ROOT/addons
git clone --recursive ssh://git@git.eclectronics.org:2222/timredfern/ofxAVS.git

# Build the chain example
cd ofxAVS/examples/chain
make
make run
```

### Usage

```cpp
#include "ofxAVS.h"
#include "ofxImGui.h"

class ofApp : public ofBaseApp {
    ofxAVS avs;
    ofxImGui::Gui gui;

    void setup() {
        gui.setup();
        avs.setup();  // Loads last session automatically
    }

    void update() {
        avs.update();
    }

    void draw() {
        avs.draw(0, 0, 600, 600);

        gui.begin();
        avs.drawUI();  // Effect chain + parameter panels
        gui.end();
    }

    void audioIn(ofSoundBuffer& buffer) {
        avs.audioIn(buffer);
    }
};
```

## Project Structure

```
ofxAVS/
├── src/                    # OpenFrameworks addon layer
│   ├── ofxAVS.cpp/h        # Main addon: rendering, audio, UI
│   └── AVSui.cpp/h         # ImGui control rendering
├── libs/avs_lib/           # Portable AVS library (no dependencies)
│   ├── core/               # Renderer, effects, parameters
│   ├── effects/            # 46+ effect implementations
│   └── tests/              # Unit tests
├── examples/
│   ├── chain/              # Full-featured example with UI
│   └── simple/             # Minimal example
└── scripts/
    └── package-app.sh      # macOS app packaging
```

## Implemented Effects

### Rendering
Blur, Brightness, Bump, Channel Shift, Clear, Color Fade, Color Reduction, Fast Brightness, Grain, Invert, Mosaic, Interferences, Interleave, Mirror, Multiplier, Scatter, Shift, Unique Tone

### Transforms
Dynamic Movement, Movement, Rotoblitter

### Visualization
Dot Fountain, Dot Grid, Dot Plane, Oscilloscope, OscStar, Picture, Ring, RotStar, Starfield, Starfield Extended, SuperScope, Timescope, Moving Particle, Water

### Containers
Effect List, Effect List Root

### Timing
Custom BPM, Fadeout, OnBeat Clear, Set Render Mode

### Trans
Bass Spin, Blitter Feedback, DDM

## Building & Packaging

```bash
cd examples/chain

# Available targets
make help

# Build and run
make Release
./bin/chain.app/Contents/MacOS/chain

# Package for distribution
make package          # Creates .app and .dmg
make app-info         # Show binary info (arch, min OS)
```

## Architecture

The addon is split into two layers:

**avs_lib** (`libs/avs_lib/`) - Portable C++ library with zero dependencies. Contains the renderer, effect implementations, preset loading, and parameter system. Can be used standalone without OpenFrameworks.

**ofxAVS** (`src/`) - OpenFrameworks integration layer. Handles texture rendering, ImGui UI, audio input, and session persistence.

See `libs/avs_lib/README.md` for detailed library documentation.

## Original Credits

- **Justin Frankel** - Creator of AVS and Winamp
- **Nullsoft** - Original AVS development and open-source release
- **AVS Community** - Decades of amazing presets

## License

MIT License - see LICENSE file

Based on original AVS source code by Nullsoft, Inc.
Original AVS Copyright (C) 2005 Nullsoft, Inc.
