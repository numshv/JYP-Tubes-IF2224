program tc2;
var
    x, y, result: integer;
    average: real;
begin
    x := 10;
    y := 20;
    result := x + y;
    average := result / 2;
    if result > 25 then
        writeln('Damn bro that is huge')
    else
        writeln('Damn bro that is kinda small ..');
end.