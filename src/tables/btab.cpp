#include "btab.hpp"
#include <iostream>
using namespace std;

// Inisialisasi global variables
vector<BtabEntry> btab;
vector<int> display;
int currentLevel = 0;

// Inisialisasi btab dengan block global (indeks 0)
void initializeBtab() {
    btab.clear();
    display.clear();
    
    // Buat block global (indeks 0)
    BtabEntry globalBlock;
    globalBlock.last = 0;  // Akan diupdate saat ada deklarasi
    globalBlock.lpar = 0;  // Global block tidak punya parameter
    globalBlock.psze = 0;  // Tidak ada parameter
    globalBlock.vsze = 0;  // Akan diupdate saat ada variabel global
    
    btab.push_back(globalBlock);
    
    // Inisialisasi display untuk level 0
    display.push_back(0);  // display[0] = block index 0 (global)
    currentLevel = 0;
    
    cout << "BTAB initialized with global block at index 0" << endl;
}

// Membuat block baru dan return indeksnya
int createNewBlock() {
    BtabEntry newBlock;
    newBlock.last = 0;
    newBlock.lpar = 0;
    newBlock.psze = 0;
    newBlock.vsze = 0;
    
    btab.push_back(newBlock);
    int newIndex = btab.size() - 1;
    
    cout << "Created new block at index " << newIndex << endl;
    return newIndex;
}

// Masuk ke block baru (increment level)
void enterBlock(int blockIndex) {
    currentLevel++;
    
    // Pastikan display vector cukup besar
    if ((int)display.size() <= currentLevel) {
        display.resize(currentLevel + 1);
    }
    
    display[currentLevel] = blockIndex;
    
    cout << "Entered block " << blockIndex << " at level " << currentLevel << endl;
}

// Keluar dari block (decrement level)
void exitBlock() {
    if (currentLevel > 0) {
        cout << "Exited block " << display[currentLevel] 
             << " from level " << currentLevel << endl;
        currentLevel--;
    } else {
        cerr << "Warning: Cannot exit from global level" << endl;
    }
}

// Mendapatkan indeks block saat ini
int getCurrentBlock() {
    if (currentLevel >= 0 && currentLevel < (int)display.size()) {
        return display[currentLevel];
    }
    return 0; // Default ke global block
}

// Update last identifier di block
void updateBlockLast(int blockIndex, int lastIdentifier) {
    if (blockIndex >= 0 && blockIndex < (int)btab.size()) {
        btab[blockIndex].last = lastIdentifier;
        cout << "Updated btab[" << blockIndex << "].last = " 
             << lastIdentifier << endl;
    } else {
        cerr << "Error: Invalid block index " << blockIndex << endl;
    }
}

// Update last parameter di block
void updateBlockLpar(int blockIndex, int lastParameter) {
    if (blockIndex >= 0 && blockIndex < (int)btab.size()) {
        btab[blockIndex].lpar = lastParameter;
        cout << "Updated btab[" << blockIndex << "].lpar = " 
             << lastParameter << endl;
    } else {
        cerr << "Error: Invalid block index " << blockIndex << endl;
    }
}

// Update total ukuran parameter
void updateBlockPsze(int blockIndex, int parameterSize) {
    if (blockIndex >= 0 && blockIndex < (int)btab.size()) {
        btab[blockIndex].psze = parameterSize;
        cout << "Updated btab[" << blockIndex << "].psze = " 
             << parameterSize << endl;
    } else {
        cerr << "Error: Invalid block index " << blockIndex << endl;
    }
}

// Update total ukuran variabel
void updateBlockVsze(int blockIndex, int variableSize) {
    if (blockIndex >= 0 && blockIndex < (int)btab.size()) {
        btab[blockIndex].vsze = variableSize;
        cout << "Updated btab[" << blockIndex << "].vsze = " 
             << variableSize << endl;
    } else {
        cerr << "Error: Invalid block index " << blockIndex << endl;
    }
}

// Increment ukuran variabel (untuk deklarasi variabel tambahan)
void incrementBlockVsze(int blockIndex, int additionalSize) {
    if (blockIndex >= 0 && blockIndex < (int)btab.size()) {
        btab[blockIndex].vsze += additionalSize;
        cout << "Incremented btab[" << blockIndex << "].vsze by " 
             << additionalSize << " (now = " << btab[blockIndex].vsze << ")" << endl;
    } else {
        cerr << "Error: Invalid block index " << blockIndex << endl;
    }
}

// Increment ukuran parameter (untuk deklarasi parameter tambahan)
void incrementBlockPsze(int blockIndex, int additionalSize) {
    if (blockIndex >= 0 && blockIndex < (int)btab.size()) {
        btab[blockIndex].psze += additionalSize;
        cout << "Incremented btab[" << blockIndex << "].psze by " 
             << additionalSize << " (now = " << btab[blockIndex].psze << ")" << endl;
    } else {
        cerr << "Error: Invalid block index " << blockIndex << endl;
    }
}

// Print seluruh btab untuk debugging
void printBtab() {
    cout << "\n========== BTAB (Block Table) ==========" << endl;
    cout << "idx\tlast\tlpar\tpsze\tvsze" << endl;
    cout << "---------------------------------------" << endl;
    
    for (size_t i = 0; i < btab.size(); i++) {
        cout << i << "\t"
             << btab[i].last << "\t"
             << btab[i].lpar << "\t"
             << btab[i].psze << "\t"
             << btab[i].vsze << endl;
    }
    
    cout << "========================================" << endl;
    cout << "Current Level: " << currentLevel << endl;
    cout << "Display Stack: ";
    for (size_t i = 0; i < display.size(); i++) {
        cout << "L" << i << "->B" << display[i] << " ";
    }
    cout << "\n========================================\n" << endl;
}

// Debug info untuk block tertentu
void debugBlockInfo(int btab_index) {
    if (btab_index < 0 || btab_index >= (int)btab.size()) {
        cout << "Invalid btab index: " << btab_index << endl;
        return;
    }
    
    BtabEntry e = btab[btab_index];
    cout << "=== Block Info (btab[" << btab_index << "]) ===" << endl;
    cout << "Last identifier: " << e.last << endl;
    cout << "Last parameter: " << e.lpar << endl;
    cout << "Parameter size: " << e.psze << " bytes" << endl;
    cout << "Variable size: " << e.vsze << " bytes" << endl;
    cout << "=======================================" << endl;
}