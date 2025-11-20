#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include "parser.hpp" 

using std::string;
using std::vector;

struct ASTNode {
    string nodeType;              // Jenis node 
    string dataType;              // Hasil type checking 
    int symbolIndex = -1;         // Indeks ke tabel simbol 
    int scopeLevel = -1;          // Scope tempat node ini berada

    vector<ASTNode*> children;

    ASTNode(const string &type) : nodeType(type) {}
    virtual ~ASTNode() {}
};

// NOTE: tolong proofread cmiiw banget siapa tau salah/kurang

// Simple Literals

struct NumberNode : ASTNode {
    int value;
    NumberNode(int v) : ASTNode("Number"), value(v) {}
};

struct RealNode : ASTNode {
    double value;
    RealNode(double v) : ASTNode("Real"), value(v) {}
};

struct StringNode : ASTNode {
    string value;
    StringNode(const string &v) : ASTNode("String"), value(v) {}
};

struct CharNode : ASTNode {
    char value;
    CharNode(char v) : ASTNode("Char"), value(v) {}
};

struct BoolNode : ASTNode {
    bool value;
    BoolNode(bool v) : ASTNode("Boolean"), value(v) {}
};

// Identifier

struct VarNode : ASTNode {
    string name;
    VarNode(const string &n) : ASTNode("Var"), name(n) {}
};

// Expressions

struct BinOpNode : ASTNode {
    string op;
    ASTNode *left;
    ASTNode *right;

    BinOpNode(const string &o, ASTNode* l, ASTNode* r)
        : ASTNode("BinOp"), op(o), left(l), right(r) {}
};

// Statements

struct AssignNode : ASTNode {
    VarNode* target;
    ASTNode* value;

    AssignNode(VarNode* t, ASTNode* v)
        : ASTNode("Assign"), target(t), value(v) {}
};

struct ProcedureCallNode : ASTNode {
    string procName;
    vector<ASTNode*> args;

    ProcedureCallNode(const string &name) : ASTNode("ProcedureCall"), procName(name) {}
};

struct IfNode : ASTNode {
    ASTNode* condition;
    ASTNode* thenBranch;
    ASTNode* elseBranch;

    IfNode(ASTNode* cond, ASTNode* t, ASTNode* e = nullptr)
        : ASTNode("If"), condition(cond), thenBranch(t), elseBranch(e) {}
};

struct WhileNode : ASTNode {
    ASTNode* condition;
    ASTNode* body;

    WhileNode(ASTNode* cond, ASTNode* b)
        : ASTNode("While"), condition(cond), body(b) {}
};

struct ForNode : ASTNode {
    string counter;
    ASTNode *start, *end, *body;
    bool ascending; // ke / turun-ke

    ForNode(const string &ctr, ASTNode* s, ASTNode* e, ASTNode* b, bool asc)
        : ASTNode("For"), counter(ctr), start(s), end(e), body(b), ascending(asc) {}
};

// Declarations

struct VarDeclNode : ASTNode {
    vector<string> names;
    string typeName;

    VarDeclNode() : ASTNode("VarDecl") {}
};

struct ConstDeclNode : ASTNode {
    string name;
    ASTNode* value;
    ConstDeclNode(const string &n, ASTNode* v)
        : ASTNode("ConstDecl"), name(n), value(v) {}
};

struct TypeDeclNode : ASTNode {
    string name;
    ASTNode* definition;
    TypeDeclNode(const string &n, ASTNode* def)
        : ASTNode("TypeDecl"), name(n), definition(def) {}
};

struct BlockNode : ASTNode {
    vector<ASTNode*> statements;
    BlockNode() : ASTNode("Block") {}
};

// Program Root

struct ProgramNode : ASTNode {
    string name;
    vector<ASTNode*> declarations;
    BlockNode* block;

    ProgramNode(const string &n)
        : ASTNode("Program"), name(n), block(nullptr) {}
};



ASTNode* buildAST(ParseNode* root);
ASTNode* ASTMain(ParseNode* parseRoot);


#endif
