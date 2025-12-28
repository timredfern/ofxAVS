// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "script_engine.h"
#include "lexer.h" 
#include "parser.h"
#include <memory>
#include <map>
#include <stdexcept>
#include <cstring>

namespace avs {

class ScriptEngine::Impl {
public:
    std::map<std::string, double> variables;
    std::string last_error;
    
    // AVS-specific state
    struct PixelContext {
        int x, y, width, height;
    } pixel_context = {0, 0, 1, 1};
    
    struct ColorContext {
        double r, g, b;
    } color_context = {0.0, 0.0, 0.0};
    
    struct AudioContext {
        AudioData visdata;
        bool is_beat;
        bool has_data;
        
        AudioContext() : is_beat(false), has_data(false) {}
    } audio_context;
};

ScriptEngine::ScriptEngine() : pImpl(std::make_unique<Impl>()) {
}

ScriptEngine::~ScriptEngine() = default;

double ScriptEngine::evaluate(const std::string& expression) {
    try {
        pImpl->last_error.clear();
        
        // Create a combined variable map with AVS-specific variables
        auto combined_vars = pImpl->variables;
        
        // Add AVS-specific variables only if not overridden by user variables
        if (combined_vars.find("x") == combined_vars.end()) {
            combined_vars["x"] = pImpl->pixel_context.width > 1 ? 
                static_cast<double>(pImpl->pixel_context.x) / (pImpl->pixel_context.width - 1) : 0.0;
        }
        if (combined_vars.find("y") == combined_vars.end()) {
            combined_vars["y"] = pImpl->pixel_context.height > 1 ? 
                static_cast<double>(pImpl->pixel_context.y) / (pImpl->pixel_context.height - 1) : 0.0;
        }
        if (combined_vars.find("w") == combined_vars.end()) {
            combined_vars["w"] = static_cast<double>(pImpl->pixel_context.width);
        }
        if (combined_vars.find("h") == combined_vars.end()) {
            combined_vars["h"] = static_cast<double>(pImpl->pixel_context.height);
        }
        if (combined_vars.find("r") == combined_vars.end()) {
            combined_vars["r"] = pImpl->color_context.r;
        }
        if (combined_vars.find("g") == combined_vars.end()) {
            combined_vars["g"] = pImpl->color_context.g;
        }
        if (combined_vars.find("b") == combined_vars.end()) {
            combined_vars["b"] = pImpl->color_context.b;
        }
        
        // Add mathematical constants
        if (combined_vars.find("$PI") == combined_vars.end()) {
            combined_vars["$PI"] = M_PI;
        }
        if (combined_vars.find("$E") == combined_vars.end()) {
            combined_vars["$E"] = M_E;
        }
        
        // Add audio data variables
        if (combined_vars.find("beat") == combined_vars.end()) {
            combined_vars["beat"] = pImpl->audio_context.is_beat ? 1.0 : 0.0;
        }
        
        // Add audio waveform and spectrum variables (limited subset for common use)
        if (pImpl->audio_context.has_data) {
            const AudioData& vis = pImpl->audio_context.visdata;
            
            // Common waveform access points (v1-v8 for left channel waveform samples)
            for (int i = 0; i < 8; i++) {
                std::string var_name = "v" + std::to_string(i + 1);
                if (combined_vars.find(var_name) == combined_vars.end()) {
                    int sample_idx = i * 72; // Sample every 72nd sample for 8 points
                    if (sample_idx < 576) {
                        combined_vars[var_name] = static_cast<double>(vis[0][0][sample_idx]) / 127.0;
                    }
                }
            }
            
            // Right channel equivalents (vr1-vr8)  
            for (int i = 0; i < 8; i++) {
                std::string var_name = "vr" + std::to_string(i + 1);
                if (combined_vars.find(var_name) == combined_vars.end()) {
                    int sample_idx = i * 72;
                    if (sample_idx < 576) {
                        combined_vars[var_name] = static_cast<double>(vis[0][1][sample_idx]) / 127.0;
                    }
                }
            }
            
            // Spectrum data (s1-s8 for frequency bins)
            for (int i = 0; i < 8; i++) {
                std::string var_name = "s" + std::to_string(i + 1);
                if (combined_vars.find(var_name) == combined_vars.end()) {
                    int sample_idx = i * 72;
                    if (sample_idx < 576) {
                        combined_vars[var_name] = static_cast<double>(vis[1][0][sample_idx]) / 127.0;
                    }
                }
            }
        }
        
        Lexer lexer(expression);
        Parser parser(lexer);
        auto ast = parser.parse();
        
        double result = ast->evaluate(combined_vars);
        
        // Update user variables with any assignments that occurred
        for (const auto& [name, value] : combined_vars) {
            // Check if this variable existed in the user variables before evaluation
            // If it did, or if it's a new assignment, update it
            bool was_user_variable = (pImpl->variables.find(name) != pImpl->variables.end());
            
            // AVS built-in variables that should not be persisted
            bool is_builtin = (name == "w" || name == "h" || name == "beat");
            
            // Audio variables that should not be persisted (v1-v8, vr1-vr8, s1-s8)
            if (!is_builtin && name.length() >= 2) {
                if ((name[0] == 'v' && name.length() == 2 && name[1] >= '1' && name[1] <= '8') ||
                    (name.substr(0, 2) == "vr" && name.length() == 3 && name[2] >= '1' && name[2] <= '8') ||
                    (name[0] == 's' && name.length() == 2 && name[1] >= '1' && name[1] <= '8')) {
                    is_builtin = true;
                }
            }
            
            // Allow assignment to coordinate variables (x, y, r, g, b) - these can be set by user
            if (!is_builtin || was_user_variable || name == "x" || name == "y" || name == "r" || name == "g" || name == "b") {
                pImpl->variables[name] = value;
            }
        }
        
        return result;
    } catch (const std::exception& e) {
        // All errors handled gracefully - visualizations should never crash
        // Errors are stored and can be checked with has_error()/get_error()
        pImpl->last_error = e.what();
        return 0.0;
    }
}

void ScriptEngine::set_variable(const std::string& name, double value) {
    pImpl->variables[name] = value;
}

double ScriptEngine::get_variable(const std::string& name) {
    // Check user-defined variables first
    auto it = pImpl->variables.find(name);
    if (it != pImpl->variables.end()) {
        return it->second;
    }
    
    // Then check AVS-specific variables
    if (name == "x") return pImpl->pixel_context.width > 1 ? 
        static_cast<double>(pImpl->pixel_context.x) / (pImpl->pixel_context.width - 1) : 0.0;
    if (name == "y") return pImpl->pixel_context.height > 1 ? 
        static_cast<double>(pImpl->pixel_context.y) / (pImpl->pixel_context.height - 1) : 0.0;
    if (name == "w") return static_cast<double>(pImpl->pixel_context.width);
    if (name == "h") return static_cast<double>(pImpl->pixel_context.height);
    if (name == "r") return pImpl->color_context.r;
    if (name == "g") return pImpl->color_context.g;
    if (name == "b") return pImpl->color_context.b;
    
    // Audio variables
    if (name == "beat") return pImpl->audio_context.is_beat ? 1.0 : 0.0;
    
    if (pImpl->audio_context.has_data) {
        const AudioData& vis = pImpl->audio_context.visdata;
        
        // Waveform variables v1-v8 (left channel)
        for (int i = 0; i < 8; i++) {
            std::string var_name = "v" + std::to_string(i + 1);
            if (name == var_name) {
                int sample_idx = i * 72;
                if (sample_idx < 576) {
                    return static_cast<double>(vis[0][0][sample_idx]) / 127.0;
                }
            }
        }
        
        // Right channel variables vr1-vr8
        for (int i = 0; i < 8; i++) {
            std::string var_name = "vr" + std::to_string(i + 1);
            if (name == var_name) {
                int sample_idx = i * 72;
                if (sample_idx < 576) {
                    return static_cast<double>(vis[0][1][sample_idx]) / 127.0;
                }
            }
        }
        
        // Spectrum variables s1-s8
        for (int i = 0; i < 8; i++) {
            std::string var_name = "s" + std::to_string(i + 1);
            if (name == var_name) {
                int sample_idx = i * 72;
                if (sample_idx < 576) {
                    return static_cast<double>(vis[1][0][sample_idx]) / 127.0;
                }
            }
        }
    }
    
    return 0.0; // Default to 0 if variable not found
}

void ScriptEngine::set_pixel_context(int pixel_x, int pixel_y, int width, int height) {
    pImpl->pixel_context.x = pixel_x;
    pImpl->pixel_context.y = pixel_y;
    pImpl->pixel_context.width = width;
    pImpl->pixel_context.height = height;
}

void ScriptEngine::set_color_context(double red, double green, double blue) {
    pImpl->color_context.r = red;
    pImpl->color_context.g = green;
    pImpl->color_context.b = blue;
}

void ScriptEngine::set_audio_context(AudioData visdata, bool is_beat) {
    std::memcpy(pImpl->audio_context.visdata, visdata, sizeof(AudioData));
    pImpl->audio_context.is_beat = is_beat;
    pImpl->audio_context.has_data = true;
}

bool ScriptEngine::has_error() const {
    return !pImpl->last_error.empty();
}

std::string ScriptEngine::get_error() const {
    return pImpl->last_error;
}

} // namespace avs