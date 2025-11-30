#ifndef BTAB_HPP
#define BTAB_HPP

#include <vector>
#include <string>
using namespace std;

// Struktur btab (Block Table)
// Digunakan untuk menyimpan informasi blok prosedur dan definisi tipe record
struct BtabEntry {
    int last;  // Pointer/indeks ke identifier terakhir yang dideklarasikan di dalam block
    int lpar;  // Pointer/indeks ke parameter terakhir (0 jika record)
    int psze;  // Total ukuran parameter block (dalam byte/unit memori)
    int vsze;  // Total ukuran variabel lokal block (dalam byte/unit memori)
};

// Global btab vector
extern vector<BtabEntry> btab;

// Display stack untuk melacak block pada setiap level
extern vector<int> display;

// Current lexical level
extern int currentLevel;

// Fungsi-fungsi untuk manajemen btab
void initializeBtab();
int createNewBlock();
void enterBlock(int blockIndex);
void exitBlock();
int getCurrentBlock();
void updateBlockLast(int blockIndex, int lastIdentifier);
void updateBlockLpar(int blockIndex, int lastParameter);
void updateBlockPsze(int blockIndex, int parameterSize);
void updateBlockVsze(int blockIndex, int variableSize);
void incrementBlockVsze(int blockIndex, int additionalSize);
void incrementBlockPsze(int blockIndex, int additionalSize);

// Fungsi untuk debugging dan output
void printBtab();
void debugBlockInfo(int btab_index);

#endif