#pragma once
#include <memory>
#include <string>
#include <vector>
#include "tokens.hpp"

struct Node {
    virtual ~Node() = default;
};

struct NumberNode : Node {
    Token tok;
    explicit NumberNode(Token t) : tok(std::move(t)) {}
    std::string getValue() const { return tok.value; }
};

struct IdentifierNode : Node {
    Token tok;
    explicit IdentifierNode(Token t) : tok(std::move(t)) {}
    std::string getValue() const { return tok.value; }
};

struct BinOpNode : Node {
    std::shared_ptr<Node> left;
    Token op_tok;
    std::shared_ptr<Node> right;
};

struct UnaryOpNode : Node {
    Token op_tok;
    std::shared_ptr<Node> operand;
};

struct AssignmentNode : Node {
    Token identifier;
    std::shared_ptr<Node> expression;
};

struct BlockNode : Node {
    std::vector<std::shared_ptr<Node>> statements;
};

struct IfNode : Node {
    std::shared_ptr<Node> condition;
    std::shared_ptr<Node> then_branch;
    std::shared_ptr<Node> else_branch;
};

struct WhileNode : Node {
    std::shared_ptr<Node> condition;
    std::shared_ptr<Node> body;
};

struct PrintNode : Node {
    std::shared_ptr<Node> value;
};

struct ProgramNode : Node {
    std::vector<std::shared_ptr<Node>> statements;
};

enum class ValueType {
    Int,
    String
};

struct DeclarationNode : Node {
    ValueType var_type;
    Token identifier;
};

