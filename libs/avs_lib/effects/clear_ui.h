#pragma once
#include "../core/ui_layout.h"

namespace avs {

class ClearUI : public EffectUILayout {
public:
    std::string getEffectName() const override {
        return "clear";
    }
    
    std::vector<ControlLayout> getControls() const override {
        return {
            // Color selection button
            {
                .id = "color",
                .text = "Color", 
                .type = ControlType::COLOR_BUTTON,
                .x = 4, .y = 18, .w = 40, .h = 16
            },
            
            // Blend mode selection
            {
                .id = "blend_replace",
                .text = "Replace",
                .type = ControlType::RADIO_BUTTON,
                .x = 3, .y = 40, .w = 40, .h = 10
            },
            
            {
                .id = "blend_additive",
                .text = "Additive",
                .type = ControlType::RADIO_BUTTON, 
                .x = 3, .y = 51, .w = 38, .h = 10
            },
            
            {
                .id = "blend_5050", 
                .text = "50/50",
                .type = ControlType::RADIO_BUTTON,
                .x = 3, .y = 62, .w = 31, .h = 10
            },
            
            // Only on first frame checkbox
            {
                .id = "only_first",
                .text = "Only on first frame", 
                .type = ControlType::CHECKBOX,
                .x = 50, .y = 40, .w = 47, .h = 10
            }
        };
    }
};

} // namespace avs