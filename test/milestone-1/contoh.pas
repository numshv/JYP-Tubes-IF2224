program contohLengkap;

konstanta
    PI = 3;
    GAYA = 8e-3;
    HURUF = 'A';
    KALIMAT = 'Hello World';

tipe
    Warna = integer;
    Angka = integer;
    Matriks = larik[1..10] dari real;

variabel
    x, y, z: real;
    i, j: integer;
    huruf: char;
    benar: boolean;

prosedur tampilkan(angka: integer);

mulai
    hasil := angka + 10;
    jika hasil > 100 maka
        hasil := hasil - 1;
    selain-itu
        hasil := hasil + 1;
    tulis('Hasil:', hasil);
selesai;

fungsi kuadrat(a: real): real;
mulai
    kuadrat := a * a;
selesai;

mulai
    x := 1;
    y := 5;
    z := x / y - 1;
    benar := true dan tidak false;
    teks := 'Contoh string literal';
    huruf := 'Z';
    tampilkan(10);
    i := 1;
    selama i <= 10 lakukan
    mulai
        j := i * 2;
        i := i + 1;
    selesai;

    untuk j := 1 ke 5 lakukan
        tulis('Iterasi ke-', j);

    untuk j := 5 turun-ke 1 lakukan
        tulis('Mundur ke-', j);

    kasus i dari
        1: tulis('Satu');
        2: tulis('Dua');
        selain-itu: tulis('Lainnya');
    selesai;
selesai.