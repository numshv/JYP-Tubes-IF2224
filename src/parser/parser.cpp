#include "parser.hpp"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

vector<Token> tokens;
int current = 0;
bool gDebug = false;

// ========== UTILITY ==========

Token getCurrentToken() {
    if (current < tokens.size()) return tokens[current];
    return {"EOF", ""};
}

Token cur_tok = getCurrentToken();

void advance() {
    if (current < tokens.size()) current++;
    cur_tok = getCurrentToken();
}

void debugEnter(const string &rule) {
    if (!gDebug) return;
    cerr << ">>> Entering rule: " << rule << " | Current token: (" 
         << cur_tok.type << ", '" << cur_tok.lexeme << "')\n";
}

void debugExit(const string &rule) {
    if (!gDebug) return;
    cerr << "<<< Exiting rule: " << rule << " | Next token: (" 
         << cur_tok.type << ", '" << cur_tok.lexeme << "')\n";
}

// ========== Parse Tree ==========
ParseNode* makeNode(const string &label) {
    auto *n = new ParseNode{label, {}};
    return n;
}

ParseNode* makeTokenNode(const Token &t) {
    auto *n = new ParseNode{t.type + "(" + t.lexeme + ")", {}};
    return n;
}

void addChild(ParseNode* parent, ParseNode* child) {
    if (parent && child) parent->children.push_back(child);
}

void printTree(ParseNode* node, const string &prefix, bool isLast) {
    if (!node) return;

    static bool isRootPrinted = false;
    if (!isRootPrinted) {
        cout << node->label << "\n";
        isRootPrinted = true;
    }
    for (size_t i = 0; i < node->children.size(); ++i) {
        bool last = (i + 1 == node->children.size());
        cout << prefix << (last ? "└── " : "├── ") << node->children[i]->label << "\n";
        string newPrefix = prefix + (last ? "    " : "│   ");
        printTree(node->children[i], newPrefix, last);
    }
    if (prefix.empty()) {
        isRootPrinted = false;
    }
}


ParseNode* matchType(const string &expectedType) {
    if (cur_tok.type == expectedType) {
        if (gDebug) cerr << "Matched type: " << expectedType << " (" << cur_tok.lexeme << ")\n";
        Token t = cur_tok;
        advance();
        return makeTokenNode(t);
    } else {
        cerr << "Syntax error: expected type '" << expectedType 
             << "' but got (" << cur_tok.type << ", '" << cur_tok.lexeme << "')\n";
        return makeNode("<missing-" + expectedType + ">");
    }
}

ParseNode* matchToken(const Token &expected) {
    if (cur_tok.type == expected.type && cur_tok.lexeme == expected.lexeme) {
        if (gDebug) cerr << "Matched token: <" << expected.type << ", '" << expected.lexeme << "'>\n";
        Token t = cur_tok;
        advance();
        return makeTokenNode(t);
    } else {
        cerr << "Syntax error: expected token (" << expected.type << ", '" 
             << expected.lexeme << "') but got (" << cur_tok.type << ", '" 
             << cur_tok.lexeme << "')\n";
        return makeNode("<missing-" + expected.type + ">");
    }
}

ParseNode* tryMatchToken(const Token &expected) {
    if (cur_tok.type == expected.type && cur_tok.lexeme == expected.lexeme) {
        Token t = cur_tok;
        advance();
        return makeTokenNode(t);
    }
    return nullptr;
}

// ========== GRAMMAR RULES ==========

// program → program-header + declaration-part + compound-statement + DOT
ParseNode* program() {
    debugEnter("program");
    auto *node = makeNode("<program>");
    addChild(node, program_header());
    addChild(node, declaration_part());
    addChild(node, compound_statement());
    addChild(node, matchType("DOT"));
    debugExit("program");
    return node;
}

// program-header → KEYWORD(program) + IDENTIFIER + SEMICOLON
ParseNode* program_header() {
    debugEnter("program_header");
    auto *node = makeNode("<program-header>");
    addChild(node, matchToken({"KEYWORD", "program"}));
    addChild(node, matchType("IDENTIFIER"));
    addChild(node, matchType("SEMICOLON"));
    debugExit("program_header");
    return node;
}

// declaration-part → (const-declaration)* + (type-declaration)* + (var-declaration)* + (subprogram-declaration)*
ParseNode* declaration_part() {
    debugEnter("declaration_part");
    auto *node = makeNode("<declaration-part>");
    while (cur_tok.lexeme == "konstanta") addChild(node, const_declaration());
    while (cur_tok.lexeme == "tipe") addChild(node, type_declaration());
    while (cur_tok.lexeme == "variabel") addChild(node, var_declaration());
    while (cur_tok.lexeme == "prosedur" || cur_tok.lexeme == "fungsi") 
        addChild(node, subprogram_declaration());
    debugExit("declaration_part");
    return node;
}

// const-declaration → KEYWORD(konstanta) + (IDENTIFIER := value + SEMICOLON)+
ParseNode* const_declaration() {
    debugEnter("const_declaration");
    auto *node = makeNode("<const-declaration>");
    addChild(node, matchToken({"KEYWORD", "konstanta"}));
    do {
        addChild(node, matchType("IDENTIFIER"));
        addChild(node, matchType("ASSIGN_OPERATOR"));
        if (cur_tok.type == "NUMBER" || cur_tok.type == "CHAR_LITERAL" ||
            cur_tok.type == "STRING_LITERAL" || cur_tok.type == "BOOLEAN" ||
            cur_tok.type == "IDENTIFIER") {
            addChild(node, makeTokenNode(cur_tok));
            advance();
        } else {
            cerr << "Expected constant value but got " << cur_tok.lexeme << endl;
        }
        addChild(node, matchType("SEMICOLON"));
    } while (cur_tok.type == "IDENTIFIER");
    debugExit("const_declaration");
    return node;
}

// type-declaration → KEYWORD(tipe) + (IDENTIFIER := type-definition + SEMICOLON)+
ParseNode* type_declaration() {
    debugEnter("type_declaration");
    auto *node = makeNode("<type-declaration>");
    addChild(node, matchToken({"KEYWORD", "tipe"}));
    do {
        addChild(node, matchType("IDENTIFIER"));
        addChild(node, matchType("ASSIGN_OPERATOR"));
        addChild(node, type_definition());
        addChild(node, matchType("SEMICOLON"));
    } while (cur_tok.type == "IDENTIFIER");
    debugExit("type_declaration");
    return node;
}

// type-definition → type | range
ParseNode* type_definition() {
    debugEnter("type_definition");
    auto *node = makeNode("<type-definition>");
    if (cur_tok.lexeme == "larik" || cur_tok.lexeme == "integer" ||
        cur_tok.lexeme == "real" || cur_tok.lexeme == "boolean" || 
        cur_tok.lexeme == "char") {
        addChild(node, type_spec());
    } else {
        addChild(node, range());
    }
    debugExit("type_definition");
    return node;
}

// var-declaration → KEYWORD(variabel) + (identifier-list + COLON + type + SEMICOLON)+
ParseNode* var_declaration() {
    debugEnter("var_declaration");
    auto *node = makeNode("<var-declaration>");
    addChild(node, matchToken({"KEYWORD", "variabel"}));
    do {
        addChild(node, identifier_list());
        addChild(node, matchType("COLON"));
        addChild(node, type_spec());
        addChild(node, matchType("SEMICOLON"));
    } while (cur_tok.type == "IDENTIFIER");
    debugExit("var_declaration");
    return node;
}

// var declaration with boolean -> KEYWORD(variabel) + (identifier-list + COLON + KEYWORD(boolean) + SEMICOLON)+
ParseNode* var_declaration_boolean() {
    debugEnter("var_declaration_boolean");
    auto *node = makeNode("<var-declaration-boolean>");
    addChild(node, matchToken({"KEYWORD", "variabel"}));
    do {
        addChild(node, identifier_list());
        addChild(node, matchType("COLON"));
        addChild(node, matchToken({"KEYWORD", "boolean"}));
        addChild(node, matchType("SEMICOLON"));
    } while (cur_tok.type == "IDENTIFIER");
    debugExit("var_declaration_boolean");
    return node;
}

// identifier-list → IDENTIFIER (COMMA + IDENTIFIER)*
ParseNode* identifier_list() {
    debugEnter("identifier_list");
    auto *node = makeNode("<identifier-list>");
    addChild(node, matchType("IDENTIFIER"));
    while (cur_tok.type == "COMMA") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
        addChild(node, matchType("IDENTIFIER"));
    }
    debugExit("identifier_list");
    return node;
}

// type → KEYWORD(integer/real/boolean/char) | array-type
ParseNode* type_spec() {
    debugEnter("type_spec");
    auto *node = makeNode("<type>");
    if (cur_tok.lexeme == "integer" || cur_tok.lexeme == "real" ||
        cur_tok.lexeme == "boolean" || cur_tok.lexeme == "char") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
    } else if (cur_tok.lexeme == "larik") {
        addChild(node, array_type());
    } else {
        cerr << "Unknown type: " << cur_tok.lexeme << endl;
    }
    debugExit("type_spec");
    return node;
}

// array-type → KEYWORD(larik) + LBRACKET + range + RBRACKET + KEYWORD(dari) + type
ParseNode* array_type() {
    debugEnter("array_type");
    auto *node = makeNode("<array-type>");
    addChild(node, matchToken({"KEYWORD", "larik"}));
    addChild(node, matchType("LBRACKET"));
    addChild(node, range());
    addChild(node, matchType("RBRACKET"));
    addChild(node, matchToken({"KEYWORD", "dari"}));
    addChild(node, type_spec());
    debugExit("array_type");
    return node;
}

// range → expression + RANGE_OPERATOR(..) + expression
ParseNode* range() {
    debugEnter("range");
    auto *node = makeNode("<range>");
    addChild(node, expression());
    addChild(node, matchType("RANGE_OPERATOR"));
    addChild(node, expression());
    debugExit("range");
    return node;
}

// subprogram-declaration → procedure-declaration | function-declaration
ParseNode* subprogram_declaration() {
    debugEnter("subprogram_declaration");
    auto *node = makeNode("<subprogram-declaration>");
    if (cur_tok.lexeme == "prosedur") addChild(node, procedure_declaration());
    else if (cur_tok.lexeme == "fungsi") addChild(node, function_declaration());
    debugExit("subprogram_declaration");
    return node;
}

// procedure-declaration → KEYWORD(prosedur) + IDENTIFIER + (formal-parameter-list)? + SEMICOLON + block + SEMICOLON
ParseNode* procedure_declaration() {
    debugEnter("procedure_declaration");
    auto *node = makeNode("<procedure-declaration>");
    addChild(node, matchToken({"KEYWORD", "prosedur"}));
    addChild(node, matchType("IDENTIFIER"));
    if (cur_tok.type == "LPARENTHESIS") addChild(node, formal_parameter_list());
    addChild(node, matchType("SEMICOLON"));
    addChild(node, block());
    addChild(node, matchType("SEMICOLON"));
    debugExit("procedure_declaration");
    return node;
}

// function-declaration → KEYWORD(fungsi) + IDENTIFIER + (formal-parameter-list)? + COLON + type + SEMICOLON + block + SEMICOLON
ParseNode* function_declaration() {
    debugEnter("function_declaration");
    auto *node = makeNode("<function-declaration>");
    addChild(node, matchToken({"KEYWORD", "fungsi"}));
    addChild(node, matchType("IDENTIFIER"));
    if (cur_tok.type == "LPARENTHESIS") addChild(node, formal_parameter_list());
    addChild(node, matchType("COLON"));
    addChild(node, type_spec());
    addChild(node, matchType("SEMICOLON"));
    addChild(node, block());
    addChild(node, matchType("SEMICOLON"));
    debugExit("function_declaration");
    return node;
}

// formal-parameter-list → LPARENTHESIS + parameter-group (SEMICOLON + parameter-group)* + RPARENTHESIS
ParseNode* formal_parameter_list() {
    debugEnter("formal_parameter_list");
    auto *node = makeNode("<formal-parameter-list>");
    addChild(node, matchType("LPARENTHESIS"));
    addChild(node, parameter_group());
    while (cur_tok.type == "SEMICOLON") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
        addChild(node, parameter_group());
    }
    addChild(node, matchType("RPARENTHESIS"));
    debugExit("formal_parameter_list");
    return node;
}

// parameter-group → identifier-list + COLON + type
ParseNode* parameter_group() {
    debugEnter("parameter_group");
    auto *node = makeNode("<parameter-group>");
    addChild(node, identifier_list());
    addChild(node, matchType("COLON"));
    addChild(node, type_spec());
    debugExit("parameter_group");
    return node;
}

// block → declaration-part? + compound-statement
ParseNode* block() {
    debugEnter("block");
    auto *node = makeNode("<block>");

    if (cur_tok.lexeme == "konstanta" ||
        cur_tok.lexeme == "tipe" ||
        cur_tok.lexeme == "variabel") {
        addChild(node, declaration_part());
    }
    
    addChild(node, compound_statement());

    debugExit("block");
    return node;
}

// compound-statement → KEYWORD(mulai) + statement-list + KEYWORD(selesai)
ParseNode* compound_statement() {
    debugEnter("compound_statement");
    auto *node = makeNode("<compound-statement>");
    addChild(node, matchToken({"KEYWORD", "mulai"}));
    addChild(node, statement_list());
    addChild(node, matchToken({"KEYWORD", "selesai"}));
    debugExit("compound_statement");
    return node;
}

bool isStatementStart(const Token &t) {
    return t.type == "IDENTIFIER" || t.lexeme == "jika" ||
           t.lexeme == "selama" || t.lexeme == "untuk";
}

// statement-list → statement (SEMICOLON + statement)*
ParseNode* statement_list() {
    debugEnter("statement_list");
    auto *node = makeNode("<statement-list>");

    if (isStatementStart(cur_tok))
        addChild(node, statement());
    while (cur_tok.type == "SEMICOLON") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
        if (isStatementStart(cur_tok))   // Only add a statement if valid
            addChild(node, statement());
    }
    debugExit("statement_list");
    return node;
}

// statement → assignment-statement | if-statement | while-statement | for-statement | procedure/function-call
ParseNode* statement() {
    debugEnter("statement");
    auto *node = makeNode("<statement>");
    
    if (cur_tok.type == "IDENTIFIER") {
        int lookahead = current + 1;
        
        // Skip array subscript jika ada: identifier[...]
        if (lookahead < tokens.size() && tokens[lookahead].type == "LBRACKET") {
            // Skip sampai ketemu RBRACKET
            int bracket_count = 0;
            while (lookahead < tokens.size()) {
                if (tokens[lookahead].type == "LBRACKET") bracket_count++;
                if (tokens[lookahead].type == "RBRACKET") {
                    bracket_count--;
                    if (bracket_count == 0) {
                        lookahead++;
                        break;
                    }
                }
                lookahead++;
            }
        }
        
        // Sekarang cek token setelah identifier (atau setelah subscript)
        if (lookahead < tokens.size() && tokens[lookahead].type == "ASSIGN_OPERATOR") {
            addChild(node, assignment_statement());
        } else {
            addChild(node, procedure_function_call());
        }
    } 
    else if (cur_tok.lexeme == "jika")
        addChild(node, if_statement());
    else if (cur_tok.lexeme == "selama")
        addChild(node, while_statement());
    else if (cur_tok.lexeme == "untuk")
        addChild(node, for_statement());
    
    debugExit("statement");
    return node;
}

// assignment-statement → IDENTIFIER [subscript]? + ASSIGN_OPERATOR(:=) + expression
ParseNode* assignment_statement() {
    debugEnter("assignment_statement");
    auto *node = makeNode("<assignment-statement>");
    
    addChild(node, matchType("IDENTIFIER"));
    
    // Handle array subscript on left-hand side
    if (cur_tok.type == "LBRACKET") {
        addChild(node, makeTokenNode(cur_tok));  
        advance();
        addChild(node, expression());           
        addChild(node, matchType("RBRACKET"));  
    }
    
    addChild(node, matchType("ASSIGN_OPERATOR"));
    addChild(node, expression());
    
    debugExit("assignment_statement");
    return node;
}

// if-statement → KEYWORD(jika) + expression + KEYWORD(maka) + statement + (KEYWORD(selain-itu) + statement)?
ParseNode* if_statement() {
    debugEnter("if_statement");
    auto *node = makeNode("<if-statement>");

    // jika <expr> maka <statement>
    addChild(node, matchToken({"KEYWORD", "jika"}));
    addChild(node, expression());
    addChild(node, matchToken({"KEYWORD", "maka"}));
    addChild(node, statement());

    // optionally: selain-itu <statement>
    if (cur_tok.type == "SEMICOLON" && current + 1 < tokens.size() && 
        tokens[current + 1].type == "KEYWORD" && 
        tokens[current + 1].lexeme == "selain-itu") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
        addChild(node, makeTokenNode(cur_tok));
        advance();
        addChild(node, statement());
    }

    debugExit("if_statement");
    return node;
}


// while-statement → KEYWORD(selama) + expression + KEYWORD(lakukan) + compound-statement
ParseNode* while_statement() {
    debugEnter("while_statement");
    auto *node = makeNode("<while-statement>");
    addChild(node, matchToken({"KEYWORD", "selama"}));
    addChild(node, expression());
    addChild(node, matchToken({"KEYWORD", "lakukan"}));

    addChild(node, compound_statement());
    debugExit("while_statement");
    return node;
}

// for-statement → KEYWORD(untuk) + IDENTIFIER + ASSIGN_OPERATOR + expression + (KEYWORD(ke)/KEYWORD(turun-ke)) + expression + KEYWORD(lakukan) + compound-statement
ParseNode* for_statement() {
    debugEnter("for_statement");
    auto *node = makeNode("<for-statement>");
    addChild(node, matchToken({"KEYWORD", "untuk"}));
    addChild(node, matchType("IDENTIFIER"));
    addChild(node, matchType("ASSIGN_OPERATOR"));
    addChild(node, expression());
    
    ParseNode* t = tryMatchToken({"KEYWORD", "ke"});
    if (t == nullptr) {
        t = tryMatchToken({"KEYWORD", "turun-ke"});
    }

    if (t != nullptr) {
        addChild(node, t);
    }



    addChild(node, expression());
    addChild(node, matchToken({"KEYWORD", "lakukan"}));

    addChild(node, compound_statement());
    debugExit("for_statement");
    return node;
}

// procedure/function-call → IDENTIFIER + (LPARENTHESIS + parameter-list + RPARENTHESIS)
ParseNode* procedure_function_call() {
    debugEnter("procedure_function_call");
    auto *node = makeNode("<procedure/function-call>");
    addChild(node, matchType("IDENTIFIER"));
    addChild(node, matchType("LPARENTHESIS"));
    if (cur_tok.type != "RPARENTHESIS")
        addChild(node, parameter_list());
    addChild(node, matchType("RPARENTHESIS"));
    debugExit("procedure_function_call");
    return node;
}

// parameter-list → expression (COMMA + expression)*
ParseNode* parameter_list() {
    debugEnter("parameter_list");
    auto *node = makeNode("<parameter-list>");
    addChild(node, expression());
    while (cur_tok.type == "COMMA") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
        addChild(node, expression());
    }
    debugExit("parameter_list");
    return node;
}

// expression → simple-expression (RELATIONAL_OPERATOR + simple-expression)?
ParseNode* expression() {
    debugEnter("expression");
    auto *node = makeNode("<expression>");
    addChild(node, simple_expression());
    if (cur_tok.lexeme == "=" || cur_tok.lexeme == "<>" || cur_tok.lexeme == "<" ||
        cur_tok.lexeme == "<=" || cur_tok.lexeme == ">" || cur_tok.lexeme == ">=") {
        addChild(node, relational_operator());
        addChild(node, simple_expression());
    }
    debugExit("expression");
    return node;
}

// simple-expression → (ARITHMETIC_OPERATOR(+/-))? term (additive-operator + term)*
ParseNode* simple_expression() {
    debugEnter("simple_expression");
    auto *node = makeNode("<simple-expression>");
    if (cur_tok.type == "ARITHMETIC_OPERATOR" &&
        (cur_tok.lexeme == "+" || cur_tok.lexeme == "-")) {
        addChild(node, makeTokenNode(cur_tok));
        advance();
    }

    addChild(node, term());
    while (cur_tok.type == "ARITHMETIC_OPERATOR" || cur_tok.lexeme == "atau") {
        addChild(node, additive_operator());
        addChild(node, term());
    }
    debugExit("simple_expression");
    return node;
}

// term → factor (multiplicative-operator + factor)*
ParseNode* term() {
    debugEnter("term");
    auto *node = makeNode("<term>");
    addChild(node, factor());
    while (cur_tok.lexeme == "*" || cur_tok.lexeme == "/" ||
           cur_tok.lexeme == "bagi" || cur_tok.lexeme == "mod" ||
           cur_tok.lexeme == "dan") {
        addChild(node, multiplicative_operator());
        addChild(node, factor());
    }
    debugExit("term");
    return node;
}

// factor → IDENTIFIER / NUMBER / CHAR_LITERAL / STRING_LITERAL / (LPARENTHESIS + expression + RPARENTHESIS) / LOGICAL_OPERATOR(tidak) + factor / procedure/function-call
ParseNode* factor() {
    debugEnter("factor");
    auto *node = makeNode("<factor>");
    if (cur_tok.type == "IDENTIFIER") {
        Token next = tokens[current + 1];
        if (next.type == "LBRACKET") {
            addChild(node, makeTokenNode(cur_tok));  
            advance();
            addChild(node, makeTokenNode(cur_tok)); 
            advance();
            addChild(node, expression());            
            addChild(node, matchType("RBRACKET"));
        }
        else if (next.type == "LPARENTHESIS") {
            addChild(node, procedure_function_call());
        }
        else {
            addChild(node, makeTokenNode(cur_tok));
            advance();
        }
    } else if (cur_tok.type == "NUMBER" || 
               cur_tok.type == "CHAR_LITERAL" || 
               cur_tok.type == "STRING_LITERAL") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
    } else if (cur_tok.lexeme == "tidak") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
        addChild(node, factor());
    } else if (cur_tok.type == "LPARENTHESIS") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
        addChild(node, expression());
        addChild(node, matchType("RPARENTHESIS"));
    } else {
        cerr << "Unexpected token in factor: " << cur_tok.lexeme << endl;
    }
    debugExit("factor");
    return node;
}

// relational-operator → =, <>, <, <=, >, >=
ParseNode* relational_operator() {
    debugEnter("relational_operator");
    auto *node = makeNode("<relational-operator>");
    if (cur_tok.lexeme == "=" || cur_tok.lexeme == "<>" || cur_tok.lexeme == "<" ||
        cur_tok.lexeme == "<=" || cur_tok.lexeme == ">" || cur_tok.lexeme == ">=") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
    } else {
        cerr << "Expected relational operator but got " << cur_tok.lexeme << endl;
        advance();
    }
    debugExit("relational_operator");
    return node;
}

// additive-operator → +, -, atau
ParseNode* additive_operator() {
    debugEnter("additive-operator");
    auto *node = makeNode("<additive-operator>");
    if (cur_tok.lexeme == "+" || cur_tok.lexeme == "-" || cur_tok.lexeme == "atau") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
    } else {
        cerr << "Expected additive operator but got " << cur_tok.lexeme << endl;
        advance();
    }
    debugExit("additive-operator");
    return node;
}

// multiplicative-operator → *, /, bagi, mod, dan
ParseNode* multiplicative_operator() {
    debugEnter("multiplicative-operator");
    auto *node = makeNode("<multiplicative-operator>");
    if (cur_tok.lexeme == "*" || cur_tok.lexeme == "/" || cur_tok.lexeme == "bagi" ||
        cur_tok.lexeme == "mod" || cur_tok.lexeme == "dan") {
        addChild(node, makeTokenNode(cur_tok));
        advance();
    } else {
        cerr << "Expected multiplicative operator but got " << cur_tok.lexeme << endl;
        advance();
    }
    debugExit("multiplicative-operator");
    return node;
}

// ========================
// BUILD TREE
// ========================

ParseNode* buildTree(vector<Token> inputTokens) {
    tokens = inputTokens;
    current = 0;
    cur_tok = getCurrentToken();

    auto *root = program();

    if (cur_tok.type != "EOF")
        cerr << "Syntax error: unexpected token '" << cur_tok.lexeme << "' after program end\n";

    return root;
}

void parser_main(vector<Token> inputTokens) {
    auto *root = buildTree(inputTokens);
    if (root) {
        printTree(root);
    }
}