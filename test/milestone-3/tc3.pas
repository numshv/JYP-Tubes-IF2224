program UjiFaktorial;
variabel
    hasil, n : integer;

fungsi faktorial(k : integer) : integer;
mulai
    jika (k <= 1) maka
        faktorial := 1;
    selain-itu
        faktorial := k * faktorial(k - 1);
selesai;

mulai
    n := 5;
    hasil := faktorial(n);
    writeln('Faktorial 5 = ', hasil);
selesai.