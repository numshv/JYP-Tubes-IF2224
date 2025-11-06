#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"
#include <vector>
#include <string>
#include <iostream>
using namespace std;

extern vector<Token> tokens;
extern int current;
extern Token cur_tok;

Token getCurrentToken();
void advance();
void matchType(const string &expectedType);
void matchToken(const Token &expected);
void debugEnter(const string &rule);
void debugExit(const string &debugExit);

void program();
void program_header();
void declaration_part();
void const_declaration();
void type_declaration();
void type_definition();
void var_declaration();
void identifier_list();
void type_spec();
void array_type();
void range();
void subprogram_declaration();
void procedure_declaration();
void function_declaration();
void formal_parameter_list();
void parameter_group();
void block();
void compound_statement();
void statement_list();
void statement();
void assignment_statement();
void if_statement();
void while_statement();
void for_statement();
void procedure_call();
void parameter_list();
void expression();
void simple_expression();
void term();
void factor();
void function_call();

void parser_main(vector<Token> inputTokens);

#endif