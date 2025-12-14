#include "lexer.h"
#include <cctype>

namespace avs {

Lexer::Lexer(const std::string& input) : input(input), current_token(TokenType::END_OF_INPUT) {
}

Token Lexer::next_token() {
    if (has_current) {
        has_current = false;
        return current_token;
    }
    
    skip_whitespace();
    
    if (position >= input.length()) {
        return Token(TokenType::END_OF_INPUT);
    }
    
    char ch = input[position];
    
    if (std::isdigit(ch) || ch == '.') {
        return read_number();
    }
    
    if (std::isalpha(ch) || ch == '_') {
        return read_identifier();
    }
    
    // Single-character tokens
    position++;
    switch (ch) {
        case '+': return Token(TokenType::PLUS);
        case '-': return Token(TokenType::MINUS);
        case '*': return Token(TokenType::MULTIPLY);
        case '/': return Token(TokenType::DIVIDE);
        case '(': return Token(TokenType::LPAREN);
        case ')': return Token(TokenType::RPAREN);
        case ',': return Token(TokenType::COMMA);
        case '=': return Token(TokenType::ASSIGN);
        default:
            // Unknown character, skip it
            return next_token();
    }
}

Token Lexer::peek_token() {
    if (!has_current) {
        current_token = next_token();
        has_current = true;
    }
    return current_token;
}

void Lexer::skip_whitespace() {
    while (position < input.length() && std::isspace(input[position])) {
        position++;
    }
}

Token Lexer::read_number() {
    size_t start = position;
    
    while (position < input.length() && 
           (std::isdigit(input[position]) || input[position] == '.')) {
        position++;
    }
    
    std::string value = input.substr(start, position - start);
    return Token(TokenType::NUMBER, value);
}

Token Lexer::read_identifier() {
    size_t start = position;
    
    while (position < input.length() && 
           (std::isalnum(input[position]) || input[position] == '_')) {
        position++;
    }
    
    std::string value = input.substr(start, position - start);
    return Token(TokenType::IDENTIFIER, value);
}

} // namespace avs