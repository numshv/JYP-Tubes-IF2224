program loopChar;
variabel
    poin: larik['a' .. 'e'] dari char; 
    c: char;
    total: integer;
mulai
    total := 0;

    { Loop menggunakan iterator Char }
    untuk c := 'a' ke 'e' lakukan
    mulai
        { Akses array menggunakan Char }
        poin[c] := 10; 
        total := total + poin[c];
    selesai;
    
    writeln('Total poin a-e = ', total);
    
    { Test akses manual }
    poin['c'] := 999;
    writeln('Nilai pada indeks c diganti jadi: ', poin['c']);
selesai.