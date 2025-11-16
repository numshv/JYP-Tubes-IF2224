#include "lexer.hpp"

unordered_map<char, string> buildCharMap(const json &charClasses) {
    unordered_map<char, string> charMap;
    
    for (auto &cls : charClasses.items()) {
        string className = cls.key();
        string chars = cls.value();
        
        for (char c : chars) {
            if (c == '\\' && chars.length() > 1) {
                size_t pos = chars.find(c);
                if (pos != string::npos && pos + 1 < chars.length()) {
                    char escaped = chars[pos + 1];
                    switch (escaped) {
                        case 'n': charMap['\n'] = className; break;
                        case 't': charMap['\t'] = className; break;
                        case 'r': charMap['\r'] = className; break;
                        case '\\': charMap['\\'] = className; break;
                        case '\'': charMap['\''] = className; break;
                        default: charMap[escaped] = className; break;
                    }
                    chars = chars.substr(pos + 2); 
                    continue;
                }
            }
            charMap[c] = className;
        }
    }
    
    charMap['\0'] = "eof";
    return charMap;
}

string classifyChar(char c, const unordered_map<char, string> &charMap) {
    auto it = charMap.find(c);
    if (it != charMap.end()) {
        return it->second;
    }
    return "any";
}

vector<Token> runDFA(
    const string &input,
    const json &rules,
    const unordered_set<string> &keywords,
    const unordered_set<string> &logical_ops,
    const unordered_set<string> &arith_word_ops
) 
{
    // Build character classification map
    auto charMap = buildCharMap(rules["character_classes"]);
    
    // Build transition table
    unordered_map<string, unordered_map<string,string>> transition;
    for (auto &block : rules["transitions"]) {
        for (auto &r : block["rules"]) {
            string from = r["from"], inp = r["input"], to = r["to"];
            transition[from][inp] = to;
        }
    }

    unordered_set<string> finals;
    for (auto &s : rules["dfa_config"]["final_states"])
        finals.insert(s);

    unordered_map<string,string> stateToToken =
        rules["state_token_map"].get<unordered_map<string,string>>();

    vector<Token> tokens;
    string state = rules["dfa_config"]["start_state"];
    string cur;

    // Tracking line and column (1-based)
    int line = 1;
    int column = 1;
    int token_start_line = 1;
    int token_start_column = 1;

    for (size_t i = 0; i <= input.size(); ++i) {
        char c = (i < input.size()) ? input[i] : '\0';
        string cls = classifyChar(c, charMap);

        if (state == "q0" && (cls == "space" || cls == "tab" || cls == "newline")) {
            // consume whitespace: update line/column
            if (c == '\n') { line++; column = 1; }
            else { column++; }
            continue;
        }

        string next;
        bool transition_found = false;

        // Try exact character match first
        if (transition[state].count(string(1, c))) {
            next = transition[state][string(1, c)];
            transition_found = true;
        }
        // Try character class match
        else if (transition[state].count(cls)) {
            next = transition[state][cls];
            transition_found = true;
        }
        // Try 'any' transition
        else if (transition[state].count("any") && c != '\0') {
            next = transition[state]["any"];
            transition_found = true;
        }

        if (transition_found) {
            if (cur.empty()) {
                token_start_line = line;
                token_start_column = column;
            }
            cur += c;
            state = next;

            // update line/column after consuming the character
            if (c == '\n') { line++; column = 1; }
            else { column++; }
        } else {
            // No valid transition -> check if we ended a token
            if (finals.count(state)) {
                string tokType = stateToToken[state];
                if (state == "q_identifier") {
                    if (logical_ops.count(cur)) {
                        tokType = "LOGICAL_OPERATOR";
                    } else if (arith_word_ops.count(cur)) {
                        tokType = "ARITHMETIC_OPERATOR";
                    } else if (keywords.count(cur)) {
                        tokType = "KEYWORD";
                    }
                }
                
                if (tokType == "ERROR" || state == "q_error") {
                    tokens.push_back({tokType, cur, token_start_line, token_start_column});
                    cerr << "Lexical Error: Invalid token '" << cur << "' at line "
                         << token_start_line << ", column " << token_start_column << endl;
                    return tokens;
                }
                
                tokens.push_back({tokType, cur, token_start_line, token_start_column});
                cur.clear();
                state = rules["dfa_config"]["start_state"];
                --i; // reprocess current character in start state (we did not consume it)
            } else if (!cur.empty()) {
                cerr << "Lexical Error: Invalid token '" << cur << "' at line "
                     << token_start_line << ", column " << token_start_column << endl;
                tokens.push_back({"ERROR", cur, token_start_line, token_start_column});
                return tokens;
            }
        }
    }

    if (!cur.empty()) {
        if (finals.count(state)) {
            string tokType = stateToToken[state];
            if (state == "q_identifier") {
                if (logical_ops.count(cur)) {
                    tokType = "LOGICAL_OPERATOR";
                } else if (arith_word_ops.count(cur)) {
                    tokType = "ARITHMETIC_OPERATOR";
                } else if (keywords.count(cur)) {
                    tokType = "KEYWORD";
                }
            }
            if (tokType == "ERROR" || state == "q_error") {
                tokens.push_back({tokType, cur, token_start_line, token_start_column});
                cerr << "Lexical Error: Invalid token '" << cur << "' at line "
                     << token_start_line << ", column " << token_start_column << " (end of input)" << endl;
                return tokens;
            }
            tokens.push_back({tokType, cur, token_start_line, token_start_column});
        } else {
            cerr << "Lexical Error: Invalid token '" << cur << "' at line "
                 << token_start_line << ", column " << token_start_column << " (end of input)" << endl;
            tokens.push_back({"ERROR", cur, token_start_line, token_start_column});
        }
    }

    return tokens;
}

int lexer_main(int argc, char* argv[]) {
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
        return 0;
    }

    return 1;
}