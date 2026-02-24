# AVS_simple

Minimal AVS visualization example with microphone input.

## Building

```bash
export OF_ROOT=/path/to/openFrameworks
make
make run
```

## Features

- Basic effect chain with UI
- Microphone input (auto-selects first available device)
- Effect parameter editing

This example demonstrates the minimal code needed to integrate ofxAVS. For a full-featured example with audio device selection and file playback, see `AVS_standard`.

## Code

```cpp
#include "ofxAVS.h"
#include "ofxImGui.h"

class ofApp : public ofBaseApp {
    ofxAVS avs;
    ofxImGui::Gui gui;
    ofSoundStream soundStream;

    void setup() {
        gui.setup();
        avs.setup();
        // ... audio setup ...
    }

    void draw() {
        avs.draw(0, 0, 600, 600);
        gui.begin();
        avs.drawUI();
        gui.end();
    }

    void audioIn(ofSoundBuffer& buffer) {
        avs.audioIn(buffer);
    }
};
```
