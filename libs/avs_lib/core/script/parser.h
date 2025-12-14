#pragma once

#include "lexer.h"
#include <memory>
#include <map>
#include <vector>
#include <cmath>

namespace avs {

// Abstract syntax tree nodes
struct ASTNode {
    virtual ~ASTNode() = default;
    virtual double evaluate(const std::map<std::string, double>& variables) const = 0;
};

struct NumberNode : public ASTNode {
    double value;
    NumberNode(double v) : value(v) {}
    double evaluate(const std::map<std::string, double>& variables) const override {
        return value;
    }
};

struct VariableNode : public ASTNode {
    std::string name;
    VariableNode(const std::string& n) : name(n) {}
    double evaluate(const std::map<std::string, double>& variables) const override {
        auto it = variables.find(name);
        return (it != variables.end()) ? it->second : 0.0;
    }
};

struct BinaryOpNode : public ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    TokenType op;
    
    BinaryOpNode(std::unique_ptr<ASTNode> l, TokenType o, std::unique_ptr<ASTNode> r)
        : left(std::move(l)), right(std::move(r)), op(o) {}
        
    double evaluate(const std::map<std::string, double>& variables) const override {
        double l_val = left->evaluate(variables);
        double r_val = right->evaluate(variables);
        
        switch (op) {
            case TokenType::PLUS: return l_val + r_val;
            case TokenType::MINUS: return l_val - r_val;
            case TokenType::MULTIPLY: return l_val * r_val;
            case TokenType::DIVIDE: return (r_val != 0.0) ? l_val / r_val : 0.0;
            default: return 0.0;
        }
    }
};

struct UnaryOpNode : public ASTNode {
    std::unique_ptr<ASTNode> operand;
    TokenType op;
    
    UnaryOpNode(TokenType o, std::unique_ptr<ASTNode> operand)
        : operand(std::move(operand)), op(o) {}
        
    double evaluate(const std::map<std::string, double>& variables) const override {
        double val = operand->evaluate(variables);
        switch (op) {
            case TokenType::MINUS: return -val;
            case TokenType::PLUS: return val;
            default: return val;
        }
    }
};

struct AssignmentNode : public ASTNode {
    std::string variable_name;
    std::unique_ptr<ASTNode> value;
    
    AssignmentNode(const std::string& name, std::unique_ptr<ASTNode> val)
        : variable_name(name), value(std::move(val)) {}
        
    double evaluate(const std::map<std::string, double>& variables) const override {
        double result = value->evaluate(variables);
        // For assignment, we need a way to modify the ScriptEngine's state
        // This is a simplified implementation - in a real parser, we'd need
        // a callback mechanism or different evaluation context
        return result;
    }
};

struct FunctionCallNode : public ASTNode {
    std::string function_name;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    
    FunctionCallNode(const std::string& name) : function_name(name) {}
    
    double evaluate(const std::map<std::string, double>& variables) const override {
        std::vector<double> arg_values;
        for (const auto& arg : arguments) {
            arg_values.push_back(arg->evaluate(variables));
        }
        
        if (function_name == "sin") {
            return arg_values.size() > 0 ? std::sin(arg_values[0]) : 0.0;
        } else if (function_name == "cos") {
            return arg_values.size() > 0 ? std::cos(arg_values[0]) : 0.0;
        } else if (function_name == "tan") {
            return arg_values.size() > 0 ? std::tan(arg_values[0]) : 0.0;
        } else if (function_name == "sqrt") {
            return arg_values.size() > 0 ? std::sqrt(arg_values[0]) : 0.0;
        } else if (function_name == "abs") {
            return arg_values.size() > 0 ? std::fabs(arg_values[0]) : 0.0;
        } else if (function_name == "log") {
            return arg_values.size() > 0 ? std::log(arg_values[0]) : 0.0;
        } else if (function_name == "log10") {
            return arg_values.size() > 0 ? std::log10(arg_values[0]) : 0.0;
        } else if (function_name == "pow") {
            return arg_values.size() > 1 ? std::pow(arg_values[0], arg_values[1]) : 0.0;
        }
        
        return 0.0; // Unknown function
    }
};

class Parser {
public:
    explicit Parser(Lexer& lexer);
    std::unique_ptr<ASTNode> parse();
    
private:
    Lexer& lexer;
    
    std::unique_ptr<ASTNode> parse_expression();
    std::unique_ptr<ASTNode> parse_term();
    std::unique_ptr<ASTNode> parse_factor();
};

} // namespace avs