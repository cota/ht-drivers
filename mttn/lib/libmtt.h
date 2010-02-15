/**************************************************************************/
/* MTT Library routines header file to support FESA class                 */
/* Fri 20th October 2006                                                  */
/* Julian Lewis AB/CO/HT                                                  */
/**************************************************************************/

#ifndef LIBMTT
#define LIBMTT

/* ================================================================ */
/* Some useful definitions have been made in the driver hardware    */
/* description header file "mtthard.h". This file is included from  */
/* "mttdrvr.h" below. This defines types for Interupts, Status and  */
/* module commands.                                                 */

#include <stdint.h>
#include <stdio.h>
#include <mtt.h>

#include "asm.h"
#include "opcodes.h"
#include "mttRegisters.h"


/* ================================================================ */
/* Event Tables reside in MTT tasks 1..16 and are accessed by name  */
/* and never by task number. The actual task number used for a      */
/* table is of no interest. The driver stores the table names with  */
/* each task.                                                       */

#define MttLibMIN_NAME_SIZE 3
#define MttLibMAX_NAME_SIZE 32
#define MttLibTASKS		16
#define MttLibTASK_SIZE		(4096 / (MttLibTASKS))

#ifdef __linux__
#define MttLibDEFAULT_OBJECT_PATH "/tmp/"
#else
#define MttLibDEFAULT_OBJECT_PATH "/dsc/data/Mtt/"
#endif

/* ================================================================ */
/* Names used in Mtt library                                        */

typedef char MttLibName[MttLibMAX_NAME_SIZE];

/* ================================================================ */
/* Task registers                                                   */
typedef MttDrvrTaskRegBuf MttLibTaskRegisters;

/* ================================================================ */
/* The MttLibrary returns error codes as follows..                  */

typedef enum {
   MttLibErrorNONE,   /* No error, all OK */
   MttLibErrorINIT,   /* The MTT library has not been initialized */
   MttLibErrorOPEN,   /* Unable to open the MTT driver */
   MttLibErrorIO,     /* Mtt driver IO error */
   MttLibErrorSYS,    /* Operating system error, see errno */
   MttLibErrorPATH,   /* Bad path name */
   MttLibErrorFILE,   /* Could not find file, see errno */
   MttLibErrorREAD,   /* Can not read file, or file empty */
   MttLibErrorNOMEM,  /* Not enough memory */
   MttLibErrorNORELO, /* Task object binary is not relocatable */
   MttLibErrorTOOBIG, /* Task size is too big */
   MttLibErrorEMPTY,  /* Object file is empty */
   MttLibErrorFULL,   /* No more tasks can be loaded, full up */
   MttLibErrorNOLOAD, /* No such task is loaded */
   MttLibErrorISLOAD, /* Task is already loaded */
   MttLibErrorNAME,   /* Illegal task name */
   MttLibErrorLREG,   /* No such local register */
   MttLibErrorGREG,   /* No such global register */
   MttLibERRORS
 } MttLibError;

/* ================================================================ */
/* Initialize the MTT library. This routine opens the driver, and   */
/* initializes the MTT module. The supplied path is where the       */
/* library will look to find the compiled event table object files. */
/* Notice we don't pass event tables through reflective memory.     */
/* The path name must end with the character '/' and be less than   */
/* LN (128) characters long. If Path is NULL the default is path is */
/* used as defined in libmtt.h DEFAULT_OBJECT_PATH                  */

MttLibError MttLibInit(char *path);

/* ================================================================ */
/* Thread safe sleep in microseconds                                */

void MttLibUsleep(int dly);

/* ================================================================ */
/* Get a string for an MttLibError code                             */

char *MttLibErrorToString(MttLibError err);

/* ================================================================ */
/* Load/UnLoad a compiled table object from the path specified in   */
/* the initialization routine into a spare task slot if one exists. */
/* Memory handling may result in running tasks being moved around.  */
/* Task names must be alpha numeric strings with the first char a   */
/* letter and less than MttLibNameSIZE characters. The special name */
/* "ALL" can be used in calls to the UnLoad function to force all   */
/* tasks and any zombies to be unloaded.                            */

MttLibError MttLibLoadTaskObject(char *name, ProgramBuf *pbf);
MttLibError MttLibLoadTask(char *name);
MttLibError MttLibUnloadTask(char *name);
MttLibError MttLibUnloadTasks(void);

/* ================================================================ */
/* Here the names pointer should be able to store MttLibTABLES char */
/* pointers. Loaded tasks reside in MTT memory, and are running if  */
/* they are not stopped. Do not confuse this state with the run and */
/* wait bit masks in the event table task logic.                    */

MttLibError MttLibGetLoadedTasks(MttLibName *names);
MttLibError MttLibGetRunningTasks(MttLibName *names);

/* ================================================================ */
/* Each task running an event table has MttLibLocakREGISTERS that   */
/* can be used to contain task parameters such as the run count.    */
/* The caller must know what these parameters mean for event tasks. */

MttLibError MttLibSetTaskRegister(char *name,
				  MttLibLocalRegister treg,
				  unsigned long  val);

MttLibError MttLibGetTaskRegister(char *name,
				  MttLibLocalRegister treg,
				  unsigned long *val);
MttLibError MttLibGetTaskRegisters(char *name, MttLibTaskRegisters *phLRegs);

/* ================================================================ */
/* Convert register names to and from register numbers.             */
/* If the regnum is out of range the string "???" is returned.      */
/* If the string is not a register name 0 is returned.              */
/* As global and lo0cal register numbers overlap, a Local or Global */
/* flag "lorg" must be supplied when converting a register number   */
/* to a string. "lorg" must be set accordingly.                     */

typedef enum {
   MttLibRegTypeNONE,
   MttLibRegTypeGLOBAL,
   MttLibRegTypeLOCAL
 } MttLibRegType;

unsigned long MttLibStringToReg(char *name, MttLibRegType *lorg);

char         *MttLibRegToString(int regnum, MttLibRegType lorg);

/* ================================================================ */
/* Tasks running event tables can be started, stopped or continued. */
/* Starting a task loads its program counter with its start address */
/* Stopping a task stops it dead, Continuing a task continues from  */
/* the last program counter value without reloading it.             */

MttLibError MttLibStartTask(char *name);
MttLibError MttLibStopTask(char *name);
MttLibError MttLibContinueTask(char *name);

/* ================================================================ */
/* The task status enumeration is defined in the driver includes.   */
/* By including mttdrvr.h then mtthard.h is also included, and in   */
/* this hardware description file MtttDrvrTaskStatus is defined.    */
/* The task status has values like Running, Stopped, Waiting etc    */

uint32_t MttLibGetTaskStatus(char *name);

/* ================================================================ */
/* Calling this routine sends out the specified frame imediatley.   */

MttLibError MttLibSendEvent(unsigned long frame);

/**
 * @brief Send out a frame
 *
 * @param frame - event frame to send
 * @param priority - 0 (high priority) or 1 (low)
 *
 * @return appropriate MttLibError code
 */
MttLibError MttLibSendEventPrio(unsigned long frame, int priority);

/* ================================================================ */
/* This routine allows you to connect to MTT interrupts and wait    */
/* for them to arrive. You supply a mask defined in mtthard.h that  */
/* defines which interrupts you want to wait for. The possible      */
/* interrupts are error conditions, task interrupts, the PPS, SYNC, */
/* and software triggered events. The noqueue flag kills the queue  */
/* if it has a non zero value. Tmo is the timeout in 10ms units.    */
/* A tmo value of zero means no timeout, hence wait indefinatley.   */

uint32_t MttLibWait(uint32_t mask, int noqueue, int tmo);

/* ================================================================ */
/* The Mtt module status can be read to check its functioning.      */

uint32_t MttLibGetStatus();

/* ================================================================ */
/* Global Mtt module registers contain stuff like telegrams, UTC    */
/* and other stuff. The exact meaning of these registers will       */
/* depend completely on the tasks running in the Mtt module. The    */
/* caller needs to be aware of the register usage conventions.      */

MttLibError MttSetGlobalRegister(MttLibGlobalRegister greg,
				 unsigned long  val);

MttLibError MttGetGlobalRegister(MttLibGlobalRegister greg,
				 unsigned long *val);

/* ================================================================ */
/* This routine returns the mtt driver file handle so that direct   */
/* client ioctl calls can be made.                                  */

int MttLibGetHandle();

/* ================================================================ */
/* Get/Set task range.                                              */
/* first: First task number 1..MttLibTASKS			    */
/* last : Last  task number 1..MttLibTASKS			    */
/* isize: Number of instructions per task                           */

MttLibError MttLibSetTaskRange(unsigned int first,
			       unsigned int last,
			       unsigned int isize);

void MttLibGetTaskRange(unsigned int *first,
			unsigned int *last,
			unsigned int *isize);

/* ================================================================ */
/* Get the TCB number for a given named task, 1..16. If the task is */
/* not found the routine returns zero.                              */

int MttLibGetTcbNum(char *name);

/* ================================================================ */
/* Get a file configuration path                                    */

char *MttLibGetFile(char *name);

/* ================================================================ */
/* Read object code binary from file                                */

MttLibError MttLibReadObject(FILE *objFile, ProgramBuf *code);

/* ================================================================ */
/* Read object code binary from mtt                                 */
MttLibError MttLibReadTaskObject(char *name, ProgramBuf *pbf,Instruction **instructions);

/* ================================================================ */
/* Get host configuration character                                 */

char MttLibGetConfgChar(int n);

/* ================================================================ */
/* Get host configuration ID (M, A or B)                            */

char MttLibGetHostId();

/* ================================================================= */
/* Compile an event table by sending a message to the table compiler */
/* server message queue. The resulting object file is transfered to  */
/* the LHC MTGs accross reflective memory. This routine avoids the   */
/* need for issuing system calls within complex multithreaded FESA   */
/* classes. The table must be loaded as usual and completion must    */
/* be checked as usual by looking at the response xmem tables.       */
/* If the return code is an IO error this probably means the cmtsrv  */
/* task is not running. A SYS error indicates a message queue error. */

MttLibError MttLibCompileTable();

#endif
