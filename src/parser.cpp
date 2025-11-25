#include "parser.hpp"
#include <stdexcept>

struct Statement {};

struct Program
{
    std::vector<std::unique_ptr<Statement>> statements;
};

struct AssignStmt : Statement {
    std::string name;
    std::unique_ptr<Expr> value;
};

struct BlockStmt : Statement {
    std::vector<std::unique_ptr<Statement>> statements;
};

struct IfStmt : Statement {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch; // null si pas de else
};

struct WhileStmt : Statement {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> body;
};

struct VarExpr : Expr {
    std::string name;
};

static bool is_value(const Token &t, const char *v) { return t.value == v; }

Parser::Parser(TokenArray tokens) : tokens(std::move(tokens))
{
    this->tokens.appendEndIfMissing();
}

void Parser::read_token_pass(const std::string &expected, const std::string &message)
{
    const Token &t = tokens.current();
    if (t.value != expected)
        throw std::runtime_error(message + " in line " + std::to_string(t.line));
    tokens.next();
}

std::shared_ptr<Node> Parser::factor()
{
    const Token &tok = tokens.current();
    if (tok.type == TokenType::IntLit)
    {
        tokens.next();
        return std::make_shared<NumberNode>(tok);
    }
    if (tok.type == TokenType::Var)
    {
        tokens.next();
        return std::make_shared<IdentifierNode>(tok);
    }
    if (is_value(tok, "("))
    {
        tokens.next();
        auto e = expr();
        if (!is_value(tokens.current(), ")"))
            throw std::runtime_error("Expected ) in line " + std::to_string(tokens.current().line));
        tokens.next();
        return e;
    }
    throw std::runtime_error("Syntax Error: Expected Integer in line " + std::to_string(tok.line));
}

std::shared_ptr<Node> Parser::term()
{
    auto left = factor();
    return left;
}

std::shared_ptr<Node> Parser::expr()
{
    auto left = term();
    while (tokens.current().value == "+" || tokens.current().value == "-")
    {
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

std::shared_ptr<Condition> Parser::condition()
{
    return nullptr;
}

std::shared_ptr<Node> Parser::if_statement()
{
    return nullptr;
}

std::shared_ptr<Node> Parser::declarations()
{
    return nullptr;
}

std::shared_ptr<Node> Parser::assignment()
{
    Token identifier = tokens.current();
    tokens.next();
    read_token_pass("=", "Missing =");
    auto expression = expr();
    read_token_pass(";", "Missing ;");
    auto a = std::make_shared<Assignment>();
    a->identifier = identifier;
    a->expression = expression;
    return a;
}

std::shared_ptr<Node> Parser::while_statement()
{
    return nullptr;
}

std::shared_ptr<Node> Parser::printing(const std::string &type)
{
    return nullptr;
}

std::shared_ptr<Node> Parser::statements()
{
    std::shared_ptr<Node> left = nullptr;
    std::shared_ptr<Node> right = nullptr;
    Token t = tokens.current();
    while (t.value != "END" && t.value != "}")
    {
        if (t.type == TokenType::If)
        {
           
        }
        else if (t.type == TokenType::While)
        {

        }
        else if (t.type == TokenType::StringKw || t.type == TokenType::Int)
        {
            right = declarations();
        }
        else if (t.type == TokenType::Print)
        {
       
        }
        else if (t.type == TokenType::Prints)
        {
            
        }
        else if (t.type == TokenType::Var)
        {
            right = assignment();
        }
        else
        {
            throw std::runtime_error("Syntax Error in line " + std::to_string(t.line));
        }
        auto st = std::make_shared<Statement>();
        st->left = left;
        st->right = right;
        left = st;
        t = tokens.current();
    }
    if (t.value == "END")
        return left;
    if (t.value == "}")
    {

    }
    return left;
}

std::shared_ptr<Node> Parser::get_root()
{
    auto root = statements();
    if (tokens.current().type != TokenType::End)
        throw std::runtime_error("Syntax Error");
    return root;
}
