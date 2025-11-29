#include "header/lexer.hpp"
#include "header/parser.hpp"
#include "header/ast.hpp"
#include "header/semantic.hpp"

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

    unordered_set<string> keywords;
    unordered_set<string> logical_ops;
    unordered_set<string> arith_word_ops;
    for (auto &kw : rules["keyword_lookup"]["keywords"]) keywords.insert(kw);
    for (auto &kw : rules["keyword_lookup"]["logical_operators"]) logical_ops.insert(kw);
    for (auto &kw : rules["keyword_lookup"]["arithmetic_word_operators"]) arith_word_ops.insert(kw);

    // Read Pascal 
    ifstream f(argv[1]);
    if (!f) { cerr << "Cannot open file\n"; return 1; }
    stringstream buf; buf << f.rdbuf(); string input = buf.str();

    // Run DFA
    cout << "\n========== Generated Token ==========\n";
    vector<Token> toks = runDFA(input, rules, keywords, logical_ops, arith_word_ops);

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

    ParseNode* parseTree = parser_main(toks);
    ASTNode* ast = ASTMain(parseTree);
    
    if (ast) {
        semanticAnalysis(ast);
    
        printSymbolTables();
        
        delete ast;
    }
    
    return 0;
}