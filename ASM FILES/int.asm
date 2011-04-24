.ORIG x1200
LEA R0, memloc; load x4000 into R0 for manipulation
LDW R1, R0, #0; load value into R1
LDW R2, R1, #0; load value at x4000 into R1
ADD R2, R2, #1; increment R1
STW R2, R1, #0;
RTI
memloc .FILL x4000
.END