program EXAMPLE1;
var
    x, y, sum: real;
begin
    x := 5.;
    y := 10;
    sum := x + y;

    writeln('Sum = ', sum);

    if sum > 10 then
        writeln('Sum > 10')
    else
        writeln('Sum <= 10');

    writeln('Done!');
end.
