program EdgeCaseNumbersAndStrings;
var
    zero: integer;
    negative: integer;
    huge: integer;
    pi: real;
    epsilon: real;
    empty: char;
    quote: char;
begin
    zero := 0;
    negative := -999;
    huge := 2147483647;
    pi := 3.14159265359;
    epsilon := 0.00001;
    numbers: array[1..10] of integer    ;
    
    empty := ' ';
    quote := '''';
    
    if zero = 0 then
        writeln('Zero works');
    
    if negative < 0 then
        writeln('Negative: ', negative);
    
    writeln('Pi = ', pi);
    writeln('String with spaces:  multiple  spaces');
    writeln('Special: ', quote);
    writeln('Numbers:', 1, 22, 333, 4444);
end.