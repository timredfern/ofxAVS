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

    // Parameter system
    ParameterGroup& parameters() override { return parameters_; }
    const ParameterGroup& parameters() const override { return parameters_; }
    
    // Configuration - modern replacement for binary config
    virtual void load_parameters(const std::vector<uint8_t>& data) {}
    virtual std::vector<uint8_t> save_parameters() const { return {}; }
    
    // Enable/disable state
    virtual bool is_enabled() const { return parameters_.get_bool("enabled", true); }
    virtual void set_enabled(bool enabled) { parameters_.set_bool("enabled", enabled); }

protected:
    // Initialize parameters automatically from UI layout
    // Call this in effect constructor instead of manually creating parameters
    void init_parameters_from_layout(const EffectUILayout& layout) {
        for (const auto& control : layout.getControls()) {
            switch (control.type) {
                case ControlType::CHECKBOX:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::BOOL, control.default_val != 0));
                    break;
                case ControlType::RADIO_GROUP:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::INT, control.default_val,
                        0, static_cast<int>(control.radio_options.size()) - 1));
                    break;
                case ControlType::SLIDER:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::INT, control.default_val,
                        control.range.min, control.range.max));
                    break;
                case ControlType::COLOR_BUTTON:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::COLOR, static_cast<uint32_t>(control.default_val)));
                    break;
                case ControlType::BUTTON:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::BOOL, false));
                    break;
                case ControlType::TEXT_INPUT:
                    // Single-line text or integer input
                    if (control.range.max > 0) {
                        // Has range, treat as integer
                        parameters_.add_parameter(std::make_shared<Parameter>(
                            control.id, ParameterType::INT, control.default_val,
                            control.range.min, control.range.max));
                    } else {
                        parameters_.add_parameter(std::make_shared<Parameter>(
                            control.id, ParameterType::STRING, std::string("")));
                    }
                    break;
                case ControlType::EDITTEXT:
                    // Multi-line text edit - create STRING parameter
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::STRING, std::string("")));
                    break;
                case ControlType::DROPDOWN:
                    parameters_.add_parameter(std::make_shared<Parameter>(
                        control.id, ParameterType::INT, control.default_val,
                        0, static_cast<int>(control.options.size()) - 1));
                    break;
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