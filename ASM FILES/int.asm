	.ORIG x1200
	ADD R6, R6, #-2  	;push onto stack
	STW R0, R6, #0
	ADD R6, R6, #-2
	STW R1, R6, #0
	AND R0, R0, #0
	AND R1, R1, #0
	LEA R0, mem1
	LDW R0, R0, #0		;R0=x4000
	ADD R1, R1, #1
	STW R1, R0, #0		;initialize M[x400] to 1
	LDW R1, R6, #0		;pop off from stack
	ADD R6, R6, #2
	LDW R0, R6, #0
	ADD R6, R6, #2
	RTI	
mem1	.FILL x4000
	.END 
