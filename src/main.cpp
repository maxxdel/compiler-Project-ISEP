#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include "tokens.hpp"
#include "parser.hpp"
#include "ast.hpp"

extern void scan_string_to_tokens(const std::string&, std::vector<Token>&);

const char* value_type_to_string(ValueType t)
{
    switch (t)
    {
        case ValueType::Int: return "int";
        case ValueType::String: return "string";
        default: return "unknown";
    }
}

const char* token_type_to_string(TokenType t)
{
    switch (t)
    {
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::While: return "While";
        case TokenType::IntKw: return "Int";
        case TokenType::StringKw: return "StringKw";
        case TokenType::Var: return "Var";
        case TokenType::IntLit: return "IntLit";
        case TokenType::String: return "String";
        case TokenType::Assign: return "Assign";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Equal: return "Equal";
        case TokenType::NotEqual: return "NotEqual";
        case TokenType::Less: return "Less";
        case TokenType::LessEq: return "LessEq";
        case TokenType::Greater: return "Greater";
        case TokenType::GreaterEq: return "GreaterEq";
        case TokenType::And: return "And";
        case TokenType::Or: return "Or";
        case TokenType::Not: return "Not";
        case TokenType::Print: return "Cout";
        case TokenType::PrintBrackets: return "<<";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen";
        case TokenType::LBrace: return "LBrace";
        case TokenType::RBrace: return "RBrace";
        case TokenType::End: return "End";
        default: return "Unknown";
    }
}

void print_tokens(const std::vector<Token>& tokens)
{
    std::cout << "=== TOKENS ===\n";
    for (const auto& t : tokens)
    {
        std::cout << "(" << token_type_to_string(t.type)
                  << ", \"" << t.value
                  << "\", line " << t.line << ")\n";
    }
    std::cout << "===============\n\n";
}

void print_ast(const std::shared_ptr<Node>& node, int indent = 0)
{
    if (!node) return;

    auto pad = [&]() {
        for (int i = 0; i < indent; ++i)
            std::cout << "  ";
    };

    if (auto n = std::dynamic_pointer_cast<NumberNode>(node))
    {
        pad(); std::cout << "Number(" << n->getValue() << ")\n";
    }
    else if (auto id = std::dynamic_pointer_cast<IdentifierNode>(node))
    {
        pad(); std::cout << "Identifier(" << id->getValue() << ")\n";
    }
    else if (auto bin = std::dynamic_pointer_cast<BinOpNode>(node))
    {
        pad(); std::cout << "BinOp(" << bin->op_tok.value << ")\n";
        print_ast(bin->left, indent + 1);
        print_ast(bin->right, indent + 1);
    }
    else if (auto asg = std::dynamic_pointer_cast<AssignmentNode>(node))
    {
        pad(); std::cout << "Assignment(" << asg->identifier.value << ")\n";
        print_ast(asg->expression, indent + 1);
    }
    else if (auto dec = std::dynamic_pointer_cast<DeclarationNode>(node))
    {
        pad(); std::cout << "Declaration(type=" << value_type_to_string(dec->var_type) << ", name=" << dec->identifier.value << ")\n";
    }

    else if (auto p = std::dynamic_pointer_cast<PrintNode>(node))
    {
        pad(); std::cout << "Print\n";
        print_ast(p->value, indent + 1);
    }
    else if (auto blk = std::dynamic_pointer_cast<BlockNode>(node))
    {
        pad(); std::cout << "Block\n";
        for (auto &st : blk->statements)
        {
            print_ast(st, indent + 1);
        }
    }
    else if (auto iff = std::dynamic_pointer_cast<IfNode>(node))
    {
        pad(); std::cout << "If\n";

        pad(); std::cout << "Condition:\n";
        print_ast(iff->condition, indent + 1);

        pad(); std::cout << "Then:\n";
        print_ast(iff->then_branch, indent + 1);

        if (iff->else_branch)
        {
            pad(); std::cout << "Else:\n";
            print_ast(iff->else_branch, indent + 1);
        }
    }
    else if (auto wh = std::dynamic_pointer_cast<WhileNode>(node))
    {
        pad(); std::cout << "While\n";

        pad(); std::cout << "Condition:\n";
        print_ast(wh->condition, indent + 1);

        pad(); std::cout << "Body:\n";
        print_ast(wh->body, indent + 1);
    }
}

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        std::cerr << "usage: ./mini_compiler file.txt\n";
        return 1;
    }

    std::ifstream in(argv[1]);
    if (!in)
    {
        std::cerr << "Cannot open file\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string src = buffer.str();

    std::vector<Token> toks;
    scan_string_to_tokens(src, toks);

    print_tokens(toks);

    TokenArray arr(std::move(toks));
    Parser parser(arr);

    auto root = parser.get_root();

    std::cout << "=== AST ===\n";
    print_ast(root);
    std::cout << "===========\n";

    return 0;
}
