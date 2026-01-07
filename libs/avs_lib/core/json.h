// avs_lib - Portable Advanced Visualization Studio library
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace avs {

// Forward declaration
class JsonValue;

// Type aliases for JSON containers
using JsonArray = std::vector<JsonValue>;
using JsonObject = std::map<std::string, JsonValue>;

// JSON value - can hold any JSON type
class JsonValue {
public:
    // Null by default
    JsonValue() : value_(nullptr) {}

    // Constructors for each type
    JsonValue(std::nullptr_t) : value_(nullptr) {}
    JsonValue(bool b) : value_(b) {}
    JsonValue(int i) : value_(static_cast<double>(i)) {}
    JsonValue(double d) : value_(d) {}
    JsonValue(const char* s) : value_(std::string(s)) {}
    JsonValue(const std::string& s) : value_(s) {}
    JsonValue(std::string&& s) : value_(std::move(s)) {}
    JsonValue(const JsonArray& arr) : value_(arr) {}
    JsonValue(JsonArray&& arr) : value_(std::move(arr)) {}
    JsonValue(const JsonObject& obj) : value_(obj) {}
    JsonValue(JsonObject&& obj) : value_(std::move(obj)) {}

    // Type checks
    bool is_null() const { return std::holds_alternative<std::nullptr_t>(value_); }
    bool is_bool() const { return std::holds_alternative<bool>(value_); }
    bool is_number() const { return std::holds_alternative<double>(value_); }
    bool is_string() const { return std::holds_alternative<std::string>(value_); }
    bool is_array() const { return std::holds_alternative<JsonArray>(value_); }
    bool is_object() const { return std::holds_alternative<JsonObject>(value_); }

    // Value getters (throw if wrong type)
    bool as_bool() const { return std::get<bool>(value_); }
    double as_number() const { return std::get<double>(value_); }
    int as_int() const { return static_cast<int>(std::get<double>(value_)); }
    const std::string& as_string() const { return std::get<std::string>(value_); }
    const JsonArray& as_array() const { return std::get<JsonArray>(value_); }
    const JsonObject& as_object() const { return std::get<JsonObject>(value_); }

    // Mutable access for building
    JsonArray& as_array() { return std::get<JsonArray>(value_); }
    JsonObject& as_object() { return std::get<JsonObject>(value_); }

    // Object access helpers
    bool has(const std::string& key) const {
        if (!is_object()) return false;
        return as_object().count(key) > 0;
    }

    const JsonValue& operator[](const std::string& key) const {
        return as_object().at(key);
    }

    JsonValue& operator[](const std::string& key) {
        if (!is_object()) value_ = JsonObject{};
        return std::get<JsonObject>(value_)[key];
    }

    // Array access
    const JsonValue& operator[](size_t idx) const {
        return as_array().at(idx);
    }

    size_t size() const {
        if (is_array()) return as_array().size();
        if (is_object()) return as_object().size();
        return 0;
    }

private:
    std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject> value_;
};

// JSON Parser
class JsonParser {
public:
    static JsonValue parse(const std::string& json);

private:
    JsonParser(const std::string& json) : json_(json), pos_(0) {}

    JsonValue parse_value();
    JsonValue parse_object();
    JsonValue parse_array();
    JsonValue parse_string();
    JsonValue parse_number();
    JsonValue parse_literal();

    void skip_whitespace();
    char peek() const;
    char consume();
    bool match(char c);
    void expect(char c);

    const std::string& json_;
    size_t pos_;
};

// JSON Writer
class JsonWriter {
public:
    static std::string write(const JsonValue& value, bool pretty = true);

private:
    JsonWriter(bool pretty) : pretty_(pretty), indent_(0) {}

    std::string write_value(const JsonValue& value);
    std::string write_object(const JsonObject& obj);
    std::string write_array(const JsonArray& arr);
    std::string write_string(const std::string& s);

    std::string newline() const { return pretty_ ? "\n" : ""; }
    std::string indent() const { return pretty_ ? std::string(indent_ * 2, ' ') : ""; }

    bool pretty_;
    int indent_;
};

// Convenience functions
inline JsonValue json_parse(const std::string& json) {
    return JsonParser::parse(json);
}

inline std::string json_write(const JsonValue& value, bool pretty = true) {
    return JsonWriter::write(value, pretty);
}

} // namespace avs
