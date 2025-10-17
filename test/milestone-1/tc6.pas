program InvalidOperators;
var
    x, y: integer;
    result: real;
    message: string;
begin
    x := 10;
    y := 5;
    result := x @ y;
    x := x & 3;
    
    if x != y then
        writeln('Not equal');
    
    message := "This is double quote";
    result := 3.14.159;
    
    x := 100%;
    y := #invalid;
end.