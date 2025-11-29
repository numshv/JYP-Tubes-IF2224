#include "atab.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>
using namespace std;

vector<AtabEntry> atab;

void semanticError(const string& message) {
    cerr << "Semantic Error: " << message << endl;
    throw runtime_error(message);
}

void semanticWarning(const string& message) {
    cerr << "Semantic Warning: " << message << endl;
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
    
    AtabEntry entry;
    
    entry.low = getLowerBound(arrayTypeNode->rangeStart);
    entry.high = getUpperBound(arrayTypeNode->rangeEnd);
    
    if (entry.low > entry.high) {
        semanticError("Array lower bound (" + to_string(entry.low) + 
                     ") > upper bound (" + to_string(entry.high) + ")");
        return -1;
    }
    
    // tipe indeks 
    entry.xtyp = 1; // integer
    
    // tipe elemen
    entry.etyp = getTypeCode(arrayTypeNode->elementType);
    
    if (entry.etyp == 0) {
        semanticError("Invalid array element type: " + arrayTypeNode->elementType);
        return -1;
    }
    
    // hitung ukuran
    entry.elsz = getTypeSize(entry.etyp);
    entry.size = (entry.high - entry.low + 1) * entry.elsz;
    entry.eref = 0; 
    
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

