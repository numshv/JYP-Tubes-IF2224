program GerbangLogika;
variabel
    gateNOT: larik[false .. true] dari boolean;
    inputVal, outputVal: boolean;
mulai
    gateNOT[false] := true;
    gateNOT[true] := false;

    inputVal := false;
    

    jika (gateNOT[inputVal]) maka
        outputVal := true;
    
    outputVal := gateNOT[true]; 
selesai.