
#ifndef OPCODES_H
#define OPCODES_H
/* Describe the MTT CTG module instruction Set */


typedef enum {

   HALT = 0x00,
   NOOP = 0x4d,
   INT  = 0xC1,

   WEQV = 0x91, WEQR = 0x11,
   WRLV = 0x92, WRLR = 0x12,
   WORV = 0x93, WORR = 0x13,

   MOVV = 0xe2, MOVR = 0x62,

   MOVIR= 0x64,
   MOVRI= 0x65,

   ADDV = 0xe6, ADDR = 0x66,
   SUBV = 0xe7, SUBR = 0x67,
   LORV = 0xe8, LORR = 0x68,
   ANDV = 0xe9, ANDR = 0x69,
   XORV = 0xea, XORR = 0x6a,
   LSV  = 0xeb, LSR  = 0x6b,
   RSV  = 0xec, RSR  = 0x6c,

   WDOG = 0x34,
   JMP  = 0x85,
   BEQ  = 0x86,
   BNE  = 0x87,
   BLT  = 0x88,
   BGT  = 0x89,
   BLE  = 0x8a,
   BGE  = 0x8b,
   BCR  = 0x8c,

   OPCODES = 36

 } IdNumber;

/* Describe the MTT CTG module register Set */

#define REGISTERS 256
#define REGNAMES 15
#define GLOBALS 224
#define LOCALS 32
#define GLOBAL_REG 1
#define LOCAL_REG 2

typedef struct {
   char *Name;
   char *Comment;
   int   Start;
   int   End;
   int   Offset;
   int   LorG;      /* 1=>Global 2=>Local */
 } RegisterDsc;



extern OpCode InstructionSet[OPCODES];
extern RegisterDsc Regs[REGNAMES];

#endif
