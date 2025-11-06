program loopdeloop;
variabel
    numbers: larik[1 .. 10] dari integer;
    i, total: integer;
mulai
    total := 0;
    
    i := 10;
    selama (i >= 1 dan i <= 10) dan i <> 0 lakukan
    mulai
        writeln('Element ', i, ' = ', numbers[i]);
        i := i - 1;
    selesai;
selesai.