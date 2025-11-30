program GerbangLogika;
variabel
    gateNOT: larik[false .. true] dari bool;
    inputVal, outputVal: boolean;
mulai
    gateNOT[false] := true;
    gateNOT[true] := false;

    inputVal := false;
    

    if (gateNOT[inputVal]) then
        outputVal := true;
    
    outputVal := gateNOT[true]; 
selesai.