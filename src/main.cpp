#include "lexer.hpp"
#include "parser.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <source_file.pas>\n";
        return 1;
    }

    const string ruleFile = "test/milestone-1/rule.json";
    ifstream jfile(ruleFile);
    if (!jfile) {
        cerr << "Error: cannot open " << ruleFile << "\n";
        return 1;
    }
    json rules;
    jfile >> rules;

    // Load keyword/operator sets
    unordered_set<string> keywords;
    for (auto &kw : rules["keyword_lookup"]["keywords"]) keywords.insert(kw);
    for (auto &kw : rules["keyword_lookup"]["logical_operators"]) keywords.insert(kw);
    for (auto &kw : rules["keyword_lookup"]["arithmetic_word_operators"]) keywords.insert(kw);

    // Read Pascal 
    ifstream f(argv[1]);
    if (!f) { cerr << "Cannot open file\n"; return 1; }
    stringstream buf; buf << f.rdbuf(); string input = buf.str();

    // Run DFA
    vector<Token> toks = runDFA(input, rules, keywords);

    bool hasError = false;
    for (auto &t : toks) {
        if (t.type == "ERROR") {
            hasError = true;
            break;
        }
    }

    if (!hasError) {
        for (auto &t : toks) {
            std::cerr << "<" << t.type << "(" << t.lexeme << ")>\n";
        }
    }

    parser_main(toks);
    return 0;
}