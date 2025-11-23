#include "ast.hpp"
#include <iostream>
#include <sstream>

using namespace std;

ProcedureDeclNode::~ProcedureDeclNode() {
    for (auto p : params) delete p;
    if (body) delete body;
}

FunctionDeclNode::~FunctionDeclNode() {
    for (auto p : params) delete p;
    if (body) delete body;
}

// HELPER
static string getTokenText(ParseNode* p) {
    if (!p) return "";
    string s = p->label;
    size_t l = s.find("(");
    size_t r = s.find_last_of(")");
    if (l == string::npos || r == string::npos) return s;
    return s.substr(l + 1, r - l - 1);
}

static bool has(ParseNode* p, const string& key) {
    if (!p) return false;
    return p->label.find(key) != string::npos;
}

static bool isToken(ParseNode* p, const string& prefix) {
    if (!p) return false;
    return p->label.rfind(prefix, 0) == 0;
}

ASTNode* convert(ParseNode* p);

ASTNode* buildFactor(ParseNode* p) {
    if (!p || p->children.empty()) return nullptr;
    
    ParseNode* c = p->children[0];

    // NUMBER
    if (isToken(c, "NUMBER")) {
        string val = getTokenText(c);
        if (val.find('.') != string::npos) {
            return new RealNode(stod(val));
        }
        return new NumberNode(stoi(val));
    }

    // STRING LITERAL
    if (isToken(c, "STRING_LITERAL")) {
        return new StringNode(getTokenText(c));
    }

    // CHAR_LITERAL
    if (isToken(c, "CHAR_LITERAL")) {
        string val = getTokenText(c);
        return new CharNode(val.empty() ? '\0' : val[0]);
    }

    // Boolean literals (KEYWORD benar/salah)
    if (isToken(c, "KEYWORD")) {
        string kw = getTokenText(c);
        if (kw == "true" || kw == "benar") return new BoolNode(true);
        if (kw == "false" || kw == "salah") return new BoolNode(false);
    }

    // IDENTIFIER: could be variable or array access or function call
    if (isToken(c, "IDENTIFIER")) {
        string name = getTokenText(c);
        
        // Check array access: IDENTIFIER [ expr ]
        if (p->children.size() >= 4 && isToken(p->children[1], "LBRACKET")) {
            ASTNode* indexExpr = convert(p->children[2]);
            return new ArrayAccessNode(name, indexExpr);
        }
        
        // Check function call
        if (p->children.size() >= 2 && has(p->children[1], "<procedure/function-call>")) {
            return convert(p->children[1]);
        }
        
        return new VarNode(name);
    }

    // "(" expression ")"
    if (isToken(c, "LPARENTHESIS") && p->children.size() >= 2) {
        return convert(p->children[1]);
    }

    // NOT operator (tidak)
    if (has(c, "tidak") && p->children.size() >= 2) {
        ASTNode* operand = convert(p->children[1]);
        return new UnaryOpNode("not", operand);
    }

    // Unary minus/plus
    if ((c->label.find("+") != string::npos || c->label.find("-") != string::npos) && 
        p->children.size() >= 2) {
        string op = getTokenText(c);
        ASTNode* operand = convert(p->children[1]);
        return new UnaryOpNode(op, operand);
    }

    // procedure/function call node
    if (has(p->children[0], "<procedure/function-call>")) {
        return convert(p->children[0]);
    }

    cerr << "Invalid factor: " << p->label << endl;
    return nullptr;
}

ASTNode* buildTerm(ParseNode* p) {
    if (!p || p->children.empty()) return nullptr;
    
    ASTNode* node = convert(p->children[0]);
    
    for (size_t i = 1; i + 1 < p->children.size(); i += 2) {
        if (has(p->children[i], "operator")) {
            string op = getTokenText(p->children[i]->children[0]);
            ASTNode* right = convert(p->children[i + 1]);
            node = new BinOpNode(op, node, right);
        }
    }
    
    return node;
}

ASTNode* buildSimpleExpression(ParseNode* p) {
    if (!p || p->children.empty()) return nullptr;
    
    size_t idx = 0;
    
    // Check unary op di awal
    if (has(p->children[0], "ARITHMETIC_OPERATOR")) {
        string op = getTokenText(p->children[0]);
        ASTNode* operand = convert(p->children[1]);
        ASTNode* node = new UnaryOpNode(op, operand);
        idx = 2;
        
        // binary operations
        while (idx + 1 < p->children.size()) {
            if (has(p->children[idx], "operator")) {
                string binOp = getTokenText(p->children[idx]->children[0]);
                ASTNode* right = convert(p->children[idx + 1]);
                node = new BinOpNode(binOp, node, right);
                idx += 2;
            } else {
                break;
            }
        }
        return node;
    }
    
    // No unary operator
    ASTNode* node = convert(p->children[0]);
    idx = 1;
    
    while (idx + 1 < p->children.size()) {
        if (has(p->children[idx], "operator")) {
            string op = getTokenText(p->children[idx]->children[0]);
            ASTNode* right = convert(p->children[idx + 1]);
            node = new BinOpNode(op, node, right);
            idx += 2;
        } else {
            break;
        }
    }
    
    return node;
}

ASTNode* buildExpression(ParseNode* p) {
    if (!p || p->children.empty()) return nullptr;
    
    if (p->children.size() == 1) {
        return convert(p->children[0]);
    }
    
    // expression -> simple-expr relop simple-expr
    if (p->children.size() >= 3 && has(p->children[1], "relational")) {
        ASTNode* left = convert(p->children[0]);
        string op = getTokenText(p->children[1]->children[0]);
        ASTNode* right = convert(p->children[2]);
        return new BinOpNode(op, left, right);
    }
    
    return convert(p->children[0]);
}

ASTNode* convert(ParseNode* p) {
    if (!p) return nullptr;

    string L = p->label;

    // PROGRAM
    if (has(p, "<program>")) {
        string name = getTokenText(p->children[0]->children[1]);
        ProgramNode* prog = new ProgramNode(name);

        // Parse declaration part
        if (p->children.size() > 1) {
            ParseNode* declPart = p->children[1];
            
            for (auto c : declPart->children) {
                if (has(c, "<var-declaration>")) {
                    VarDeclNode* v = new VarDeclNode();
                    
                    for (auto cc : c->children) {
                        if (has(cc, "<identifier-list>")) {
                            for (auto id : cc->children) {
                                if (isToken(id, "IDENTIFIER")) {
                                    v->names.push_back(getTokenText(id));
                                }
                            }
                        }
                        else if (has(cc, "<type>")) {
                            if (!cc->children.empty()) {
                                v->typeName = getTokenText(cc->children[0]);
                            }
                        }
                    }
                    
                    prog->declarations.push_back(v);
                }
                else if (has(c, "<const-declaration>")) {
                    // Parse const declarations
                    for (size_t i = 1; i < c->children.size(); ) {
                        if (isToken(c->children[i], "IDENTIFIER")) {
                            string constName = getTokenText(c->children[i]);
                            ASTNode* constValue = nullptr;
                            
                            if (i + 2 < c->children.size()) {
                                ParseNode* valNode = c->children[i + 2];
                                if (isToken(valNode, "NUMBER")) {
                                    string val = getTokenText(valNode);
                                    if (val.find('.') != string::npos) {
                                        constValue = new RealNode(stod(val));
                                    } else {
                                        constValue = new NumberNode(stoi(val));
                                    }
                                } else if (isToken(valNode, "STRING_LITERAL")) {
                                    constValue = new StringNode(getTokenText(valNode));
                                } else if (isToken(valNode, "CHAR_LITERAL")) {
                                    string v = getTokenText(valNode);
                                    constValue = new CharNode(v.empty() ? '\0' : v[0]);
                                } else if (isToken(valNode, "IDENTIFIER")) {
                                    constValue = new VarNode(getTokenText(valNode));
                                }
                            }
                            
                            if (constValue) {
                                prog->declarations.push_back(new ConstDeclNode(constName, constValue));
                            }
                            
                            i += 4; // IDENTIFIER := value SEMICOLON
                        } else {
                            i++;
                        }
                    }
                }
                else if (has(c, "<type-declaration>")) {
                    // Parse tipe declarations
                    for (size_t i = 1; i < c->children.size(); ) {
                        if (isToken(c->children[i], "IDENTIFIER")) {
                            string typeName = getTokenText(c->children[i]);
                            string typeDef = "";
                            
                            if (i + 2 < c->children.size() && has(c->children[i + 2], "<type-definition>")) {
                                ParseNode* defNode = c->children[i + 2];
                                if (!defNode->children.empty()) {
                                    typeDef = getTokenText(defNode->children[0]);
                                }
                            }
                            
                            prog->declarations.push_back(new TypeDeclNode(typeName, typeDef));
                            i += 4; // IDENTIFIER := type-definition SEMICOLON
                        } else {
                            i++;
                        }
                    }
                }
                else if (has(c, "<subprogram-declaration>")) {
                    ASTNode* subprog = convert(c);
                    if (subprog) {
                        prog->declarations.push_back(subprog);
                    }
                }
            }
        }

        // Parse compound statement (main block)
        if (p->children.size() > 2) {
            prog->block = dynamic_cast<BlockNode*>(convert(p->children[2]));
        }

        return prog;
    }

    // SUBPROGRAM DECLARATION
    if (has(p, "<subprogram-declaration>")) {
        if (!p->children.empty()) {
            return convert(p->children[0]);
        }
        return nullptr;
    }

    // PROCEDURE DECLARATION
    if (has(p, "<procedure-declaration>")) {
        string procName = getTokenText(p->children[1]);
        ProcedureDeclNode* proc = new ProcedureDeclNode(procName);
        
        // Parse parameters
        size_t blockIdx = 3; // Default: prosedur ID ; block ;
        if (p->children.size() > 3 && has(p->children[2], "<formal-parameter-list>")) {
            ParseNode* paramList = p->children[2];
            // Parse parameter groups
            for (auto child : paramList->children) {
                if (has(child, "<parameter-group>")) {
                    ParamNode* param = new ParamNode();
                    for (auto pc : child->children) {
                        if (has(pc, "<identifier-list>")) {
                            for (auto id : pc->children) {
                                if (isToken(id, "IDENTIFIER")) {
                                    param->names.push_back(getTokenText(id));
                                }
                            }
                        }
                        else if (has(pc, "<type>")) {
                            if (!pc->children.empty()) {
                                param->typeName = getTokenText(pc->children[0]);
                            }
                        }
                    }
                    proc->params.push_back(param);
                }
            }
            blockIdx = 4; // prosedur ID (params) ; block ;
        }
        
        // Parse block
        if (blockIdx < p->children.size()) {
            proc->body = dynamic_cast<BlockNode*>(convert(p->children[blockIdx]));
        }
        
        return proc;
    }

    // FUNCTION DECLARATION
    if (has(p, "<function-declaration>")) {
        string funcName = getTokenText(p->children[1]);
        string returnType = "";
        
        // Find return type
        size_t blockIdx = 6; // Default: fungsi ID : type ; block ;
        for (size_t i = 0; i < p->children.size(); i++) {
            if (has(p->children[i], "<type>")) {
                if (!p->children[i]->children.empty()) {
                    returnType = getTokenText(p->children[i]->children[0]);
                }
                break;
            }
        }
        
        FunctionDeclNode* func = new FunctionDeclNode(funcName, returnType);
        
        // Parse parameters
        if (p->children.size() > 3 && has(p->children[2], "<formal-parameter-list>")) {
            ParseNode* paramList = p->children[2];
            for (auto child : paramList->children) {
                if (has(child, "<parameter-group>")) {
                    ParamNode* param = new ParamNode();
                    for (auto pc : child->children) {
                        if (has(pc, "<identifier-list>")) {
                            for (auto id : pc->children) {
                                if (isToken(id, "IDENTIFIER")) {
                                    param->names.push_back(getTokenText(id));
                                }
                            }
                        }
                        else if (has(pc, "<type>")) {
                            if (!pc->children.empty()) {
                                param->typeName = getTokenText(pc->children[0]);
                            }
                        }
                    }
                    func->params.push_back(param);
                }
            }
            blockIdx = 7; // fungsi ID (params) : type ; block ;
        }
        
        // Parse block
        if (blockIdx < p->children.size()) {
            func->body = dynamic_cast<BlockNode*>(convert(p->children[blockIdx]));
        }
        
        return func;
    }

    // BLOCK
    if (has(p, "<block>")) {
        BlockNode* block = new BlockNode();
        
        for (auto child : p->children) {
            if (has(child, "<declaration-part>")) {
                // Parse declarations
                for (auto decl : child->children) {
                    if (has(decl, "<var-declaration>")) {
                        ASTNode* varDecl = convert(decl);
                        if (varDecl) block->declarations.push_back(varDecl);
                    }
                    else if (has(decl, "<const-declaration>")) {
                        ASTNode* constDecl = convert(decl);
                        if (constDecl) block->declarations.push_back(constDecl);
                    }
                }
            }
            else if (has(child, "<compound-statement>")) {
                // Parse statements
                BlockNode* stmtBlock = dynamic_cast<BlockNode*>(convert(child));
                if (stmtBlock) {
                    block->statements = stmtBlock->statements;
                    stmtBlock->statements.clear(); // Buat prevent double delete
                    delete stmtBlock;
                }
            }
        }
        
        return block;
    }

    // COMPOUND STATEMENT
    if (has(p, "<compound-statement>")) {
        BlockNode* block = new BlockNode();
        
        // statement-list
        for (auto child : p->children) {
            if (has(child, "<statement-list>")) {
                for (auto s : child->children) {
                    if (has(s, "<statement>")) {
                        if (!s->children.empty()) {
                            ASTNode* stmt = convert(s->children[0]);
                            if (stmt) block->statements.push_back(stmt);
                        }
                    }
                }
            }
        }
        
        return block;
    }

    // ASSIGNMENT
    if (has(p, "<assignment-statement>")) {
        ASTNode* target = nullptr;
        ASTNode* value = nullptr;
        
        string varName = getTokenText(p->children[0]);
        
        // Check for array access
        if (p->children.size() >= 4 && isToken(p->children[1], "LBRACKET")) {
            ASTNode* index = convert(p->children[2]);
            target = new ArrayAccessNode(varName, index);
            value = convert(p->children.back());
        } else {
            target = new VarNode(varName);
            value = convert(p->children.back());
        }
        
        return new AssignNode(target, value);
    }

    // IF STATEMENT
    if (has(p, "<if-statement>")) {
        ASTNode* condition = nullptr;
        ASTNode* thenBranch = nullptr;
        ASTNode* elseBranch = nullptr;
        
        size_t idx = 0;
        
        // "jika"
        if (idx < p->children.size() && isToken(p->children[idx], "KEYWORD")) {
            idx++;
        }
        
        // condition exp
        if (idx < p->children.size() && has(p->children[idx], "<expression>")) {
            condition = convert(p->children[idx]);
            idx++;
        }
        
        // "maka"
        if (idx < p->children.size() && isToken(p->children[idx], "KEYWORD")) {
            idx++;
        }
        
        // then
        if (idx < p->children.size() && has(p->children[idx], "<statement>")) {
            thenBranch = convert(p->children[idx]);
            idx++;
        }
        
        // else
        if (idx < p->children.size() && isToken(p->children[idx], "SEMICOLON")) {
            idx++;
        }
        
        if (idx < p->children.size() && isToken(p->children[idx], "KEYWORD")) {
            string kw = getTokenText(p->children[idx]);
            if (kw == "selain-itu") {
                idx++;
                if (idx < p->children.size() && has(p->children[idx], "<statement>")) {
                    elseBranch = convert(p->children[idx]);
                }
            }
        }
        
        return new IfNode(condition, thenBranch, elseBranch);
    }

    // WHILE STATEMENT
    if (has(p, "<while-statement>")) {
        ASTNode* condition = nullptr;
        ASTNode* body = nullptr;
        
        size_t idx = 0;
        
        // "selama"
        if (idx < p->children.size() && isToken(p->children[idx], "KEYWORD")) {
            idx++;
        }
        
        if (idx < p->children.size() && has(p->children[idx], "<expression>")) {
            condition = convert(p->children[idx]);
            idx++;
        }
        
        // "lakukan"
        if (idx < p->children.size() && isToken(p->children[idx], "KEYWORD")) {
            idx++;
        }
        
        // compound-statement
        if (idx < p->children.size() && has(p->children[idx], "<compound-statement>")) {
            body = convert(p->children[idx]);
        }
        
        return new WhileNode(condition, body);
    }

    // FOR STATEMENT
    if (has(p, "<for-statement>")) {
        string counter = "";
        ASTNode* start = nullptr;
        ASTNode* end = nullptr;
        ASTNode* body = nullptr;
        bool ascending = true;
        
        size_t idx = 0;
        
        // "untuk"
        if (idx < p->children.size() && isToken(p->children[idx], "KEYWORD")) {
            idx++;
        }
        
        if (idx < p->children.size() && isToken(p->children[idx], "IDENTIFIER")) {
            counter = getTokenText(p->children[idx]);
            idx++;
        }
        
        // ":=" 
        if (idx < p->children.size() && isToken(p->children[idx], "ASSIGN_OPERATOR")) {
            idx++;
        }
        
        if (idx < p->children.size() && has(p->children[idx], "<expression>")) {
            start = convert(p->children[idx]);
            idx++;
        }
        
        // Check "ke" or "turun-ke"
        if (idx < p->children.size() && isToken(p->children[idx], "KEYWORD")) {
            string kw = getTokenText(p->children[idx]);
            if (kw == "turun-ke") {
                ascending = false;
            }
            idx++;
        }
        
        if (idx < p->children.size() && has(p->children[idx], "<expression>")) {
            end = convert(p->children[idx]);
            idx++;
        }
        
        // "lakukan"
        if (idx < p->children.size() && isToken(p->children[idx], "KEYWORD")) {
            idx++;
        }
        
        if (idx < p->children.size() && has(p->children[idx], "<compound-statement>")) {
            body = convert(p->children[idx]);
        }
        
        return new ForNode(counter, start, end, body, ascending);
    }

    // PROCEDURE/FUNCTION CALL
    if (has(p, "<procedure/function-call>")) {
        if (p->children.empty()) return nullptr;
        
        string name = getTokenText(p->children[0]);
        ProcedureCallNode* call = new ProcedureCallNode(name);
        
        // Parse arguments
        for (auto child : p->children) {
            if (has(child, "<parameter-list>")) {
                for (auto param : child->children) {
                    if (has(param, "<expression>")) {
                        ASTNode* arg = convert(param);
                        if (arg) call->args.push_back(arg);
                    }
                }
            }
        }
        
        return call;
    }

    // EXPRESSIONS
    if (has(p, "<expression>")) return buildExpression(p);
    if (has(p, "<simple-expression>")) return buildSimpleExpression(p);
    if (has(p, "<term>")) return buildTerm(p);
    if (has(p, "<factor>")) return buildFactor(p);

    if (has(p, "<statement>")) {
        if (p->children.empty()) return nullptr;
        return convert(p->children[0]);
    }

    // Defaultnya try to convert first child
    if (!p->children.empty()) {
        return convert(p->children[0]);
    }

    return nullptr;
}

void printAST(ASTNode* node, const string& prefix, bool isLast) {
    if (!node) return;
    
    cout << prefix;
    cout << (isLast ? "└── " : "├── ");
    
    cout << node->nodeType;
    
    if (node->nodeType == "Number") {
        cout << ": " << static_cast<NumberNode*>(node)->value;
    }
    else if (node->nodeType == "Real") {
        cout << ": " << static_cast<RealNode*>(node)->value;
    }
    else if (node->nodeType == "String") {
        cout << ": \"" << static_cast<StringNode*>(node)->value << "\"";
    }
    else if (node->nodeType == "Char") {
        cout << ": '" << static_cast<CharNode*>(node)->value << "'";
    }
    else if (node->nodeType == "Boolean") {
        cout << ": " << (static_cast<BoolNode*>(node)->value ? "true" : "false");
    }
    else if (node->nodeType == "Var") {
        cout << ": " << static_cast<VarNode*>(node)->name;
    }
    else if (node->nodeType == "ArrayAccess") {
        cout << ": " << static_cast<ArrayAccessNode*>(node)->arrayName;
    }
    else if (node->nodeType == "BinOp") {
        cout << ": " << static_cast<BinOpNode*>(node)->op;
    }
    else if (node->nodeType == "UnaryOp") {
        cout << ": " << static_cast<UnaryOpNode*>(node)->op;
    }
    else if (node->nodeType == "Program") {
        cout << ": " << static_cast<ProgramNode*>(node)->name;
    }
    else if (node->nodeType == "VarDecl") {
        VarDeclNode* vd = static_cast<VarDeclNode*>(node);
        cout << ": ";
        for (size_t i = 0; i < vd->names.size(); i++) {
            cout << vd->names[i];
            if (i + 1 < vd->names.size()) cout << ", ";
        }
        cout << " : " << vd->typeName;
    }
    else if (node->nodeType == "ConstDecl") {
        cout << ": " << static_cast<ConstDeclNode*>(node)->name;
    }
    else if (node->nodeType == "TypeDecl") {
        cout << ": " << static_cast<TypeDeclNode*>(node)->name;
    }
    else if (node->nodeType == "ProcedureDecl") {
        cout << ": " << static_cast<ProcedureDeclNode*>(node)->name;
    }
    else if (node->nodeType == "FunctionDecl") {
        FunctionDeclNode* fd = static_cast<FunctionDeclNode*>(node);
        cout << ": " << fd->name << " -> " << fd->returnType;
    }
    else if (node->nodeType == "ProcedureCall") {
        cout << ": " << static_cast<ProcedureCallNode*>(node)->procName;
    }
    else if (node->nodeType == "For") {
        ForNode* fn = static_cast<ForNode*>(node);
        cout << ": " << fn->counter << " (" << (fn->ascending ? "ke" : "turun-ke") << ")";
    }
    
    cout << endl;
    
    string newPrefix = prefix + (isLast ? "    " : "│   ");
    
    if (node->nodeType == "Program") {
        ProgramNode* prog = static_cast<ProgramNode*>(node);
        size_t totalChildren = prog->declarations.size() + (prog->block ? 1 : 0);
        
        for (size_t i = 0; i < prog->declarations.size(); i++) {
            printAST(prog->declarations[i], newPrefix, i + 1 == totalChildren && !prog->block);
        }
        if (prog->block) {
            printAST(prog->block, newPrefix, true);
        }
    }
    else if (node->nodeType == "Block") {
        BlockNode* block = static_cast<BlockNode*>(node);
        size_t totalChildren = block->declarations.size() + block->statements.size();
        size_t count = 0;
        
        for (size_t i = 0; i < block->declarations.size(); i++) {
            count++;
            printAST(block->declarations[i], newPrefix, count == totalChildren);
        }
        for (size_t i = 0; i < block->statements.size(); i++) {
            count++;
            printAST(block->statements[i], newPrefix, count == totalChildren);
        }
    }
    else if (node->nodeType == "VarDecl") {
        // no child
    }
    else if (node->nodeType == "ConstDecl") {
        ConstDeclNode* cd = static_cast<ConstDeclNode*>(node);
        if (cd->value) {
            printAST(cd->value, newPrefix, true);
        }
    }
    else if (node->nodeType == "ProcedureDecl") {
        ProcedureDeclNode* pd = static_cast<ProcedureDeclNode*>(node);
        size_t totalChildren = pd->params.size() + (pd->body ? 1 : 0);
        
        for (size_t i = 0; i < pd->params.size(); i++) {
            ParamNode* param = pd->params[i];
            cout << newPrefix << (i + 1 == totalChildren && !pd->body ? "└── " : "├── ");
            cout << "Param: ";
            for (size_t j = 0; j < param->names.size(); j++) {
                cout << param->names[j];
                if (j + 1 < param->names.size()) cout << ", ";
            }
            cout << " : " << param->typeName << endl;
        }
        if (pd->body) {
            printAST(pd->body, newPrefix, true);
        }
    }
    else if (node->nodeType == "FunctionDecl") {
        FunctionDeclNode* fd = static_cast<FunctionDeclNode*>(node);
        size_t totalChildren = fd->params.size() + (fd->body ? 1 : 0);
        
        for (size_t i = 0; i < fd->params.size(); i++) {
            ParamNode* param = fd->params[i];
            cout << newPrefix << (i + 1 == totalChildren && !fd->body ? "└── " : "├── ");
            cout << "Param: ";
            for (size_t j = 0; j < param->names.size(); j++) {
                cout << param->names[j];
                if (j + 1 < param->names.size()) cout << ", ";
            }
            cout << " : " << param->typeName << endl;
        }
        if (fd->body) {
            printAST(fd->body, newPrefix, true);
        }
    }
    else if (node->nodeType == "Assign") {
        AssignNode* an = static_cast<AssignNode*>(node);
        if (an->target) printAST(an->target, newPrefix, !an->value);
        if (an->value) printAST(an->value, newPrefix, true);
    }
    else if (node->nodeType == "BinOp") {
        BinOpNode* bn = static_cast<BinOpNode*>(node);
        if (bn->left) printAST(bn->left, newPrefix, !bn->right);
        if (bn->right) printAST(bn->right, newPrefix, true);
    }
    else if (node->nodeType == "UnaryOp") {
        UnaryOpNode* un = static_cast<UnaryOpNode*>(node);
        if (un->operand) printAST(un->operand, newPrefix, true);
    }
    else if (node->nodeType == "ArrayAccess") {
        ArrayAccessNode* aan = static_cast<ArrayAccessNode*>(node);
        if (aan->index) printAST(aan->index, newPrefix, true);
    }
    else if (node->nodeType == "If") {
        IfNode* ifn = static_cast<IfNode*>(node);
        size_t childCount = (ifn->condition ? 1 : 0) + (ifn->thenBranch ? 1 : 0) + (ifn->elseBranch ? 1 : 0);
        size_t count = 0;
        
        if (ifn->condition) {
            count++;
            printAST(ifn->condition, newPrefix, count == childCount);
        }
        if (ifn->thenBranch) {
            count++;
            printAST(ifn->thenBranch, newPrefix, count == childCount);
        }
        if (ifn->elseBranch) {
            count++;
            printAST(ifn->elseBranch, newPrefix, count == childCount);
        }
    }
    else if (node->nodeType == "While") {
        WhileNode* wn = static_cast<WhileNode*>(node);
        if (wn->condition) printAST(wn->condition, newPrefix, !wn->body);
        if (wn->body) printAST(wn->body, newPrefix, true);
    }
    else if (node->nodeType == "For") {
        ForNode* fn = static_cast<ForNode*>(node);
        size_t childCount = (fn->start ? 1 : 0) + (fn->end ? 1 : 0) + (fn->body ? 1 : 0);
        size_t count = 0;
        
        if (fn->start) {
            count++;
            printAST(fn->start, newPrefix, count == childCount);
        }
        if (fn->end) {
            count++;
            printAST(fn->end, newPrefix, count == childCount);
        }
        if (fn->body) {
            count++;
            printAST(fn->body, newPrefix, count == childCount);
        }
    }
    else if (node->nodeType == "ProcedureCall") {
        ProcedureCallNode* pc = static_cast<ProcedureCallNode*>(node);
        for (size_t i = 0; i < pc->args.size(); i++) {
            printAST(pc->args[i], newPrefix, i + 1 == pc->args.size());
        }
    }
}

// ENTRY POINT
ASTNode* buildAST(ParseNode* root) {
    if (!root) {
        cerr << "Error: Parse tree root is null\n";
        return nullptr;
    }
    return convert(root);
}

ASTNode* ASTMain(ParseNode* root) {
    ASTNode* ast = buildAST(root);
    
    if (!ast) {
        cerr << "Error: Failed to build AST\n";
        return nullptr;
    }
    
    cout << "\n========== AST Structure ==========\n";
    printAST(ast);
    
    return ast;
}