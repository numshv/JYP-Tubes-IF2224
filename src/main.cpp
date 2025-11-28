#include "header/lexer.hpp"
#include "header/parser.hpp"
#include "header/ast.hpp"  
#include "header/atab.hpp"
#include "header/btab.hpp"

void visitAST(ASTNode* node, int currentBlockIndex = 0) {
    if (!node) return;
    
    // Jika ketemu Program Node - initialize btab
    if (node->nodeType == "Program") {
        ProgramNode* prog = static_cast<ProgramNode*>(node);
        
        cout << "\n>>> Found Program: " << prog->name << endl;
        
        // Block global sudah diinit, update nama program jika perlu
        cout << ">>> Using global block (btab[0])" << endl;
        
        // Process declarations di level global
        for (auto decl : prog->declarations) {
            visitAST(decl, 0);  // 0 = global block
        }
        
        // Process main block
        if (prog->block) {
            // Buat block baru untuk compound statement utama
            int mainBlockIndex = createNewBlock();
            enterBlock(mainBlockIndex);
            
            visitAST(prog->block, mainBlockIndex);
            
            exitBlock();
        }
        
        return; 
    }
    
    // Jika ketemu Block Node
    if (node->nodeType == "Block") {
        BlockNode* block = static_cast<BlockNode*>(node);
        
        cout << ">>> Processing Block at btab[" << currentBlockIndex << "]" << endl;
        
        // Process declarations dalam block ini
        for (auto decl : block->declarations) {
            visitAST(decl, currentBlockIndex);
        }
        
        // Process statements
        for (auto stmt : block->statements) {
            visitAST(stmt, currentBlockIndex);
        }
        
        return; 
    }
    
    // Jika ketemu VarDeclNode
    if (node->nodeType == "VarDecl") {
        VarDeclNode* varDecl = static_cast<VarDeclNode*>(node);
        
        cout << ">>> Found VarDecl in block " << currentBlockIndex << ": ";
        for (const auto& name : varDecl->names) {
            cout << name << " ";
        }
        cout << "of type: " << varDecl->typeName << endl;
        
        // Hitung ukuran variabel untuk update btab
        int varCount = varDecl->names.size();
        int varSize = 0;
        
        // Cek apakah ada array type dalam children
        bool isArray = false;
        for (auto child : varDecl->children) {
            if (child->nodeType == "ArrayType") {
                ArrayTypeNode* arrayType = static_cast<ArrayTypeNode*>(child);
                
                cout << "  >>> Processing ArrayType..." << endl;
                
                // Process dan simpan ke atab
                int atab_index = processArrayDeclaration(arrayType);
                
                if (atab_index >= 0) {
                    child->symbolIndex = atab_index;
                    varSize = atab[atab_index].size;
                    isArray = true;
                    
                    cout << "  >>> Array declared with atab index: " << atab_index << endl;
                    cout << "  >>> Array size: " << varSize << " units" << endl;
                } else {
                    cout << "  >>> Failed to create atab entry" << endl;
                }
            }
        }
        
        // Jika bukan array, hitung ukuran berdasarkan tipe dasar
        if (!isArray) {
            int typeCode = getTypeCode(varDecl->typeName);
            int singleVarSize = getTypeSize(typeCode);
            varSize = singleVarSize * varCount;
        }
        
        // Update vsze di btab untuk block saat ini
        incrementBlockVsze(currentBlockIndex, varSize);
        
        // Update last identifier (simplified - dalam implementasi lengkap, 
        // ini harus menunjuk ke tab entry)
        // updateBlockLast(currentBlockIndex, tabIndex);
        
        return;
    }
    
    // Jika ketemu ProcedureDecl atau FunctionDecl
    if (node->nodeType == "ProcedureDecl" || node->nodeType == "FunctionDecl") {
        cout << ">>> Found " << node->nodeType << endl;
        
        // Buat block baru untuk procedure/function
        int procBlockIndex = createNewBlock();
        enterBlock(procBlockIndex);
        
        // TODO: Process parameters dan update lpar, psze
        // Contoh sederhana:
        // for (auto param : proc->parameters) {
        //     int paramSize = calculateParamSize(param);
        //     incrementBlockPsze(procBlockIndex, paramSize);
        // }
        
        // Process body
        for (auto child : node->children) {
            visitAST(child, procBlockIndex);
        }
        
        exitBlock();
        return;
    }
    
    // BinOp Node
    if (node->nodeType == "BinOp") {
        BinOpNode* binOp = static_cast<BinOpNode*>(node);
        visitAST(binOp->left, currentBlockIndex);
        visitAST(binOp->right, currentBlockIndex);
        return;
    }
    
    // Assign Node
    if (node->nodeType == "Assign") {
        AssignNode* assign = static_cast<AssignNode*>(node);
        visitAST(assign->target, currentBlockIndex);
        visitAST(assign->value, currentBlockIndex);
        return;
    }
    
    // Default: traverse children
    for (auto child : node->children) {
        visitAST(child, currentBlockIndex);
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
        cout << "\n========== SEMANTIC ANALYSIS ==========\n";
        
        // Initialize symbol tables
        atab.clear();
        initializeBtab();  // Initialize btab dengan global block
        
        cout << "\n>>> Starting AST traversal...\n" << endl;
        
        // Visit AST untuk populate atab dan btab
        visitAST(ast, 0);  // Start dari block 0 (global)
        
        // Print hasil
        cout << "\n========== SYMBOL TABLES ==========\n";
        
        if (!atab.empty()) {
            printAtab();
        } else {
            cout << "No arrays declared in this program.\n";
        }
        
        printBtab();
        
        // Debug info untuk block tertentu jika diperlukan
        if (btab.size() > 0) {
            cout << "\n>>> Detailed info for global block:" << endl;
            debugBlockInfo(0);
        }
        
    } catch (const exception& e) {
        cerr << "\nSemantic analysis error: " << e.what() << endl;
        return 1;
    }

    cout << "\n========== SEMANTIC ANALYSIS COMPLETE ==========\n";
    return 0;
}