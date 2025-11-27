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

static bool isArrayType(ParseNode* typeNode) {
    if (!typeNode || typeNode->children.empty()) return false;
    return has(typeNode->children[0], "<array-type>");
}

static string getTypeName(ParseNode* typeNode) {
    if (!typeNode || typeNode->children.empty()) return "";
    
    ParseNode* child = typeNode->children[0];
    
    
    if (isToken(child, "KEYWORD")) {
        return getTokenText(child);
    }
    
    // Array type: larik[...] dari <type>
    if (has(child, "<array-type>")) {
        string arrayType = "larik[";
        
        if (child->children.size() > 2 && has(child->children[2], "<range>")) {
            ParseNode* rangeNode = child->children[2];
            if (rangeNode->children.size() >= 3) {
                arrayType += getTokenText(rangeNode->children[0]->children[0]); // start
                arrayType += " .. ";
                arrayType += getTokenText(rangeNode->children[2]->children[0]); // end
            }
        }
        
        arrayType += "] dari ";
        
        if (child->children.size() > 5) {
            arrayType += getTypeName(child->children[5]);
        }
        
        return arrayType;
    }
    
    return "";
}

static ArrayTypeNode* buildArrayTypeNode(ParseNode* typeNode) {
    if (!typeNode || typeNode->children.empty()) return nullptr;
    
    ParseNode* child = typeNode->children[0];
    
    if (!has(child, "<array-type>")) return nullptr;
    
    ASTNode* rangeStart = nullptr;
    ASTNode* rangeEnd = nullptr;
    string elementType = "";
    
    if (child->children.size() > 2 && has(child->children[2], "<range>")) {
        ParseNode* rangeNode = child->children[2];
        if (rangeNode->children.size() >= 3) {
            rangeStart = convert(rangeNode->children[0]);
            rangeEnd = convert(rangeNode->children[2]);
        }
    }
    
    if (child->children.size() > 5) {
        elementType = getTypeName(child->children[5]);
    }
    
    return new ArrayTypeNode(rangeStart, rangeEnd, elementType);
}

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
        prog->declarations = new DeclarationsNode();

        // Parse declaration part
        if (p->children.size() > 1) {
            ParseNode* declPart = p->children[1];
            
            for (auto c : declPart->children) {
                if (has(c, "<var-declaration>")) {
                   
                    vector<string> currentVarNames;
                    
                    for (size_t i = 0; i < c->children.size(); i++) {
                        auto cc = c->children[i];
                        
                        if (has(cc, "<identifier-list>")) {
                            currentVarNames.clear();
                            for (auto id : cc->children) {
                                if (isToken(id, "IDENTIFIER")) {
                                    currentVarNames.push_back(getTokenText(id));
                                }
                            }
                        }
                        else if (has(cc, "<type>")) {
                            for (const string& varName : currentVarNames) {
                                VarDeclNode* v = new VarDeclNode();
                                v->names.push_back(varName);
                                
                                // Check if array type
                                if (isArrayType(cc)) {
                                    v->arrayType = buildArrayTypeNode(cc);
                                    v->typeName = ""; 
                                } else {
                                    v->typeName = getTypeName(cc);
                                    v->arrayType = nullptr;
                                }
                                
                                prog->declarations->declarations.push_back(v);
                            }
                            
                            currentVarNames.clear();
                        }
                    }
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
                                prog->declarations->declarations.push_back(new ConstDeclNode(constName, constValue));
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
                            TypeDeclNode* typeDecl = new TypeDeclNode(typeName, "");
                            
                            if (i + 2 < c->children.size() && has(c->children[i + 2], "<type-definition>")) {
                                ParseNode* defNode = c->children[i + 2];
                                if (!defNode->children.empty()) {
                                    ParseNode* typeNode = defNode->children[0];
                                    
                                    if (has(typeNode, "<type>")) {
                                        if (isArrayType(typeNode)) {
                                            typeDecl->arrayType = buildArrayTypeNode(typeNode);
                                        } else {
                                            typeDecl->definition = getTypeName(typeNode);
                                        }
                                    } else {
                                        typeDecl->definition = getTokenText(typeNode);
                                    }
                                }
                            }
                            
                            prog->declarations->declarations.push_back(typeDecl);
                            i += 4; // IDENTIFIER := type-definition SEMICOLON
                        } else {
                            i++;
                        }
                    }
                }
                else if (has(c, "<subprogram-declaration>")) {
                    ASTNode* subprog = convert(c);
                    if (subprog) {
                        prog->declarations->declarations.push_back(subprog);
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
        cout << "(" << static_cast<NumberNode*>(node)->value << ")";
    }
    else if (node->nodeType == "Real") {
        cout << "(" << static_cast<RealNode*>(node)->value << ")";
    }
    else if (node->nodeType == "String") {
        cout << "(\"" << static_cast<StringNode*>(node)->value << "\"" << ")";
    }
    else if (node->nodeType == "Char") {
        cout << "('" << static_cast<CharNode*>(node)->value << "'" << ")";
    }
    else if (node->nodeType == "Boolean") {
        cout << "(" << (static_cast<BoolNode*>(node)->value ? "true" : "false") << ")";
    }
    else if (node->nodeType == "Var") {
        cout << "(" << static_cast<VarNode*>(node)->name << ")";
    }
    else if (node->nodeType == "ArrayAccess") {
        cout << ": " << static_cast<ArrayAccessNode*>(node)->arrayName;
    }
    else if (node->nodeType == "BinOp") {
        cout << "(" << static_cast<BinOpNode*>(node)->op << ")";
    }
    else if (node->nodeType == "UnaryOp") {
        cout << "(" << static_cast<UnaryOpNode*>(node)->op << ")";
    }
    else if (node->nodeType == "Program") {
        cout << "(name: " << static_cast<ProgramNode*>(node)->name << ")";
    }
    else if (node->nodeType == "VarDecl") {
        VarDeclNode* vd = static_cast<VarDeclNode*>(node);
        cout << "(name: '" << vd->names[0] << "', type: ";
        if (vd->arrayType) {
            cout << "ArrayType)";
        } else {
            cout << "'" << vd->typeName << "')";
        }
    }
    else if (node->nodeType == "ConstDecl") {
        ConstDeclNode* cd = static_cast<ConstDeclNode*>(node);
        cout << " (name: " << cd->name << ", value: ";
        
        if (cd->value) {
            cout << cd->value->nodeType << "(";
            if (cd->value->nodeType == "Number") {
                cout << static_cast<NumberNode*>(cd->value)->value;
            } else if (cd->value->nodeType == "Real") {
                cout << static_cast<RealNode*>(cd->value)->value;
            } else if (cd->value->nodeType == "String") {
                cout << "\"" << static_cast<StringNode*>(cd->value)->value << "\"";
            } else if (cd->value->nodeType == "Char") {
                cout << "'" << static_cast<CharNode*>(cd->value)->value << "'";
            } else if (cd->value->nodeType == "Boolean") {
                cout << (static_cast<BoolNode*>(cd->value)->value ? "true" : "false");
            }
            cout << ")";
        } else {
            cout << "null";
        }
        cout << ")";
    }
    else if (node->nodeType == "TypeDecl") {
        TypeDeclNode* td = static_cast<TypeDeclNode*>(node);
        cout << "(name: '" << td->name << "', type: ";
        if (td->arrayType) {
            cout << "ArrayType)";
        } else {
            cout << "'" << td->definition << "')";
        }
    }
    else if (node->nodeType == "ProcedureDecl") {
        ProcedureDeclNode* pd = static_cast<ProcedureDeclNode*>(node);
        cout << "(name: '" << pd->name << "', params: [";
        for (size_t i = 0; i < pd->params.size(); i++) {
            ParamNode* param = pd->params[i];
            if (i > 0) cout << ", ";
            cout << "(";
            for (size_t j = 0; j < param->names.size(); j++) {
                if (j > 0) cout << ", ";
                cout << param->names[j];
            }
            cout << " : " << param->typeName << ")";
        }
        cout << "])";
    }
    else if (node->nodeType == "FunctionDecl") {
        FunctionDeclNode* fd = static_cast<FunctionDeclNode*>(node);
        cout << "(name: '" << fd->name << "', params: [";
        for (size_t i = 0; i < fd->params.size(); i++) {
            ParamNode* param = fd->params[i];
            if (i > 0) cout << ", ";
            cout << "(";
            for (size_t j = 0; j < param->names.size(); j++) {
                if (j > 0) cout << ", ";
                cout << param->names[j];
            }
            cout << " : " << param->typeName << ")";
        }
        cout << "], returnType: '" << fd->returnType << "')";
    }
    else if (node->nodeType == "ProcedureCall") {
        cout << "(name: " << static_cast<ProcedureCallNode*>(node)->procName << ")";
    }
    else if (node->nodeType == "For") {
        ForNode* fn = static_cast<ForNode*>(node);
        cout << ": " << fn->counter << " (" << (fn->ascending ? "ke" : "turun-ke") << ")";
    }
    else if (node->nodeType == "ArrayType") {
        ArrayTypeNode* atn = static_cast<ArrayTypeNode*>(node);
        cout << "(elementType: '" << atn->elementType << "')";
    }
    
    cout << endl;
    
    string newPrefix = prefix + (isLast ? "    " : "│   ");
    
    if (node->nodeType == "Program") {
        ProgramNode* prog = static_cast<ProgramNode*>(node);
        size_t totalChildren = (prog->declarations ? 1 : 0) + (prog->block ? 1 : 0);
        size_t count = 0;
        
        if (prog->declarations) {
            count++;
            printAST(prog->declarations, newPrefix, count == totalChildren);
        }
        if (prog->block) {
            printAST(prog->block, newPrefix, true);
        }
    }
    else if (node->nodeType == "Declarations") {
        DeclarationsNode* decls = static_cast<DeclarationsNode*>(node);
        for (size_t i = 0; i < decls->declarations.size(); i++) {
            printAST(decls->declarations[i], newPrefix, i + 1 == decls->declarations.size());
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
        VarDeclNode* vd = static_cast<VarDeclNode*>(node);
        if (vd->arrayType) {
            printAST(vd->arrayType, newPrefix, true);
        }
    }
    else if (node->nodeType == "TypeDecl") {
        TypeDeclNode* td = static_cast<TypeDeclNode*>(node);
        if (td->arrayType) {
            printAST(td->arrayType, newPrefix, true);
        }
    }
    else if (node->nodeType == "ProcedureDecl") {
        ProcedureDeclNode* pd = static_cast<ProcedureDeclNode*>(node);
        if (pd->body) {
            printAST(pd->body, newPrefix, true);
        }
    }
    else if (node->nodeType == "FunctionDecl") {
        FunctionDeclNode* fd = static_cast<FunctionDeclNode*>(node);
        if (fd->body) {
            printAST(fd->body, newPrefix, true);
        }
    }
    else if (node->nodeType == "Assign") {
        AssignNode* an = static_cast<AssignNode*>(node);
        if (an->target) {
            cout << newPrefix << (!an->value ? "└── " : "├── ") << "target: ";
            if (an->target->nodeType == "Var") {
                cout << "Var(" << static_cast<VarNode*>(an->target)->name << ")" << endl;
            } else if (an->target->nodeType == "ArrayAccess") {
                cout << "ArrayAccess: " << static_cast<ArrayAccessNode*>(an->target)->arrayName << endl;
                if (static_cast<ArrayAccessNode*>(an->target)->index) {
                    printAST(static_cast<ArrayAccessNode*>(an->target)->index, newPrefix + (!an->value ? "    " : "│   "), true);
                }
            } else {
                cout << endl;
                printAST(an->target, newPrefix + (!an->value ? "    " : "│   "), true);
            }
        }
        if (an->value) {
            cout << newPrefix << "└── value: ";
            if (an->value->nodeType == "Number") {
                cout << "Number(" << static_cast<NumberNode*>(an->value)->value << ")" << endl;
            } else if (an->value->nodeType == "Real") {
                cout << "Real(" << static_cast<RealNode*>(an->value)->value << ")" << endl;
            } else if (an->value->nodeType == "String") {
                cout << "String(" << static_cast<StringNode*>(an->value)->value << "\")" << endl;
            } else if (an->value->nodeType == "Char") {
                cout << "Char('" << static_cast<CharNode*>(an->value)->value << "')" << endl;
            } else if (an->value->nodeType == "Boolean") {
                cout << "Boolean(" << (static_cast<BoolNode*>(an->value)->value ? "true" : "false") << ")" << endl;
            } else if (an->value->nodeType == "Var") {
                cout << "Var(" << static_cast<VarNode*>(an->value)->name << ")" << endl;
            } else {
                cout << endl;
                printAST(an->value, newPrefix + "    ", true);
            }
        }
    }
    else if (node->nodeType == "BinOp") {
        BinOpNode* bn = static_cast<BinOpNode*>(node);
        if (bn->left) {
            cout << newPrefix << (!bn->right ? "└── " : "├── ") << "left: ";
            if (bn->left->nodeType == "Number") {
                cout << "Number(" << static_cast<NumberNode*>(bn->left)->value << ")" << endl;
            } else if (bn->left->nodeType == "Var") {
                cout << "Var(" << static_cast<VarNode*>(bn->left)->name << ")" << endl;
            } else {
                cout << endl;
                printAST(bn->left, newPrefix + (!bn->right ? "    " : "│   "), true);
            }
        }
        if (bn->right) {
            cout << newPrefix << "└── right: ";
            if (bn->right->nodeType == "Number") {
                cout << "Number(" << static_cast<NumberNode*>(bn->right)->value << ")" << endl;
            } else if (bn->right->nodeType == "Var") {
                cout << "Var(" << static_cast<VarNode*>(bn->right)->name << ")" << endl;
            } else {
                cout << endl;
                printAST(bn->right, newPrefix + "    ", true);
            }
        }
    }
    else if (node->nodeType == "UnaryOp") {
        UnaryOpNode* un = static_cast<UnaryOpNode*>(node);
        if (un->operand) {
            cout << newPrefix << "└── operand: ";
            if (un->operand->nodeType == "Number") {
                cout << "Number(" << static_cast<NumberNode*>(un->operand)->value << ")" << endl;
            } else if (un->operand->nodeType == "Var") {
                cout << "Var(" << static_cast<VarNode*>(un->operand)->name << ")" << endl;
            } else {
                cout << endl;
                printAST(un->operand, newPrefix + "    ", true);
            }
        }
    }
    else if (node->nodeType == "ArrayAccess") {
        ArrayAccessNode* aan = static_cast<ArrayAccessNode*>(node);
        if (aan->index) {
            cout << newPrefix << "└── index: ";
            if (aan->index->nodeType == "Number") {
                cout << "Number(" << static_cast<NumberNode*>(aan->index)->value << ")" << endl;
            } else if (aan->index->nodeType == "Var") {
                cout << "Var(" << static_cast<VarNode*>(aan->index)->name << ")" << endl;
            } else {
                cout << endl;
                printAST(aan->index, newPrefix + "    ", true);
            }
        }
    }
    else if (node->nodeType == "If") {
        IfNode* ifn = static_cast<IfNode*>(node);
        size_t childCount = (ifn->condition ? 1 : 0) + (ifn->thenBranch ? 1 : 0) + (ifn->elseBranch ? 1 : 0);
        size_t count = 0;
        
        if (ifn->condition) {
            count++;
            cout << newPrefix << (count == childCount ? "└── " : "├── ") << "condition: " << endl;
            printAST(ifn->condition, newPrefix + (count == childCount ? "    " : "│   "), true);
        }
        if (ifn->thenBranch) {
            count++;
            cout << newPrefix << (count == childCount ? "└── " : "├── ") << "thenBranch: " << endl;
            printAST(ifn->thenBranch, newPrefix + (count == childCount ? "    " : "│   "), true);
        }
        if (ifn->elseBranch) {
            count++;
            cout << newPrefix << (count == childCount ? "└── " : "├── ") << "elseBranch: " << endl;
            printAST(ifn->elseBranch, newPrefix + (count == childCount ? "    " : "│   "), true);
        }
    }
    else if (node->nodeType == "While") {
        WhileNode* wn = static_cast<WhileNode*>(node);
        if (wn->condition) {
            cout << newPrefix << (!wn->body ? "└── " : "├── ") << "condition: " << endl;
            printAST(wn->condition, newPrefix + (!wn->body ? "    " : "│   "), true);
        }
        if (wn->body) {
            cout << newPrefix << "└── body: " << endl;
            printAST(wn->body, newPrefix + "    ", true);
        }
    }
    else if (node->nodeType == "For") {
        ForNode* fn = static_cast<ForNode*>(node);
        size_t childCount = (fn->start ? 1 : 0) + (fn->end ? 1 : 0) + (fn->body ? 1 : 0);
        size_t count = 0;
        
        if (fn->start) {
            count++;
            cout << newPrefix << (count == childCount ? "└── " : "├── ") << "start: " << endl;
            printAST(fn->start, newPrefix + (count == childCount ? "    " : "│   "), true);
        }
        if (fn->end) {
            count++;
            cout << newPrefix << (count == childCount ? "└── " : "├── ") << "end: " << endl;
            printAST(fn->end, newPrefix + (count == childCount ? "    " : "│   "), true);
        }
        if (fn->body) {
            count++;
            cout << newPrefix << (count == childCount ? "└── " : "├── ") << "body: " << endl;
            printAST(fn->body, newPrefix + (count == childCount ? "    " : "│   "), true);
        }
    }
    else if (node->nodeType == "ProcedureCall") {
        ProcedureCallNode* pc = static_cast<ProcedureCallNode*>(node);
        for (size_t i = 0; i < pc->args.size(); i++) {
            bool isLast = (i + 1 == pc->args.size());
            cout << newPrefix << (isLast ? "└── " : "├── ") << "arg: ";
            
            if (pc->args[i]->nodeType == "Number") {
                cout << "Number(" << static_cast<NumberNode*>(pc->args[i])->value << ")" << endl;
            } else if (pc->args[i]->nodeType == "Var") {
                cout << "Var(" << static_cast<VarNode*>(pc->args[i])->name << ")" << endl;
            } else if (pc->args[i]->nodeType == "String") {
                cout << "String(" << static_cast<StringNode*>(pc->args[i])->value << ")" << endl;
            } else if (pc->args[i]->nodeType == "Boolean") {
                cout << "Boolean(" << (static_cast<BoolNode*>(pc->args[i])->value ? "true" : "false") << ")" << endl;
            } else {
                cout << endl;
                printAST(pc->args[i], newPrefix + (isLast ? "    " : "│   "), true);
            }
        }
    }
    else if (node->nodeType == "ArrayType") {
        ArrayTypeNode* atn = static_cast<ArrayTypeNode*>(node);
        size_t childCount = (atn->rangeStart ? 1 : 0) + (atn->rangeEnd ? 1 : 0);
        size_t count = 0;
        
        if (atn->rangeStart) {
            count++;
            cout << newPrefix << (count == childCount ? "└── " : "├── ");
            cout << "startRange: ";
            if (atn->rangeStart->nodeType == "Number") {
                cout << static_cast<NumberNode*>(atn->rangeStart)->value << endl;
            } else {
                cout << endl;
                printAST(atn->rangeStart, newPrefix + (count == childCount ? "    " : "│   "), count == childCount);
            }
        }
        if (atn->rangeEnd) {
            count++;
            cout << newPrefix << (count == childCount ? "└── " : "├── ");
            cout << "endRange: ";
            if (atn->rangeEnd->nodeType == "Number") {
                cout << static_cast<NumberNode*>(atn->rangeEnd)->value << endl;
            } else {
                cout << endl;
                printAST(atn->rangeEnd, newPrefix + (count == childCount ? "    " : "│   "), count == childCount);
            }
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