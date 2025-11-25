#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "tokens.hpp"
#include "parser.hpp"
#include <queue>

extern void scan_string_to_tokens(const std::string &src, std::vector<Token> &out);

void print_ast(const std::shared_ptr<Node>& node, std::string prefix = "", bool isLast = true) {
    std::cerr << "Print AST printer to be implemented \n";
}

int main()
{
    std::ifstream fin("./read.txt");
    if (!fin)
    {
        std::cerr << "Cannot open read.txt\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << fin.rdbuf();
    std::string input = buffer.str();
    std::vector<Token> toks;
    try
    {
        scan_string_to_tokens(input, toks);
        TokenArray arr;
        for (auto &t : toks)
            arr.push(t);
        for (auto &t : toks)
            std::cout << t.value << " ";
        std::cout << "\n";
        Parser parser(arr);
        auto root = parser.get_root();
        print_ast(root);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    std::cout << "\n";
}

