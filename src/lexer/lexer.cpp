#include "lexer.hpp"

// === Character classification from JSON ===
unordered_map<char, string> buildCharMap(const json &charClasses) {
    unordered_map<char, string> charMap;
    
    for (auto &cls : charClasses.items()) {
        string className = cls.key();
        string chars = cls.value();
        
        for (char c : chars) {
            if (c == '\\' && chars.length() > 1) {
                // Handle escape sequences
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

// === Generic DFA Engine ===
vector<Token> runDFA(
    const string &input,
    const json &rules,
    const unordered_set<string> &keywords
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

    for (size_t i = 0; i <= input.size(); ++i) {
        char c = (i < input.size()) ? input[i] : '\0';
        string cls = classifyChar(c, charMap);

        if (state == "q0" && (cls == "space" || cls == "tab" || cls == "newline")) {
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
                --i; 
            } 
           
        }
    }

    if (!cur.empty()) {
        if (finals.count(state)) {
            string tokType = stateToToken[state];
            if (state == "q_identifier" && keywords.count(cur)) {
                tokType = "KEYWORD";
            }
            tokens.push_back({tokType, cur});
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
    for (auto &kw : rules["keyword_lookup"]["keywords"]) keywords.insert(kw);
    for (auto &kw : rules["keyword_lookup"]["logical_operators"]) keywords.insert(kw);
    for (auto &kw : rules["keyword_lookup"]["arithmetic_word_operators"]) keywords.insert(kw);

    // Read Pascal 
    ifstream f(argv[1]);
    if (!f) { cerr << "Cannot open file\n"; return 1; }
    stringstream buf; buf << f.rdbuf(); string input = buf.str();

    // Run DFA
    vector<Token> toks = runDFA(input, rules, keywords);

    // Print tokens
    for (auto &t : toks)
        cout << "<" << t.type << "(" << t.lexeme << ")>\n";

    return 0;
}