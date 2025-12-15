#include "script_engine.h"
#include "lexer.h"
#include "parser.h"
#include <memory>
#include <map>
#include <stdexcept>

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
        const AudioData* visdata;
        bool is_beat;
        
        AudioContext() : visdata(nullptr), is_beat(false) {}
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
        
        // Add audio data variables
        if (combined_vars.find("beat") == combined_vars.end()) {
            combined_vars["beat"] = pImpl->audio_context.is_beat ? 1.0 : 0.0;
        }
        
        // Add audio waveform and spectrum variables (limited subset for common use)
        if (pImpl->audio_context.visdata != nullptr) {
            const AudioData& vis = *pImpl->audio_context.visdata;
            
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
            // Only update user variables (not AVS built-ins)
            if (name != "x" && name != "y" && name != "w" && name != "h" && 
                name != "r" && name != "g" && name != "b" && name != "beat" &&
                name.substr(0, 1) != "v" && name.substr(0, 1) != "s") {
                pImpl->variables[name] = value;
            }
        }
        
        return result;
    } catch (const std::exception& e) {
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
    
    if (pImpl->audio_context.visdata != nullptr) {
        const AudioData& vis = *pImpl->audio_context.visdata;
        
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

void ScriptEngine::set_audio_context(const AudioData& visdata, bool is_beat) {
    pImpl->audio_context.visdata = &visdata;
    pImpl->audio_context.is_beat = is_beat;
}

bool ScriptEngine::has_error() const {
    return !pImpl->last_error.empty();
}

std::string ScriptEngine::get_error() const {
    return pImpl->last_error;
}

} // namespace avs