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
};

string classifyChar(char c);
vector<Token> runDFA(
    const string &input,
    const json &rules,
    const unordered_set<string> &keywords,
    const unordered_map<string,string> &singleCharTokens,
    const unordered_map<string,string> &multiCharTokens
);
int lexer_main(int argc, char* argv[]);

#endif