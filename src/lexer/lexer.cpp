
#include "lexer.hpp"

// === Character classification ===
string classifyChar(char c) {
    if (isalpha(c)) return "letter";
    if (isdigit(c)) return "digit";
    if (c == '_') return "_";
    if (c == '.') return ".";
    if (c == '\'') return "'";
    if (c == '{') return "{";
    if (c == '}') return "}";
    if (c == '(') return "(";
    if (c == ')') return ")";
    if (c == '*') return "*";
    if (c == ' ') return "space";
    if (c == '\t') return "tab";
    if (c == '\n') return "newline";
    if (c == '+') return "+";
    if (c == '-') return "-";
    if (c == 'e') return "e";
    if (c == 'E') return "E";
    return "any";
}

// === Generic DFA Engine ===
vector<Token> runDFA(
    const string &input,
    const json &rules,
    const unordered_set<string> &keywords,
    const unordered_map<string,string> &singleCharTokens,
    const unordered_map<string,string> &multiCharTokens
) {
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

    // Load special contexts for negative numbers
    unordered_set<string> negative_contexts;
    if (rules.contains("special_cases") && rules["special_cases"].contains("negative_number_contexts")) {
        for (auto &ctx : rules["special_cases"]["negative_number_contexts"]) {
            negative_contexts.insert(ctx);
        }
    }

    vector<Token> tokens;
    string state = rules["dfa_config"]["start_state"];
    string cur;

    // Helper function to check if last token is in negative number context
    auto isNegativeNumberContext = [&]() -> bool {
        if (tokens.empty()) return true; // Beginning of input
        string last_type = tokens.back().type;
        return negative_contexts.count(last_type) || 
               last_type == "LPARENTHESIS" || 
               last_type == "COMMA" ||
               last_type == "ASSIGN_OPERATOR" ||
               last_type == "RELATIONAL_OPERATOR" ||
               last_type == "ARITHMETIC_OPERATOR";
    };

    for (size_t i = 0; i <= input.size(); ++i) {
        char c = (i < input.size()) ? input[i] : '\0';
        string cls = classifyChar(c);

        // Handle whitespace in q0 only
        if (state == "q0" && (cls == "space" || cls == "tab" || cls == "newline")) {
            continue;
        }

        // Handle single/multi-character tokens only in q0
        if (state == "q0") {
            string one(1, c);
            
            // Check for multi-char tokens first
            if (i + 1 < input.size()) {
                string two = one + input[i + 1];
                if (multiCharTokens.count(two)) {
                    tokens.push_back({multiCharTokens.at(two), two});
                    ++i;
                    continue;
                }
            }
            
            // Handle minus sign specially for negative numbers
            if (c == '-' && isNegativeNumberContext()) {
                // This could be start of negative number
                cur += c;
                state = "q_minus";
                continue;
            }
            
            // Other single char tokens
            if (singleCharTokens.count(one)) {
                tokens.push_back({singleCharTokens.at(one), one});
                continue;
            }
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
            cur += c;
            state = next;
        } else {
            // No valid transition -> check if we ended a token
            if (finals.count(state)) {
                string tokType = stateToToken[state];
                if (state == "q_identifier" && keywords.count(cur)) {
                    tokType = "KEYWORD";
                }
                tokens.push_back({tokType, cur});
                cur.clear();
                state = rules["dfa_config"]["start_state"];
                --i; // reprocess current char
            } else if (state == "q_minus") {
                // Minus followed by non-digit -> it's a subtraction operator
                tokens.push_back({"ARITHMETIC_OPERATOR", "-"});
                cur.clear();
                state = rules["dfa_config"]["start_state"];
                --i;
            }
        }
    }

    // Handle any remaining buffer at end of input
    if (!cur.empty()) {
        if (finals.count(state)) {
            string tokType = stateToToken[state];
            if (state == "q_identifier" && keywords.count(cur)) {
                tokType = "KEYWORD";
            }
            tokens.push_back({tokType, cur});
        } else if (state == "q_minus") {
            tokens.push_back({"ARITHMETIC_OPERATOR", "-"});
        }
    }

    return tokens;
}

int lexer_main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <source_file.pas>\n";
        return 1;
    }

    // Load rule.json
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

    unordered_map<string,string> singleCharTokens =
        rules["single_char_tokens"].get<unordered_map<string,string>>();
    unordered_map<string,string> multiCharTokens =
        rules["multi_char_tokens"].get<unordered_map<string,string>>();

    // Read Pascal source
    ifstream f(argv[1]);
    if (!f) { cerr << "Cannot open file\n"; return 1; }
    stringstream buf; buf << f.rdbuf(); string input = buf.str();

    // Run DFA using only the rules
    vector<Token> toks = runDFA(input, rules, keywords, singleCharTokens, multiCharTokens);

    // Print tokens
    for (auto &t : toks)
        cout << "<" << t.type << "(" << t.lexeme << ")>\n";

    return 0;
}