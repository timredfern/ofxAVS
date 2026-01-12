// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "AVSui.h"
#include "ofxImGui.h"
#include "core/expression_help.h"
#include <cmath>
#include <array>
#include <unordered_map>
#include <cstring>

namespace avs_ui {

// Color format conversion between ImGui (RGBA floats) and AVS framebuffer format
// AVS framebuffer format: 0xAABBGGRR (R in bits 0-7, G in bits 8-15, B in bits 16-23, A in bits 24-31)
// This matches OF_PIXELS_BGRA on little-endian systems.
// ImGui expects col[0]=R, col[1]=G, col[2]=B, col[3]=A as floats 0-1

// Extract ImGui float array from 0xAABBGGRR color
inline void color_to_imgui(uint32_t color, float* col) {
    col[0] = (color & 0xFF) / 255.0f;          // R from bits 0-7
    col[1] = ((color >> 8) & 0xFF) / 255.0f;   // G from bits 8-15
    col[2] = ((color >> 16) & 0xFF) / 255.0f;  // B from bits 16-23
    col[3] = ((color >> 24) & 0xFF) / 255.0f;  // A from bits 24-31
}

// Build 0xAABBGGRR color from ImGui float array
inline uint32_t imgui_to_color(const float* col) {
    return ((uint32_t)(col[3] * 255) << 24) |  // A to bits 24-31
           ((uint32_t)(col[2] * 255) << 16) |  // B to bits 16-23
           ((uint32_t)(col[1] * 255) << 8) |   // G to bits 8-15
           ((uint32_t)(col[0] * 255));         // R to bits 0-7
}

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
    // Size to fit largest dialog (SuperScope: 233x214 at 2x scale = 466x428)
    ImGui::BeginChild(child_id.c_str(), ImVec2(480, 440), true);

    auto& params = configurable->parameters();

    // Push universal slider styling
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

    // Check for beat detector mode to disable Advanced-only controls in Standard mode
    // See EFFECTS.md "Beat Detector" section for documentation
    bool is_standard_mode = false;
    auto mode_param = params.get_parameter("mode");
    if (mode_param) {
        is_standard_mode = (mode_param->as_int() == 0);
    }

    // Track if layout has its own HELP_BUTTON (so we don't double-render)
    bool has_help_button_control = false;

    for (const auto& control : layout.getControls()) {
        if (control.type == avs::ControlType::HELP_BUTTON) {
            has_help_button_control = true;
        }

        // Absolute positioning from original Windows dialog coordinates
        float scale=2.0f;

        ImGui::SetCursorPos(ImVec2(control.x * scale, control.y * scale));

        int controlwidth=control.w*scale;
        int controlheight=(control.h*scale)-12; //only used for edit boxes

        // Disable Advanced-only controls when in Standard mode
        bool disable_control = is_standard_mode && (
            control.id == "sticky_beats" ||
            control.id == "only_sticky" ||
            control.id == "on_new_song" ||
            control.id == "double_beat" ||
            control.id == "half_beat" ||
            control.id == "reset" ||
            control.id == "stick" ||
            control.id == "unstick"
        );
        if (disable_control) {
            ImGui::BeginDisabled();
        }

        switch (control.type) {
            case avs::ControlType::CHECKBOX: {
                bool value = params.get_bool(control.id);
                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
                if (ImGui::Checkbox(unique_label.c_str(), &value)) {
                    params.set_bool(control.id, value);
                    configurable->on_parameter_changed(control.id);
                }
                break;
            }

            case avs::ControlType::SLIDER: {
                bool is_brightness_rgb = (control.id == "red_adjust" ||
                                          control.id == "green_adjust" ||
                                          control.id == "blue_adjust");

                auto param = params.get_parameter(control.id);
                if (!param) break;
                int value = param->as_int();
                int min_val = std::get<int>(param->min_value());
                int max_val = std::get<int>(param->max_value());

                // Always use hidden label (##) - labels come from separate LABEL controls
                // This matches Windows dialogs where trackbars and LTEXT are separate
                std::string unique_label = "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));

                // Set exact width to match Windows trackbar dimensions
                ImGui::SetNextItemWidth(controlwidth);

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
                    // Display multiplier value to the right of the reset button
                    // Button ends at x=137 (edge of original dialog), so place value at x=140
                    ImGui::SetCursorPos(ImVec2(140 * scale, control.y * scale));
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
                if (ImGui::Button(unique_label.c_str(), ImVec2(control.w * scale, control.h * scale))) {
                    // Set button parameter to true (consumed by effect's process())
                    params.set_bool(control.id, true);

                    // Handle brightness reset buttons specifically
                    if (control.id == "red_reset" || control.id == "green_reset" || control.id == "blue_reset") {
                        int default_val = 4096;  // Center position = 1.0x multiplier
                        if (!params.get_bool("dissoc")) {
                            // Reset all three when linked
                            params.set_int("red_adjust", default_val);
                            params.set_int("green_adjust", default_val);
                            params.set_int("blue_adjust", default_val);
                        } else {
                            // Reset just the corresponding slider
                            std::string adjust_param = control.id.substr(0, control.id.find('_')) + "_adjust";
                            params.set_int(adjust_param, default_val);
                        }
                    }

                    // Handle dot grid zero buttons
                    if (control.id == "x_reset") {
                        params.set_int("x_move", 0);
                    }
                    if (control.id == "y_reset") {
                        params.set_int("y_move", 0);
                    }
                }
                break;
            }

            case avs::ControlType::RADIO_GROUP: {
                // Radio group with explicit positions for each option
                // Option positions are relative to control position
                int current_value = params.get_int(control.id);
                int option_index = 0;
                for (const auto& option : control.radio_options) {
                    ImGui::SetCursorPos(ImVec2((control.x + option.x) * scale, (control.y + option.y) * scale));
                    std::string unique_label = option.label + "##" + control.id + "_" + std::to_string(option_index) + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
                    if (ImGui::RadioButton(unique_label.c_str(), current_value == option_index)) {
                        params.set_int(control.id, option_index);
                        configurable->on_parameter_changed(control.id);
                    }
                    option_index++;
                }
                break;
            }

            case avs::ControlType::COLOR_BUTTON: {
                uint32_t color = params.get_color(control.id);
                float col[4];
                color_to_imgui(color, col);

                std::string unique_label = control.text + "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
                std::string popup_id = "ColorPicker##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
                if (ImGui::ColorButton(unique_label.c_str(), ImVec4(col[0], col[1], col[2], col[3]))) {
                    ImGui::OpenPopup(popup_id.c_str());
                }
                if (ImGui::BeginPopup(popup_id.c_str())) {
                    if (ImGui::ColorPicker4("Color", col)) {
                        params.set_color(control.id, imgui_to_color(col));
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
                // Text edit for scripts - single-line for small heights, multi-line otherwise
                // Original AVS: h<=14 uses ES_AUTOHSCROLL (single-line), h>14 uses ES_MULTILINE
                std::string value = params.get_string(control.id);

                // Use static maps to store edit buffers and last known values
                static std::unordered_map<std::string, std::array<char, 4096>> edit_buffers;
                static std::unordered_map<std::string, std::string> last_param_values;
                std::string buffer_key = control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));

                // Initialize or sync buffer when parameter changes externally
                bool buffer_exists = edit_buffers.find(buffer_key) != edit_buffers.end();
                bool param_changed = buffer_exists && (last_param_values[buffer_key] != value);
                bool buffer_unedited = buffer_exists && (std::string(edit_buffers[buffer_key].data()) == last_param_values[buffer_key]);
                bool needs_sync = !buffer_exists || (param_changed && buffer_unedited);

                if (needs_sync) {
                    strncpy(edit_buffers[buffer_key].data(), value.c_str(), 4095);
                    edit_buffers[buffer_key][4095] = '\0';
                    last_param_values[buffer_key] = value;
                }

                // Labels are now separate LABEL controls (matching Windows LTEXT)
                std::string unique_label = "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));

                bool text_changed = false;
                // Use single-line InputText for small heights (matching original AVS ES_AUTOHSCROLL)
                // Use multi-line InputTextMultiline for larger heights (matching ES_MULTILINE)
                if (control.h <= 20) {
                    // Single-line text input
                    ImGui::SetNextItemWidth(controlwidth);
                    text_changed = ImGui::InputText(unique_label.c_str(),
                        edit_buffers[buffer_key].data(),
                        edit_buffers[buffer_key].size());
                } else {
                    // Multi-line text input
                    text_changed = ImGui::InputTextMultiline(unique_label.c_str(),
                        edit_buffers[buffer_key].data(),
                        edit_buffers[buffer_key].size(),
                        ImVec2(controlwidth, controlheight),
                        ImGuiInputTextFlags_AllowTabInput);
                }

                if (text_changed) {
                    // Update parameter when text changes
                    std::string new_value(edit_buffers[buffer_key].data());
                    params.set_string(control.id, new_value);
                    last_param_values[buffer_key] = new_value;
                    configurable->on_parameter_changed(control.id);
                }
                break;
            }

            case avs::ControlType::COLOR_ARRAY: {
                // Multi-color bar with clickable segments
                // Uses "num_colors" param for count, and "color_0", "color_1", etc. for colors
                int num_colors = params.get_int("num_colors");
                int max_colors = control.max_items > 0 ? control.max_items : 16;
                if (num_colors < 1) num_colors = 1;
                if (num_colors > max_colors) num_colors = max_colors;

                float bar_width = control.w * scale;
                float bar_height = control.h * scale;
                float segment_width = bar_width / num_colors;

                ImVec2 bar_pos = ImGui::GetCursorScreenPos();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                // Draw each color segment
                for (int i = 0; i < num_colors; i++) {
                    std::string color_param = "color_" + std::to_string(i);
                    uint32_t color = params.get_color(color_param);
                    float col[4];
                    color_to_imgui(color, col);
                    ImU32 im_color = IM_COL32(
                        (int)(col[0] * 255), (int)(col[1] * 255),
                        (int)(col[2] * 255), 255
                    );

                    float x1 = bar_pos.x + i * segment_width;
                    float x2 = bar_pos.x + (i + 1) * segment_width;
                    draw_list->AddRectFilled(
                        ImVec2(x1, bar_pos.y),
                        ImVec2(x2, bar_pos.y + bar_height),
                        im_color
                    );
                }

                // Draw border
                draw_list->AddRect(
                    bar_pos,
                    ImVec2(bar_pos.x + bar_width, bar_pos.y + bar_height),
                    IM_COL32(128, 128, 128, 255)
                );

                // Invisible button to capture clicks
                std::string unique_id = "##colorarray_" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));
                ImGui::InvisibleButton(unique_id.c_str(), ImVec2(bar_width, bar_height));

                // Static map to track which color segment is being edited
                static std::unordered_map<std::string, int> editing_color;

                if (ImGui::IsItemClicked()) {
                    // Determine which segment was clicked
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    float rel_x = mouse_pos.x - bar_pos.x;
                    int clicked_index = static_cast<int>(rel_x / segment_width);
                    if (clicked_index >= 0 && clicked_index < num_colors) {
                        // Store which color to edit
                        editing_color[unique_id] = clicked_index;
                        ImGui::OpenPopup((unique_id + "_picker").c_str());
                    }
                }

                // Color picker popup
                std::string popup_id = unique_id + "_picker";
                if (ImGui::BeginPopup(popup_id.c_str())) {
                    int edit_idx = editing_color[unique_id];
                    std::string color_param = "color_" + std::to_string(edit_idx);
                    uint32_t color = params.get_color(color_param);
                    float col[4];
                    color_to_imgui(color, col);
                    col[3] = 1.0f;  // Force full alpha for display

                    if (ImGui::ColorPicker3(("Color " + std::to_string(edit_idx + 1)).c_str(), col)) {
                        col[3] = 1.0f;  // Ensure alpha stays 1.0
                        params.set_color(color_param, imgui_to_color(col));
                    }
                    ImGui::EndPopup();
                }
                break;
            }

            case avs::ControlType::DROPDOWN: {
                int current_value = params.get_int(control.id);
                // Use hidden label (##) when .text is empty, matching Windows combo box behavior
                std::string unique_label = (control.text.empty() ? "##" : control.text + "##") + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));

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
                            configurable->on_parameter_changed(control.id);
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                break;
            }

            case avs::ControlType::LISTBOX: {
                int current_value = params.get_int(control.id);
                std::string unique_label = "##" + control.id + "_" + std::to_string(reinterpret_cast<uintptr_t>(configurable));

                // Scrollable list box showing multiple items
                if (ImGui::BeginListBox(unique_label.c_str(), ImVec2(controlwidth, control.h * scale))) {
                    for (int i = 0; i < static_cast<int>(control.options.size()); i++) {
                        bool is_selected = (current_value == i);
                        if (ImGui::Selectable(control.options[i].c_str(), is_selected)) {
                            params.set_int(control.id, i);
                            configurable->on_parameter_changed(control.id);
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndListBox();
                }
                break;
            }

            case avs::ControlType::LABEL: {
                // Static text label
                ImGui::Text("%s", control.text.c_str());
                break;
            }

            case avs::ControlType::GROUPBOX: {
                // Visual section header - ImGui flow layout doesn't support true groupboxes
                // Just show the title as a section separator
                if (!control.text.empty()) {
                    ImGui::SeparatorText(control.text.c_str());
                }
                break;
            }

            case avs::ControlType::HELP_BUTTON: {
                // Expression help button - only renders if effect has help_text
                std::string help = configurable->get_help_text();
                if (!help.empty()) {
                    std::string eff_name = help;
                    size_t nl = eff_name.find('\n');
                    if (nl != std::string::npos) {
                        eff_name = eff_name.substr(0, nl);
                    }
                    renderExpressionHelpButton(eff_name, help, controlwidth, 13 * scale);
                }
                break;
            }

            default:
                ImGui::Text("%s (unsupported)", control.text.c_str());
                break;
        }

        if (disable_control) {
            ImGui::EndDisabled();
        }
    }

    // Pop universal styling
    ImGui::PopStyleColor(5);

    // Render expression help button if effect has help text (fallback if no HELP_BUTTON control)
    if (!has_help_button_control) {
        std::string help_text = configurable->get_help_text();
        if (!help_text.empty()) {
            // Position at bottom-right area (typical position from res.rc dialogs)
            float scale = 2.0f;
            ImGui::SetCursorPos(ImVec2(158 * scale, 200 * scale));

            // Extract effect name from first line of help text (format: "Effect Name\n...")
            std::string effect_name = help_text;
            size_t newline = effect_name.find('\n');
            if (newline != std::string::npos) {
                effect_name = effect_name.substr(0, newline);
            }

            renderExpressionHelpButton(effect_name, help_text, 73 * scale, 13 * scale);
        }
    }

    ImGui::EndChild();
}

// Global state for expression help popup
static bool s_help_popup_open = false;
static std::string s_help_effect_name;
static std::string s_help_effect_text;
static int s_help_last_tab = 0;  // Remember last viewed tab

void renderExpressionHelpButton(const std::string& effect_name, const std::string& effect_help,
                                 float width, float height) {
    if (effect_help.empty()) return;

    // Caller should position with ImGui::SetCursorPos() before calling
    if (ImGui::Button("Expression Help", ImVec2(width, height))) {
        s_help_popup_open = true;
        s_help_effect_name = effect_name;
        s_help_effect_text = effect_help;
        // Default to effect-specific tab (index 4) when opening
        s_help_last_tab = 4;
    }
}

void renderExpressionHelpPopup() {
    if (!s_help_popup_open) return;

    // Center the window on first appearance
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("AVS Expression Help", &s_help_popup_open)) {
        if (ImGui::BeginTabBar("HelpTabs")) {
            // Tab 0: General
            if (ImGui::BeginTabItem("General")) {
                s_help_last_tab = 0;
                ImGui::BeginChild("GeneralScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::TextWrapped("%s", avs::expression_help::general());
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // Tab 1: Operators
            if (ImGui::BeginTabItem("Operators")) {
                s_help_last_tab = 1;
                ImGui::BeginChild("OperatorsScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::TextWrapped("%s", avs::expression_help::operators());
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // Tab 2: Functions
            if (ImGui::BeginTabItem("Functions")) {
                s_help_last_tab = 2;
                ImGui::BeginChild("FunctionsScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::TextWrapped("%s", avs::expression_help::functions());
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // Tab 3: Constants
            if (ImGui::BeginTabItem("Constants")) {
                s_help_last_tab = 3;
                ImGui::BeginChild("ConstantsScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::TextWrapped("%s", avs::expression_help::constants());
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // Tab 4: Effect-specific (only if we have effect help text)
            if (!s_help_effect_name.empty() && !s_help_effect_text.empty()) {
                // Use SetSelected to open this tab when first opening the popup
                ImGuiTabItemFlags flags = (s_help_last_tab == 4) ? ImGuiTabItemFlags_SetSelected : 0;
                if (ImGui::BeginTabItem(s_help_effect_name.c_str(), nullptr, flags)) {
                    s_help_last_tab = 4;
                    ImGui::BeginChild("EffectScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    ImGui::TextWrapped("%s", s_help_effect_text.c_str());
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

} // namespace avs_ui
