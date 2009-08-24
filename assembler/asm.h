/* **************************************** */
/* Assembler public definitions header file */
/* Fri 3rd Nov 2006 Julian Lewis AB/CO/HT   */
/* **************************************** */

#ifndef ASMH
#define ASMH

#define MAX_OP_CODE_STRING_LENGTH 8
#define MAX_REGISTER_STRING_LENGTH 8

/* Op codes are triadic, the three fields are Src1, Src2, and Dest */
/* Src1 can be a register or a 32bit literal value. Src2  and Dest */
/* are the second source and destination registers. Depending on   */
/* InstructionSet defined in opcodes, the three fields may contain */
/* either nothing, a 32bit literal (only in Src1), a 32bit address */
/* (again only in Src1), or a register.                            */

typedef enum {
   AtNothing,   /* The field is empty for this instruction */
   AtLiteral,   /* Src1 may contain a 32 bit literal value */
   AtAddress,   /* Src1 may contain a program address      */
   AtRegister   /* The field is a register number          */
 } ArgType;

/* Each Opcode is described in the InstructionSet array of */
/* the opcodes file. Here we define the triadic structure  */

typedef struct {
   char          *Name;     /* The name of the instruction */
   char          *Comment;  /* Dissasembler comment        */
   unsigned long  Number;   /* The opcode number           */
   ArgType        Src1;     /* First source argument type  */
   ArgType        Src2;     /* Second source argument type */
   ArgType        Dest;     /* Destination argument type   */
 } OpCode;

/* Mtt hardware Instruction implemented in 2 longs; hence  */
/* Dest and Src2 are one byte each, and the op-code number */
/* and Crc are also one byte. On a big-endian platform the */
/* Fields long can be represented by a four ordered bytes. */

typedef struct {
   unsigned char  Dest;   /* Destination reg in most significant byte */
   unsigned char  Src2;   /* Second source reg low byte of high short */
   unsigned char  Number; /* Opcode number in high byte of low short  */
   unsigned char  Crc;    /* Check sum in the least significant byte  */
	    long  Src1;
 } Instruction;

/* A program is just an array of instructions and the load address. */
/* When the load address is zero, the program can be relocated in   */
/* program memory using the Mtt hardware PcOffset register.         */

typedef struct {
   unsigned long LoadAddress;       /* Load address or zero if relocate */
   unsigned long InstructionCount;  /* Number of instructions to load   */
   Instruction   *Program;          /* Pointer to array of instructions */
 } ProgramBuf;

#endif
