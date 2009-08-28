/* ************************************************************************ */
/* CTG (Controls Timing Generator) device driver.  MTT Version.             */
/* Julian Lewis 18th/March/2003                                             */
/* ************************************************************************ */

#ifndef MTTDRVR
#define MTTDRVR

#include <mtthard.h>
#include <skeluser.h>
#include <skeluser_ioctl.h>
#include <skel.h>

/* ------------------------------- */
/* Time UTC second and millisecond */

typedef struct {
	unsigned long Second;
	unsigned long MilliSecond;   /* Read this first to latch Seconds */
} MttDrvrTime;

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

/**
 * MttDrvrEvent - event descriptor
 * @Frame: frame number to send
 * @Priority: 0 (high) or 1 (low)
 */
typedef struct {
	int	Frame;
	int	Priority;
} MttDrvrEvent;

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

#endif
