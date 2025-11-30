program TesIndeks;
variabel
    skorHuruf: larik['a' .. 'c'] dari integer;
    
    status: larik[false .. true] dari integer;
    
    val : integer;
mulai
    skorHuruf['a'] := 10;
    skorHuruf['b'] := 20;
    
    status[true] := 1;
    status[false] := 0;
    
    val := skorHuruf['a'] + status[true];
selesai.