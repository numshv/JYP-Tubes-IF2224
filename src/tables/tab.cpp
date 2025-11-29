#include "tab.hpp"
#include "atab.hpp"
#include "btab.hpp"
#include <iostream>

vector<TabEntry> tab;

void initializeTab(){
    if(!tab.empty()) return;
    tab.reserve(256);

    vector<string> keywords = {
        "dan",          // AND
        "larik",        // ARRAY
        "mulai",        // BEGIN
        "kasus",        // CASE
        "konstanta",    // CONST
        "bagi",         // DIV
        "turun-ke",     // DOWNTO
        "lakukan",      // DO
        "selain-itu",   // ELSE
        "selesai",      // END
        "untuk",        // FOR
        "fungsi",       // FUNCTION
        "jika",         // IF
        "mod",          // MOD
        "tidak",        // NOT
        "dari",         // OF
        "atau",         // OR
        "prosedur",     // PROCEDURE
        "program",      // PROGRAM
        "rekaman",      // RECORD
        "ulangi",       // REPEAT
        "string",       // STRING
        "maka",         // THEN
        "ke",           // TO
        "tipe",         // TYPE
        "sampai",       // UNTIL
        "variabel",     // VAR
        "selama",       // WHILE
        "packed"        // PACKED
    };
    
    for (const string& kw : keywords) {
        TabEntry e;
        e.name = kw;
        e.link = 0;
        e.obj = 0;  
        e.type = 0;
        e.ref = 0;
        e.nrm = 1;  
        e.lev = 0;
        e.adr = 0;
        e.initialized = false;
        tab.push_back(e);
    }
}

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
    e.initialized = (obj == OBJ_CONSTANT);

    int newIndex = (int)tab.size();
    tab.push_back(e);

    btab[curBlock].last = newIndex;
    return newIndex;
}

int lookupIdentifier(const string& name){
    initializeTab();

    // reserved words
    for(size_t i = 0; i < 29 && i < tab.size(); i++){
        if(tab[i].name == name){
            return i;  
        }
    }

    // user-defined
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

    // predefined procedures
    if(name == "writeln" || name == "write" || name == "readln" || name == "read"){
        for(size_t i = 29; i < tab.size(); i++){
            if(tab[i].name == name && tab[i].nrm == 1 && tab[i].obj == OBJ_PROCEDURE){
                return i;  
            }
        }
        
        TabEntry e;
        e.name = name;
        e.link = 0;
        e.obj = OBJ_PROCEDURE;
        e.type = 0;
        e.ref = 0;
        e.nrm = 1; 
        e.lev = 0;
        e.adr = 0;
        e.initialized = false;
        tab.push_back(e);
        return tab.size() - 1;
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
    cout << "\n================================ TAB (Symbol Table) =====================================" << endl;
    cout << "idx\tid\t\t\tobj\ttype\tref\tnrm\tlev\tadr\tlink" << endl;
    cout << "-----------------------------------------------------------------------------------------" << endl;
    
    for (size_t i = 0; i < tab.size(); i++) {
        cout << i << "\t";
        
        string name = tab[i].name;
        if (name.length() < 16) {
            cout << name;
            for (size_t j = name.length(); j < 16; j++) cout << " ";
        } else {
            cout << name.substr(0, 15) << ".";
        }
        
        cout << "\t"
             << tab[i].obj << "\t"
             << tab[i].type << "\t"
             << tab[i].ref << "\t"
             << tab[i].nrm << "\t"
             << tab[i].lev << "\t"
             << tab[i].adr << "\t"
             << tab[i].link << endl;
    }
    
    cout << "=========================================================================================\n" << endl;
}

void debugTabEntry(int index) {
    if (index < 0 || index >= (int)tab.size()) {
        cout << "Invalid tab index: " << index << endl;
        return;
    }
    
    TabEntry e = tab[index];
    cout << "=== Tab Entry Info (tab[" << index << "]) ===" << endl;
    cout << "Name: " << e.name << endl;
    cout << "Link: " << e.link << endl;
    cout << "Object: " << e.obj << endl;
    cout << "Type: " << e.type << endl;
    cout << "Reference: " << e.ref << endl;
    cout << "Normal param: " << e.nrm << endl;
    cout << "Level: " << e.lev << endl;
    cout << "Address: " << e.adr << endl;
    cout << "=======================================" << endl;
}