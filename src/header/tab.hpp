#ifndef TAB_HPP
#define TAB_HPP

#include <string>
#include <vector>
using namespace std;

enum ObjectClass {
    OBJ_CONSTANT,
    OBJ_VARIABLE,
    OBJ_TYPE,
    OBJ_PROCEDURE,
    OBJ_FUNCTION
};

struct TabEntry {
    string name;
    int link;
    int obj;
    int type;
    int ref;
    int nrm;
    int lev;
    int adr;
    bool initialized;
};

extern vector<TabEntry> tab;

void initializeTab();
bool isDuplicateInCurrentBlock(const string& name);

int insertIdentifier(const string& name, int obj, int type, int ref, int nrm, int adr);
int lookupIdentifier(const string& name);

void openScope();
void closeScope();

void printTab();
void debugTabEntry(int index);


#endif