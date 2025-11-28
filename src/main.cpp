#include "header/lexer.hpp"
#include "header/parser.hpp"
#include "header/ast.hpp"  
#include "header/atab.hpp"

void visitAST(ASTNode* node) {
    if (!node) return;
    

    // Jika ketemu VarDeclNode
    if (node->nodeType == "VarDecl") {
        VarDeclNode* varDecl = static_cast<VarDeclNode*>(node);
        
        cout << ">>> Found VarDecl: ";
        for (const auto& name : varDecl->names) {
            cout << name << " ";
        }
        cout << "of type: " << varDecl->typeName << endl;
        
        // Cek apakah ada array type dalam children
        for (auto child : varDecl->children) {
            if (child->nodeType == "ArrayType") {
                ArrayTypeNode* arrayType = static_cast<ArrayTypeNode*>(child);
                
                cout << "  >>> Processing ArrayType..." << endl;
                
                // Process dan simpan ke atab
                int atab_index = processArrayDeclaration(arrayType);
                
                if (atab_index >= 0) {
                    child->symbolIndex = atab_index;
                    
                    cout << "  >>> Array declared with atab index: " << atab_index << endl;
                } else {
                    cout << "  >>> Failed to create atab entry" << endl;
                }
            }
        }
    }
    
    if (node->nodeType == "Program") {
        ProgramNode* prog = static_cast<ProgramNode*>(node);
        
        for (auto decl : prog->declarations) {
            visitAST(decl);
        }
        
        if (prog->block) {
            visitAST(prog->block);
        }
        
        return; 
    }
    
    if (node->nodeType == "Block") {
        BlockNode* block = static_cast<BlockNode*>(node);
        
        for (auto decl : block->declarations) {
            visitAST(decl);
        }
        
        for (auto stmt : block->statements) {
            visitAST(stmt);
        }
        
        return; 
    }
    
    if (node->nodeType == "BinOp") {
        BinOpNode* binOp = static_cast<BinOpNode*>(node);
        visitAST(binOp->left);
        visitAST(binOp->right);
        return;
    }
    
    if (node->nodeType == "Assign") {
        AssignNode* assign = static_cast<AssignNode*>(node);
        visitAST(assign->target);
        visitAST(assign->value);
        return;
    }
    
    for (auto child : node->children) {
        visitAST(child);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <source_file.pas>\n";
        return 1;
    }

    const string ruleFile = "test/milestone-1/rule.json";
    ifstream jfile(ruleFile);
    if (!jfile) {
        cerr << "Error: cannot open " << ruleFile << "\n";
        return 1;
    }
    json rules;
    jfile >> rules;

    unordered_set<string> keywords;
    unordered_set<string> logical_ops;
    unordered_set<string> arith_word_ops;
    for (auto &kw : rules["keyword_lookup"]["keywords"]) keywords.insert(kw);
    for (auto &kw : rules["keyword_lookup"]["logical_operators"]) logical_ops.insert(kw);
    for (auto &kw : rules["keyword_lookup"]["arithmetic_word_operators"]) arith_word_ops.insert(kw);

    // Read Pascal 
    ifstream f(argv[1]);
    if (!f) { cerr << "Cannot open file\n"; return 1; }
    stringstream buf; buf << f.rdbuf(); string input = buf.str();

    // Run DFA
    cout << "\n========== Generated Token ==========\n";
    vector<Token> toks = runDFA(input, rules, keywords, logical_ops, arith_word_ops);

    bool hasError = false;
    for (auto &t : toks) {
        if (t.type == "ERROR") {
            hasError = true;
            break;
        }
    }

    if (!hasError) {
        for (auto &t : toks) {
            std::cerr << "<" << t.type << "(" << t.lexeme << ")>\n";
        }
    }

    ParseNode* parseTree = parser_main(toks);
    ASTNode* ast = ASTMain(parseTree);

    try {
        atab.clear();
        
        // Visit AST untuk populate atab
        visitAST(ast);
        
        // Print hasil
        if (!atab.empty()) {
            printAtab();
        } else {
            cout << "No arrays declared in this program.\n";
        }
        
    } catch (const exception& e) {
        cerr << "Semantic analysis error: " << e.what() << endl;
        return 1;
    }

    return 0;
}