#pragma once

// Include all UI layout classes
#include "../effects/brightness_ui.h" 
#include "../effects/oscilloscope_ui.h"
#include "../effects/clear_ui.h"

#include "../core/ui_layout_registry.h"

namespace avs {

// Register all UI layouts
REGISTER_UI_LAYOUT("brightness", BrightnessUI)
REGISTER_UI_LAYOUT("oscilloscope", OscilloscopeUI)  
REGISTER_UI_LAYOUT("clear", ClearUI)

} // namespace avs