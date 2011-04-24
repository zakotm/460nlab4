.ORIG x3000
LEA R2,asdf
ADD R1,R1,#-1
STB R1,R2,#0
BRzp error
LEA R3,asdf
error HALT
asdf .FILL x0 
.END