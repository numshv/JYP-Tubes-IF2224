#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include "parser.hpp" 

using std::string;
using std::vector;

// Forward declarations
struct BlockNode;
struct ParamNode;

struct ASTNode {
    string nodeType;              // Jenis node 
    string dataType;              // Hasil type checking 
    int symbolIndex = -1;         // Indeks ke tabel simbol 
    int scopeLevel = -1;          // Scope tempat node ini berada

    vector<ASTNode*> children;

    ASTNode(const string &type) : nodeType(type) {}
    virtual ~ASTNode() {
        for (auto child : children) {
            delete child;
        }
    }
};

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

// Array Access
struct ArrayAccessNode : ASTNode {
    string arrayName;
    ASTNode* index;
    
    ArrayAccessNode(const string &name, ASTNode* idx)
        : ASTNode("ArrayAccess"), arrayName(name), index(idx) {}
    
    ~ArrayAccessNode() {
        delete index;
    }
};

// Expressions

struct BinOpNode : ASTNode {
    string op;
    ASTNode *left;
    ASTNode *right;

    BinOpNode(const string &o, ASTNode* l, ASTNode* r)
        : ASTNode("BinOp"), op(o), left(l), right(r) {}
    
    ~BinOpNode() {
        delete left;
        delete right;
    }
};

struct UnaryOpNode : ASTNode {
    string op;
    ASTNode* operand;
    
    UnaryOpNode(const string &o, ASTNode* operand)
        : ASTNode("UnaryOp"), op(o), operand(operand) {}
    
    ~UnaryOpNode() {
        delete operand;
    }
};

// Statements

struct AssignNode : ASTNode {
    ASTNode* target;  
    ASTNode* value;

    AssignNode(ASTNode* t, ASTNode* v)
        : ASTNode("Assign"), target(t), value(v) {}
    
    ~AssignNode() {
        delete target;
        delete value;
    }
};

struct ProcedureCallNode : ASTNode {
    string procName;
    vector<ASTNode*> args;

    ProcedureCallNode(const string &name) : ASTNode("ProcedureCall"), procName(name) {}
    
    ~ProcedureCallNode() {
        for (auto arg : args) delete arg;
    }
};

struct IfNode : ASTNode {
    ASTNode* condition;
    ASTNode* thenBranch;
    ASTNode* elseBranch;

    IfNode(ASTNode* cond, ASTNode* t, ASTNode* e = nullptr)
        : ASTNode("If"), condition(cond), thenBranch(t), elseBranch(e) {}
    
    ~IfNode() {
        delete condition;
        delete thenBranch;
        if (elseBranch) delete elseBranch;
    }
};

struct WhileNode : ASTNode {
    ASTNode* condition;
    ASTNode* body;

    WhileNode(ASTNode* cond, ASTNode* b)
        : ASTNode("While"), condition(cond), body(b) {}
    
    ~WhileNode() {
        delete condition;
        delete body;
    }
};

struct ForNode : ASTNode {
    string counter;
    ASTNode *start, *end, *body;
    bool ascending; // ke / turun-ke

    ForNode(const string &ctr, ASTNode* s, ASTNode* e, ASTNode* b, bool asc)
        : ASTNode("For"), counter(ctr), start(s), end(e), body(b), ascending(asc) {}
    
    ~ForNode() {
        delete start;
        delete end;
        delete body;
    }
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
    
    ~ConstDeclNode() {
        delete value;
    }
};

struct TypeDeclNode : ASTNode {
    string name;
    string definition;  // Type name or definition
    
    TypeDeclNode(const string &n, const string &def)
        : ASTNode("TypeDecl"), name(n), definition(def) {}
};

struct ArrayTypeNode : ASTNode {
    ASTNode* rangeStart;
    ASTNode* rangeEnd;
    string elementType;
    
    ArrayTypeNode(ASTNode* start, ASTNode* end, const string &elemType)
        : ASTNode("ArrayType"), rangeStart(start), rangeEnd(end), elementType(elemType) {}
    
    ~ArrayTypeNode() {
        delete rangeStart;
        delete rangeEnd;
    }
};

struct ParamNode : ASTNode {
    vector<string> names;
    string typeName;
    bool isVar;  // true if "var" parameter
    
    ParamNode() : ASTNode("Param"), isVar(false) {}
};

struct ProcedureDeclNode : ASTNode {
    string name;
    vector<ParamNode*> params;
    BlockNode* body;
    
    ProcedureDeclNode(const string &n)
        : ASTNode("ProcedureDecl"), name(n), body(nullptr) {}
    
    ~ProcedureDeclNode();
};

struct FunctionDeclNode : ASTNode {
    string name;
    vector<ParamNode*> params;
    string returnType;
    BlockNode* body;
    
    FunctionDeclNode(const string &n, const string &retType)
        : ASTNode("FunctionDecl"), name(n), returnType(retType), body(nullptr) {}
    
    ~FunctionDeclNode();
};

struct BlockNode : ASTNode {
    vector<ASTNode*> declarations;  // const, type, var, subprogram declarations
    vector<ASTNode*> statements;
    
    BlockNode() : ASTNode("Block") {}
    
    ~BlockNode() {
        for (auto d : declarations) delete d;
        for (auto s : statements) delete s;
    }
};

// Program Root

struct ProgramNode : ASTNode {
    string name;
    vector<ASTNode*> declarations;
    BlockNode* block;

    ProgramNode(const string &n)
        : ASTNode("Program"), name(n), block(nullptr) {}
    
    ~ProgramNode() {
        for (auto d : declarations) delete d;
        if (block) delete block;
    }
};

// Function declarations
ASTNode* buildAST(ParseNode* root);
ASTNode* ASTMain(ParseNode* parseRoot);
void printAST(ASTNode* node, const string& prefix = "", bool isLast = true);

#endif