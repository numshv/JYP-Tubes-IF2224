#include "parser.hpp"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

vector<Token> tokens;
int current = 0;

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
    cerr << ">>> Entering rule: " << rule << " | Current token: (" 
         << cur_tok.type << ", '" << cur_tok.lexeme << "')\n";
}

void debugExit(const string &rule) {
    cerr << "<<< Exiting rule: " << rule << " | Next token: (" 
         << cur_tok.type << ", '" << cur_tok.lexeme << "')\n";
}

void matchType(const string &expectedType) {
    if (cur_tok.type == expectedType) {
        cerr << "Matched type: " << expectedType << " (" << cur_tok.lexeme << ")\n";
        advance();
    } else {
        cerr << "❌ Syntax error: expected type '" << expectedType 
             << "' but got (" << cur_tok.type << ", '" << cur_tok.lexeme << "')\n";
    }
}

void matchToken(const Token &expected) {
    if (cur_tok.type == expected.type && cur_tok.lexeme == expected.lexeme) {
        cerr << "Matched token: <" << expected.type << ", '" << expected.lexeme << "'>\n";
        advance();
    } else {
        cerr << "❌ Syntax error: expected token (" << expected.type << ", '" 
             << expected.lexeme << "') but got (" << cur_tok.type << ", '" 
             << cur_tok.lexeme << "')\n";
    }
}

// ========== GRAMMAR RULES ==========

// program → program-header + declaration-part + compound-statement + DOT
void program() {
    debugEnter("program");
    program_header();
    declaration_part();
    compound_statement();
    matchType("DOT");
    debugExit("program");
}

// program-header → KEYWORD(program) + IDENTIFIER + SEMICOLON
void program_header() {
    debugEnter("program_header");
    matchToken({"KEYWORD", "program"});
    matchType("IDENTIFIER");
    matchType("SEMICOLON");
    debugExit("program_header");
}

// declaration-part → (const-declaration)* + (type-declaration)* + (var-declaration)* + (subprogram-declaration)*
void declaration_part() {
    debugEnter("declaration_part");
    while (cur_tok.lexeme == "konstanta") const_declaration();
    while (cur_tok.lexeme == "tipe") type_declaration();
    while (cur_tok.lexeme == "variabel") var_declaration();
    while (cur_tok.lexeme == "prosedur" || cur_tok.lexeme == "fungsi") 
        subprogram_declaration();
    debugExit("declaration_part");
}

// const-declaration → KEYWORD(konstanta) + (IDENTIFIER = value + SEMICOLON)+
void const_declaration() {
    debugEnter("const_declaration");
    matchToken({"KEYWORD", "konstanta"});
    do {
        matchType("IDENTIFIER");
        matchToken({"RELATIONAL_OPERATOR", "="});
        if (cur_tok.type == "NUMBER" || cur_tok.type == "CHAR_LITERAL" ||
            cur_tok.type == "STRING_LITERAL" || cur_tok.type == "BOOLEAN" ||
            cur_tok.type == "IDENTIFIER") {
            advance();
        } else {
            cerr << "❌ Expected constant value but got " << cur_tok.lexeme << endl;
        }
        matchType("SEMICOLON");
    } while (cur_tok.type == "IDENTIFIER");
    debugExit("const_declaration");
}

// type-declaration → KEYWORD(tipe) + (IDENTIFIER = type-definition + SEMICOLON)+
void type_declaration() {
    debugEnter("type_declaration");
    matchToken({"KEYWORD", "tipe"});
    do {
        matchType("IDENTIFIER");
        matchToken({"RELATIONAL_OPERATOR", "="});
        type_definition();
        matchType("SEMICOLON");
    } while (cur_tok.type == "IDENTIFIER");
    debugExit("type_declaration");
}

// type-definition → type | range
void type_definition() {
    debugEnter("type_definition");
    if (cur_tok.lexeme == "larik" || cur_tok.lexeme == "integer" ||
        cur_tok.lexeme == "real" || cur_tok.lexeme == "boolean" || 
        cur_tok.lexeme == "char") {
        type_spec();
    } else {
        range();
    }
    debugExit("type_definition");
}

// var-declaration → KEYWORD(variabel) + (identifier-list + COLON + type + SEMICOLON)+
void var_declaration() {
    debugEnter("var_declaration");
    matchToken({"KEYWORD", "variabel"});
    do {
        identifier_list();
        matchType("COLON");
        type_spec();
        matchType("SEMICOLON");
    } while (cur_tok.type == "IDENTIFIER");
    debugExit("var_declaration");
}

// identifier-list → IDENTIFIER (COMMA + IDENTIFIER)*
void identifier_list() {
    debugEnter("identifier_list");
    matchType("IDENTIFIER");
    while (cur_tok.type == "COMMA") {
        advance();
        matchType("IDENTIFIER");
    }
    debugExit("identifier_list");
}

// type → KEYWORD(integer/real/boolean/char) | array-type
void type_spec() {
    debugEnter("type_spec");
    if (cur_tok.lexeme == "integer" || cur_tok.lexeme == "real" ||
        cur_tok.lexeme == "boolean" || cur_tok.lexeme == "char") {
        advance();
    } else if (cur_tok.lexeme == "larik") {
        array_type();
    } else {
        cerr << "❌ Unknown type: " << cur_tok.lexeme << endl;
    }
    debugExit("type_spec");
}

// array-type → KEYWORD(larik) + LBRACKET + range + RBRACKET + KEYWORD(dari) + type
void array_type() {
    debugEnter("array_type");
    matchToken({"KEYWORD", "larik"});
    matchType("LBRACKET");
    range();
    matchType("RBRACKET");
    matchToken({"KEYWORD", "dari"});
    type_spec();
    debugExit("array_type");
}

// range → expression + RANGE_OPERATOR(..) + expression
void range() {
    debugEnter("range");
    expression();
    matchType("RANGE_OPERATOR");
    expression();
    debugExit("range");
}

// subprogram-declaration → procedure-declaration | function-declaration
void subprogram_declaration() {
    debugEnter("subprogram_declaration");
    if (cur_tok.lexeme == "prosedur") procedure_declaration();
    else if (cur_tok.lexeme == "fungsi") function_declaration();
    debugExit("subprogram_declaration");
}

// procedure-declaration → KEYWORD(prosedur) + IDENTIFIER + (formal-parameter-list)? + SEMICOLON + block + SEMICOLON
void procedure_declaration() {
    debugEnter("procedure_declaration");
    matchToken({"KEYWORD", "prosedur"});
    matchType("IDENTIFIER");
    if (cur_tok.type == "LPARENTHESIS") formal_parameter_list();
    matchType("SEMICOLON");
    block();
    matchType("SEMICOLON");
    debugExit("procedure_declaration");
}

// function-declaration → KEYWORD(fungsi) + IDENTIFIER + (formal-parameter-list)? + COLON + type + SEMICOLON + block + SEMICOLON
void function_declaration() {
    debugEnter("function_declaration");
    matchToken({"KEYWORD", "fungsi"});
    matchType("IDENTIFIER");
    if (cur_tok.type == "LPARENTHESIS") formal_parameter_list();
    matchType("COLON");
    type_spec();
    matchType("SEMICOLON");
    block();
    matchType("SEMICOLON");
    debugExit("function_declaration");
}

// formal-parameter-list → LPARENTHESIS + parameter-group (SEMICOLON + parameter-group)* + RPARENTHESIS
void formal_parameter_list() {
    debugEnter("formal_parameter_list");
    matchType("LPARENTHESIS");
    parameter_group();
    while (cur_tok.type == "SEMICOLON") {
        advance();
        parameter_group();
    }
    matchType("RPARENTHESIS");
    debugExit("formal_parameter_list");
}

// parameter-group → identifier-list + COLON + type
void parameter_group() {
    debugEnter("parameter_group");
    identifier_list();
    matchType("COLON");
    type_spec();
    debugExit("parameter_group");
}

// block → compound-statement
void block() {
    compound_statement();
}

// compound-statement → KEYWORD(mulai) + statement-list + KEYWORD(selesai)
void compound_statement() {
    debugEnter("compound_statement");
    matchToken({"KEYWORD", "mulai"});
    statement_list();
    matchToken({"KEYWORD", "selesai"});
    debugExit("compound_statement");
}

// statement-list → statement (SEMICOLON + statement)*
void statement_list() {
    debugEnter("statement_list");
    statement();
    while (cur_tok.type == "SEMICOLON") {
        advance();
        statement();
    }
    debugExit("statement_list");
}

// statement → assignment-statement | if-statement | while-statement | for-statement | procedure-call
void statement() {
    debugEnter("statement");
    if (cur_tok.type == "IDENTIFIER") {
        Token next = tokens[current + 1];
        if (next.type == "ASSIGN_OPERATOR") assignment_statement();
        else procedure_call();
    } else if (cur_tok.lexeme == "jika") if_statement();
    else if (cur_tok.lexeme == "selama") while_statement();
    else if (cur_tok.lexeme == "untuk") for_statement();
    debugExit("statement");
}

// assignment-statement → IDENTIFIER + ASSIGN_OPERATOR(:=) + expression
void assignment_statement() {
    debugEnter("assignment_statement");
    matchType("IDENTIFIER");
    matchType("ASSIGN_OPERATOR");
    expression();
    debugExit("assignment_statement");
}

// if-statement → KEYWORD(jika) + expression + KEYWORD(maka) + statement + (KEYWORD(selain-itu) + statement)?
void if_statement() {
    debugEnter("if_statement");
    matchToken({"KEYWORD", "jika"});
    expression();
    matchToken({"KEYWORD", "maka"});
    statement();
    if (cur_tok.lexeme == "selain-itu") {
        advance();
        statement();
    }
    debugExit("if_statement");
}

// while-statement → KEYWORD(selama) + expression + KEYWORD(lakukan) + statement
void while_statement() {
    debugEnter("while_statement");
    matchToken({"KEYWORD", "selama"});
    expression();
    matchToken({"KEYWORD", "lakukan"});
    statement();
    debugExit("while_statement");
}

// for-statement → KEYWORD(untuk) + IDENTIFIER + ASSIGN_OPERATOR + expression + (KEYWORD(ke)/KEYWORD(turun-ke)) + expression + KEYWORD(lakukan) + statement
void for_statement() {
    debugEnter("for_statement");
    matchToken({"KEYWORD", "untuk"});
    matchType("IDENTIFIER");
    matchType("ASSIGN_OPERATOR");
    expression();
    if (cur_tok.lexeme == "ke" || cur_tok.lexeme == "turun-ke") advance();
    expression();
    matchToken({"KEYWORD", "lakukan"});
    statement();
    debugExit("for_statement");
}

// procedure-call → IDENTIFIER + (LPARENTHESIS + parameter-list + RPARENTHESIS)?
void procedure_call() {
    debugEnter("procedure_call");
    matchType("IDENTIFIER");
    if (cur_tok.type == "LPARENTHESIS") {
        advance();
        parameter_list();
        matchType("RPARENTHESIS");
    }
    debugExit("procedure_call");
}

// parameter-list → expression (COMMA + expression)*
void parameter_list() {
    debugEnter("parameter_list");
    expression();
    while (cur_tok.type == "COMMA") {
        advance();
        expression();
    }
    debugExit("parameter_list");
}

// expression → simple-expression (RELATIONAL_OPERATOR + simple-expression)?
void expression() {
    debugEnter("expression");
    simple_expression();
    if (cur_tok.type == "RELATIONAL_OPERATOR") {
        advance();
        simple_expression();
    }
    debugExit("expression");
}

// simple-expression → (ARITHMETIC_OPERATOR(+/-))? term (additive-operator + term)*
void simple_expression() {
    debugEnter("simple_expression");
    if (cur_tok.type == "ARITHMETIC_OPERATOR" &&
        (cur_tok.lexeme == "+" || cur_tok.lexeme == "-")) advance();

    term();
    while (cur_tok.type == "ARITHMETIC_OPERATOR" || cur_tok.lexeme == "atau") {
        advance();
        term();
    }
    debugExit("simple_expression");
}

// term → factor (multiplicative-operator + factor)*
void term() {
    debugEnter("term");
    factor();
    while (cur_tok.type == "ARITHMETIC_OPERATOR" ||
           cur_tok.lexeme == "bagi" || cur_tok.lexeme == "mod" ||
           cur_tok.lexeme == "dan") {
        advance();
        factor();
    }
    debugExit("term");
}

// factor → IDENTIFIER / NUMBER / CHAR_LITERAL / STRING_LITERAL / (LPARENTHESIS + expression + RPARENTHESIS) / LOGICAL_OPERATOR(tidak) + factor / function-call
void factor() {
    debugEnter("factor");
    if (cur_tok.type == "IDENTIFIER") {
        Token next = tokens[current + 1];
        if (next.type == "LPARENTHESIS") function_call();
        else advance();
    } else if (cur_tok.type == "NUMBER" || 
               cur_tok.type == "CHAR_LITERAL" || 
               cur_tok.type == "STRING_LITERAL") {
        advance();
    } else if (cur_tok.lexeme == "tidak") {
        advance();
        factor();
    } else if (cur_tok.type == "LPARENTHESIS") {
        advance();
        expression();
        matchType("RPARENTHESIS");
    } else {
        cerr << "❌ Unexpected token in factor: " << cur_tok.lexeme << endl;
    }
    debugExit("factor");
}

// function-call → IDENTIFIER + LPARENTHESIS + (parameter-list)? + RPARENTHESIS
void function_call() {
    debugEnter("function_call");
    matchType("IDENTIFIER");
    matchType("LPARENTHESIS");
    if (cur_tok.type != "RPARENTHESIS") parameter_list();
    matchType("RPARENTHESIS");
    debugExit("function_call");
}

// ========================
// ENTRY POINT
// ========================

void parser_main(vector<Token> inputTokens) {
    tokens = inputTokens;
    current = 0;
    cur_tok = getCurrentToken();

    program();

    if (cur_tok.type != "EOF")
        cerr << "❌ Syntax error: unexpected token '" << cur_tok.lexeme << "' after program end\n";
    else
        cout << "✅ Parsing completed successfully!\n";
}