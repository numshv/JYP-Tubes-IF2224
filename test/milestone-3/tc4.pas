program TesScope;
variabel
    globalA, globalB : integer;

prosedur hitung(paramX : integer);
variabel
    lokalA : integer;
mulai
    lokalA := paramX + 10;
    globalA := lokalA;
selesai;

mulai
    globalA := 0;
    globalB := 5;
    
    hitung(globalB);
selesai.