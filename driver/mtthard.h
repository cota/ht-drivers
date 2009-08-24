/* ************************************************************************ */
/* Julian Lewis 08th/August/2006                                            */
/* Multi-Tasking CTG (MTT) Hardware description                             */
/* This is a D32 module, Only 32 bit access from VME                        */
/* ************************************************************************ */

#ifndef MTTDRVR_HARD
#define MTTDRVR_HARD

/* ---------------------------------------------------------------------- */
/* Names and values of the interrupt source bits that can be connected to */

typedef enum {
   MttDrvrIntINT_1           = 0x1,         /* Interrupt from INT Op Code */
   MttDrvrIntINT_2           = 0x2,         /* Interrupt from INT Op Code */
   MttDrvrIntINT_3           = 0x4,         /* Interrupt from INT Op Code */
   MttDrvrIntINT_4           = 0x8,         /* Interrupt from INT Op Code */
   MttDrvrIntINT_5           = 0x10,        /* Interrupt from INT Op Code */
   MttDrvrIntINT_6           = 0x20,        /* Interrupt from INT Op Code */
   MttDrvrIntINT_7           = 0x40,        /* Interrupt from INT Op Code */
   MttDrvrIntINT_8           = 0x80,        /* Interrupt from INT Op Code */
   MttDrvrIntINT_9           = 0x100,       /* Interrupt from INT Op Code */
   MttDrvrIntINT_10          = 0x200,       /* Interrupt from INT Op Code */
   MttDrvrIntINT_11          = 0x400,       /* Interrupt from INT Op Code */
   MttDrvrIntINT_12          = 0x800,       /* Interrupt from INT Op Code */
   MttDrvrIntINT_13          = 0x1000,      /* Interrupt from INT Op Code */
   MttDrvrIntINT_14          = 0x2000,      /* Interrupt from INT Op Code */
   MttDrvrIntINT_15          = 0x4000,      /* Interrupt from INT Op Code */
   MttDrvrIntINT_16          = 0x8000,      /* Interrupt from INT Op Code */

   MttDrvrIntRESERVED_0      = 0x10000,     /* Reserved for future use */
   MttDrvrIntNO_40_MHZ       = 0x20000,     /* 40MHz clock stopped */
   MttDrvrIntUNSTABLE_SYNC   = 0x40000,     /* Not one sync period between sync pulses */
   MttDrvrIntUNSTABLE_PPS    = 0x80000,     /* Not one second measured in 40MHz */
   MttDrvrIntILLEGAL_OP_CODE = 0x100000,    /* Not a valid op code execution in task */
   MttDrvrIntPROGRAM_HALT    = 0x200000,    /* Task program halted */
   MttDrvrIntFRAME_MOVED     = 0x400000,    /* Frame moved in the millisecond */
   MttDrvrIntFIFO_OVERFLOW   = 0x800000,    /* FiFo over flow in output stream */
   MttDrvrIntPPS             = 0x1000000,   /* One PPS interrupt */
   MttDrvrIntSYNC            = 0x2000000,   /* Sync interrupt */

   MttDrvrIntRESERVED_1      = 0x4000000,   /* Reserved for future use */
   MttDrvrIntRESERVED_2      = 0x8000000,   /* Reserved for future use */

   MttDrvrIntSOFT_1          = 0x10000000,  /* Software interrupt */
   MttDrvrIntSOFT_2          = 0x20000000,  /* Software interrupt */
   MttDrvrIntSOFT_3          = 0x40000000,  /* Software interrupt */
   MttDrvrIntSOFT_4          = 0x80000000,  /* Software interrupt */

   MttDrvrINTS               = 32           /* Number of HARDWARE interrupts */

 } MttDrvrInt;

#define MttDrvrIntMASK 0x03FFFFFF           /* HARDWARE Interrupt mask */

/* ------------------------------------------ */
/* Bit fields of the interrupt setup register */

typedef enum {
   MttDrvrIrqLevel_2 = 0x200, /* Level 2 value */
   MttDrvrIrqLevel_3 = 0x300, /* Level 3 value */

   MttDrvrIrqLEVELS  = 2      /* Total number of available IRQ Levels */

 } MttDrvrIrqLevel;

/* ----------------------------------- */
/* Names and values of the status bits */

typedef enum {
   MttDrvrStatusENABLED          = 0x01, /* Event output is enabled */
   MttDrvrStatusUTC_SET          = 0x02, /* UTC time has been set by the PPS */
   MttDrvrStatusUTC_SENDING_ON   = 0x04, /* UTC sending is on */
   MttDrvrStatusBAD_40_MHZ       = 0x08, /* 40.000 MHz clock missing */
   MttDrvrStatusBAD_SYNC         = 0x10, /* Either Sync missing or wrong period */
   MttDrvrStatusBAD_PPS          = 0x20, /* Either PPS is missing or wrong period */
   MttDrvrStatusEXTERNAL_1KHZ    = 0x40, /* External 1KHz is in use (CTF3) */

   MttDrvrStatusNOT_USED         = 0x80, /* For the future if needed */

   /* Software status bits added here. They are not hardware status bits */

   MttDrvrStatusBUS_FAULT        = 0x100, /* Bus error detected on the module */
   MttDrvrStatusFLASH_OPEN       = 0x200, /* Flash Memory is open for update  */

   MttDrvrSTATAE                 = 10

 } MttDrvrStatus;

/* ---------------------- */
/* Command function codes */

typedef enum {
   MttDrvrCommandRESET,                   /* Reset the module                      No value     */
   MttDrvrCommandENABLE,                  /* Enable/Disable event output           => 1/0       */
   MttDrvrCommandEXTERNAL_1KHZ,           /* Select/Deselect external 1KHZ clock   => 1/0       */
   MttDrvrCommandUTC_SENDING_ON,          /* Enable/Disable UTC sending            => 1/0       */
   MttDrvrCommandSET_UTC,                 /* Update time to UTC Second at next PPS => UTC       */
   MttDrvrCommandSET_OUT_DELAY,           /* Update Output Delay in 25ns ticks     => Delay     */
   MttDrvrCommandSET_SYNC_PERIOD,         /* Update Sync Period in milliseconds    => Period    */
   MttDrvrCommandSEND_EVENT,              /* Send out the event                    => Frame     */

   MttDrvrCOMMANDS                        /* Total number of control functions */

 } MttDrvrCommand;

/* -------------------------------------------------------------- */
/* Global and Local RAM memory.                                   */
/* The sum of LRAM and GRAM must be 256 or less. In this way the  */
/* Destination register, Second source register can be encoded in */
/* a one byte each and hence an instruction size is two longs.    */

#define MttDrvrGRAM_SIZE 224
#define MttDrvrLRAM_SIZE 32

/* -------------------------------------------------------------- */
/* Instruction implemented in 2 longs. Dest and Src2 are one byte */
/* each, and the op-code number and Crc are also one byte. On a   */
/* big-endian platform the Fields long could be represented as... */
/*                                                                */
/* unsigned char Dest;  Destination reg in most significant byte  */
/* unsigned char Src2;  Second source reg low byte of high short  */
/* unsigned char Numb;  Opcode number in high byte of low short   */
/* unsigned char Crc;   Check sum in the least significant byte   */

typedef struct {
   unsigned long Fields; /* Encoded bytes: Dest, Srce2, Numb, Crc */
	    long Srce1;  /* Type Source register/value */
 } MttDrvrInstruction;

/* ---------------------------------------------------------- */
/* Task Control block                                         */

typedef enum {
   MttDrvrTaskStatusILLEGAL_OP_CODE  = 0x01,
   MttDrvrTaskStatusILLEGAL_VALUE    = 0x02,
   MttDrvrTaskStatusILLEGAL_REGISTER = 0x04,
   MttDrvrTaskStatusWAITING          = 0x08,
   MttDrvrTaskStatusSTOPPED          = 0x10, /* Self stopped */
   MttDrvrTaskStatusRUNNING          = 0x20,

   MttDrvrTaskSTATAE                 = 6

 } MttDrvrTaskStatus;

typedef enum {
   MttDrvrProcessorStatusEQ = 0x1, /* Last instruction result was Zero */
   MttDrvrProcessorStatusLT = 0x2, /* Last instruction result was Less Than Zero */
   MttDrvrProcessorStatusGT = 0x4, /* Last instruction result was Greater Than Zero */
   MttDrvrProcessorStatusCR = 0x8, /* Last instruction result Set the Carry */

   MttDrvrProcessorSTATAE   = 4    /* Total number of processor status bits */

 } MttDrvrProcessorStatus;

typedef struct {
   unsigned long          Pc;              /* Program counter */
   unsigned long          PcOffset;        /* Relocater */
   MttDrvrTaskStatus      TaskStatus;      /* Status of Task, see above */
   MttDrvrProcessorStatus ProcessorStatus; /* Processor status word */
 } MttDrvrTaskBlock;

/* ------------------------------ */
/* Some extra scalors we need     */

#define MttDrvrTASKS 16
#define MttDrvrTASK_MASK 0xFFFF
#define MttDrvrINSTRUCTIONS 4096

/* ------------------------------ */
/* Module Memory Map              */

typedef struct {
   MttDrvrInt         IrqSource;    /* RO: Interrupt source register: Cleared On Read */
   MttDrvrInt         IrqEnable;    /* RW: Interrupt Enable          */
   MttDrvrIrqLevel    IrqLevel;     /* RW: Interrupt setup level 2/3 */
   unsigned long      VhdlDate;     /* RO: VHDL Bit stream date */
   MttDrvrStatus      Status;       /* RO: Status register */
   MttDrvrCommand     Command;      /* RW: Command function register */
   unsigned long      CommandValue; /* RW: Parameter for control function */
   unsigned long      Second;       /* RO: UTC time in seconds */
   unsigned long      MilliSecond;  /* RO: Read this first to latch Seconds */

   unsigned long      StartTasks;          /* RW: Set these tasks to running status */
   unsigned long      StopTasks;           /* RW: Stop these tasks */
   unsigned long      RunningTasks;        /* RO: These tasks are running  */
   MttDrvrTaskBlock   Tasks[MttDrvrTASKS]; /* RW: Task Control Blocks */

   unsigned long      GlobalMem [MttDrvrGRAM_SIZE];                 /* RW: Global registers */
   unsigned long      LocalMem  [MttDrvrTASKS][MttDrvrLRAM_SIZE];   /* RW: Local task memory */
   MttDrvrInstruction ProgramMem[MttDrvrINSTRUCTIONS];              /* RW: Event program memory */

 } MttDrvrMap;

#endif
