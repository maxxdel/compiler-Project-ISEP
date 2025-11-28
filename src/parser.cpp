#include "parser.hpp"
#include <stdexcept>

static bool is_value(const Token &t, const char *v) { return t.value == v; }

Parser::Parser(TokenArray tokens) : tokens(std::move(tokens)) {
    this->tokens.appendEndIfMissing();
}

void Parser::read_token_pass(const std::string &expected, const std::string &message) {
    const Token &t = tokens.current();
    if (t.value != expected)
        throw std::runtime_error(message + " in line " + std::to_string(t.line));
    tokens.next();
}

std::shared_ptr<Node> Parser::factor() {
    const Token &tok = tokens.current();
    if (tok.type == TokenType::IntLit) {
        tokens.next();
        return std::make_shared<NumberNode>(tok);
    }
    if (tok.type == TokenType::Var) {
        tokens.next();
        return std::make_shared<IdentifierNode>(tok);
    }
    if (is_value(tok, "(")) {
        tokens.next();
        auto e = expr();
        read_token_pass(")", "Expected )");
        return e;
    }
    throw std::runtime_error("Syntax Error");
}

std::shared_ptr<Node> Parser::term() {
    auto left = factor();
    while (tokens.current().value == "*" || tokens.current().value == "/") {
        Token op = tokens.current();
        tokens.next();
        auto right = factor();
        auto bin = std::make_shared<BinOpNode>();
        bin->left = left;
        bin->op_tok = op;
        bin->right = right;
        left = bin;
    }
    return left;
}

std::shared_ptr<Node> Parser::expr() {
    auto left = term();
    while (tokens.current().value == "+" || tokens.current().value == "-") {
        Token op = tokens.current();
        tokens.next();
        auto right = term();
        auto bin = std::make_shared<BinOpNode>();
        bin->left = left;
        bin->op_tok = op;
        bin->right = right;
        left = bin;
    }
    return left;
}

std::shared_ptr<Node> Parser::comparison() {
    auto left = expr();
    while (tokens.current().value == "==" || tokens.current().value == "!=" ||
           tokens.current().value == "<" || tokens.current().value == ">") {
        Token op = tokens.current();
        tokens.next();
        auto right = expr();
        auto bin = std::make_shared<BinOpNode>();
        bin->left = left;
        bin->op_tok = op;
        bin->right = right;
        left = bin;
    }
    return left;
}

std::shared_ptr<Node> Parser::unary()
{
    if (tokens.current().value == "!")
    {
        Token op = tokens.current();
        tokens.next();
        auto right = unary();

        auto node = std::make_shared<BinOpNode>();
        node->op_tok = op;
        node->left = nullptr;
        node->right = right;
        return node;
    }
    return comparison();
}

std::shared_ptr<Node> Parser::logical_and()
{
    auto left = unary();

    while (tokens.current().value == "&&")
    {
        Token op = tokens.current();
        tokens.next();
        auto right = unary();

        auto bin = std::make_shared<BinOpNode>();
        bin->left = left;
        bin->op_tok = op;
        bin->right = right;
        left = bin;
    }
    return left;
}

std::shared_ptr<Node> Parser::logical_or()
{
    auto left = logical_and();

    while (tokens.current().value == "||")
    {
        Token op = tokens.current();
        tokens.next();
        auto right = logical_and();

        auto bin = std::make_shared<BinOpNode>();
        bin->left = left;
        bin->op_tok = op;
        bin->right = right;
        left = bin;
    }
    return left;
}

std::shared_ptr<Node> Parser::if_statement()
{
    read_token_pass("if", "Expected 'if'");
    read_token_pass("(", "Expected '('");

    auto cond = logical_or();

    read_token_pass(")", "Expected ')'");
    read_token_pass("{", "Expected '{'");

    auto then_block = statements();

    read_token_pass("}", "Expected '}'");

    auto node = std::make_shared<IfNode>();
    node->condition = cond;
    node->then_branch = then_block;
    node->else_branch = nullptr;

    if (tokens.current().type == TokenType::Else)
    {
        tokens.next();
        read_token_pass("{", "Expected '{' after else'");
        node->else_branch = statements();
        read_token_pass("}", "Expected '}' after else");
    }

    return node;
}

std::shared_ptr<Node> Parser::printing()
{
    read_token_pass("cout", "Expected 'cout'");
    read_token_pass("<<", "Expected '<<'");

    std::shared_ptr<Node> value;

    if (tokens.current().type == TokenType::String)
    {
        Token t = tokens.current();
        tokens.next();
        value = std::make_shared<NumberNode>(t);
    }
    else
    {
        value = expr();
    }

    read_token_pass(";", "Expected ';'");

    auto node = std::make_shared<PrintNode>();
    node->value = value;
    return node;
}

std::shared_ptr<Node> Parser::while_statement() {
    read_token_pass("while", "Expected while");
    read_token_pass("(", "Expected (");
    auto cond = logical_or();
    read_token_pass(")", "Expected )");
    read_token_pass("{", "Expected {");
    auto body = statements();
    read_token_pass("}", "Expected }");

    auto node = std::make_shared<WhileNode>();
    node->condition = cond;
    node->body = body;
    return node;
}

std::shared_ptr<Node> Parser::assignment()
{
    Token ident = tokens.current();

    if (!symbol_table.count(ident.value))
        throw std::runtime_error("Undeclared variable: " + ident.value);

    tokens.next();
    read_token_pass("=", "Expected '='");

    auto expr_node = expr();

    read_token_pass(";", "Expected ';'");

    auto node = std::make_shared<AssignmentNode>();
    node->identifier = ident;
    node->expression = expr_node;
    return node;
}

std::shared_ptr<Node> Parser::get_root() {
    return statements();
}


std::shared_ptr<Node> Parser::statements() {
    auto block = std::make_shared<BlockNode>();
    while (tokens.current().value != "END" && tokens.current().value != "}") {
        if (tokens.current().type == TokenType::If)
            block->statements.push_back(if_statement());
        else if (tokens.current().type == TokenType::IntKw || tokens.current().type == TokenType::StringKw)
            block->statements.push_back(declarations());
        else if (tokens.current().type == TokenType::While)
            block->statements.push_back(while_statement());
        else if (tokens.current().type == TokenType::Var)
            block->statements.push_back(assignment());
        else if (tokens.current().type == TokenType::Print)
            block->statements.push_back(printing());
        else
            throw std::runtime_error("Syntax error");
    }
    return block;
}

std::shared_ptr<Node> Parser::declarations()
{
    ValueType type;

    if (tokens.current().type == TokenType::IntKw)
        type = ValueType::Int;
    else if (tokens.current().type == TokenType::StringKw)
        type = ValueType::String;
    else
        throw std::runtime_error("Expected type");

    tokens.next();

    Token ident = tokens.current();
    if (ident.type != TokenType::Var)
        throw std::runtime_error("Expected identifier");
    tokens.next();

    read_token_pass(";", "Expected ';'");

    symbol_table[ident.value] = type;

    auto node = std::make_shared<DeclarationNode>();
    node->var_type = type;
    node->identifier = ident;
    return node;
}