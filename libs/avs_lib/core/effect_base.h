// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "parameter.h"
#include <string>
#include <vector>
#include <cstdint>

namespace avs {

// Forward declarations
struct PluginInfo;

// Keep original audio data format for easy porting
typedef char AudioData[2][2][576];

// Base class for all AVS effects
class EffectBase {
public:
    virtual ~EffectBase() = default;
    
    // Core render function - keep identical signature to original for easy porting
    // Returns: 0 = use input buffer, 1 = use output buffer
    virtual int render(AudioData visdata, int isBeat,
                      uint32_t* framebuffer, uint32_t* fbout,
                      int w, int h) = 0;
    
    // Effect identification
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    
    // Plugin information
    virtual const PluginInfo& get_plugin_info() const = 0;
    
    // Parameter system
    virtual ParameterGroup& parameters() { return parameters_; }
    virtual const ParameterGroup& parameters() const { return parameters_; }
    
    // Configuration - modern replacement for binary config
    virtual void load_parameters(const std::vector<uint8_t>& data) {}
    virtual std::vector<uint8_t> save_parameters() const { return {}; }
    
    // Enable/disable state
    virtual bool is_enabled() const { return parameters_.get_bool("enabled", true); }
    virtual void set_enabled(bool enabled) { parameters_.set_bool("enabled", enabled); }

protected:
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