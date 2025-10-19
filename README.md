<br/>
<h1 align="center">PASCAL-S Compiler 🧑‍💻 </h1>

<br/>

> Tugas Besar IF2224 - Teori Bahasa Formal dan Automata
> By Kelompok JagoYapping (JYP) - K02 - IF'23

Anggota kelompok:

| NIM | Nama |
| :--- | :---: |
| 13523058 | Noumisyifa Nabila Nareswari |
| 13523066 | M. Ghifary Komara Putra |
| 13523072 | Sabilul Huda |
| 13523080 | Diyah Susan Nugrahani |
| 13523108 | Henry Filberto Shinelo |

<br/>

## Deskripsi Program

Program ini adalah implementasi compiler untuk bahasa Pascal-S, sebuah subset bahasa dari bahasa pemrograman Pascal. Compiler ini diimplementasikan dengan berbagai teori yang diperkenalkan di mata kuliah Teori Bahasa Formal dan Automata. 

Compiler ini dirancang untuk melakukan proses kompilasi lengkap dari source code Pascal-S menjadi executable code, melalui beberapa tahapan:

1. Lexical Analysis
2. Syntax Analysis [TBA]
3. Semantic Analysis [TBA]
4. Intermediate Code Generation [TBA]
5. Interpreter [TBA]
<br/>

## Requirements
- C++ Compiler: g++ dengan support C++17 or above
- Make: GNU Make untuk build automation
- Operating System: Linux/WSL (Windows Subsystem for Linux) atau Unix-based OS
<br/>

## Cara Instalasi dan Penggunaan
### Instalasi

1. Clone repository

``` bash   
git clone https://github.com/[username]/JYP-Tubes-IF2224.git cd JYP-Tubes-IF2224
```

2. Install dependencies (jika diperlukan)

``` bash   
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install g++ make nlohmann-json3-dev

# macOS
brew install gcc make nlohmann-json
```

3. Compile program

``` bash
make
```

Program akan dikompilasi dan executable akan tersimpan di folder bin/.

### Penggunaan
**[Milestone 1]** Menjalankan Lexical Analyzer
``` bash
# Format
make run ARGS=<path_to_pascal_file>

# Contoh
make run ARGS=test/milestone-1/tc2.pas
```

## Progress Update per Milestone


### Milestone 1 (Lexer)

#### Deskripsi Komponen Compiler yang Dibangun

Pada milestone ini, kami membangun bagian lexer, bagian dari compiler yang paling dasar dan merupakan aktor dari fase pertama proses kompilasi. Komponen ini mengubah source code Pascal-S dari rangkaian karakter mentah menjadi sequence of tokens yang bermakna. Rule yang dibangun untuk aturan tokenisasi dibangun dengan model automaton DFA. Semua rule DFA untuk tokenisasi disimpan di dalam file JSON yang akan diload oleh lexer.

Secara umum, berikut adalah proses yang terjadi dalam lexer:
1. Membaca file aturan DFA (rule.json)
2. Membangun character classification map
3. Membangun transition table dari aturan DFA
4. Membaca source code Pascal-S karakter per karakter
5. Melakukan state transition berdasarkan input
6. Mengenali dan menghasilkan token saat mencapai final state
7. Mendeteksi error jika tidak ada transisi invalid


#### Pembagian Tugas

| NIM | Nama | Tugas | persentase kontribusi |
| :--- | :---: | :---: | ---: |
| 13523058 | Noumisyifa Nabila Nareswari | - Membantu dalam menyusun DFA<br/> - Membuat laporan | 20% |
| 13523066 | M. Ghifary Komara Putra | - Mengerjakan implementasi kode bagian engine DFA | 20% |
| 13523072 | Sabilul Huda | - Memimpin penyusunan rule DFA dan graft | 20% |
| 13523080 | Diyah Susan Nugrahani | - Memimpin penyusunan rule DFA dan graft | 20% |
| 13523108 | Henry Filberto Shinelo | - Mengerjakan implementasi kode bagian engine DFA | 20% |
