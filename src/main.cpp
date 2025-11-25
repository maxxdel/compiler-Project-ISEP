#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include "tokens.hpp"
#include "parser.hpp"

extern void scan_string_to_tokens(const std::string&, std::vector<Token>&);

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "usage: ./mini_compiler file.txt\n";
        return 1;
    }

    std::ifstream in(argv[1]);
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string src = buffer.str();

    std::vector<Token> toks;
    scan_string_to_tokens(src, toks);

    TokenArray arr(std::move(toks));
    Parser parser(arr);

    auto root = parser.get_root();

    std::cout << "Parsed successfully.\n";
    return 0;
}
