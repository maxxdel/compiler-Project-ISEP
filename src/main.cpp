#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include "tokens.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include "ir.hpp"
#include "codegen.hpp"

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

void print_ir(const GeneratedIR& ir)
{
    std::cout << "=== IR ===\n";
    if (!ir.constants.empty())
    {
        std::cout << "[constants]\n";
        for (const auto& kv : ir.constants)
            std::cout << "  " << kv.first << " = " << kv.second << "\n";
    }

    std::cout << "[code]\n";
    for (const auto& instr : ir.code.code)
    {
        switch (instr->kind())
        {
            case IRKind::Label:
            {
                auto& l = *static_cast<LabelCode*>(instr.get());
                std::cout << l.label << ":\n";
                break;
            }
            case IRKind::Jump:
            {
                auto& j = *static_cast<JumpCode*>(instr.get());
                std::cout << "  goto " << j.dist << "\n";
                break;
            }
            case IRKind::Compare:
            {
                auto& c = *static_cast<CompareCodeIR*>(instr.get());
                std::cout << "  if " << c.left << " " << c.operation << " " << c.right
                          << " goto " << c.jump << "\n";
                break;
            }
            case IRKind::Assignment:
            {
                auto& a = *static_cast<AssignmentCode*>(instr.get());
                if (a.op.empty())
                    std::cout << "  " << a.var << " = " << a.left << "\n";
                else
                    std::cout << "  " << a.var << " = " << a.left << " " << a.op << " " << a.right << "\n";
                break;
            }
            case IRKind::Print:
            {
                auto& p = *static_cast<PrintCodeIR*>(instr.get());
                std::cout << "  print_" << p.type << " " << p.value << "\n";
                break;
            }
        }
    }
    std::cout << "==========\n";
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

    IntermediateCodeGen irgen(root);
    auto ir = irgen.get();
    print_ir(ir);

    CodeGenerator cg(ir.code, ir.identifiers, ir.constants, ir.tempmap);
    cg.writeAsm("output.asm");
    std::cout << "\n[codegen] wrote NASM assembly to output.asm\n";

    return 0;
}

