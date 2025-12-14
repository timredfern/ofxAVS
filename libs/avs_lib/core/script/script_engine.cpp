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
            combined_vars["x"] = static_cast<double>(pImpl->pixel_context.x) / pImpl->pixel_context.width;
        }
        if (combined_vars.find("y") == combined_vars.end()) {
            combined_vars["y"] = static_cast<double>(pImpl->pixel_context.y) / pImpl->pixel_context.height;
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
        
        Lexer lexer(expression);
        Parser parser(lexer);
        auto ast = parser.parse();
        
        return ast->evaluate(combined_vars);
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
    if (name == "x") return static_cast<double>(pImpl->pixel_context.x) / pImpl->pixel_context.width;
    if (name == "y") return static_cast<double>(pImpl->pixel_context.y) / pImpl->pixel_context.height;
    if (name == "w") return static_cast<double>(pImpl->pixel_context.width);
    if (name == "h") return static_cast<double>(pImpl->pixel_context.height);
    if (name == "r") return pImpl->color_context.r;
    if (name == "g") return pImpl->color_context.g;
    if (name == "b") return pImpl->color_context.b;
    
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

bool ScriptEngine::has_error() const {
    return !pImpl->last_error.empty();
}

std::string ScriptEngine::get_error() const {
    return pImpl->last_error;
}

} // namespace avs