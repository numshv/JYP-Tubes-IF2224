#ifndef TAB_HPP
#define TAB_HPP

#include <string>
#include <vector>
using namespace std;

enum ObjectClass {
    OBJ_PROGRAM = 0,
    OBJ_CONSTANT = 1,
    OBJ_VARIABLE = 2,
    OBJ_TYPE = 3,
    OBJ_PROCEDURE = 4,
    OBJ_FUNCTION = 5
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