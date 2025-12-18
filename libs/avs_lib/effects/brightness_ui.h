#pragma once
#include "../core/ui_layout.h"

namespace avs {

class BrightnessUI : public EffectUILayout {
public:
    std::string getEffectName() const override {
        return "brightness";
    }
    
    std::vector<ControlLayout> getControls() const override {
        return {
            // Enable checkbox
            {
                .id = "enabled",
                .text = "Enable Brightness filter", 
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 87, .h = 10
            },
            
            // Red slider  
            {
                .id = "red_adjust",
                .text = "Red",
                .type = ControlType::SLIDER,
                .x = 25, .y = 13, .w = 97, .h = 13,
                .range = {-255, 255, 0, 25}
            },
            
            // Red reset button (><)
            {
                .id = "red_reset", 
                .text = "><",
                .type = ControlType::BUTTON,
                .x = 125, .y = 12, .w = 12, .h = 14
            },
            
            // Green slider
            {
                .id = "green_adjust",
                .text = "Green", 
                .type = ControlType::SLIDER,
                .x = 25, .y = 28, .w = 97, .h = 13,
                .range = {-255, 255, 0, 25}
            },
            
            // Green reset button
            {
                .id = "green_reset",
                .text = "><", 
                .type = ControlType::BUTTON,
                .x = 125, .y = 28, .w = 12, .h = 14
            },
            
            // Blue slider
            {
                .id = "blue_adjust",
                .text = "Blue",
                .type = ControlType::SLIDER, 
                .x = 25, .y = 44, .w = 97, .h = 13,
                .range = {-255, 255, 0, 25}
            },
            
            // Blue reset button
            {
                .id = "blue_reset",
                .text = "><",
                .type = ControlType::BUTTON,
                .x = 125, .y = 44, .w = 12, .h = 14
            },
            
            // Dissociate RGB checkbox
            {
                .id = "dissoc",
                .text = "Dissociate RGB values",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 60, .w = 89, .h = 10
            },
            
            // Blend mode radio buttons
            {
                .id = "replace",
                .text = "Replace",
                .type = ControlType::RADIO_BUTTON,
                .x = 0, .y = 71, .w = 43, .h = 10
            },
            
            {
                .id = "additive", 
                .text = "Additive blend",
                .type = ControlType::RADIO_BUTTON,
                .x = 0, .y = 81, .w = 61, .h = 10
            },
            
            {
                .id = "5050",
                .text = "50/50 blend", 
                .type = ControlType::RADIO_BUTTON,
                .x = 0, .y = 91, .w = 53, .h = 10
            }
        };
    }
};

} // namespace avs