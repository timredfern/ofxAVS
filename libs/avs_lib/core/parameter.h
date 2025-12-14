#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <variant>

namespace avs {

enum class ParameterType {
    FLOAT,
    INT,
    BOOL,
    COLOR,
    STRING
};

class Parameter {
public:
    using Value = std::variant<double, int, bool, uint32_t, std::string>;
    using ChangeCallback = std::function<void(const Value&)>;

    Parameter(const std::string& name, ParameterType type, Value default_value,
              Value min_value = 0.0, Value max_value = 1.0)
        : name_(name), type_(type), value_(default_value), 
          min_value_(min_value), max_value_(max_value) {}

    // Value access
    void set_value(const Value& value);
    const Value& get_value() const { return value_; }
    
    // Typed getters for convenience
    double as_float() const;
    int as_int() const;
    bool as_bool() const;
    uint32_t as_color() const;
    std::string as_string() const;
    
    // Metadata
    const std::string& name() const { return name_; }
    ParameterType type() const { return type_; }
    const Value& min_value() const { return min_value_; }
    const Value& max_value() const { return max_value_; }
    
    // Change notifications
    void add_change_callback(ChangeCallback callback) {
        callbacks_.push_back(callback);
    }

private:
    std::string name_;
    ParameterType type_;
    Value value_;
    Value min_value_;
    Value max_value_;
    std::vector<ChangeCallback> callbacks_;
    
    void notify_callbacks();
    Value clamp_value(const Value& value) const;
};

class ParameterGroup {
public:
    void add_parameter(std::shared_ptr<Parameter> param);
    std::shared_ptr<Parameter> get_parameter(const std::string& name);
    
    // Convenience methods
    void set_float(const std::string& name, double value);
    void set_int(const std::string& name, int value);
    void set_bool(const std::string& name, bool value);
    void set_color(const std::string& name, uint32_t value);
    void set_string(const std::string& name, const std::string& value);
    
    double get_float(const std::string& name, double default_val = 0.0) const;
    int get_int(const std::string& name, int default_val = 0) const;
    bool get_bool(const std::string& name, bool default_val = false) const;
    uint32_t get_color(const std::string& name, uint32_t default_val = 0xFFFFFF) const;
    std::string get_string(const std::string& name, const std::string& default_val = "") const;
    
    // Iteration
    const std::map<std::string, std::shared_ptr<Parameter>>& all_parameters() const {
        return parameters_;
    }

private:
    std::map<std::string, std::shared_ptr<Parameter>> parameters_;
};

} // namespace avs