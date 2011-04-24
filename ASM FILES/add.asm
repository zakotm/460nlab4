.ORIG x3000
LEA R0, Init
LDW R0, R0, #0
ADD R1, R1, #1
STW R1, R0, #0 ; store R1 = 1 into R0 = x4000 in memory
LEA R0, Start ; load in where numbers begin for summing, xC014
LDW R0, R0, #0
LEA R1, Sum ;where sum is located
LDW R1, R1, #0
ADD R2, R2, #10; load in 10 for counter, 20 locations
ADD R2, R2, #10; incremented it another 10 times
LOOP LDW R3, R0, #0 ;loading in word at R0 into R3
ADD R4, R4, R3 ; add loaded word with what is currently summed
ADD R2, R2, #-1 ; decrement counter
ADD R0, R0, #2 ; increment pointer pointing to array
BRp LOOP ; go back thru loop
STW R4, R1, #0 ; store Sum in R0 to memory location, address which was stored in R1
HALT
Init .FILL x4000
Start .FILL xC000
Sum .FILL x0000 ; xC014 for perfect, x0000 for testing exception
.END