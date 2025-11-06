program loopdeloop;
variabel
    numbers: larik[1 .. 10] dari integer;
    i, total: integer;
mulai
    total := 0;
    untuk i := 1 turun-ke 10 lakukan
    mulai
        numbers := i * 2;
        total := total + numbers;
    selesai;
    
    i := 10;
    selama (i >= 1) dan (i <= 10) lakukan
    mulai
        writeln('Sum = ', sum);
        i := i - 1;
    selesai;
selesai.