// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ui.h"
#include "effect_base.h"
#include "ofxImGui.h"

namespace avs {

void EffectUILayout::renderImGui(EffectBase* effect) const {
    if (!effect) return;
    
    // All AVS dialogs are 137x137
    ImGui::BeginChild(effect_name.c_str(), ImVec2(137, 137), true);
    
    auto& params = effect->parameters();
    
    for (const auto& control : controls) {
        ImGui::SetCursorPos(ImVec2(control.x, control.y));
        ImGui::PushID(control.id.c_str());
        
        switch (control.type) {
            case ControlType::CHECKBOX: {
                bool value = params.get_bool(control.id);
                if (ImGui::Checkbox(control.text.c_str(), &value)) {
                    params.set_bool(control.id, value);
                }
                break;
            }
            
            case ControlType::SLIDER: {
                int value = params.get_int(control.id);
                if (ImGui::SliderInt(control.text.c_str(), &value, 
                                   control.range.min, control.range.max)) {
                    params.set_int(control.id, value);
                }
                break;
            }
            
            case ControlType::BUTTON: {
                if (ImGui::Button(control.text.c_str(), ImVec2(control.w, control.h))) {
                    // Reset button behavior - set to default value
                    if (control.type == ControlType::SLIDER) {
                        params.set_int(control.id, control.range.default_val);
                    }
                }
                break;
            }
            
            case ControlType::RADIO_BUTTON: {
                int current_mode = params.get_int("blend_mode", 0);
                bool selected = false;
                // Parse radio button index from control ID
                if (control.id.find("replace") != std::string::npos) selected = (current_mode == 0);
                else if (control.id.find("additive") != std::string::npos) selected = (current_mode == 1);
                else if (control.id.find("5050") != std::string::npos) selected = (current_mode == 3);
                
                if (ImGui::RadioButton(control.text.c_str(), selected)) {
                    if (control.id.find("replace") != std::string::npos) params.set_int("blend_mode", 0);
                    else if (control.id.find("additive") != std::string::npos) params.set_int("blend_mode", 1);
                    else if (control.id.find("5050") != std::string::npos) params.set_int("blend_mode", 3);
                }
                break;
            }
            
            case ControlType::COLOR_BUTTON: {
                uint32_t color = params.get_color(control.id);
                float col[4] = {
                    ((color >> 16) & 0xFF) / 255.0f,  // R
                    ((color >> 8) & 0xFF) / 255.0f,   // G
                    (color & 0xFF) / 255.0f,          // B
                    ((color >> 24) & 0xFF) / 255.0f   // A
                };
                if (ImGui::ColorButton(control.text.c_str(), ImVec4(col[0], col[1], col[2], col[3]))) {
                    ImGui::OpenPopup("ColorPicker");
                }
                if (ImGui::BeginPopup("ColorPicker")) {
                    if (ImGui::ColorPicker4("Color", col)) {
                        uint32_t new_color = 
                            ((uint32_t)(col[3] * 255) << 24) |  // A
                            ((uint32_t)(col[0] * 255) << 16) |  // R
                            ((uint32_t)(col[1] * 255) << 8) |   // G
                            ((uint32_t)(col[2] * 255));         // B
                        params.set_color(control.id, new_color);
                    }
                    ImGui::EndPopup();
                }
                break;
            }
            
            default:
                ImGui::Text("%s (unsupported)", control.text.c_str());
                break;
        }
        
        ImGui::PopID();
    }
    
    ImGui::EndChild();
}

} // namespace avs