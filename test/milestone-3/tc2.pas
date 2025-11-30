program loopChar;
variabel
    poin: larik['a' .. 'e'] dari char; 
    c: char;
    total: integer;
mulai
    total := 0;

    untuk c := 'a' ke 'e' lakukan
    mulai
        poin[c] := 10; 
        total := total + poin[c];
    selesai;
    
    writeln('Total poin a-e = ', total);
    
    poin['c'] := 999;
    writeln('Nilai pada indeks c diganti jadi: ', poin['c']);
selesai.