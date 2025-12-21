// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "AVSui.h"
#include "ofxImGui.h"

namespace avs_ui {

void renderImGui(const avs::EffectUILayout& layout, avs::EffectBase* effect) {
    if (!effect) return;

    // All AVS dialogs are 233x214 but we are doubling the spacing. not using original windows UI sizes
    // Create unique child window ID using effect pointer
    std::string child_id = "EffectDialog##" + std::to_string(reinterpret_cast<uintptr_t>(effect));
    ImGui::BeginChild(child_id.c_str(), ImVec2(385,365), true);

    auto& params = effect->parameters();

    // Push universal slider styling
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

    for (const auto& control : layout.getControls()) {
        ImGui::SetCursorPos(ImVec2(control.x*2, control.y*2)); //double the spacing

        switch (control.type) {
            case avs::ControlType::CHECKBOX: {
                bool value = params.get_bool(control.id);
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(effect));
                if (ImGui::Checkbox(unique_label.c_str(), &value)) {
                    params.set_bool(control.id, value);
                }
                break;
            }

            case avs::ControlType::SLIDER: {
                auto param = params.get_parameter(control.id);
                if (!param) break;
                int value = param->as_int();
                int min_val = std::get<int>(param->min_value());
                int max_val = std::get<int>(param->max_value());
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(effect));
                if (ImGui::SliderInt(unique_label.c_str(), &value, min_val, max_val)) {
                    params.set_int(control.id, value);
                }
                break;
            }

            case avs::ControlType::BUTTON: {
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(effect));
                if (ImGui::Button(unique_label.c_str(), ImVec2(control.w, control.h))) {
                    // Reset button behavior - set to default value
                    if (control.type == avs::ControlType::SLIDER) {
                        params.set_int(control.id, control.default_val);
                    }
                }
                break;
            }

            case avs::ControlType::RADIO_BUTTON: {
                bool value = params.get_bool(control.id);
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(effect));
                if (ImGui::RadioButton(unique_label.c_str(), value)) {
                    // Set this radio button to true and others in the group to false
                    params.set_bool(control.id, true);

                    // Handle radio button groups - find similar controls and disable them
                    if (control.id.find("blend_") == 0) {
                        // Clear effect blend mode group
                        if (control.id != "blend_replace") params.set_bool("blend_replace", false);
                        if (control.id != "blend_additive") params.set_bool("blend_additive", false);
                        if (control.id != "blend_5050") params.set_bool("blend_5050", false);
                    } else if (control.id.find("channel_") == 0) {
                        // Oscilloscope channel group
                        if (control.id != "channel_left") params.set_bool("channel_left", false);
                        if (control.id != "channel_right") params.set_bool("channel_right", false);
                        if (control.id != "channel_both") params.set_bool("channel_both", false);
                    } else if (control.id == "replace" || control.id == "additive" || control.id == "5050") {
                        // Brightness blend mode group
                        if (control.id != "replace") params.set_bool("replace", false);
                        if (control.id != "additive") params.set_bool("additive", false);
                        if (control.id != "5050") params.set_bool("5050", false);
                    }
                }
                break;
            }

            case avs::ControlType::COLOR_BUTTON: {
                uint32_t color = params.get_color(control.id);
                float col[4] = {
                    ((color >> 16) & 0xFF) / 255.0f,  // R
                    ((color >> 8) & 0xFF) / 255.0f,   // G
                    (color & 0xFF) / 255.0f,          // B
                    ((color >> 24) & 0xFF) / 255.0f   // A
                };
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(effect));
                std::string popup_id = "ColorPicker##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(effect));
                if (ImGui::ColorButton(unique_label.c_str(), ImVec4(col[0], col[1], col[2], col[3]))) {
                    ImGui::OpenPopup(popup_id.c_str());
                }
                if (ImGui::BeginPopup(popup_id.c_str())) {
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
    }

    // Pop universal styling
    ImGui::PopStyleColor(5);

    ImGui::EndChild();
}

} // namespace avs_ui
