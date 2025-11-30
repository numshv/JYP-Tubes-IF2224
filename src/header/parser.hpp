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
extern bool gDebug;

struct ParseNode {
	string label;                 // ex: "<program>" or token like "IDENTIFIER(x)"
	vector<ParseNode*> children;  
};

ParseNode* makeNode(const string &label);
ParseNode* makeTokenNode(const Token &t);
void addChild(ParseNode* parent, ParseNode* child);
void printTree(ParseNode* node, const string &prefix = "", bool isLast = true);

Token getCurrentToken();
void advance();

ParseNode* matchType(const string &expectedType);
ParseNode* matchToken(const Token &expected);
void debugEnter(const string &rule);
void debugExit(const string &debugExit);

ParseNode* program();
ParseNode* program_header();
ParseNode* declaration_part();
ParseNode* const_declaration();
ParseNode* type_declaration();
ParseNode* type_definition();
ParseNode* var_declaration();
ParseNode* identifier_list();
ParseNode* type_spec();
ParseNode* array_type();
ParseNode* range();
ParseNode* subprogram_declaration();
ParseNode* procedure_declaration();
ParseNode* function_declaration();
ParseNode* formal_parameter_list();
ParseNode* parameter_group();
ParseNode* block();
ParseNode* compound_statement();
ParseNode* statement_list();
ParseNode* statement();
ParseNode* assignment_statement();
ParseNode* if_statement();
ParseNode* while_statement();
ParseNode* for_statement();
ParseNode* procedure_function_call();
ParseNode* parameter_list();
ParseNode* expression();
ParseNode* simple_expression();
ParseNode* term();
ParseNode* factor();
ParseNode* relational_operator();
ParseNode* additive_operator();
ParseNode* multiplicative_operator();

ParseNode* buildTree(vector<Token> inputTokens);
ParseNode* parser_main(vector<Token> inputTokens);

#endif