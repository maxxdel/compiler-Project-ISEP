#pragma once
#include <memory>
#include <string>
#include <vector>
#include "tokens.hpp"

struct Node
{
    virtual ~Node() = default;
};

struct NumberNode : Node
{
    Token tok;
    explicit NumberNode(Token t) : tok(std::move(t)) {}
    std::string getValue() const { return tok.value; }
};

struct IdentifierNode : Node
{
    Token tok;
    explicit IdentifierNode(Token t) : tok(std::move(t)) {}
    std::string getValue() const { return tok.value; }
};

struct BinOpNode : Node
{
    std::shared_ptr<Node> left;
    Token op_tok;
    std::shared_ptr<Node> right;
};

struct Statement : Node
{
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right; // may be null
};

struct Condition : Node
{
    
};

struct IfStatement : Node
{

};

struct WhileStatement : Node
{

};

struct PrintStatement : Node
{

};

struct Assignment : Node
{
    Token identifier;
    std::shared_ptr<Node> expression;
};

struct Declaration : Node
{

};
