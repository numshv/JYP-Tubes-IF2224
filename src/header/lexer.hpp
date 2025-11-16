#ifndef LEXER_HPP
#define LEXER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

struct Token {
    string type;
    string lexeme;
    int line;
    int column;
};

unordered_map<char, string> buildCharMap(const json &charClasses);
string classifyChar(char c, const unordered_map<char, string> &charMap);
vector<Token> runDFA(
    const string &input,
    const json &rules,
    const unordered_set<string> &keywords,
    const unordered_set<string> &logical_ops,
    const unordered_set<string> &arith_word_ops
);
int lexer_main(int argc, char* argv[]);

#endif