/*
    Name 1: Oliver Zhou
    UTEID 1: ohz56
*/

/***************************************************************/
/*                                                             */
/*   LC-3b Simulator                                           */
/*                                                             */
/*   EE 460N                                                   */
/*   The University of Texas at Austin                         */
/*                                                             */
/***************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file     */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_bus_drivers();
void drive_bus();
void latch_datapath_values();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS {                                                  
    IRD,
    COND1, COND0,
    J5, J4, J3, J2, J1, J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    PCMUX1, PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1, ADDR2MUX0,
    MARMUX,
    ALUK1, ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x)           { return(x[IRD]); }
int GetCOND(int *x)          { return((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x)             { return((x[J5] << 5) + (x[J4] << 4) +
				      (x[J3] << 3) + (x[J2] << 2) +
				      (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x)        { return(x[LD_MAR]); }
int GetLD_MDR(int *x)        { return(x[LD_MDR]); }
int GetLD_IR(int *x)         { return(x[LD_IR]); }
int GetLD_BEN(int *x)        { return(x[LD_BEN]); }
int GetLD_REG(int *x)        { return(x[LD_REG]); }
int GetLD_CC(int *x)         { return(x[LD_CC]); }
int GetLD_PC(int *x)         { return(x[LD_PC]); }
int GetGATE_PC(int *x)       { return(x[GATE_PC]); }
int GetGATE_MDR(int *x)      { return(x[GATE_MDR]); }
int GetGATE_ALU(int *x)      { return(x[GATE_ALU]); }
int GetGATE_MARMUX(int *x)   { return(x[GATE_MARMUX]); }
int GetGATE_SHF(int *x)      { return(x[GATE_SHF]); }
int GetPCMUX(int *x)         { return((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x)         { return(x[DRMUX]); }
int GetSR1MUX(int *x)        { return(x[SR1MUX]); }
int GetADDR1MUX(int *x)      { return(x[ADDR1MUX]); }
int GetADDR2MUX(int *x)      { return((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x)        { return(x[MARMUX]); }
int GetALUK(int *x)          { return((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x)        { return(x[MIO_EN]); }
int GetR_W(int *x)           { return(x[R_W]); }
int GetDATA_SIZE(int *x)     { return(x[DATA_SIZE]); } 
int GetLSHF1(int *x)         { return(x[LSHF1]); }

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A 
   There are two write enable signals, one for each byte. WE0 is used for 
   the least significant byte of a word. WE1 is used for the most significant 
   byte of a word. */

#define WORDS_IN_MEM    0x08000 
#define MEM_CYCLES      5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */
int BUS;	/* value of the bus */

typedef struct System_Latches_Struct{

int PC,		/* program counter */
    MDR,	/* memory data register */
    MAR,	/* memory address register */
    IR,		/* instruction register */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,		/* p condition bit */
    BEN;        /* ben register */

int READY;	/* ready bit */
  /* The ready bit is also latched as you dont want the memory system to assert it 
     at a bad point in the cycle*/

int REGS[LC_3b_REGS]; /* register file. */

int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microintruction */

int STATE_NUMBER; /* Current State Number - Provided for debugging */ 
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {                                                    
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & bus values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {                                                

  eval_micro_sequencer();   
  cycle_memory();
  eval_bus_drivers();
  drive_bus();
  latch_datapath_values();

  CURRENT_LATCHES = NEXT_LATCHES;

  CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {                                      
    int i;

    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
	if (CURRENT_LATCHES.PC == 0x0000) {
	    RUN_BIT = FALSE;
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {                                                     
    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
	cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/ 
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {          
    int address; /* this is a byte address */

    printf("\nMemory content [0x%0.4x..0x%0.4x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%0.4x (%d) : 0x%0.2x%0.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%0.4x..0x%0.4x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%0.4x (%d) : 0x%0.2x%0.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */   
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {                               
    int k; 

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%0.4x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%0.4x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%0.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%0.4x\n", BUS);
    printf("MDR          : 0x%0.4x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%0.4x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	printf("%d: 0x%0.4x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%0.4x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%0.4x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%0.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%0.4x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%0.4x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%0.4x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%0.4x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */  
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {                         
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
    case 'G':
    case 'g':
	go();
	break;

    case 'M':
    case 'm':
	scanf("%i %i", &start, &stop);
	mdump(dumpsim_file, start, stop);
	break;

    case '?':
	help();
	break;
    case 'Q':
    case 'q':
	printf("Bye.\n");
	exit(0);

    case 'R':
    case 'r':
	if (buffer[1] == 'd' || buffer[1] == 'D')
	    rdump(dumpsim_file);
	else {
	    scanf("%d", &cycles);
	    run(cycles);
	}
	break;

    default:
	printf("Invalid Command\n");
	break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */ 
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {                 
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < CONTROL_STORE_BITS; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}

	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

/************************************************************/
/*                                                          */
/* Procedure : init_memory                                  */
/*                                                          */
/* Purpose   : Zero out the memory array                    */
/*                                                          */
/************************************************************/
void init_memory() {                                           
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {                   
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
	printf("Error: Can't open program file %s\n", program_filename);
	exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
	program_base = word >> 1;
    else {
	printf("Error: Program file is empty\n");
	exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
	/* Make sure it fits. */
	if (program_base + ii >= WORDS_IN_MEM) {
	    printf("Error: Program file %s is too long to fit in memory. %x\n",
		   program_filename, ii);
	    exit(-1);
	}

	/* Write the word to memory array. */
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
	ii++;
    }

    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = (program_base << 1);

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */ 
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *ucode_filename, char *program_filename, int num_prog_files) { 
    int i;
    init_control_store(ucode_filename);

    init_memory();
    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(program_filename);
	while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);

    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {                              
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 3) {
	printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }

    while (1)
	get_command(dumpsim_file);

}
/***************************************************************/
/* Do not modify the above code.
   You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/
int SEX(int num, int size){
/* Taken from our Lab 2
* sign extends the number based on the size of bits
* input: num and size
*/
       int masker = 0x0001 << (size - 1); /* checks the most siginificant bit of the number */
       if(num & masker) /* if the number is a negative, we extend the 1 */
       {
               return Low16bits(num | (0xFFFF << size)); 
       }
       else /* if the number is positive, we extend the 0 */
       {
               return Low16bits(num & (0xFFFF >> (16 - size)));
       }
}
   int temp;
void eval_micro_sequencer() {
  /* 
   * Evaluate the address of the next state according to the 
   * micro sequencer logic. Latch the next microinstruction.
   */	

	if(GetIRD(CURRENT_LATCHES.MICROINSTRUCTION)==1) /*check IRD*/
	{
		NEXT_LATCHES.STATE_NUMBER= ((CURRENT_LATCHES.IR&0xF000)>>12); /*0,0, IR[15:12] if IRD = 1*/
		memcpy(NEXT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[((CURRENT_LATCHES.IR&0xF000)>>12)], sizeof(int)*CONTROL_STORE_BITS);
	}
	else 
	{
		temp = GetJ(CURRENT_LATCHES.MICROINSTRUCTION); /* set temp next address to J*/
		switch(GetCOND(CURRENT_LATCHES.MICROINSTRUCTION))
		{ /*cases represent what the COND1 and COND0 responses are */
		case 0:
			break;
		case 1:
			temp = (temp |(CURRENT_LATCHES.READY<<1)); /*r*/
            printf("case 1 temp = %i \n current latches ready = %i", temp, CURRENT_LATCHES.READY);
			break;
		case 2: 
			temp = ((temp) | (CURRENT_LATCHES.BEN<<2)); /*Branch*/
			printf("BRANCH case 2 temp = %i \n", temp);
			break;
		case 3:
			temp = ((temp) | ((CURRENT_LATCHES.IR &0x0800)>>11)); /*IR[11]*/
			printf("case 3 temp = %i \n", temp);
			break;
		default:
			break;
		}
		memcpy(NEXT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[temp], sizeof(int)*CONTROL_STORE_BITS);
		printf("\n next state is going to be %d\n", temp);
		NEXT_LATCHES.STATE_NUMBER = temp;
	}

}

int cyclecount = 0;
int READMDR = 0;
void cycle_memory() 
{
   
  /* 
   * This function emulates memory and the WE logic. 
   * Keep track of which cycle of MEMEN we are dealing with.  
   * If fourth, we need to latch Ready bit at the end of 
   * cycle to prepare microsequencer for the fifth cycle.  5 cylces for memory read 
	MIO.EN, R/W, DATA.SIZE
   */
   	if(GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION)==1) /*  */
	{  
    cyclecount++;
	if(cyclecount == 4) /*latching ready bit on 4th cycle*/
	{
		NEXT_LATCHES.READY = 1;	
	if(GetR_W(CURRENT_LATCHES.MICROINSTRUCTION)==0) /*read, 0=byte, 1= word*/
			{
					READMDR = Low16bits((MEMORY[CURRENT_LATCHES.MAR >> 1][1]<<8) + MEMORY[CURRENT_LATCHES.MAR >> 1][0]);
			}
			/*If DATA_SIZE is BYTE, MAR[0] determines whether WE1 or WE0 is asserted.*/
	else if(GetR_W(CURRENT_LATCHES.MICROINSTRUCTION) ==1) /*write*/
			{
				if(GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) /* data.size = 1 = word*/
				{
					MEMORY[CURRENT_LATCHES.MAR>>1][0] = CURRENT_LATCHES.MDR&0x00FF; /* changed from xFF to x00FF?? Does this help*/
					MEMORY[CURRENT_LATCHES.MAR>>1][1] = Low16bits((CURRENT_LATCHES.MDR&0xFF00) >>8);
				}
				else if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)==0) /* data.size = 0 = byte*/
				{
					if(CURRENT_LATCHES.MAR&0x0001 == 1) 
					{
					MEMORY[CURRENT_LATCHES.MAR>>1][1]= (CURRENT_LATCHES.MDR&0xFF00 >> 8); /*clear it out */
					}
					else /*if ((CURRENT_LATCHES.MAR&0x0001) ==0)*/
					{
                    MEMORY[CURRENT_LATCHES.MAR>>1][0]= CURRENT_LATCHES.MDR&0x00FF;     
					}
				}
			}
	}
	else if(cyclecount ==5)
	{
		NEXT_LATCHES.READY = 0;
		cyclecount = 0;
	}
	}
}

int SR1Temp, SR2Temp, ALUOUT, MARMUXTEMP, SHFTemp, MDROUT;
int BUSOUT;
int ADD1,ADD2;
int A, B;
void eval_bus_drivers() {
 /*values of temporary storage of results of  ADD1MUX, ADD2MUX, SR1, SR2, A and B for ALU*/
  /* 
   * Datapath routine emulating operations before driving the bus.
   * Evaluate the input of tristate drivers 
   *         Gate_MARMUX,
   *		 Gate_PC,
   *		 Gate_ALU,
   *		 Gate_SHF,
   *		 Gate_MDR.
   */   
   /*check sr1mux */
if(GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)==1) /*SR1MUX*/
{
	SR1Temp = Low16bits(CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR&0x01C0)>>6]); /* IR 8:6 because SR1MUX = 1*/
	/*SEX(SR1Temp,16);*/
}
else if(GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)==0)
{
	SR1Temp = Low16bits(CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR&0x0E00)>>9]); /* IR 9:11 because SR1MUX = 0*/
	/*SEX(SR1Temp,16);*/
}

if((CURRENT_LATCHES.IR&0x0020)==0) /*SR2MUX check steering bit for SR2MUX 0 = SR2, 1 = imm5*/
{
	B= (CURRENT_LATCHES.REGS[CURRENT_LATCHES.IR&0x0007]);
	printf("value of B sr2 = %i \n ", B);
}
else
{
	B= Low16bits((SEX((CURRENT_LATCHES.IR&0x001F),5)));
	printf("value of B imm5 = %i \n ", B);
}
/*ADDR1MUX*/
if(GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION))
{
	ADD1= SR1Temp;
}
else if((GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)==0))
{
	ADD1= CURRENT_LATCHES.PC;
}
/*ADDR2MUX*/
switch(GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION))
{/* zero, offset6 offset9 offset11*/
	case 0:
	ADD2 = 0; 
	break;
	case 1:
	ADD2 = SEX(CURRENT_LATCHES.IR&0x3F,6);
	break;
	case 2:
	ADD2 = SEX(CURRENT_LATCHES.IR&0x1FF,9);
	printf("add 2 dump: %i , CURRENT LATCHES IR = %i \n", ADD2, CURRENT_LATCHES.IR&0x1FF);
	break;
	case 3:
	ADD2 = SEX(CURRENT_LATCHES.IR&0x7FF,11);
	break;
	default:
	break;
}
	A = Low16bits(SR1Temp);
	printf("value of A = %i \n ", A);
switch(GetALUK(CURRENT_LATCHES.MICROINSTRUCTION)) /*ALU */
{ /*add, and, xor, passa*/
	/*A = SR1Temp;  removed for testing to outside*/
	case 0:
	ALUOUT = Low16bits(A + B);
	break;
	case 1:
	ALUOUT = Low16bits(A&B);
	break;
	case 2:
	ALUOUT = Low16bits(A^B);
	break;
	case 3:
	ALUOUT = Low16bits(A);
	break; 
}
ALUOUT = ALUOUT&0xFFFF; /*do i need to sign extend this? */

	if(GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0)/*LSHF IR or adder*/
	{
		MARMUXTEMP= Low16bits((CURRENT_LATCHES.IR&0xFF) << 1);
	}	
	else if(GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
		if(GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)==0)
		{
		MARMUXTEMP = Low16bits(ADD1 + ADD2);
		}
		else if (GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)==1) /*LSHF ADD2 when GetLSHF1 is 1*/
		{
		MARMUXTEMP = Low16bits(ADD1 + (ADD2<<1));
		}
   }
   
   int amount4 = CURRENT_LATCHES.IR&0x000F;
   int SR15Orig = CURRENT_LATCHES.IR;
   /*shf, bits 5 and 4 determine direction, LSHF, RSHFL, RSHFA = 0, 1, 2, and 4 digits to be shifted each time*/
   switch((CURRENT_LATCHES.IR&0x30)>>4)
   {
   case 0: 
   SHFTemp = Low16bits(Low16bits(SR1Temp) << amount4);
   break;
   case 1: 
   SHFTemp = Low16bits(Low16bits(SR1Temp) >> amount4);
   break;
   case 3: /*SR15 should be moved in
   		from last lab -NEXT_LATCHES.REGS[dr] = Low16bits(sex((CURRENT_LATCHES.REGS[sr1] >> amount4), 16 - amount4));  */
      SHFTemp = Low16bits(SEX((Low16bits(SR1Temp) >> amount4), 16 - amount4));
   break;
   default:
   break;
   
   }
   
   if(GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
  /* printf("\n mdrout = mdr = %d \n", CURRENT_LATCHES.MDR);*/
   MDROUT = CURRENT_LATCHES.MDR;
   }
   else if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION) ==0)
   {
		if((CURRENT_LATCHES.MAR&0x0001) == 0)
		{
			MDROUT = SEX(CURRENT_LATCHES.MDR&0x00FF,8);
		}
		else if((CURRENT_LATCHES.MAR&0x0001) ==1)
		{
             MDROUT = SEX((CURRENT_LATCHES.MDR&0xFF00)>>8,8);
		}
   }
   
}


void drive_bus() {

  /* 
   * Datapath routine for driving the bus from one of the 5 possible 
   * tristate drivers. 
   */       
   
   /* Gates determine which variables get on the bus */

   if(GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION))
   {
		BUS = CURRENT_LATCHES.PC;
   }
	else if(GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION))
   {
         printf("\n GET GATE MDR : %d \n", MDROUT);
		BUS = MDROUT; /*check for data.size later*/
   }
	else if(GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION))
   {
		BUS = ALUOUT;
   }
	else if(GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION))
   {
		BUS = MARMUXTEMP;
   }
	else if(GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION))
   {
		BUS = SHFTemp;
   }
   else
   {
       BUS = 0;
   }
}


void latch_datapath_values() {
  /* 
   * Datapath routine for computing all functions that need to latch
   * values in the data path at the end of this cycle.  Some values
   * require sourcing the bus; therefore, this routine has to come 
   * after drive_bus.
   */       
/*LD.MAR, LD.MDR, LD.IR, LD.BEN, LD.REG, LD.CC, LD.PC*/

if(GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION))
{
	printf("MAR = BUS = %i \n", BUS);
	NEXT_LATCHES.MAR = BUS;
}

if(GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION)) /*questions about memory, looks like MDROUT*/
{
    /*printf("\n MDR %d \n", READMDR); */    
	if(GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION) == 0)
	{
		if((CURRENT_LATCHES.MAR&0x0001) == 1)
		{
			NEXT_LATCHES.MDR = ((BUS & 0x00FF)<<8)|(BUS&0x00FF); /*when MAR[0]=1,  MDR= BUS 7:0 ' BUS 7:0, because when storing to odd address, you must set MDR to CD'CDinstead of ABCD*/
		}
		else /*take value from bus*/
			NEXT_LATCHES.MDR = BUS;
	}
	else
		NEXT_LATCHES.MDR = READMDR;
}
if(GetLD_IR(CURRENT_LATCHES.MICROINSTRUCTION))
{
	NEXT_LATCHES.IR = BUS;
}
if(GetLD_BEN(CURRENT_LATCHES.MICROINSTRUCTION))
{
/*from fig C.6 bits 11, 10, 9 are n, z, p respectively*/  
       printf("DUMP N Z P  %i %i %i \n" , (CURRENT_LATCHES.IR&0x0800), (CURRENT_LATCHES.IR&0x0400), (CURRENT_LATCHES.IR&0x0200));
	if(CURRENT_LATCHES.N)
	{
		printf(" BEN Test: %i \n", NEXT_LATCHES.BEN);
		if((CURRENT_LATCHES.IR&0x0800)>>11)
		NEXT_LATCHES.BEN = 1;
		else
		NEXT_LATCHES.BEN = 0;

	}
	else if(CURRENT_LATCHES.Z)
	{
		if((CURRENT_LATCHES.IR&0x0400)>>10)
		NEXT_LATCHES.BEN = 1;
		else
		NEXT_LATCHES.BEN = 0;

	}
	else if(CURRENT_LATCHES.P)
	{
		printf("DUMP IR %i \n" , (CURRENT_LATCHES.IR&0x0200));
		if((CURRENT_LATCHES.IR&0x0200)>>9)
		NEXT_LATCHES.BEN = 1;
		else
		NEXT_LATCHES.BEN = 0;
	
	}
	else
	{
		NEXT_LATCHES.BEN = 0;
	}
	printf(" BEN Test: %i \n", NEXT_LATCHES.BEN);
	/*
	switch(CURRENT_LATCHES.IR*0x0E00>>9)
	{
	case 0:
		
		break;
	case 1:
	break;
	case 2:
	break;
	case 3:
	break;
	case 4:
	break;
	}
	*/
}
if(GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION))
{
	/*DR MUX */
	if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION))
	{
		NEXT_LATCHES.REGS[7] = BUS;
	}
	else if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0)
	{
		NEXT_LATCHES.REGS[(CURRENT_LATCHES.IR&0x0E00)>>9] = BUS;
	}
	
}
if(GetLD_CC(CURRENT_LATCHES.MICROINSTRUCTION))
{
	if((BUS&0x8000) == 0x8000)
	{	
		NEXT_LATCHES.N = 1;
		NEXT_LATCHES.Z = 0;
		NEXT_LATCHES.P = 0;
	}
	else if(BUS==0)
	{
		NEXT_LATCHES.Z = 1;
		NEXT_LATCHES.P = 0;
		NEXT_LATCHES.N = 0;
	}
	else
	{
	NEXT_LATCHES.Z = 0;
	NEXT_LATCHES.N = 0;
	NEXT_LATCHES.P = 1;
	}
}
/*printf("\n get ld pc %d \n", CURRENT_LATCHES.MICROINSTRUCTION);
printf("\n index 15 %d \n", CURRENT_LATCHES.MICROINSTRUCTION[15]); */
if(GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION)==1)
{                                       
	switch(GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION)) /*PC+2, BUS, ADDER*/
	{
	case 0:
	NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC + 2);
	break;
	case 1:
  /* printf("\n PC = BUS = %d \n", BUS);*/
	NEXT_LATCHES.PC = Low16bits(BUS);
	break;
	case 2:
	if(GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)==0)
	{
	NEXT_LATCHES.PC  = Low16bits(ADD1 + ADD2);
	printf("\n PCCCCCCCCCCCCCC = add1+add2 = %d \n", Low16bits(ADD1 + ADD2));
	}
	else if (GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)==1) /*LSHF ADD2 when GetLSHF1 is 1*/
	{
	NEXT_LATCHES.PC  = Low16bits((ADD1) + (ADD2<<1));
	printf("\n Current pc = %i , PCCCCCCCCCCCCCC = add1+add2 LSHF = %d \n Add1 = %i , Add2 = %i, ADD2<<1 = %i \n", CURRENT_LATCHES.PC ,Low16bits(ADD1 + (ADD2<<1)), ADD1, ADD2, ADD2<<1);
	}
	 
	break;
	}
}
   
}
