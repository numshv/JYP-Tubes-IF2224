program hitungluas;
variabel
    panjang, lebar, luas: integer;
    
prosedur cetak_hasil(l)
mulai
    writeln('Luas = ', l);
selesai;

mulai
    panjang := 5;
    lebar := 7;
    luas := panjang * lebar;

    cetak_hasil(luas);

selesai.
