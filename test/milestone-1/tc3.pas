program loopdeloop;
var
    numbers: array[1 .. 10] of integer;
    i, total: integer;
begin
    total := 0;
    for i := 1 to 10 do
    begin
        numbers[i] := i * 2;
        total := total + numbers[i];
    end;
    
    i := 10;
    while i >= 1 do
    begin
        writeln('Sum = ', sum);
        i := i - 1;
    end;
end.