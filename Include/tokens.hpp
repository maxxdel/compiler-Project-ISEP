#pragma once
#include <string>
#include <vector>
#include <stdexcept>

enum class TokenType {
    If,
    Else,
    While,
    IntKw,
    StringKw,
    Print,
    PrintBrackets,

    Var,
    String,
    IntLit,

    Assign,
    Equal,
    NotEqual,
    Less,
    LessEq,
    Greater,
    GreaterEq,
    And,
    Or,
    Not,

    Semicolon,
    Comma,
    LParen,
    RParen,
    LBrace,
    RBrace,

    Plus,
    Minus,
    Slash,
    Star,

    End
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

struct TokenArray {
    std::vector<Token> tokens;
    size_t pos{0};

    TokenArray() = default;
    explicit TokenArray(std::vector<Token> t)
        : tokens(std::move(t)), pos(0) {}

    const Token& current() const {
        if (tokens.empty())
            throw std::runtime_error("TokenArray is empty");
        if (pos >= tokens.size())
            return tokens.back();
        return tokens[pos];
    }

    void next() {
        if (pos + 1 < tokens.size())
            ++pos;
    }

    void appendEndIfMissing() {
        if (tokens.empty() || tokens.back().type != TokenType::End) {
            int line = tokens.empty() ? 1 : tokens.back().line;
            tokens.push_back(Token{TokenType::End, "END", line});
        }
    }
};
