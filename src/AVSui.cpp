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

void renderImGui(const avs::EffectUILayout& layout, avs::Configurable* configurable) {
    if (!configurable) return;

    std::string child_id = "ConfigDialog##" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
    ImGui::BeginChild(child_id.c_str(), ImVec2(420, 440), true);

    auto& params = configurable->parameters();

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
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
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
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));

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
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
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

            case avs::ControlType::RADIO_GROUP: {
                // Radio group with explicit positions for each option
                int current_value = params.get_int(control.id);
                int option_index = 0;
                for (const auto& option : control.radio_options) {
                    ImGui::SetCursorPos(ImVec2(option.x * scale, option.y * scale));
                    std::string unique_label = option.label + "##" + control.id + "_" + std::to_string(option_index) + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
                    if (ImGui::RadioButton(unique_label.c_str(), current_value == option_index)) {
                        params.set_int(control.id, option_index);
                    }
                    option_index++;
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
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
                std::string popup_id = "ColorPicker##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
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
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));

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

                // Use static maps to store edit buffers and last known values
                static std::unordered_map<std::string, std::array<char, 4096>> edit_buffers;
                static std::unordered_map<std::string, std::string> last_param_values;
                std::string buffer_key = control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));

                // Initialize or sync buffer when parameter changes externally
                bool needs_sync = (edit_buffers.find(buffer_key) == edit_buffers.end()) ||
                                  (last_param_values[buffer_key] != value &&
                                   std::string(edit_buffers[buffer_key].data()) == last_param_values[buffer_key]);

                if (needs_sync) {
                    strncpy(edit_buffers[buffer_key].data(), value.c_str(), 4095);
                    edit_buffers[buffer_key][4095] = '\0';
                    last_param_values[buffer_key] = value;
                }

                // Label above the edit box
                ImGui::Text("%s:", control.text.c_str());
                ImGui::SetCursorPos(ImVec2(control.x , ImGui::GetCursorPosY())); //* 2.0f

                std::string unique_label = "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));

                // Multiline input (sizes scaled 2x)
                if (ImGui::InputTextMultiline(unique_label.c_str(),
                    edit_buffers[buffer_key].data(),
                    edit_buffers[buffer_key].size(),
                    ImVec2(controlwidth , controlheight ),  //2.0f
                    ImGuiInputTextFlags_AllowTabInput)) {
                    // Update parameter when text changes
                    std::string new_value(edit_buffers[buffer_key].data());
                    params.set_string(control.id, new_value);
                    last_param_values[buffer_key] = new_value;
                }
                break;
            }

            case avs::ControlType::DROPDOWN: {
                int current_value = params.get_int(control.id);
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));

                // Get current item name for preview
                const char* preview = (current_value >= 0 && current_value < static_cast<int>(control.options.size()))
                    ? control.options[current_value].c_str()
                    : "(none)";

                ImGui::SetNextItemWidth(controlwidth);
                if (ImGui::BeginCombo(unique_label.c_str(), preview)) {
                    for (int i = 0; i < static_cast<int>(control.options.size()); i++) {
                        bool is_selected = (current_value == i);
                        if (ImGui::Selectable(control.options[i].c_str(), is_selected)) {
                            params.set_int(control.id, i);
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
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
