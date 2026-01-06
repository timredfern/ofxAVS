# ofxAVS Architecture

## Overview

ofxAVS is an OpenFrameworks addon that provides:
- Integration with OpenFrameworks texture/pixel handling
- ImGui-based UI rendering for avs_lib effect configuration
- Audio input processing (FFT spectrum and waveform)

It serves as a reference implementation showing how to integrate avs_lib into a host application.

## Directory Structure

```
src/
├── ARCHITECTURE.md     # This file
├── ofxAVS.cpp          # Main addon: effect chain, audio, rendering
├── ofxAVS.h            # Public API
├── AVSui.cpp           # ImGui UI renderer
└── AVSui.h             # UI renderer API
```

## ImGui UI Rendering

### Translating Windows Dialog Layouts to ImGui

avs_lib stores UI layouts using original Windows dialog coordinates (137×137 pixel dialogs with absolute positioning). `AVSui.cpp` renders these layouts in ImGui.

**Key translation challenges:**

1. **Coordinate system**: Windows dialogs use absolute pixel positions. ImGui uses flow layout by default.
   - Solution: `SetCursorPos()` for absolute positioning with 2× scale factor

2. **Slider labels**: Windows trackbars (sliders) and LTEXT labels are separate controls. ImGui's `SliderInt()` combines track and label into one widget.
   - Solution: Use `##hidden_label` prefix to suppress ImGui's label, render LABEL controls separately at their original positions

3. **Control sizing**: Windows specifies exact widths for trackbars. ImGui sliders expand to fill available space.
   - Solution: `SetNextItemWidth()` constrains slider track to exact Windows width

### Rendering Approach

```cpp
for (const auto& control : layout.getControls()) {
    // Position at original Windows coordinates (scaled 2×)
    ImGui::SetCursorPos(ImVec2(control.x * 2.0f, control.y * 2.0f));

    switch (control.type) {
        case ControlType::LABEL:
            ImGui::Text("%s", control.text.c_str());
            break;

        case ControlType::SLIDER:
            // Hidden label - actual label comes from separate LABEL control
            ImGui::SetNextItemWidth(control.w * 2.0f);
            ImGui::SliderInt("##slider_id", &value, min, max);
            break;

        case ControlType::GROUPBOX:
            // Draw frame with title at position
            break;
        // ... other control types
    }
}
```

### Control Type Mapping

| avs_lib ControlType | ImGui Widget |
|---------------------|--------------|
| LABEL | `ImGui::Text()` |
| GROUPBOX | Custom frame drawing |
| CHECKBOX | `ImGui::Checkbox()` |
| RADIO_GROUP | Multiple `ImGui::RadioButton()` |
| SLIDER | `ImGui::SliderInt()` with `##` hidden label |
| BUTTON | `ImGui::Button()` |
| TEXT_INPUT | `ImGui::InputInt()` |
| EDITTEXT | `ImGui::InputTextMultiline()` |
| COLOR_BUTTON | `ImGui::ColorEdit3()` |
| COLOR_ARRAY | Custom multi-color widget |
| DROPDOWN | `ImGui::Combo()` |

### Color Format Conversion

AVS framebuffer uses `0xAABBGGRR` (little-endian BGRA). ImGui uses float arrays `[R, G, B, A]` with values 0.0-1.0.

```cpp
// AVS color → ImGui
col[0] = (color & 0xFF) / 255.0f;          // R
col[1] = ((color >> 8) & 0xFF) / 255.0f;   // G
col[2] = ((color >> 16) & 0xFF) / 255.0f;  // B
col[3] = ((color >> 24) & 0xFF) / 255.0f;  // A

// ImGui → AVS color
color = ((uint32_t)(col[3] * 255) << 24) |  // A
        ((uint32_t)(col[2] * 255) << 16) |  // B
        ((uint32_t)(col[1] * 255) << 8) |   // G
        ((uint32_t)(col[0] * 255));         // R
```

## Audio Processing

### FFT Pipeline

ofxAVS supports two FFT modes controlled by `#define AVS_ENHANCED_FFT`:

**Enhanced Mode** (default):
- 2048-sample FFT for higher frequency resolution
- Linear interpolation for bin mapping to 576 output values
- Temporal smoothing with attack/decay envelope
- dB scale normalization with 80dB range

**Original Mode**:
- 512-sample FFT matching original Winamp
- 256 bins expanded to 576 using Winamp's algorithm
- Log table compression (base ~60) matching original AVS
- No temporal smoothing

See `libs/avs_lib/ARCHITECTURE.md` for detailed documentation of the original Winamp/AVS FFT pipeline.

### AudioData Format

```cpp
typedef char AudioData[2][2][576];
// [spectrum/waveform][left/right][samples]
// visdata[0] = spectrum (unsigned 0-255)
// visdata[1] = waveform (signed, XOR 128 for unsigned)
```

## Integration Example

```cpp
#include "ofxAVS.h"

class ofApp : public ofBaseApp {
    ofxAVS avs;
    ofxImGui::Gui gui;

    void setup() {
        avs.setup(800, 600);
        gui.setup();

        // Add effects to chain
        avs.addEffect("Oscilloscope");
        avs.addEffect("Dynamic Movement");
    }

    void audioIn(ofSoundBuffer& buffer) {
        avs.audioIn(buffer);
    }

    void update() {
        avs.update();
    }

    void draw() {
        avs.draw(0, 0);

        gui.begin();
        // Render effect UIs
        for (auto& effect : avs.getEffects()) {
            avs_ui::renderImGui(effect->getUILayout(), effect.get());
        }
        gui.end();
    }
};
```
