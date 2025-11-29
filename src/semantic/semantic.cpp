#include "../header/semantic.hpp"
#include <iostream>

using namespace std;

SemanticAnalyzer::SemanticAnalyzer() : hasErrors(false) {
    initializeBtab();
    initializeTab();
    atab.clear();
}

SemanticAnalyzer::~SemanticAnalyzer() {}

string SemanticAnalyzer::inferType(ASTNode* node) {
    if (!node) return "unknown";
    
    if (node->nodeType == "Number") return "integer";
    if (node->nodeType == "Real") return "real";
    if (node->nodeType == "String") return "string";
    if (node->nodeType == "Char") return "char";
    if (node->nodeType == "Boolean") return "boolean";
    
    if (node->nodeType == "Var") {
        VarNode* varNode = static_cast<VarNode*>(node);
        int idx = lookupIdentifier(varNode->name);
        if (idx > 0 && idx < (int)tab.size()) {
            int typeCode = tab[idx].type;
            switch(typeCode) {
                case 1: return "integer";
                case 2: return "real";
                case 3: return "boolean";
                case 4: return "char";
                case 5: return "array";
                default: 
                    return "unknown";
            }
        }
        return "unknown";
    }
    
    if (node->nodeType == "ProcedureCall") {
        ProcedureCallNode* procNode = static_cast<ProcedureCallNode*>(node);
        int idx = lookupIdentifier(procNode->procName);
        if (idx > 0 && idx < (int)tab.size()) {
            int objCode = tab[idx].obj;
            if (objCode == OBJ_PROCEDURE) {
                return "void";
            } else if (objCode == OBJ_FUNCTION) {
                int typeCode = tab[idx].type;
                return getTypeName(typeCode);
            }
        }
        return "unknown";
    }
    
    if (node->nodeType == "ArrayAccess") {
        ArrayAccessNode* arrayNode = static_cast<ArrayAccessNode*>(node);
        int idx = lookupIdentifier(arrayNode->arrayName);
        if (idx > 0 && idx < (int)tab.size()) {
            int typeCode = tab[idx].type;
            // Array type 
            if (typeCode == 5) {
                int atab_idx = tab[idx].ref;
                if (atab_idx >= 0 && atab_idx < (int)atab.size()) {
                    int elemTypeCode = atab[atab_idx].etyp;
                    switch(elemTypeCode) {
                        case 1: return "integer";
                        case 2: return "real";
                        case 3: return "boolean";
                        case 4: return "char";
                        default: return "unknown";
                    }
                }
            }
        }
        return "unknown";
    }
    
    if (node->nodeType == "BinOp") {
        BinOpNode* binOp = static_cast<BinOpNode*>(node);
        string leftType = inferType(binOp->left);
        string rightType = inferType(binOp->right);
        return getOperatorResultType(binOp->op, leftType, rightType);
    }
    
    if (node->nodeType == "UnaryOp") {
        UnaryOpNode* unaryOp = static_cast<UnaryOpNode*>(node);
        return inferType(unaryOp->operand);
    }
    
    return "unknown";
}

bool SemanticAnalyzer::isCompatibleType(const string& type1, const string& type2) {
    if (type1 == type2) return true;
    if ((type1 == "integer" && type2 == "real") || (type1 == "real" && type2 == "integer")) {
        return true;
    }
    return false;
}

string SemanticAnalyzer::getOperatorResultType(const string& op, const string& leftType, const string& rightType) {
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "bagi" || op == "mod") {
        if (leftType == "real" || rightType == "real") return "real";
        if (leftType == "integer" && rightType == "integer") return "integer";
        return "real";
    }
    
    if (op == "=" || op == "<>" || op == "<" || op == ">" || op == "<=" || op == ">=") {
        return "boolean";
    }
    
    if (op == "and" || op == "or" || op == "not" || op == "dan" || op == "atau" || op == "tidak") {
        return "boolean";
    }
    
    return "unknown";
}

void SemanticAnalyzer::visitProgram(ProgramNode* node) {
    if (!node) return;
    
    cout << "\n========== Semantic Analysis Started ==========\n";
    // cout << "Program: " << node->name << endl;
    
    TabEntry progEntry;
    progEntry.name = node->name;
    progEntry.link = 0;
    progEntry.obj = 0;
    progEntry.type = 0;
    progEntry.ref = 0;
    progEntry.nrm = 1;
    progEntry.lev = 0;
    progEntry.adr = 0;
    progEntry.initialized = false;
    tab.push_back(progEntry);
    int progIdx = tab.size() - 1;
    
    node->symbolIndex = progIdx;
    node->scopeLevel = 0;
    node->dataType = "program";
    
    if (node->declarations) {
        visitDeclarations(node->declarations);
    }
    
    if (node->block) {
        visitBlock(node->block);
    }
    
    cout << "\n========== Semantic Analysis Completed ==========\n";
}

void SemanticAnalyzer::visitDeclarations(DeclarationsNode* node) {
    if (!node) return;
    
    node->scopeLevel = currentLevel;
    
    for (ASTNode* decl : node->declarations) {
        if (!decl) continue;
        
        if (decl->nodeType == "VarDecl") {
            visitVarDecl(static_cast<VarDeclNode*>(decl));
        }
        else if (decl->nodeType == "ConstDecl") {
            visitConstDecl(static_cast<ConstDeclNode*>(decl));
        }
        else if (decl->nodeType == "TypeDecl") {
            visitTypeDecl(static_cast<TypeDeclNode*>(decl));
        }
        else if (decl->nodeType == "ProcedureDecl") {
            visitProcedureDecl(static_cast<ProcedureDeclNode*>(decl));
        }
        else if (decl->nodeType == "FunctionDecl") {
            visitFunctionDecl(static_cast<FunctionDeclNode*>(decl));
        }
    }
}

void SemanticAnalyzer::visitVarDecl(VarDeclNode* node) {
    if (!node) return;
    
    for (const string& name : node->names) {
        if (isDuplicateInCurrentBlock(name)) {
            semanticError("Duplicate variable: " + name);
            hasErrors = true;
            continue;
        }
        
        int typeCode = 0;
        int ref = 0;
        int size = 1;
        
        if (node->arrayType) {
            int atab_idx = processArrayDeclaration(node->arrayType);
            typeCode = 5;  // Array type code
            ref = atab_idx;
            if (atab_idx >= 0 && atab_idx < (int)atab.size()) {
                size = atab[atab_idx].size;
            }
        } else {
            typeCode = getTypeCode(node->typeName);
            size = getTypeSize(typeCode);
        }
        
        int curBlock = getCurrentBlock();
        int previousVar = btab[curBlock].last;
        int adr = btab[curBlock].vsze;
        
        TabEntry e;
        e.name = name;
        e.link = previousVar;
        e.obj = OBJ_VARIABLE;
        e.type = typeCode;
        e.ref = ref;
        e.nrm = 1;
        e.lev = currentLevel;
        e.adr = adr;
        e.initialized = false;
        
        tab.push_back(e);
        int newIndex = tab.size() - 1;
        
        btab[curBlock].last = newIndex;
        btab[curBlock].vsze += size;
        
        node->symbolIndex = newIndex;
        node->scopeLevel = currentLevel;
        node->dataType = node->arrayType ? "array" : node->typeName;
        
        // cout << "Variable '" << name << "' at tab[" << newIndex << "] link=" << previousVar << endl;
    }
}

void SemanticAnalyzer::visitConstDecl(ConstDeclNode* node) {
    if (!node) return;
    
    if (isDuplicateInCurrentBlock(node->name)) {
        semanticError("Duplicate constant: " + node->name);
        hasErrors = true;
        return;
    }
    
    string constType = inferType(node->value);
    int typeCode = getTypeCode(constType);
    
    int curBlock = getCurrentBlock();
    int previousId = btab[curBlock].last;
    
    TabEntry e;
    e.name = node->name;
    e.link = previousId;
    e.obj = OBJ_CONSTANT;
    e.type = typeCode;
    e.ref = 0;
    e.nrm = 1;
    e.lev = currentLevel;
    e.adr = 0;
    e.initialized = true;
    
    tab.push_back(e);
    int newIndex = tab.size() - 1;
    
    btab[curBlock].last = newIndex;
    
    node->symbolIndex = newIndex;
    node->scopeLevel = currentLevel;
    node->dataType = constType;
}

void SemanticAnalyzer::visitTypeDecl(TypeDeclNode* node) {
    if (!node) return;
    
    if (isDuplicateInCurrentBlock(node->name)) {
        semanticError("Duplicate type: " + node->name);
        hasErrors = true;
        return;
    }
    
    int typeCode = 0;
    int ref = 0;
    
    if (node->arrayType) {
        int atab_idx = processArrayDeclaration(node->arrayType);
        typeCode = 5;  // Array type code
        ref = atab_idx;
    } else {
        typeCode = getTypeCode(node->definition);
    }
    
    int curBlock = getCurrentBlock();
    int previousId = btab[curBlock].last;
    
    TabEntry e;
    e.name = node->name;
    e.link = previousId;
    e.obj = OBJ_TYPE;
    e.type = typeCode;
    e.ref = ref;
    e.nrm = 1;
    e.lev = currentLevel;
    e.adr = 0;
    e.initialized = false;
    
    tab.push_back(e);
    int newIndex = tab.size() - 1;
    
    btab[curBlock].last = newIndex;
    
    node->symbolIndex = newIndex;
    node->scopeLevel = currentLevel;
    node->dataType = node->arrayType ? "array" : node->definition;
}

void SemanticAnalyzer::visitProcedureDecl(ProcedureDeclNode* node) {
    if (!node) return;
    
    if (isDuplicateInCurrentBlock(node->name)) {
        semanticError("Duplicate procedure: " + node->name);
        hasErrors = true;
        return;
    }
    
    int procBlockIdx = createNewBlock();
    
    int curBlock = getCurrentBlock();
    int previousId = btab[curBlock].last;
    
    TabEntry e;
    e.name = node->name;
    e.link = previousId;
    e.obj = OBJ_PROCEDURE;
    e.type = 0;
    e.ref = procBlockIdx;
    e.nrm = 1;
    e.lev = currentLevel;
    e.adr = 0;
    e.initialized = false;
    
    tab.push_back(e);
    int procIdx = tab.size() - 1;
    
    btab[curBlock].last = procIdx;
    
    node->symbolIndex = procIdx;
    node->scopeLevel = currentLevel;
    node->dataType = "void";
    
    enterBlock(procBlockIdx);
    
    int paramSize = 0;
    for (ParamNode* param : node->params) {
        if (!param) continue;
        
        int typeCode = getTypeCode(param->typeName);
        int size = getTypeSize(typeCode);
        
        for (const string& paramName : param->names) {
            int prevParam = btab[procBlockIdx].last;
            
            TabEntry paramEntry;
            paramEntry.name = paramName;
            paramEntry.link = prevParam;
            paramEntry.obj = OBJ_VARIABLE;
            paramEntry.type = typeCode;
            paramEntry.ref = 0;
            paramEntry.nrm = param->isVar ? 1 : 0;
            paramEntry.lev = currentLevel;
            paramEntry.adr = paramSize;
            paramEntry.initialized = true;
            
            tab.push_back(paramEntry);
            int paramIdx = tab.size() - 1;
            
            btab[procBlockIdx].last = paramIdx;
            paramSize += size;
            
            param->symbolIndex = paramIdx;
            param->scopeLevel = currentLevel;
            param->dataType = param->typeName;
        }
    }
    
    btab[procBlockIdx].psze = paramSize;
    if (!node->params.empty()) {
        btab[procBlockIdx].lpar = tab.size() - 1;
    }
    
    if (node->body) {
        // directly process declarations without creating temporary DeclarationsNode
        for (ASTNode* decl : node->body->declarations) {
            if (!decl) continue;
            
            if (decl->nodeType == "VarDecl") {
                visitVarDecl(static_cast<VarDeclNode*>(decl));
            }
            else if (decl->nodeType == "ConstDecl") {
                visitConstDecl(static_cast<ConstDeclNode*>(decl));
            }
            else if (decl->nodeType == "TypeDecl") {
                visitTypeDecl(static_cast<TypeDeclNode*>(decl));
            }
        }
        
        // Process statements
        for (ASTNode* stmt : node->body->statements) {
            visitStatement(stmt);
        }
    }
    
    exitBlock();
}

void SemanticAnalyzer::visitFunctionDecl(FunctionDeclNode* node) {
    if (!node) return;
    
    if (isDuplicateInCurrentBlock(node->name)) {
        semanticError("Duplicate function: " + node->name);
        hasErrors = true;
        return;
    }
    
    int funcBlockIdx = createNewBlock();
    int retTypeCode = getTypeCode(node->returnType);
    
    int curBlock = getCurrentBlock();
    int previousId = btab[curBlock].last;
    
    TabEntry e;
    e.name = node->name;
    e.link = previousId;
    e.obj = OBJ_FUNCTION;
    e.type = retTypeCode;
    e.ref = funcBlockIdx;
    e.nrm = 1;
    e.lev = currentLevel;
    e.adr = 0;
    e.initialized = false;
    
    tab.push_back(e);
    int funcIdx = tab.size() - 1;
    
    btab[curBlock].last = funcIdx;
    
    node->symbolIndex = funcIdx;
    node->scopeLevel = currentLevel;
    node->dataType = node->returnType;
    
    enterBlock(funcBlockIdx);
    
    int paramSize = 0;
    for (ParamNode* param : node->params) {
        if (!param) continue;
        
        int typeCode = getTypeCode(param->typeName);
        int size = getTypeSize(typeCode);
        
        for (const string& paramName : param->names) {
            int prevParam = btab[funcBlockIdx].last;
            
            TabEntry paramEntry;
            paramEntry.name = paramName;
            paramEntry.link = prevParam;
            paramEntry.obj = OBJ_VARIABLE;
            paramEntry.type = typeCode;
            paramEntry.ref = 0;
            paramEntry.nrm = param->isVar ? 1 : 0;
            paramEntry.lev = currentLevel;
            paramEntry.adr = paramSize;
            paramEntry.initialized = true;
            
            tab.push_back(paramEntry);
            int paramIdx = tab.size() - 1;
            
            btab[funcBlockIdx].last = paramIdx;
            paramSize += size;
            
            param->symbolIndex = paramIdx;
            param->scopeLevel = currentLevel;
            param->dataType = param->typeName;
        }
    }
    
    btab[funcBlockIdx].psze = paramSize;
    if (!node->params.empty()) {
        btab[funcBlockIdx].lpar = tab.size() - 1;
    }
    
    if (node->body) {
        // this too
        for (ASTNode* decl : node->body->declarations) {
            if (!decl) continue;
            
            if (decl->nodeType == "VarDecl") {
                visitVarDecl(static_cast<VarDeclNode*>(decl));
            }
            else if (decl->nodeType == "ConstDecl") {
                visitConstDecl(static_cast<ConstDeclNode*>(decl));
            }
            else if (decl->nodeType == "TypeDecl") {
                visitTypeDecl(static_cast<TypeDeclNode*>(decl));
            }
        }
        
        // Process statements
        for (ASTNode* stmt : node->body->statements) {
            visitStatement(stmt);
        }
    }
    
    exitBlock();
}

void SemanticAnalyzer::visitBlock(BlockNode* node) {
    if (!node) return;
    
    int mainBlockIdx = createNewBlock();
    enterBlock(mainBlockIdx);
    
    node->scopeLevel = currentLevel;
    
    for (ASTNode* stmt : node->statements) {
        visitStatement(stmt);
    }
    
    exitBlock();
}

void SemanticAnalyzer::visitStatement(ASTNode* node) {
    if (!node) return;
    
    if (node->nodeType == "Assign") {
        visitAssign(static_cast<AssignNode*>(node));
    }
    else if (node->nodeType == "If") {
        visitIf(static_cast<IfNode*>(node));
    }
    else if (node->nodeType == "While") {
        visitWhile(static_cast<WhileNode*>(node));
    }
    else if (node->nodeType == "For") {
        visitFor(static_cast<ForNode*>(node));
    }
    else if (node->nodeType == "ProcedureCall") {
        visitProcedureCall(static_cast<ProcedureCallNode*>(node));
    }
    else if (node->nodeType == "Block") {
        for (ASTNode* stmt : static_cast<BlockNode*>(node)->statements) {
            visitStatement(stmt);
        }
    }
}

void SemanticAnalyzer::visitAssign(AssignNode* node) {
    if (!node) return;
    
    visitExpression(node->value);
    
    if (node->target) {
        if (node->target->nodeType == "Var") {
            VarNode* varNode = static_cast<VarNode*>(node->target);
            int idx = lookupIdentifier(varNode->name);
            if (idx > 0 && idx < (int)tab.size()) {
                if (tab[idx].obj == OBJ_VARIABLE) {
                    tab[idx].initialized = true;
                }
            }
        } else if (node->target->nodeType == "ArrayAccess") {
            ArrayAccessNode* arrayNode = static_cast<ArrayAccessNode*>(node->target);
            int idx = lookupIdentifier(arrayNode->arrayName);
            if (idx > 0 && idx < (int)tab.size()) {
                if (tab[idx].obj == OBJ_VARIABLE) {
                    tab[idx].initialized = true;
                }
            }
        }
    }
    
    visitExpression(node->target);
    
    string targetType = inferType(node->target);
    string valueType = inferType(node->value);
    
    if (!isCompatibleType(targetType, valueType)) {
        semanticWarning("Type mismatch: " + targetType + " := " + valueType);
    }
    
    node->dataType = targetType;
    node->scopeLevel = currentLevel;
}

void SemanticAnalyzer::visitIf(IfNode* node) {
    if (!node) return;
    
    visitExpression(node->condition);
    
    if (node->thenBranch) visitStatement(node->thenBranch);
    if (node->elseBranch) visitStatement(node->elseBranch);
    
    node->scopeLevel = currentLevel;
}

void SemanticAnalyzer::visitWhile(WhileNode* node) {
    if (!node) return;
    
    visitExpression(node->condition);
    if (node->body) visitStatement(node->body);
    
    node->scopeLevel = currentLevel;
}

void SemanticAnalyzer::visitFor(ForNode* node) {
    if (!node) return;
    
    visitExpression(node->start);
    visitExpression(node->end);
    
    if (node->counter && node->counter->nodeType == "Var") {
        VarNode* counterVar = static_cast<VarNode*>(node->counter);
        int idx = lookupIdentifier(counterVar->name);
        if (idx > 0 && idx < (int)tab.size()) {
            if (tab[idx].obj == OBJ_VARIABLE) {
                tab[idx].initialized = true;
            }
        }
    }
    
    visitExpression(node->counter);
    
    if (node->body) visitStatement(node->body);
    
    node->scopeLevel = currentLevel;
}

void SemanticAnalyzer::visitProcedureCall(ProcedureCallNode* node) {
    if (!node) return;
    
    int idx = lookupIdentifier(node->procName);
    if (idx == 0) {
        semanticError("Undefined: " + node->procName);
        hasErrors = true;
    } else {
        node->symbolIndex = idx;
    }
    
    for (ASTNode* arg : node->args) {
        visitExpression(arg);
    }
    
    node->scopeLevel = currentLevel;
    node->dataType = inferType(node);
}

void SemanticAnalyzer::visitExpression(ASTNode* node) {
    if (!node) return;
    
    if (node->nodeType == "Var") {
        visitVar(static_cast<VarNode*>(node));
    }
    else if (node->nodeType == "ArrayAccess") {
        visitArrayAccess(static_cast<ArrayAccessNode*>(node));
    }
    else if (node->nodeType == "BinOp") {
        visitBinOp(static_cast<BinOpNode*>(node));
    }
    else if (node->nodeType == "UnaryOp") {
        visitUnaryOp(static_cast<UnaryOpNode*>(node));
    }
    else if (node->nodeType == "Number") {
        node->dataType = "integer";
        node->scopeLevel = currentLevel;
    }
    else if (node->nodeType == "Real") {
        node->dataType = "real";
        node->scopeLevel = currentLevel;
    }
    else if (node->nodeType == "String") {
        node->dataType = "string";
        node->scopeLevel = currentLevel;
    }
    else if (node->nodeType == "Char") {
        node->dataType = "char";
        node->scopeLevel = currentLevel;
    }
    else if (node->nodeType == "Boolean") {
        node->dataType = "boolean";
        node->scopeLevel = currentLevel;
    }
    else if (node->nodeType == "ProcedureCall") {
        visitProcedureCall(static_cast<ProcedureCallNode*>(node));
    }
}

void SemanticAnalyzer::visitVar(VarNode* node) {
    if (!node) return;
    
    int idx = lookupIdentifier(node->name);
    if (idx == 0) {
        semanticError("Undefined: " + node->name);
        hasErrors = true;
    } else {
        node->symbolIndex = idx;
        node->scopeLevel = tab[idx].lev;
        node->dataType = inferType(node);
        
        // Check if variable is initialized before use
        if (tab[idx].obj == OBJ_VARIABLE && !tab[idx].initialized) {
            semanticWarning("Variable '" + node->name + "' may be used before initialization");
        }
    }
}

void SemanticAnalyzer::visitArrayAccess(ArrayAccessNode* node) {
    if (!node) return;
    
    int idx = lookupIdentifier(node->arrayName);
    if (idx == 0) {
        semanticError("Undefined: " + node->arrayName);
        hasErrors = true;
    } else {
        node->symbolIndex = idx;
        node->scopeLevel = tab[idx].lev;
        node->dataType = inferType(node);
        
        // Check if array is initialized before access
        if (tab[idx].obj == OBJ_VARIABLE && !tab[idx].initialized) {
            semanticWarning("Array '" + node->arrayName + "' may be used before initialization");
        }
    }
    
    visitExpression(node->index);
}

void SemanticAnalyzer::visitBinOp(BinOpNode* node) {
    if (!node) return;
    
    visitExpression(node->left);
    visitExpression(node->right);
    
    node->dataType = inferType(node);
    node->scopeLevel = currentLevel;
}

void SemanticAnalyzer::visitUnaryOp(UnaryOpNode* node) {
    if (!node) return;
    
    visitExpression(node->operand);
    
    node->dataType = inferType(node);
    node->scopeLevel = currentLevel;
}

bool SemanticAnalyzer::analyze(ASTNode* root) {
    if (!root) {
        cerr << "Error: AST root is null\n";
        return false;
    }
    
    if (root->nodeType != "Program") {
        cerr << "Error: Root must be Program\n";
        return false;
    }
    
    try {
        visitProgram(static_cast<ProgramNode*>(root));
        return !hasErrors;
    } catch (const exception& e) {
        cerr << "Semantic analysis failed: " << e.what() << endl;
        return false;
    }
}

string SemanticAnalyzer::getTypeName(int typeCode) {
    switch(typeCode) {
        case 1: return "integer";
        case 2: return "real";
        case 3: return "boolean";
        case 4: return "char";
        case 5: return "array";
        default: return "unknown";
    }
}

string SemanticAnalyzer::getObjectName(int objCode) {
    switch(objCode) {
        case 0: return "program";
        case 1: return "constant";
        case 2: return "variable";
        case 3: return "type";
        case 4: return "procedure";
        case 5: return "function";
        default: return "unknown";
    }
}

void SemanticAnalyzer::printDecoratedAST(ASTNode* node, const string& prefix, bool isLast) {
    if (!node) return;
    
    cout << prefix;
    cout << (isLast ? "└── " : "├── ");
    
    // Print node type
    cout << node->nodeType;
    
    // Print basic info
    if (node->nodeType == "Number") {
        cout << "(" << static_cast<NumberNode*>(node)->value << ")";
    }
    else if (node->nodeType == "Real") {
        cout << "(" << static_cast<RealNode*>(node)->value << ")";
    }
    else if (node->nodeType == "String") {
        cout << "(\"" << static_cast<StringNode*>(node)->value << "\")";
    }
    else if (node->nodeType == "Char") {
        cout << "('" << static_cast<CharNode*>(node)->value << "')";
    }
    else if (node->nodeType == "Boolean") {
        cout << "(" << (static_cast<BoolNode*>(node)->value ? "true" : "false") << ")";
    }
    else if (node->nodeType == "Var") {
        cout << "('" << static_cast<VarNode*>(node)->name << "')";
    }
    else if (node->nodeType == "ArrayAccess") {
        cout << ": " << static_cast<ArrayAccessNode*>(node)->arrayName;
    }
    else if (node->nodeType == "BinOp") {
        cout << "('" << static_cast<BinOpNode*>(node)->op << "')";
    }
    else if (node->nodeType == "UnaryOp") {
        cout << "('" << static_cast<UnaryOpNode*>(node)->op << "')";
    }
    else if (node->nodeType == "Program") {
        cout << "(name: '" << static_cast<ProgramNode*>(node)->name << "')";
    }
    else if (node->nodeType == "VarDecl") {
        VarDeclNode* vd = static_cast<VarDeclNode*>(node);
        cout << "(name: '" << vd->names[0] << "')";
    }
    else if (node->nodeType == "ConstDecl") {
        ConstDeclNode* cd = static_cast<ConstDeclNode*>(node);
        cout << "(name: '" << cd->name << "')";
    }
    else if (node->nodeType == "TypeDecl") {
        TypeDeclNode* td = static_cast<TypeDeclNode*>(node);
        cout << "(name: '" << td->name << "')";
    }
    else if (node->nodeType == "ProcedureDecl") {
        ProcedureDeclNode* pd = static_cast<ProcedureDeclNode*>(node);
        cout << "(name: '" << pd->name << "')";
    }
    else if (node->nodeType == "FunctionDecl") {
        FunctionDeclNode* fd = static_cast<FunctionDeclNode*>(node);
        cout << "(name: '" << fd->name << "')";
    }
    else if (node->nodeType == "ProcedureCall") {
        cout << "(name: '" << static_cast<ProcedureCallNode*>(node)->procName << "')";
    }
    
    // Print decorations
    cout << " → ";
    
    // Print tab_index if available
    if (node->symbolIndex >= 0) {
        cout << "tab_index:" << node->symbolIndex;
        
        // Print detailed info from symbol table
        if (node->symbolIndex < (int)tab.size()) {
            TabEntry& entry = tab[node->symbolIndex];
            cout << ", obj:" << getObjectName(entry.obj);
        }
        cout << ", ";
    }
    
    // Print type
    if (!node->dataType.empty()) {
        cout << "type:'" << node->dataType << "'";
    } else {
        cout << "type:void";
    }
    
    // Print level
    if (node->scopeLevel >= 0) {
        cout << ", lev:" << node->scopeLevel;
    }
    
    cout << endl;
    
    string newPrefix = prefix + (isLast ? "    " : "│   ");
    
    // Traverse children based on node type
    if (node->nodeType == "Program") {
        ProgramNode* prog = static_cast<ProgramNode*>(node);
        size_t totalChildren = (prog->declarations ? 1 : 0) + (prog->block ? 1 : 0);
        size_t count = 0;
        
        if (prog->declarations) {
            count++;
            printDecoratedAST(prog->declarations, newPrefix, count == totalChildren);
        }
        if (prog->block) {
            printDecoratedAST(prog->block, newPrefix, true);
        }
    }
    else if (node->nodeType == "Declarations") {
        DeclarationsNode* decls = static_cast<DeclarationsNode*>(node);
        for (size_t i = 0; i < decls->declarations.size(); i++) {
            printDecoratedAST(decls->declarations[i], newPrefix, i + 1 == decls->declarations.size());
        }
    }
    else if (node->nodeType == "Block") {
        BlockNode* block = static_cast<BlockNode*>(node);
        size_t totalChildren = block->declarations.size() + block->statements.size();
        size_t count = 0;
        
        for (size_t i = 0; i < block->declarations.size(); i++) {
            count++;
            printDecoratedAST(block->declarations[i], newPrefix, count == totalChildren);
        }
        for (size_t i = 0; i < block->statements.size(); i++) {
            count++;
            printDecoratedAST(block->statements[i], newPrefix, count == totalChildren);
        }
    }
    else if (node->nodeType == "VarDecl") {
        VarDeclNode* vd = static_cast<VarDeclNode*>(node);
        if (vd->arrayType) {
            printDecoratedAST(vd->arrayType, newPrefix, true);
        }
    }
    else if (node->nodeType == "TypeDecl") {
        TypeDeclNode* td = static_cast<TypeDeclNode*>(node);
        if (td->arrayType) {
            printDecoratedAST(td->arrayType, newPrefix, true);
        }
    }
    else if (node->nodeType == "ProcedureDecl") {
        ProcedureDeclNode* pd = static_cast<ProcedureDeclNode*>(node);
        if (pd->body) {
            printDecoratedAST(pd->body, newPrefix, true);
        }
    }
    else if (node->nodeType == "FunctionDecl") {
        FunctionDeclNode* fd = static_cast<FunctionDeclNode*>(node);
        if (fd->body) {
            printDecoratedAST(fd->body, newPrefix, true);
        }
    }
    else if (node->nodeType == "Assign") {
        AssignNode* an = static_cast<AssignNode*>(node);
        if (an->target) {
            printDecoratedAST(an->target, newPrefix, !an->value);
        }
        if (an->value) {
            printDecoratedAST(an->value, newPrefix, true);
        }
    }
    else if (node->nodeType == "BinOp") {
        BinOpNode* bn = static_cast<BinOpNode*>(node);
        if (bn->left) {
            printDecoratedAST(bn->left, newPrefix, !bn->right);
        }
        if (bn->right) {
            printDecoratedAST(bn->right, newPrefix, true);
        }
    }
    else if (node->nodeType == "UnaryOp") {
        UnaryOpNode* un = static_cast<UnaryOpNode*>(node);
        if (un->operand) {
            printDecoratedAST(un->operand, newPrefix, true);
        }
    }
    else if (node->nodeType == "ArrayAccess") {
        ArrayAccessNode* aan = static_cast<ArrayAccessNode*>(node);
        if (aan->index) {
            printDecoratedAST(aan->index, newPrefix, true);
        }
    }
    else if (node->nodeType == "If") {
        IfNode* ifn = static_cast<IfNode*>(node);
        size_t childCount = (ifn->condition ? 1 : 0) + (ifn->thenBranch ? 1 : 0) + (ifn->elseBranch ? 1 : 0);
        size_t count = 0;
        
        if (ifn->condition) {
            count++;
            printDecoratedAST(ifn->condition, newPrefix, count == childCount);
        }
        if (ifn->thenBranch) {
            count++;
            printDecoratedAST(ifn->thenBranch, newPrefix, count == childCount);
        }
        if (ifn->elseBranch) {
            count++;
            printDecoratedAST(ifn->elseBranch, newPrefix, count == childCount);
        }
    }
    else if (node->nodeType == "While") {
        WhileNode* wn = static_cast<WhileNode*>(node);
        if (wn->condition) {
            printDecoratedAST(wn->condition, newPrefix, !wn->body);
        }
        if (wn->body) {
            printDecoratedAST(wn->body, newPrefix, true);
        }
    }
    else if (node->nodeType == "For") {
        ForNode* fn = static_cast<ForNode*>(node);
        size_t childCount = 4; // counter, start, end, body
        size_t count = 0;
        
        if (fn->counter) {
            count++;
            printDecoratedAST(fn->counter, newPrefix, count == childCount);
        }
        if (fn->start) {
            count++;
            printDecoratedAST(fn->start, newPrefix, count == childCount);
        }
        if (fn->end) {
            count++;
            printDecoratedAST(fn->end, newPrefix, count == childCount);
        }
        if (fn->body) {
            count++;
            printDecoratedAST(fn->body, newPrefix, count == childCount);
        }
    }
    else if (node->nodeType == "ProcedureCall") {
        ProcedureCallNode* pc = static_cast<ProcedureCallNode*>(node);
        for (size_t i = 0; i < pc->args.size(); i++) {
            bool isLast = (i + 1 == pc->args.size());
            printDecoratedAST(pc->args[i], newPrefix, isLast);
        }
    }
    else if (node->nodeType == "ArrayType") {
        ArrayTypeNode* atn = static_cast<ArrayTypeNode*>(node);
        size_t childCount = (atn->rangeStart ? 1 : 0) + (atn->rangeEnd ? 1 : 0);
        size_t count = 0;
        
        if (atn->rangeStart) {
            count++;
            printDecoratedAST(atn->rangeStart, newPrefix, count == childCount);
        }
        if (atn->rangeEnd) {
            count++;
            printDecoratedAST(atn->rangeEnd, newPrefix, count == childCount);
        }
    }
}

void semanticAnalysis(ASTNode* ast) {
    cout << "\nhere\n" << endl;
    SemanticAnalyzer analyzer;
    analyzer.analyze(ast);

    cout << "\n========== Decorated AST ==========\n";
    analyzer.printDecoratedAST(ast, "", true);
    cout << "===================================\n" << endl;
}

void printSymbolTables() {
    printTab();
    printBtab();
    printAtab();
}

// void printDecoratedASTTree(ASTNode* root) {
//     if (!root) {
//         cerr << "Error: AST root is null\n";
//         return;
//     }
    
//     cout << "\n========== Decorated AST ==========\n";
    
//     SemanticAnalyzer analyzer;
//     analyzer.printDecoratedAST(root, "", true);
    
// }
