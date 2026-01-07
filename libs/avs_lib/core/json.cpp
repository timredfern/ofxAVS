// avs_lib - Portable Advanced Visualization Studio library
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "json.h"
#include <cmath>
#include <iomanip>

namespace avs {

// ============================================================================
// JsonParser Implementation
// ============================================================================

JsonValue JsonParser::parse(const std::string& json) {
    JsonParser parser(json);
    parser.skip_whitespace();
    JsonValue result = parser.parse_value();
    parser.skip_whitespace();
    if (parser.pos_ < parser.json_.size()) {
        throw std::runtime_error("Unexpected content after JSON value");
    }
    return result;
}

JsonValue JsonParser::parse_value() {
    skip_whitespace();
    char c = peek();

    if (c == '{') return parse_object();
    if (c == '[') return parse_array();
    if (c == '"') return parse_string();
    if (c == '-' || (c >= '0' && c <= '9')) return parse_number();
    if (c == 't' || c == 'f' || c == 'n') return parse_literal();

    throw std::runtime_error("Unexpected character in JSON: " + std::string(1, c));
}

JsonValue JsonParser::parse_object() {
    expect('{');
    skip_whitespace();

    JsonObject obj;

    if (peek() == '}') {
        consume();
        return obj;
    }

    while (true) {
        skip_whitespace();

        // Parse key
        if (peek() != '"') {
            throw std::runtime_error("Expected string key in object");
        }
        std::string key = parse_string().as_string();

        skip_whitespace();
        expect(':');
        skip_whitespace();

        // Parse value
        obj[key] = parse_value();

        skip_whitespace();
        if (match(',')) {
            continue;
        }
        break;
    }

    expect('}');
    return obj;
}

JsonValue JsonParser::parse_array() {
    expect('[');
    skip_whitespace();

    JsonArray arr;

    if (peek() == ']') {
        consume();
        return arr;
    }

    while (true) {
        skip_whitespace();
        arr.push_back(parse_value());
        skip_whitespace();

        if (match(',')) {
            continue;
        }
        break;
    }

    expect(']');
    return arr;
}

JsonValue JsonParser::parse_string() {
    expect('"');
    std::string result;

    while (peek() != '"') {
        char c = consume();
        if (c == '\\') {
            char escaped = consume();
            switch (escaped) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    // Parse 4 hex digits (simplified - just skip for now)
                    for (int i = 0; i < 4; i++) consume();
                    result += '?';  // Placeholder
                    break;
                }
                default:
                    throw std::runtime_error("Invalid escape sequence");
            }
        } else {
            result += c;
        }
    }

    expect('"');
    return result;
}

JsonValue JsonParser::parse_number() {
    size_t start = pos_;

    if (peek() == '-') consume();

    // Integer part
    if (peek() == '0') {
        consume();
    } else if (peek() >= '1' && peek() <= '9') {
        while (peek() >= '0' && peek() <= '9') consume();
    } else {
        throw std::runtime_error("Invalid number");
    }

    // Fractional part
    if (peek() == '.') {
        consume();
        if (!(peek() >= '0' && peek() <= '9')) {
            throw std::runtime_error("Invalid number: expected digit after decimal");
        }
        while (peek() >= '0' && peek() <= '9') consume();
    }

    // Exponent
    if (peek() == 'e' || peek() == 'E') {
        consume();
        if (peek() == '+' || peek() == '-') consume();
        if (!(peek() >= '0' && peek() <= '9')) {
            throw std::runtime_error("Invalid number: expected digit in exponent");
        }
        while (peek() >= '0' && peek() <= '9') consume();
    }

    std::string num_str = json_.substr(start, pos_ - start);
    return std::stod(num_str);
}

JsonValue JsonParser::parse_literal() {
    if (json_.compare(pos_, 4, "true") == 0) {
        pos_ += 4;
        return true;
    }
    if (json_.compare(pos_, 5, "false") == 0) {
        pos_ += 5;
        return false;
    }
    if (json_.compare(pos_, 4, "null") == 0) {
        pos_ += 4;
        return nullptr;
    }
    throw std::runtime_error("Invalid literal");
}

void JsonParser::skip_whitespace() {
    while (pos_ < json_.size()) {
        char c = json_[pos_];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            pos_++;
        } else {
            break;
        }
    }
}

char JsonParser::peek() const {
    if (pos_ >= json_.size()) return '\0';
    return json_[pos_];
}

char JsonParser::consume() {
    if (pos_ >= json_.size()) {
        throw std::runtime_error("Unexpected end of JSON");
    }
    return json_[pos_++];
}

bool JsonParser::match(char c) {
    if (peek() == c) {
        consume();
        return true;
    }
    return false;
}

void JsonParser::expect(char c) {
    if (!match(c)) {
        throw std::runtime_error("Expected '" + std::string(1, c) + "' but got '" +
                                 std::string(1, peek()) + "'");
    }
}

// ============================================================================
// JsonWriter Implementation
// ============================================================================

std::string JsonWriter::write(const JsonValue& value, bool pretty) {
    JsonWriter writer(pretty);
    return writer.write_value(value);
}

std::string JsonWriter::write_value(const JsonValue& value) {
    if (value.is_null()) return "null";
    if (value.is_bool()) return value.as_bool() ? "true" : "false";
    if (value.is_number()) {
        double d = value.as_number();
        // Output integers without decimal point
        if (std::floor(d) == d && std::abs(d) < 1e15) {
            return std::to_string(static_cast<long long>(d));
        }
        std::ostringstream oss;
        oss << std::setprecision(15) << d;
        return oss.str();
    }
    if (value.is_string()) return write_string(value.as_string());
    if (value.is_array()) return write_array(value.as_array());
    if (value.is_object()) return write_object(value.as_object());
    return "null";
}

std::string JsonWriter::write_object(const JsonObject& obj) {
    if (obj.empty()) return "{}";

    std::string result = "{" + newline();
    indent_++;

    bool first = true;
    for (const auto& [key, value] : obj) {
        if (!first) result += "," + newline();
        first = false;
        result += indent() + write_string(key) + ": " + write_value(value);
    }

    indent_--;
    result += newline() + indent() + "}";
    return result;
}

std::string JsonWriter::write_array(const JsonArray& arr) {
    if (arr.empty()) return "[]";

    // Check if all elements are simple (not objects/arrays)
    bool all_simple = true;
    for (const auto& v : arr) {
        if (v.is_object() || v.is_array()) {
            all_simple = false;
            break;
        }
    }

    // Simple arrays on one line
    if (all_simple && arr.size() <= 10) {
        std::string result = "[";
        bool first = true;
        for (const auto& v : arr) {
            if (!first) result += ", ";
            first = false;
            result += write_value(v);
        }
        result += "]";
        return result;
    }

    // Complex arrays with newlines
    std::string result = "[" + newline();
    indent_++;

    bool first = true;
    for (const auto& v : arr) {
        if (!first) result += "," + newline();
        first = false;
        result += indent() + write_value(v);
    }

    indent_--;
    result += newline() + indent() + "]";
    return result;
}

std::string JsonWriter::write_string(const std::string& s) {
    std::string result = "\"";
    for (char c : s) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // Control character - output as \u00XX
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    result += "\"";
    return result;
}

} // namespace avs
