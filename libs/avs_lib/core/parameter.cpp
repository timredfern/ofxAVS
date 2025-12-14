#include "parameter.h"
#include <algorithm>
#include <stdexcept>

namespace avs {

void Parameter::set_value(const Value& value) {
    value_ = clamp_value(value);
    notify_callbacks();
}

double Parameter::as_float() const {
    if (std::holds_alternative<double>(value_)) {
        return std::get<double>(value_);
    } else if (std::holds_alternative<int>(value_)) {
        return static_cast<double>(std::get<int>(value_));
    } else if (std::holds_alternative<bool>(value_)) {
        return std::get<bool>(value_) ? 1.0 : 0.0;
    }
    return 0.0;
}

int Parameter::as_int() const {
    if (std::holds_alternative<int>(value_)) {
        return std::get<int>(value_);
    } else if (std::holds_alternative<double>(value_)) {
        return static_cast<int>(std::get<double>(value_));
    } else if (std::holds_alternative<bool>(value_)) {
        return std::get<bool>(value_) ? 1 : 0;
    }
    return 0;
}

bool Parameter::as_bool() const {
    if (std::holds_alternative<bool>(value_)) {
        return std::get<bool>(value_);
    } else if (std::holds_alternative<double>(value_)) {
        return std::get<double>(value_) > 0.5;
    } else if (std::holds_alternative<int>(value_)) {
        return std::get<int>(value_) != 0;
    }
    return false;
}

uint32_t Parameter::as_color() const {
    if (std::holds_alternative<uint32_t>(value_)) {
        return std::get<uint32_t>(value_);
    }
    return 0xFFFFFF;
}

std::string Parameter::as_string() const {
    if (std::holds_alternative<std::string>(value_)) {
        return std::get<std::string>(value_);
    }
    return "";
}

void Parameter::notify_callbacks() {
    for (auto& callback : callbacks_) {
        callback(value_);
    }
}

Parameter::Value Parameter::clamp_value(const Value& value) const {
    // Only clamp numeric types
    if (std::holds_alternative<double>(value) && std::holds_alternative<double>(min_value_)) {
        double val = std::get<double>(value);
        double min_val = std::get<double>(min_value_);
        double max_val = std::get<double>(max_value_);
        return std::clamp(val, min_val, max_val);
    }
    
    if (std::holds_alternative<int>(value) && std::holds_alternative<int>(min_value_)) {
        int val = std::get<int>(value);
        int min_val = std::get<int>(min_value_);
        int max_val = std::get<int>(max_value_);
        return std::clamp(val, min_val, max_val);
    }
    
    return value;
}

// ParameterGroup implementation

void ParameterGroup::add_parameter(std::shared_ptr<Parameter> param) {
    if (param) {
        parameters_[param->name()] = param;
    }
}

std::shared_ptr<Parameter> ParameterGroup::get_parameter(const std::string& name) {
    auto it = parameters_.find(name);
    return (it != parameters_.end()) ? it->second : nullptr;
}

void ParameterGroup::set_float(const std::string& name, double value) {
    auto param = get_parameter(name);
    if (param && param->type() == ParameterType::FLOAT) {
        param->set_value(value);
    }
}

void ParameterGroup::set_int(const std::string& name, int value) {
    auto param = get_parameter(name);
    if (param && param->type() == ParameterType::INT) {
        param->set_value(value);
    }
}

void ParameterGroup::set_bool(const std::string& name, bool value) {
    auto param = get_parameter(name);
    if (param && param->type() == ParameterType::BOOL) {
        param->set_value(value);
    }
}

void ParameterGroup::set_color(const std::string& name, uint32_t value) {
    auto param = get_parameter(name);
    if (param && param->type() == ParameterType::COLOR) {
        param->set_value(value);
    }
}

void ParameterGroup::set_string(const std::string& name, const std::string& value) {
    auto param = get_parameter(name);
    if (param && param->type() == ParameterType::STRING) {
        param->set_value(value);
    }
}

double ParameterGroup::get_float(const std::string& name, double default_val) const {
    auto it = parameters_.find(name);
    return (it != parameters_.end()) ? it->second->as_float() : default_val;
}

int ParameterGroup::get_int(const std::string& name, int default_val) const {
    auto it = parameters_.find(name);
    return (it != parameters_.end()) ? it->second->as_int() : default_val;
}

bool ParameterGroup::get_bool(const std::string& name, bool default_val) const {
    auto it = parameters_.find(name);
    return (it != parameters_.end()) ? it->second->as_bool() : default_val;
}

uint32_t ParameterGroup::get_color(const std::string& name, uint32_t default_val) const {
    auto it = parameters_.find(name);
    return (it != parameters_.end()) ? it->second->as_color() : default_val;
}

std::string ParameterGroup::get_string(const std::string& name, const std::string& default_val) const {
    auto it = parameters_.find(name);
    return (it != parameters_.end()) ? it->second->as_string() : default_val;
}

} // namespace avs