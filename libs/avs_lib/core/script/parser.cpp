#include "parser.h"

namespace avs {

Parser::Parser(Lexer& lexer) : lexer(lexer) {
}

std::unique_ptr<ASTNode> Parser::parse() {
    return parse_expression();
}

// Parse expression: term (('+' | '-') term)*
std::unique_ptr<ASTNode> Parser::parse_expression() {
    auto left = parse_term();
    
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