// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

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
    STRING,
    ENUM
};

class Parameter {
public:
    using Value = std::variant<double, int, bool, uint32_t, std::string>;
    using ChangeCallback = std::function<void(const Value&)>;

    Parameter(const std::string& name, ParameterType type, Value default_value,
              Value min_value = 0.0, Value max_value = 1.0)
        : name_(name), type_(type), value_(default_value), 
          min_value_(min_value), max_value_(max_value) {}

    // Constructor for enum parameters with option names
    Parameter(const std::string& name, ParameterType type, int default_value,
              const std::vector<std::string>& enum_options)
        : name_(name), type_(type), value_(default_value), 
          min_value_(0), max_value_(static_cast<int>(enum_options.size() - 1)),
          enum_options_(enum_options) {}

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
    
    // Enum-specific methods
    const std::vector<std::string>& enum_options() const { return enum_options_; }
    std::string enum_value_name() const {
        if (type_ == ParameterType::ENUM && !enum_options_.empty()) {
            int idx = as_int();
            if (idx >= 0 && idx < static_cast<int>(enum_options_.size())) {
                return enum_options_[idx];
            }
        }
        return "";
    }
    
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
    std::vector<std::string> enum_options_; // For ENUM type parameters
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

    // Copy parameter values from another group (for duplicating effects)
    void copy_from(const ParameterGroup& other) {
        for (const auto& [name, param] : other.parameters_) {
            auto it = parameters_.find(name);
            if (it != parameters_.end()) {
                it->second->set_value(param->get_value());
            }
        }
    }

private:
    std::map<std::string, std::shared_ptr<Parameter>> parameters_;
};

} // namespace avs