// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "configurable.h"
#include "parameter.h"
#include "ui.h"
#include <string>
#include <vector>
#include <cstdint>

namespace avs {

// Forward declarations
struct PluginInfo;

// Keep original audio data format for easy porting
typedef char AudioData[2][2][576];

// Common effect enums (used by multiple effects for render modes)
enum class BlendMode { REPLACE = 0, ADDITIVE = 1, BLEND_5050 = 2, DEFAULT = 3 };
enum class DrawStyle { LINES = 0, SOLID = 1, DOTS = 2 };
enum class AudioChannel { LEFT = 0, RIGHT = 1, CENTER = 2 };
enum class VerticalPosition { TOP = 0, BOTTOM = 1, CENTER = 2 };
enum class RenderMode { SPECTRUM = 0, OSCILLOSCOPE = 1 };

// Render return value flags (from original r_defs.h / r_bpm.cpp)
// Effects can return these to modify the beat signal for subsequent effects
constexpr int RENDER_SWAP_BUFFER = 0x00000001;  // Output is in fbout, swap buffers
constexpr int RENDER_SET_BEAT    = 0x10000000;  // Force beat for subsequent effects
constexpr int RENDER_CLR_BEAT    = 0x20000000;  // Clear beat for subsequent effects

// Base class for all AVS effects
class EffectBase : public Configurable {
public:
    virtual ~EffectBase() = default;
    
    // Core render function - keep identical signature to original for easy porting
    // Returns: 0 = use input buffer, 1 = use output buffer
    virtual int render(AudioData visdata, int isBeat,
                      uint32_t* framebuffer, uint32_t* fbout,
                      int w, int h) = 0;
    
    // Plugin information - effects must implement this
    // Use get_plugin_info().name and get_plugin_info().description for effect identification
    virtual const PluginInfo& get_plugin_info() const = 0;

    // Configurable interface implementation
    std::string get_display_name() const override;
    const EffectUILayout& get_ui_layout() const override;
    std::string get_help_text() const override;

    // Parameter system
    ParameterGroup& parameters() override { return parameters_; }
    const ParameterGroup& parameters() const override { return parameters_; }
    
    // Configuration - modern replacement for binary config
    virtual void load_parameters(const std::vector<uint8_t>& data) {}
    virtual std::vector<uint8_t> save_parameters() const { return {}; }
    
    // Enable/disable state
    virtual bool is_enabled() const {
        return parameters_.has_parameter("enabled") ? parameters_.get_bool("enabled") : true;
    }
    virtual void set_enabled(bool enabled) { parameters_.set_bool("enabled", enabled); }

protected:
    // Initialize parameters automatically from UI layout
    // Call this in effect constructor instead of manually creating parameters
    void init_parameters_from_layout(const EffectUILayout& layout) {
        for (const auto& control : layout.getControls()) {
            // Helper to get int from variant (default 0)
            auto get_int = [&]() -> int {
                if (auto* v = std::get_if<int>(&control.default_val)) return *v;
                if (auto* v = std::get_if<bool>(&control.default_val)) return *v ? 1 : 0;
                return 0;
            };
            // Helper to get bool from variant (default false)
            auto get_bool = [&]() -> bool {
                if (auto* v = std::get_if<bool>(&control.default_val)) return *v;
                if (auto* v = std::get_if<int>(&control.default_val)) return *v != 0;
                return false;
            };
            // Helper to get uint32_t from variant (default 0xFFFFFF)
            auto get_color = [&]() -> uint32_t {
                if (auto* v = std::get_if<uint32_t>(&control.default_val)) return *v;
                if (auto* v = std::get_if<int>(&control.default_val)) return static_cast<uint32_t>(*v);
                return 0xFFFFFF;
            };
            // Helper to get string from variant (default "")
            auto get_string = [&]() -> std::string {
                if (auto* v = std::get_if<std::string>(&control.default_val)) return *v;
                if (auto* v = std::get_if<const char*>(&control.default_val)) return *v;
                return "";
            };

            switch (control.type) {
                case ControlType::CHECKBOX:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::BOOL, get_bool()));
                    break;
                case ControlType::RADIO_GROUP: {
                    std::vector<std::string> labels;
                    for (const auto& opt : control.radio_options) {
                        labels.push_back(opt.label);
                    }
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::ENUM, get_int(), labels));
                    break;
                }
                case ControlType::SLIDER:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::INT, get_int(),
                        control.range.min, control.range.max));
                    break;
                case ControlType::COLOR_BUTTON:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::COLOR, get_color()));
                    break;
                case ControlType::BUTTON:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::BOOL, false));
                    break;
                case ControlType::TEXT_INPUT:
                    if (control.range.max > 0) {
                        parameters_.add_parameter(std::make_shared<Parameter>(
                            control.id, ParameterType::INT, get_int(),
                            control.range.min, control.range.max));
                    } else {
                        parameters_.add_parameter(std::make_shared<Parameter>(
                            control.id, ParameterType::STRING, get_string()));
                    }
                    break;
                case ControlType::EDITTEXT:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::STRING, get_string()));
                    break;
                case ControlType::FILE_DROPDOWN:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::STRING, get_string()));
                    break;
                case ControlType::DROPDOWN:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::ENUM, get_int(), control.options));
                    break;
                case ControlType::LISTBOX:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::ENUM, get_int(), control.options));
                    break;
                case ControlType::COLOR_ARRAY: {
                    int max_colors = control.max_items > 0 ? control.max_items : 16;
                    uint32_t first_color = get_color();
                    // Create num_colors parameter (default 1)
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        "num_colors", ParameterType::INT, 1, 1, max_colors));
                    // Create color_0, color_1, ... color_N parameters
                    for (int i = 0; i < max_colors; i++) {
                        std::string color_param = "color_" + std::to_string(i);
                        uint32_t default_color = (i == 0) ? first_color : 0xFFFFFFFF;
                        parameters_.add_parameter(std::make_shared<Parameter>(
                            color_param, ParameterType::COLOR, default_color));
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    // Helper functions for common operations
    static void clear_buffer(uint32_t* buffer, int w, int h, uint32_t color = 0) {
        for (int i = 0; i < w * h; i++) {
            buffer[i] = color;
        }
    }
    
    static uint32_t make_color(int r, int g, int b, int a = 255) {
        return (a << 24) | (r << 16) | (g << 8) | b;
    }
    
    static void extract_color(uint32_t color, int& r, int& g, int& b, int& a) {
        a = (color >> 24) & 0xFF;
        r = (color >> 16) & 0xFF;
        g = (color >> 8) & 0xFF;
        b = color & 0xFF;
    }

private:
    ParameterGroup parameters_;
};

} // namespace avs