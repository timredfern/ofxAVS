// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "AVSui.h"
#include "ofxImGui.h"
#include <cmath>
#include <array>
#include <unordered_map>
#include <cstring>

namespace avs_ui {

// Non-linear slider mapping for brightness RGB controls
// Gives more sensitivity around 1.0x (center position)

// sqrt curve: expands center region (param → slider display)
static int sqrtCurve(int val) {
    float normalized = (val - 4096) / 4096.0f;  // -1 to 1
    float curved = std::copysign(std::sqrt(std::fabs(normalized)), normalized);
    return 4096 + static_cast<int>(curved * 4096);
}

// square curve: compresses center region (slider → param storage)
// Near center: small slider movement → tiny param change (fine control)
static int squareCurve(int val) {
    float normalized = (val - 4096) / 4096.0f;  // -1 to 1
    float curved = std::copysign(normalized * normalized, normalized);
    return 4096 + static_cast<int>(curved * 4096);
}

void renderImGui(const avs::EffectUILayout& layout, avs::EffectBase* effect) {
    if (!effect) return;

    
    std::string child_id = "EffectDialog##" + std::to_string(reinterpret_cast<uintptr_t>(effect));
    //ImGui::BeginChild(child_id.c_str(), ImVec2(385, 365), true);
    ImGui::BeginChild(child_id.c_str(), ImVec2(420, 440), true);

    auto& params = effect->parameters();

    // Push universal slider styling
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

    for (const auto& control : layout.getControls()) {

        // Positions and input boxes scaled from original AVS dialogs

        float scale=2.0f;

        ImGui::SetCursorPos(ImVec2(control.x * scale, control.y * scale));

        int controlwidth=control.w*scale;
        int controlheight=(control.h*scale)-12; //only used for edit boxes

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

                // Check if this is a brightness RGB slider
                bool is_brightness_rgb = (control.id == "red_adjust" ||
                                          control.id == "green_adjust" ||
                                          control.id == "blue_adjust");

                if (is_brightness_rgb) {
                    // Convert parameter to slider position (non-linear for fine control near 1.0x)
                    int slider_pos = sqrtCurve(value);

                    // Hide raw value, show multiplier instead
                    if (ImGui::SliderInt(unique_label.c_str(), &slider_pos, min_val, max_val, "")) {
                        // Convert slider position back to parameter
                        int new_param = squareCurve(slider_pos);
                        params.set_int(control.id, new_param);

                        // Link sliders when dissoc is unchecked
                        if (!params.get_bool("dissoc")) {
                            params.set_int("red_adjust", new_param);
                            params.set_int("green_adjust", new_param);
                            params.set_int("blue_adjust", new_param);
                        }
                    }
                    // Display multiplier value (calculated from actual parameter, not slider)
                    ImGui::SameLine();
                    int p = value - 4096;
                    float mult = 1.0f + (p < 0 ? 1 : 16) * (p / 4096.0f);
                    ImGui::Text("%.3fx", mult);
                } else {
                    // Normal slider
                    if (ImGui::SliderInt(unique_label.c_str(), &value, min_val, max_val)) {
                        params.set_int(control.id, value);
                    }
                }
                break;
            }

            case avs::ControlType::BUTTON: {
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(effect));
                if (ImGui::Button(unique_label.c_str(), ImVec2(control.w, control.h))) {
                    // Handle brightness reset buttons
                    if (control.id == "red_reset" || control.id == "green_reset" || control.id == "blue_reset") {
                        int default_val = 4096;  // Center position = 1.0x multiplier
                        if (!params.get_bool("dissoc")) {
                            // Reset all three when linked
                            params.set_int("red_adjust", default_val);
                            params.set_int("green_adjust", default_val);
                            params.set_int("blue_adjust", default_val);
                        } else {
                            // Reset just the corresponding slider
                            if (control.id == "red_reset") params.set_int("red_adjust", default_val);
                            if (control.id == "green_reset") params.set_int("green_adjust", default_val);
                            if (control.id == "blue_reset") params.set_int("blue_adjust", default_val);
                        }
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

            case avs::ControlType::TEXT_INPUT: {
                // Single-line text or integer input
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(effect));

                if (control.range.max > 0) {
                    // Integer input with range
                    int value = params.get_int(control.id);
                    ImGui::SetNextItemWidth(controlwidth); // * 2.0f);
                    if (ImGui::InputInt(unique_label.c_str(), &value, 0, 0)) {
                        // Clamp to range
                        if (value < control.range.min) value = control.range.min;
                        if (value > control.range.max) value = control.range.max;
                        params.set_int(control.id, value);
                    }
                } else {
                    // String input
                    std::string value = params.get_string(control.id);
                    char buffer[256];
                    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                    ImGui::SetNextItemWidth(control.w); // * 2.0f);
                    if (ImGui::InputText(unique_label.c_str(), buffer, sizeof(buffer))) {
                        params.set_string(control.id, std::string(buffer));
                    }
                }
                break;
            }

            case avs::ControlType::EDITTEXT: {
                // Multi-line text edit for scripts
                std::string value = params.get_string(control.id);

                // Use a static map to store edit buffers per control (char arrays for ImGui)
                static std::unordered_map<std::string, std::array<char, 4096>> edit_buffers;
                std::string buffer_key = control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(effect));

                // Initialize buffer if needed
                if (edit_buffers.find(buffer_key) == edit_buffers.end()) {
                    strncpy(edit_buffers[buffer_key].data(), value.c_str(), 4095);
                    edit_buffers[buffer_key][4095] = '\0';
                }

                // Label above the edit box
                ImGui::Text("%s:", control.text.c_str());
                ImGui::SetCursorPos(ImVec2(control.x , ImGui::GetCursorPosY())); //* 2.0f

                std::string unique_label = "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(effect));

                // Multiline input (sizes scaled 2x)
                if (ImGui::InputTextMultiline(unique_label.c_str(),
                    edit_buffers[buffer_key].data(),
                    edit_buffers[buffer_key].size(),
                    ImVec2(controlwidth , controlheight ),  //2.0f
                    ImGuiInputTextFlags_AllowTabInput)) {
                    // Update parameter when text changes
                    params.set_string(control.id, std::string(edit_buffers[buffer_key].data()));
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
