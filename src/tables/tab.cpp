#include "tab.hpp"
#include "atab.hpp"
#include "btab.hpp"
#include <iostream>

vector<TabEntry> tab;

bool isDuplicateInCurrentBlock(const string& name){
    if(display.empty()) return false;

    int curLevel = currentLevel;
    if(curLevel < 0 || curLevel >= (int)display.size()) return false;

    int curBlock = display[curLevel];
    if(curBlock < 0 || curBlock >= (int)btab.size()) return false;

    int idx = btab[curBlock].last;
    while(idx!=0){
        if(tab[idx].name == name) return true;
        idx = tab[idx].link;
    }
    return false;
}

void initializeTab(){
    if(!tab.empty()) return;
    tab.reserve(256);

    // TODO: initialize reserve words
}

int insertIdentifier(const string& name, int obj, int type, int ref, int nrm, int adr){
    initializeTab();

    if(display.empty()){
        semanticError("insertIdentifier: no active display (no scope opened).");
        return 0;
    }
    int curLevel = currentLevel;
    if(curLevel < 0 || curLevel >= (int)display.size()){
        semanticError("insertIdentifier: invalid current level.");
        return 0;
    }
    int curBlock = display[curLevel];

    // cek duplikat
    int head = btab[curBlock].last;
    int scan = head;
    while(scan !=0){
        if(tab[scan].name == name){
            semanticError("Redeclaration of identifier '" + name + "' in the same block.");
            return scan;
        }
        scan = tab[scan].link;
    }

    // buat entry baru
    TabEntry e;
    e.name = name;
    e.link = head;
    e.obj = obj;
    e.type = type;
    e.ref = ref;
    e.nrm = nrm;
    e.lev = currentLevel;
    e.adr = adr;

    int newIndex = (int)tab.size();
    tab.push_back(e);

    btab[curBlock].last = newIndex;
    return newIndex;
}

int lookupIdentifier(const string& name){
    initializeTab();

    for(int lv = currentLevel; lv>=0; --lv){
        if(lv >= (int)display.size()) continue;
        int block = display[lv];
        if(block < 0 || block >= (int)btab.size()) continue;

        int idx = btab[block].last;
        while(idx!=0){
            if(tab[idx].name == name) return idx;
            idx = tab[idx].link;
        }
    }

    return 0;
}

void openScope(){
    int newBlock = createNewBlock();
    enterBlock(newBlock);
}

void closeScope(){
    exitBlock();
}

void printTab(){
    cout << "\n====================== TAB  =========================" << endl;
    cout << "identifiers\tlink\tobj\ttype\tref\tnrm\tlev\tadr" << endl;
    cout << "-------------------------------------------------------" << endl;
    
    for (size_t i = 0; i < tab.size(); i++) {
        cout << tab[i].name << "\t"
             << tab[i].link << "\t"
             << tab[i].obj << "\t"
             << tab[i].type << "\t"
             << tab[i].ref << "\t"
             << tab[i].nrm << "\t"
             << tab[i].lev << "\t"
             << tab[i].adr << endl;
    }
    
    cout << "==========================================\n" << endl;
}