program ShadowTest;
variabel
    data: integer; { Global }

prosedur proses(n: integer);
variabel
    listLokal: larik[1 .. 5] dari integer; 
    i: integer;
mulai
    listLokal[1] := n * 10;
    
    data := listLokal[1]; 
selesai;

mulai
    data := 999;
    proses(5);
selesai.