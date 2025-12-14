#include "script_engine.h"
#include <memory>
#include <map>
#include <stdexcept>

namespace avs {

class ScriptEngine::Impl {
public:
    std::map<std::string, double> variables;
    std::string last_error;
};

ScriptEngine::ScriptEngine() : pImpl(std::make_unique<Impl>()) {
}

ScriptEngine::~ScriptEngine() = default;

double ScriptEngine::evaluate(const std::string& expression) {
    // TODO: Implement expression parsing and evaluation
    // For now, throw to make tests fail
    throw std::runtime_error("ScriptEngine::evaluate not implemented yet");
}

void ScriptEngine::set_variable(const std::string& name, double value) {
    pImpl->variables[name] = value;
}

double ScriptEngine::get_variable(const std::string& name) {
    auto it = pImpl->variables.find(name);
    if (it != pImpl->variables.end()) {
        return it->second;
    }
    return 0.0; // Default to 0 if variable not found
}

bool ScriptEngine::has_error() const {
    return !pImpl->last_error.empty();
}

std::string ScriptEngine::get_error() const {
    return pImpl->last_error;
}

} // namespace avs