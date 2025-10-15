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
    const unordered_set<string> &keywords
) 
{
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
        string cls = classifyChar(c);

        // Handle whitespace in q0 only
        if (state == "q0" && (cls == "space" || cls == "tab" || cls == "newline")) {
            continue;
        }

        // SPECIAL CASE: Handle 5..7 pattern (NON DFA)
      
        if (state == "q_number_dot" && c == '.' && i < input.size()) {
            // Emit the number (without the first dot)
            string numLexeme = cur.substr(0, cur.size() - 1); // Remove trailing '.'
            tokens.push_back({"NUMBER", numLexeme});
            
            // Now process ".." as range operator
            // Start from q0, read first '.', then second '.'
            cur = "..";
            tokens.push_back({"RANGE_OPERATOR", ".."});
            
            cur.clear();
            state = "q0";
            continue; // Skip to next character after the second '.'
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

        // Handle "other" = any except a specific character (NON DFA)
        else if (transition[state].count("other")) {
            if (state == "q_lparen_or_comment" && c != '*') {
                // Treat it as a normal '(' token
                string tokType = "LPARENTHESIS";
                tokens.push_back({tokType, "("});
                
                // Reset DFA to start state and reprocess this char
                state = rules["dfa_config"]["start_state"];
                cur.clear();
                --i;
                continue;
            } else {
                next = transition[state]["other"];
                transition_found = true;
            }
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

    // // All token definitions are now in DFA rules
    // // Removed: singleCharTokens and multiCharTokens loading

    // Read Pascal source
    ifstream f(argv[1]);
    if (!f) { cerr << "Cannot open file\n"; return 1; }
    stringstream buf; buf << f.rdbuf(); string input = buf.str();

    // Run DFA using only the rules
    vector<Token> toks = runDFA(input, rules, keywords);

    // Print tokens
    for (auto &t : toks)
        cout << "<" << t.type << "(" << t.lexeme << ")>\n";

    return 0;
}