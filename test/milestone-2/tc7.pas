program loopdeloop;
variabel
    numbers: larik[1 .. 10] dari integer;
    i, total: integer;
mulai
    numbers[i+1] := 42; 
    writeln('Element ', i, ' = ', numbers[i]);
selesai.