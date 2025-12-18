// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "parser.h"

namespace avs {

Parser::Parser(Lexer& lexer) : lexer(lexer) {
}

std::unique_ptr<ASTNode> Parser::parse() {
    return parse_statement_sequence();
}

// Parse statement sequence: statement (';' statement)*
std::unique_ptr<ASTNode> Parser::parse_statement_sequence() {
    auto sequence = std::make_unique<StatementSequenceNode>();
    
    // Parse first statement
    sequence->statements.push_back(parse_statement());
    
    // Parse additional statements separated by semicolons
    while (lexer.peek_token().type == TokenType::SEMICOLON) {
        lexer.next_token(); // consume ';'
        
        // Allow optional trailing semicolon
        if (lexer.peek_token().type == TokenType::END_OF_INPUT) {
            break;
        }
        
        sequence->statements.push_back(parse_statement());
    }
    
    // If only one statement and it's not an assignment, return it directly
    if (sequence->statements.size() == 1) {
        return std::move(sequence->statements[0]);
    }
    
    return std::move(sequence);
}

// Parse statement: assignment or expression
std::unique_ptr<ASTNode> Parser::parse_statement() {
    return parse_assignment_or_expression();
}

// Parse assignment or expression: IDENTIFIER '=' expression | expression
std::unique_ptr<ASTNode> Parser::parse_assignment_or_expression() {
    // Check for assignment: IDENTIFIER '='
    // But need to distinguish from function calls: IDENTIFIER '('
    if (lexer.peek_token().type == TokenType::IDENTIFIER) {
        // Look ahead to see if this is assignment (IDENTIFIER '=') or something else
        Lexer temp_lexer = lexer;  // Make a copy to look ahead
        temp_lexer.next_token(); // consume identifier
        TokenType next_type = temp_lexer.peek_token().type;
        
        if (next_type == TokenType::ASSIGN) {
            // This is an assignment
            Token id_token = lexer.next_token();
            lexer.next_token(); // consume '='
            auto value = parse_expression();
            return std::make_unique<AssignmentNode>(id_token.value, std::move(value));
        }
    }
    
    // Not an assignment, parse as regular expression
    return parse_expression();
}

// Parse expression: term (('+' | '-') term)*
std::unique_ptr<ASTNode> Parser::parse_expression() {
    auto left = parse_term();
    return parse_expression_with_first_term(std::move(left));
}

// Continue parsing expression with first term already parsed
std::unique_ptr<ASTNode> Parser::parse_expression_with_first_term(std::unique_ptr<ASTNode> first_term) {
    auto left = std::move(first_term);
    
    while (true) {
        Token token = lexer.peek_token();
        if (token.type != TokenType::PLUS && token.type != TokenType::MINUS) {
            break;
        }
        
        lexer.next_token(); // consume the operator
        auto right = parse_term();
        left = std::make_unique<BinaryOpNode>(std::move(left), token.type, std::move(right));
    }
    
    return left;
}

// Parse term: factor (('*' | '/') factor)*
std::unique_ptr<ASTNode> Parser::parse_term() {
    auto left = parse_factor();
    
    while (true) {
        Token token = lexer.peek_token();
        if (token.type != TokenType::MULTIPLY && token.type != TokenType::DIVIDE) {
            break;
        }
        
        lexer.next_token(); // consume the operator
        auto right = parse_factor();
        left = std::make_unique<BinaryOpNode>(std::move(left), token.type, std::move(right));
    }
    
    return left;
}

// Parse factor: NUMBER | IDENTIFIER | IDENTIFIER '(' arguments ')' | '(' expression ')' | ('-'|'+') factor
std::unique_ptr<ASTNode> Parser::parse_factor() {
    Token token = lexer.next_token();
    
    switch (token.type) {
        case TokenType::NUMBER:
            return std::make_unique<NumberNode>(token.number_value);
            
        case TokenType::IDENTIFIER: {
            // Check if this is a function call
            if (lexer.peek_token().type == TokenType::LPAREN) {
                lexer.next_token(); // consume '('
                auto func_node = std::make_unique<FunctionCallNode>(token.value);
                
                // Parse arguments
                if (lexer.peek_token().type != TokenType::RPAREN) {
                    func_node->arguments.push_back(parse_expression());
                    
                    while (lexer.peek_token().type == TokenType::COMMA) {
                        lexer.next_token(); // consume ','
                        func_node->arguments.push_back(parse_expression());
                    }
                }
                
                lexer.next_token(); // consume ')'
                return std::move(func_node);
            } else {
                // Regular variable
                return std::make_unique<VariableNode>(token.value);
            }
        }
            
        case TokenType::LPAREN: {
            auto node = parse_expression();
            lexer.next_token(); // consume ')'
            return node;
        }
        
        case TokenType::MINUS:
        case TokenType::PLUS: {
            // Unary operator
            auto operand = parse_factor();
            return std::make_unique<UnaryOpNode>(token.type, std::move(operand));
        }
        
        default:
            // Error: unexpected token
            return std::make_unique<NumberNode>(0.0);
    }
}

} // namespace avs