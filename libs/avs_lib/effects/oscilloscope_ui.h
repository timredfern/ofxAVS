#pragma once
#include "../core/ui_layout.h"

namespace avs {

class OscilloscopeUI : public EffectUILayout {
public:
    std::string getEffectName() const override {
        return "oscilloscope";
    }
    
    std::vector<ControlLayout> getControls() const override {
        return {
            // Effect color button
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 4, .y = 22, .w = 40, .h = 16
            },
            
            // Channel selection
            {
                .id = "channel_left",
                .text = "Left channel",
                .type = ControlType::RADIO_BUTTON, 
                .x = 3, .y = 42, .w = 51, .h = 10
            },
            
            {
                .id = "channel_right", 
                .text = "Right channel",
                .type = ControlType::RADIO_BUTTON,
                .x = 3, .y = 53, .w = 56, .h = 10
            },
            
            // Drawing mode
            {
                .id = "solid",
                .text = "Solid",
                .type = ControlType::CHECKBOX,
                .x = 3, .y = 75, .w = 30, .h = 10
            }
        };
    }
};

} // namespace avs