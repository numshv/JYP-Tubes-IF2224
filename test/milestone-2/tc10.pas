program EXAMPLE1;
variabel
    x, y, sum: real;
mulai
    x := (5 + 10) - (110 - 3*(2 - 3) ) ;
    y := 10;
    sum := x + y;

    writeln('Sum = ', sum);

    jika (sum > 10) dan (sum < 20) dan ((sum < 1000) dan (sum)) maka
        writeln('Sum > 10');
    selain-itu
        writeln('Sum <= 10');

    writeln('Done!');
selesai.
