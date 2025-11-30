#include "atab.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>
using namespace std;

vector<AtabEntry> atab;

void semanticError(const string& message) {
    // cerr << "Semantic Error: " << message << endl;
    throw runtime_error(message);
}

void semanticWarning(const string& message) {
    cerr << "Semantic Warning: " << message << endl;
}

int determineIndexType(ASTNode* node) {
    if (!node) return 0;

    if (node->nodeType == "Number") return 1;
    if (node->nodeType == "Real") return 2;
    if (node->nodeType == "Boolean") return 3; 
    if (node->nodeType == "Var") {
        VarNode* v = static_cast<VarNode*>(node);
        if (v->name == "true" || v->name == "false") return 3; // Boolean
    }
    if (node->nodeType == "Char") return 4;    
    
    return 0; 
}

int getOrdinalValue(ASTNode* node) {
    if (!node) return 0;

    if (node->nodeType == "Number") {
        return static_cast<NumberNode*>(node)->value;
    }
    else if (node->nodeType == "Real") {
        RealNode* realNode = static_cast<RealNode*>(node);
        return static_cast<int>(realNode->value); 
    }       
    else if (node->nodeType == "Char") {
        CharNode* charNode = static_cast<CharNode*>(node);
        return (int)charNode->value; 
    }
    else if (node->nodeType == "Boolean") {
        BoolNode* boolNode = static_cast<BoolNode*>(node);
        return boolNode->value ? 1 : 0;
    }
    else if (node->nodeType == "Var") {
        VarNode* varNode = static_cast<VarNode*>(node);
        if (varNode->name == "true") return 1;
        if (varNode->name == "false") return 0;
    }

    semanticError("Cannot determine ordinal value for node type: " + node->nodeType);
    return 0;
}

int getTypeCode(const string& typeName) {
    static unordered_map<string, int> typeMap = {
        {"integer", 1},
        {"real", 2},
        {"boolean", 3},
        {"char", 4},
        {"array", 5}
    };
    
    auto it = typeMap.find(typeName);
    if (it != typeMap.end()) {
        return it->second;
    }
    return 0;
}

int getTypeSize(int typeCode) {
    switch(typeCode) {
        case 1: return 1; // integer
        case 2: return 1; // real
        case 3: return 1; // boolean
        case 4: return 1; // char
        default: return 0;
    }
}

int getLowerBound(ASTNode* rangeStart) {
    if (!rangeStart) {
        semanticError("Range start is null");
        return 0;
    }
    
    if (rangeStart->nodeType == "Number") {
        NumberNode* numNode = static_cast<NumberNode*>(rangeStart);
        return numNode->value;
    }
    
    semanticError("Lower bound must be a number");
    return 0;
}

int getUpperBound(ASTNode* rangeEnd) {
    if (!rangeEnd) {
        semanticError("Range end is null");
        return 0;
    }
    
    if (rangeEnd->nodeType == "Number") {
        NumberNode* numNode = static_cast<NumberNode*>(rangeEnd);
        return numNode->value;
    }
    
    semanticError("Upper bound must be a number");
    return 0;
}

int processArrayDeclaration(ArrayTypeNode* arrayTypeNode) {
    if (!arrayTypeNode) {
        semanticError("Array type node is null");
        return -1;
    }
    
    int startType = determineIndexType(arrayTypeNode->rangeStart);
    int endType = determineIndexType(arrayTypeNode->rangeEnd);

    if (startType == 0 || endType == 0) {
        semanticError("Invalid index type. Array index must be Integer, Char, or Boolean.");
        return -1;
    }

    if (startType != endType) {
        semanticError("Array range type mismatch. Start is type " + to_string(startType) + 
                      ", End is type " + to_string(endType));
        return -1;
    }

    AtabEntry entry;
    
    entry.xtyp = startType; 

    entry.low = getOrdinalValue(arrayTypeNode->rangeStart);
    entry.high = getOrdinalValue(arrayTypeNode->rangeEnd);
    
    if (entry.low > entry.high) {
        semanticError("Array lower bound (" + to_string(entry.low) + 
                      ") > upper bound (" + to_string(entry.high) + ")");
        return -1;
    }
    
    entry.etyp = getTypeCode(arrayTypeNode->elementType);
    if (entry.etyp == 0) {
        semanticError("Invalid array element type: " + arrayTypeNode->elementType);
        return -1;
    }
    
    entry.elsz = getTypeSize(entry.etyp);
    entry.size = (entry.high - entry.low + 1) * entry.elsz;
    entry.eref = 0; 
    
    // TODO: nested loop arrays handling
    
    atab.push_back(entry);
    
    return atab.size() - 1; 
}

void printAtab() {
    cout << "\n================== ATAB (Array Table) ========================" << endl;
    cout << "idx\txtyp\tetyp\teref\tlow\thigh\telsz\tsize" << endl;
    cout << "--------------------------------------------------------------" << endl;
    
    for (size_t i = 0; i < atab.size(); i++) {
        cout << i << "\t"
             << atab[i].xtyp << "\t"
             << atab[i].etyp << "\t"
             << atab[i].eref << "\t"
             << atab[i].low << "\t"
             << atab[i].high << "\t"
             << atab[i].elsz << "\t"
             << atab[i].size << endl;
    }
    
    cout << "==============================================================\n" << endl;
}

void debugArrayInfo(int atab_index) {
    if (atab_index < 0 || atab_index >= (int)atab.size()) {
        cout << "Invalid atab index: " << atab_index << endl;
        return;
    }
    
    AtabEntry e = atab[atab_index];
    cout << "=== Array Info (atab[" << atab_index << "]) ===" << endl;
    cout << "Index type: " << e.xtyp << endl;
    cout << "Element type: " << e.etyp << endl;
    cout << "Range: [" << e.low << ".." << e.high << "]" << endl;
    cout << "Element size: " << e.elsz << endl;
    cout << "Total size: " << e.size << endl;
    cout << "Element ref: " << e.eref << endl;
}

