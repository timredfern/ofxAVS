// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include <string>
#include <vector>

namespace avs {

enum class TokenType {
    NUMBER,
    IDENTIFIER,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    LPAREN,
    RPAREN,
    COMMA,
    ASSIGN,
    SEMICOLON,
    END_OF_INPUT
};

struct Token {
    TokenType type;
    std::string value;
    double number_value = 0.0;
    
    Token(TokenType t, const std::string& v = "") : type(t), value(v) {
        if (type == TokenType::NUMBER) {
            number_value = std::stod(value);
        }
    }
};

class Lexer {
public:
    explicit Lexer(const std::string& input);
    Token next_token();
    Token peek_token();
    
private:
    std::string input;
    size_t position = 0;
    Token current_token;
    bool has_current = false;
    
    void skip_whitespace();
    Token read_number();
    Token read_identifier();
};

} // namespace avs