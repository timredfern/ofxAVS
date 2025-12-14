#pragma once

#include <string>

namespace avs {

/**
 * NS-EEL Script Engine
 * 
 * This class provides a simple interface to the NS-EEL expression evaluation library.
 * It can compile and execute mathematical expressions with support for variables,
 * built-in functions, and AVS-specific integration.
 */
class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    // Basic expression evaluation
    double evaluate(const std::string& expression);
    
    // Variable management  
    void set_variable(const std::string& name, double value);
    double get_variable(const std::string& name);
    
    // Error handling
    bool has_error() const;
    std::string get_error() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace avs