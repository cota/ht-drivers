/* ************************************************************************ */
/* Multi-Tasking CTG (MTT)                                                  */
/* PRIVATE driver structures, info, module and client contexts etc.         */
/* 08th/August/2006 Julian Lewis                                            */
/* ************************************************************************ */

#ifndef MTTDRVR_P
#define MTTDRVR_P

/* ----------------------------------------------- */
/* Info table used by the install procedure        */

typedef struct {
	unsigned long              Modules;
	MttDrvrModuleAddress       Addresses[MttDrvrMODULE_CONTEXTS];
} MttDrvrInfoTable;

/* ----------------------------------------------- */
/* Up to 32 incomming events per client are queued */

#define MttDrvrQUEUE_SIZE 32

typedef struct {
	unsigned short QueueOff;
	unsigned short Missed;
	unsigned short Size;
	unsigned long  RdPntr;
	unsigned long  WrPntr;
	MttDrvrReadBuf Entries[MttDrvrQUEUE_SIZE];
} MttDrvrQueue;

/* ------------------ */
/* The client context */

#define MttDrvrDEFAULT_TIMEOUT 2000 /* In 10 ms ticks */

typedef struct {
	unsigned long InUse;
	unsigned long Pid;
	unsigned long ClientIndex;
	unsigned long ModuleIndex;
	unsigned long Timeout;
	int  Timer;
	int  Semaphore;
	MttDrvrQueue  Queue;
	unsigned long DebugOn;
} MttDrvrClientContext;

/* ---------------- */
/* The task context */

typedef struct {
	unsigned long LoadAddress;           /* Where it is loaded in memory */
	unsigned long InstructionCount;      /* Size in instructions */
	unsigned long PcStart;               /* Start PC value */
	unsigned long PcOffset;              /* Relocator PC offset */
	unsigned long Pc;                    /* PC value */
	char          Name[MttDrvrNameSIZE]; /* Task name */
} MttDrvrTaskContext;

/* --------------------------------- */
/* Module side connection structures */

typedef struct {
	MttDrvrModuleAddress  Address;
	unsigned long         ModuleIndex;
	unsigned long         CableId;
	unsigned long         EnabledInterrupts;
	unsigned long         EnabledOutput;
	unsigned long         External1KHZ;
	unsigned long         UtcSending;
	unsigned long         OutputDelay;
	unsigned long         SyncPeriod;
	unsigned long         BusError;
	unsigned long         FlashOpen;
	unsigned long         TimeSet;
	unsigned long         Connected[MttDrvrINTS];  /* Connected Clients */
	MttDrvrTaskContext    Tasks[MttDrvrTASKS];     /* Task contexts */
} MttDrvrModuleContext;

/* ------------------- */
/* Driver Working Area */

typedef struct {
	unsigned long              Modules;
	MttDrvrModuleContext       ModuleContexts[MttDrvrMODULE_CONTEXTS];
	MttDrvrClientContext       ClientContexts[MttDrvrCLIENT_CONTEXTS];
} MttDrvrWorkingArea;

#endif
