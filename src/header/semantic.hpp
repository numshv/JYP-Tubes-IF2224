#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include "ast.hpp"
#include "tab.hpp"
#include "btab.hpp"
#include "atab.hpp"
#include <string>
#include <vector>

using namespace std;

// Semantic analyzer class
class SemanticAnalyzer {
private:
    bool hasErrors;
    
    string inferType(ASTNode* node);
    bool isCompatibleType(const string& type1, const string& type2);
    string getOperatorResultType(const string& op, const string& leftType, const string& rightType);
    
    // Visit functions
    void visitProgram(ProgramNode* node);
    void visitDeclarations(DeclarationsNode* node);
    void visitBlock(BlockNode* node);
    void visitVarDecl(VarDeclNode* node);
    void visitConstDecl(ConstDeclNode* node);
    void visitTypeDecl(TypeDeclNode* node);
    void visitProcedureDecl(ProcedureDeclNode* node);
    void visitFunctionDecl(FunctionDeclNode* node);
    void visitStatement(ASTNode* node);
    void visitExpression(ASTNode* node);
    void visitAssign(AssignNode* node);
    void visitIf(IfNode* node);
    void visitWhile(WhileNode* node);
    void visitFor(ForNode* node);
    void visitProcedureCall(ProcedureCallNode* node);
    void visitVar(VarNode* node);
    void visitArrayAccess(ArrayAccessNode* node);
    void visitBinOp(BinOpNode* node);
    void visitUnaryOp(UnaryOpNode* node);
    
public:
    SemanticAnalyzer();
    ~SemanticAnalyzer();
    
 
    bool analyze(ASTNode* root);
    
    void printDecoratedAST(ASTNode* node, const string& prefix = "", bool isLast = true);
};

void semanticAnalysis(ASTNode* ast);
void printSymbolTables();

#endif
