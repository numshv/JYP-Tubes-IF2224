#ifndef ATAB_HPP
#define ATAB_HPP

#include "ast.hpp"  // ← Ganti dari parser.hpp
#include <vector>
#include <string>
#include <unordered_map>
using namespace std;


// struktur atab
struct AtabEntry {
    int xtyp;  
    int etyp;  
    int eref;  
    int low;   
    int high;  
    int elsz;  
    int size;  
};

extern vector<AtabEntry> atab;

void semanticError(const string& message);
void semanticWarning(const string& message);
string getTypeName(int typeCode);

int getTypeCode(const string& typeName);
int getTypeSize(int typeCode);
int getLowerBound(ASTNode* rangeStart);
int getUpperBound(ASTNode* rangeEnd);
int processArrayDeclaration(ArrayTypeNode* arrayTypeNode);
void printAtab();
void debugArrayInfo(int atab_index);

#endif