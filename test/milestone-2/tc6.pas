program NestedProcedureError;
variabel
    x: integer;

prosedur outer(a: integer);
variabel
    y: integer;
mulai
    y := a + 10;
selesai;

prosedur inner(b: integer);
variabel
    z: integer;
mulai
    z := b * 2;
selesai;

mulai
    x := 5;
    outer(x);
selesai.