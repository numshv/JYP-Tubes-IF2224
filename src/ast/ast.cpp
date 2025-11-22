#include "ast.hpp"
#include <iostream>
#include <sstream>

using namespace std;

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

// Forward declaration
ASTNode* convert(ParseNode* p);

// EXPRESSION BUILDERS
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

    // STRINGL ITERAL
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
        
        // Check for array access: IDENTIFIER [ expr ]
        if (p->children.size() >= 4 && isToken(p->children[1], "LBRACKET")) {
            ASTNode* indexExpr = convert(p->children[2]);
            return new ArrayAccessNode(name, indexExpr);
        }
        
        // Check for function call
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
    
    // Check for unary op at the beginning
    if (has(p->children[0], "ARITHMETIC_OPERATOR")) {
        string op = getTokenText(p->children[0]);
        ASTNode* operand = convert(p->children[1]);
        ASTNode* node = new UnaryOpNode(op, operand);
        idx = 2;
        
        // Continue with binary operations
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
    
    // No unary operator, start with first term
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

// MAIN CONVERTER
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
        
        // Parse parameters if exists
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
        
        // Parse parameters if exists
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
                // Parse declarations within block
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
        
        // Find statement-list
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

    // EXPRESSIONS
    if (has(p, "<expression>")) return buildExpression(p);
    if (has(p, "<simple-expression>")) return buildSimpleExpression(p);
    if (has(p, "<term>")) return buildTerm(p);
    if (has(p, "<factor>")) return buildFactor(p);

    // STATEMENT (wrapper)
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