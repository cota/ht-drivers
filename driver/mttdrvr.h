/* ************************************************************************ */
/* CTG (Controls Timing Generator) device driver.  MTT Version.             */
/* Julian Lewis 18th/March/2003                                             */
/* ************************************************************************ */

#ifndef MTTDRVR
#define MTTDRVR

#include <mtthard.h>

/* Modules and clients maximum counts */

#define MttDrvrCLIENT_CONTEXTS 16
#define MttDrvrMODULE_CONTEXTS 8

/* --------------------------------------------------------------------- */
/* Module address has two base addresess in it one for X1 and one for X2 */

typedef struct {
   unsigned long  *VMEAddress;         /* Base address for main logig A24,D32 */
   unsigned short *JTGAddress;         /* Base address for       JTAG A16,D16 */
   unsigned long   InterruptVector;    /* Interrupt vector number */
   unsigned long   InterruptLevel;     /* Interrupt level (2 usually) */
   unsigned long  *CopyAddress;        /* Copy of VME address */
 } MttDrvrModuleAddress;

/* ------------------------------- */
/* Time UTC second and millisecond */

typedef struct {
   unsigned long Second;
   unsigned long MilliSecond;   /* Read this first to latch Seconds */
 } MttDrvrTime;

/* -------------------------------------- */
/* The driver Connect and Read structures */

#define MttDrvrCONNECTIONS 16

typedef struct {
   unsigned long Module;   /* The module number 1..n     */
	    long ConMask;  /* Connection mask bits */
 } MttDrvrConnection;

typedef struct {
   unsigned long Size;
   unsigned long Pid[MttDrvrCLIENT_CONTEXTS];
 } MttDrvrClientList;

typedef struct {
   unsigned long      Pid;
   unsigned long      Size;
   MttDrvrConnection  Connections[MttDrvrCONNECTIONS];
 } MttDrvrClientConnections;

typedef struct {
   MttDrvrConnection Connection;
   MttDrvrTime       Time;
 } MttDrvrReadBuf;

/* ------------------------------------------------- */
/* The compilation dates in UTC time for components. */

typedef struct {
   unsigned long VhdlVersion; /* VHDL Compile date */
   unsigned long DrvrVersion; /* Drvr Compile date */
 } MttDrvrVersion;

/* ------ */
/* Raw IO */

typedef struct {
   unsigned long Size;      /* Number long to read/write */
   unsigned long Offset;    /* Long offset address space */
   unsigned long *UserArray;/* Callers data area for  IO */
 } MttDrvrRawIoBlock;

/* ------------------------ */
/* Program ram and op codes */

typedef struct {
   unsigned long      LoadAddress;
   unsigned long      InstructionCount;
   MttDrvrInstruction *Program;
 } MttDrvrProgramBuf;

/* ------------------------- */
/* Task control block buffer */

typedef enum {
   MttDrvrTBFLoadAddress      = 0x01,
   MttDrvrTBFInstructionCount = 0x02,
   MttDrvrTBFPcStart          = 0x04,
   MttDrvrTBFPcOffset         = 0x08,
   MttDrvrTBFPc               = 0x10,
   MttDrvrTBFName             = 0x20,
   MttDrvrTBFAll              = 0x3F
 } MttDrvrTBF;

#define MttDrvrTBFbits 6
#define MttDrvrNameSIZE 32

typedef struct {
   unsigned long    Task;                  /* Task number 1..TASKS */
   MttDrvrTBF       Fields;                /* Which fields are valid */
   unsigned long    LoadAddress;           /* Memory location where loaded */
   unsigned long    InstructionCount;      /* Size in instructions */
   unsigned long    PcStart;               /* Start PC value */
   MttDrvrTaskBlock ControlBlock;          /* Pc, Offset, TaskStatus, Pswd */
   char             Name[MttDrvrNameSIZE]; /* Task name */
 } MttDrvrTaskBuf;

/* -------------- */
/* Task registers */

typedef struct {
   unsigned long Task;
   unsigned long RegMask;
   unsigned long RegVals[MttDrvrLRAM_SIZE];
 } MttDrvrTaskRegBuf;

/* ---------------- */
/* Global registers */

typedef struct {
   unsigned long RegNum;
   unsigned long RegVal;
 } MttDrvrGlobalRegBuf;

/* ----------------- */
/* Define the IOCTLs */

typedef enum {

   MttDrvrSET_SW_DEBUG,            /* Set driver debug mode */
   MttDrvrGET_SW_DEBUG,

   MttDrvrGET_VERSION,             /* Get version date */

   MttDrvrSET_TIMEOUT,             /* Set the read timeout value */
   MttDrvrGET_TIMEOUT,             /* Get the read timeout value */

   MttDrvrSET_QUEUE_FLAG,          /* Set queuing capabiulities on off */
   MttDrvrGET_QUEUE_FLAG,          /* 1=Q_off 0=Q_on */
   MttDrvrGET_QUEUE_SIZE,          /* Number of events on queue */
   MttDrvrGET_QUEUE_OVERFLOW,      /* Number of missed events */

   MttDrvrSET_MODULE,              /* Select the module to work with */
   MttDrvrGET_MODULE,              /* Which module am I working with */
   MttDrvrGET_MODULE_COUNT,        /* The number of installed modules */
   MttDrvrGET_MODULE_ADDRESS,      /* Get the VME module base address */
   MttDrvrSET_MODULE_CABLE_ID,     /* Cables telegram ID */
   MttDrvrGET_MODULE_CABLE_ID,     /* Cables telegram ID */

   MttDrvrCONNECT,                 /* Connect to and object interrupt */
   MttDrvrGET_CLIENT_LIST,         /* Get the list of driver clients */
   MttDrvrGET_CLIENT_CONNECTIONS,  /* Get the list of a client connections on module */

   MttDrvrENABLE,                  /* Enable/Disable event output */
   MttDrvrRESET,                   /* Reset the module, re-establish connections */
   MttDrvrUTC_SENDING,             /* Set UTC sending on/off */
   MttDrvrEXTERNAL_1KHZ,           /* Select external 1KHZ clock (CTF3) */
   MttDrvrSET_OUTPUT_DELAY,        /* Set the output delay in 40MHz ticks */
   MttDrvrGET_OUTPUT_DELAY,        /* Get the output delay in 40MHz ticks */
   MttDrvrSET_SYNC_PERIOD,         /* Set Sync period in milliseconds */
   MttDrvrGET_SYNC_PERIOD,         /* Get Sync period in milliseconds */
   MttDrvrSET_UTC,                 /* Set Universal Coordinated Time for next PPS tick */
   MttDrvrGET_UTC,                 /* Latch and read the current UTC time */
   MttDrvrSEND_EVENT,              /* Send an event out now */

   MttDrvrGET_STATUS,              /* Read module status */

   MttDrvrSTART_TASKS,             /* Start tasks specified by bit mask, from start address */
   MttDrvrSTOP_TASKS,              /* Stop tasks bit mask specified by bit mask */
   MttDrvrCONTINUE_TASKS,          /* Continue from current tasks PC */
   MttDrvrGET_RUNNING_TASKS,       /* Get bit mask for all running tasks */

   MttDrvrSET_TASK_CONTROL_BLOCK,  /* Set the tasks TCB */
   MttDrvrGET_TASK_CONTROL_BLOCK,  /* Get the TCB */

   MttDrvrSET_GLOBAL_REG_VALUE,    /* Set a global register value */
   MttDrvrGET_GLOBAL_REG_VALUE,    /* Get a global register value */
   MttDrvrSET_TASK_REG_VALUE,      /* Set a task register value */
   MttDrvrGET_TASK_REG_VALUE,      /* Get a task register value */

   MttDrvrSET_PROGRAM,             /* Set up program code to be executed by tasks */
   MttDrvrGET_PROGRAM,             /* Read program code back from program memory */

   MttDrvrJTAG_OPEN,               /* Open JTAG interface */
   MttDrvrJTAG_READ_BYTE,          /* Read back uploaded VHDL bit stream */
   MttDrvrJTAG_WRITE_BYTE,         /* Upload a new compiled VHDL bit stream */
   MttDrvrJTAG_CLOSE,              /* Close JTAG interface */

   MttDrvrRAW_READ,                /* Raw read  access to card for debug */
   MttDrvrRAW_WRITE,               /* Raw write access to card for debug */

   MttDrvrLAST_IOCTL

 } MttDrvrControlFunction;

#endif
