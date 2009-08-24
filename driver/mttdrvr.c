/* ************************************************************************ */
/* Julian Lewis 08th/August/2006                                            */
/* Multi-Tasking CTG (MTT) LynxOs.4 Device Driver                           */
/* ************************************************************************ */

#include <dldd.h>
#include <errno.h>
#include <kernel.h>
#include <io.h>
#include <conf.h>
#include <sys/ioctl.h>
#include <time.h>
#include <mem.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/timeout.h>
#include <string.h>

#include <mtthard.h>
#include <mttdrvr.h>
#include <mttdrvrP.h>

extern int  kkprintf(char *, ...);
extern void cprintf             _AP((char *fmt, ...));
extern int  swait               _AP((int *sem, int flag));
extern int  ssignal             _AP((int *sem));
extern int  ssignaln            _AP((int *sem, int count));
extern int  scount              _AP((int *sem));
extern int  sreset              _AP((int *sem));
extern void sysfree             _AP((char *, long));
extern int  recoset             _AP((void));
extern void noreco              _AP((void));
extern int _kill                _AP((int pid, int signal));
extern int _killpg              _AP((int pgrp, int signal));
extern pid_t getpid             _AP((void));

extern  char *sysbrk ();
extern  int  timeout ();

/* references specifics to CES PowerPC Cpus RIO806x for VME memory and VME interrupts mapping */

#include <ces/vmelib.h>

extern unsigned long find_controller();
extern unsigned long return_controller();
extern int vme_intset();
extern int vme_intclr();
extern void disable_intr();
extern void enable_intr();

/* Flash the i/o pipe line */

#define EIEIO asm("eieio")

/* -------------------------- */
/* Standard driver interfaces */

void  IntrHandler();
char *MttDrvrInstall();
int   MttDrvrUninstall();
int   MttDrvrOpen();
int   MttDrvrClose();
int   MttDrvrIoctl();
int   MttDrvrRead();
int   MttDrvrWrite();
int   MttDrvrSelect();

/*========================================================================*/

/* Drivers working area global pointer */

static MttDrvrWorkingArea *Wa = NULL;

/*========================================================================*/

static char *IoctlNames[MttDrvrLAST_IOCTL] = {

	"SET_SW_DEBUG",           /* Set driver debug mode */
	"GET_SW_DEBUG",

	"GET_VERSION",            /* Get version date */

	"SET_TIMEOUT",            /* Set the read timeout value */
	"GET_TIMEOUT",            /* Get the read timeout value */

	"SET_QUEUE_FLAG",         /* Set queuing capabiulities on off */
	"GET_QUEUE_FLAG",         /* 1=Q_off 0=Q_on */
	"GET_QUEUE_SIZE",         /* Number of events on queue */
	"GET_QUEUE_OVERFLOW",     /* Number of missed events */

	"SET_MODULE",             /* Select the module to work with */
	"GET_MODULE",             /* Which module am I working with */
	"GET_MODULE_COUNT",       /* The number of installed modules */
	"GET_MODULE_ADDRESS",     /* Get the VME module base address */
	"SET_MODULE_CABLE_ID",    /* Cables telegram ID */
	"GET_MODULE_CABLE_ID",    /* Cables telegram ID */

	"CONNECT",                /* Connect to and object interrupt */
	"GET_CLIENT_LIST",        /* Get the list of driver clients */
	"GET_CLIENT_CONNECTIONS", /* Get the list of a client connections on module */

	"ENABLE",                 /* Enable/Disable event output */
	"RESET",                  /* Reset the module, re-establish connections */
	"UTC_SENDING",            /* Set UTC sending on/off */
	"EXTERNAL_1KHZ",          /* Select external 1KHZ clock (CTF3) */
	"SET_OUTPUT_DELAY",       /* Set the output delay in 40MHz ticks */
	"GET_OUTPUT_DELAY",       /* Get the output delay in 40MHz ticks */
	"SET_SYNC_PERIOD",        /* Set Sync period in milliseconds */
	"GET_SYNC_PERIOD",        /* Get Sync period in milliseconds */
	"SET_UTC",                /* Set Universal Coordinated Time for next PPS tick */
	"GET_UTC",                /* Latch and read the current UTC time */
	"SEND_EVENT",             /* Send an event out now */

	"GET_STATUS",             /* Read module status */

	"START_TASKS",            /* Start tasks specified by bit mask, from start address */
	"STOP_TASKS",             /* Stop tasks bit mask specified by bit mask */
	"CONTINUE_TASKS",         /* Continue from current tasks PC */
	"GET_RUNNING_TASKS",      /* Get bit mask for all running tasks */

	"SET_TASK_CONTROL_BLOCK", /* Set the tasks TCB parameters */
	"GET_TASK_CONTROL_BLOCK", /* Get the TCB */

	"SET_GLOBAL_REG_VALUE",   /* Set a global register value */
	"GET_GLOBAL_REG_VALUE",   /* Get a global register value */
	"SET_TASK_REG_VALUE",     /* Set a task register value */
	"GET_TASK_REG_VALUE",     /* Get a task register value */

	"SET_PROGRAM",            /* Set up program code to be executed by tasks */
	"GET_PROGRAM",            /* Read program code back from program memory */

	"JTAG_OPEN",              /* Open JTAG interface */
	"JTAG_READ_BYTE",         /* Read back uploaded VHDL bit stream */
	"JTAG_WRITE_BYTE",        /* Upload a new compiled VHDL bit stream */
	"JTAG_CLOSE",             /* Close JTAG interface */

	"RAW_READ",               /* Raw read  access to card for debug */
	"RAW_WRITE"               /* Raw write access to card for debug */

};

/*========================================================================*/
/* 32 Bit long access for CTR structure to/from hardware copy             */
/*========================================================================*/

static void LongCopy(volatile unsigned long *dst,
		volatile unsigned long *src,
		unsigned long size) {
	int i, sb;

	sb = size/sizeof(unsigned long);
	for (i=0; i<sb; i++) dst[i] = src[i];
}

/*========================================================================*/
/* Raw IO                                                                 */
/*========================================================================*/

static int RawIo(mcon,riob,flag,debg)
MttDrvrModuleContext *mcon;
MttDrvrRawIoBlock    *riob;
unsigned long          flag;
unsigned long          debg; {

	volatile MttDrvrModuleAddress *moad;  /* Module address, vector, level, ssch */
	volatile MttDrvrMap           *mmap;  /* Module Memory map */
	int                             rval; /* Return value */
	int                             ps;   /* Processor status word */
	int                             i, j;
	unsigned long                  *uary;

	moad = &(mcon->Address);
	mmap = (MttDrvrMap *) moad->VMEAddress;
	uary = riob->UserArray;
	rval = OK;

	if (!recoset()) {         /* Catch bus errors */

		for (i=0; i<riob->Size; i++) {
			j = riob->Offset+i;
			if (flag) {
				((unsigned long *) mmap)[j] = uary[i];
				if (debg >= 2) cprintf("RawIo: Write: %d:0x%x\n",j,uary[i]);
			} else {
				uary[i] = ((unsigned long *) mmap)[j];
				if (debg >= 2) cprintf("RawIo: Read: %d:0x%x\n",j,uary[i]);
			}
			EIEIO;
			SYNC;
		}
	} else {
		disable(ps);
		kkprintf("MttDrvr: BUS-ERROR: Module:%d. VME Addr:%x Vect:%x Level:%x\n",
			mcon->ModuleIndex+1,
			moad->VMEAddress,
			moad->InterruptVector,
			moad->InterruptLevel);

		mcon->BusError = 1;

		pseterr(ENXIO);        /* No such device or address */
		rval = SYSERR;
		enable(ps);
	}
	noreco();                 /* Remove local bus trap */
	riob->Size = i+1;
	return rval;
}

/*========================================================================*/
/* Get the VHDL compilation date of the module                            */
/*========================================================================*/

static unsigned long GetVhdlVersion(mcon)
MttDrvrModuleContext *mcon; {

	volatile MttDrvrModuleAddress *moad; /* Module address, vector, level, ssch */
	volatile MttDrvrMap           *mmap; /* Module Memory map */

	moad = &(mcon->Address);
	mmap = (MttDrvrMap *) moad->VMEAddress;
	return mmap->VhdlDate;
}

/*========================================================================*/
/* Test an address on a module for a bus error                            */
/*========================================================================*/

static int PingModule(mcon)
MttDrvrModuleContext *mcon; {

	MttDrvrModuleAddress *moad; /* Module address, vector, level, ssch */
	volatile MttDrvrMap  *mmap; /* Module Memory map */
	unsigned long         test; /* Test value read back */
	int                   rval; /* Return value */
	int                   ps;   /* Processor status word */

	moad = &(mcon->Address);
	mmap = (MttDrvrMap *) moad->VMEAddress;
	if (mmap == NULL) {
		cprintf("MttDrvr: PingModule: %d Null VMEAddress\n",
			mcon->ModuleIndex+1);
		pseterr(ENXIO);        /* No such device or address */
		return SYSERR;
	}

	rval = OK;
	if (!recoset()) {         /* Catch bus errors */

		/* Read the VHDL data trapping any bus errors */

		test = GetVhdlVersion(mcon);
		EIEIO;
		SYNC;

	} else {
		disable(ps);
		kkprintf("MttDrvr: BUS-ERROR: Module:%d. VME Addr:%x Vect:%x Level:%x\n",
			mcon->ModuleIndex+1,
			moad->VMEAddress,
			moad->InterruptVector,
			moad->InterruptLevel);

		mcon->BusError = 1;

		pseterr(ENXIO);        /* No such device or address */
		rval = SYSERR;
		enable(ps);
	}
	noreco();                 /* Remove local bus trap */
	return rval;
}

/*========================================================================*/
/* Get the Driver compilation date                                        */
/*========================================================================*/

static unsigned long GetDrvrVersion() {

#ifdef COMPILE_TIME
	return COMPILE_TIME;
#else
	return 0;
#endif

}

/*========================================================================*/
/* Get/Set the UTC time                                                   */
/*========================================================================*/

static void GetSetUtc(mcon,time,flag)
MttDrvrModuleContext *mcon;
MttDrvrTime          *time;
int                   flag; {

	volatile MttDrvrModuleAddress *moad; /* Module address, vector, level, ssch */
	volatile MttDrvrMap           *mmap; /* Module Memory map */

	moad = &(mcon->Address);
	mmap = (MttDrvrMap *) moad->VMEAddress;

	/* Write the UTC second, will become valid on next PPS */

	if (flag) {
		mcon->TimeSet       = time->Second;
		mmap->CommandValue  = mcon->TimeSet;
		mmap->Command       = MttDrvrCommandSET_UTC;
		EIEIO;

		return;
	}

	/* Read back the time from the MTT module */

	time->MilliSecond = mmap->MilliSecond; /* This read causes the seconds to be latched */
	EIEIO;
	time->Second      = mmap->Second;
}

/*========================================================================*/
/* Reset the module                                                       */
/*========================================================================*/

static void Reset(MttDrvrModuleContext *mcon) {

	volatile MttDrvrModuleAddress *moad;  /* Module address, vector, level, ssch */
	volatile MttDrvrMap           *mmap;  /* Module memory map */

	unsigned long isrc, ivec;
	int ps;

	moad = &(mcon->Address);
	mmap =  (MttDrvrMap *) moad->VMEAddress;

	disable(ps); {

		mmap->IrqEnable = 0;       /* Disable all interrupts */
		isrc = mmap->IrqSource;    /* Clear all interrupt sources */
		EIEIO;

		mmap->CommandValue = 1;
		mmap->Command = MttDrvrCommandRESET;
		EIEIO;

		ivec = moad->InterruptVector;
		if (moad->InterruptLevel == 3) ivec |= MttDrvrIrqLevel_3;
		else                           ivec |= MttDrvrIrqLevel_2;
		mmap->IrqLevel = ivec;
		EIEIO;

	} restore(ps);

	/* Clear module context */

	mcon->BusError   = 0; /* No bus errors */
	mcon->FlashOpen  = 0; /* Jtag flash closed */
	mcon->TimeSet    = 0; /* Time the time was set */
	mcon->UtcSending = 0; /* UTC time needs to be set before UTC sending can be enabled */

	/* Set up the module to drivers cashed state */

	if (mcon->EnabledOutput) {
		mmap->CommandValue = 1;
		mmap->Command = MttDrvrCommandENABLE;
		EIEIO;
	}

	if (mcon->External1KHZ) {
		mmap->CommandValue = 1;
		mmap->Command = MttDrvrCommandEXTERNAL_1KHZ;
		EIEIO;
	}

	if (mcon->SyncPeriod) {
		mmap->CommandValue = mcon->SyncPeriod;
		mmap->Command = MttDrvrCommandSET_SYNC_PERIOD;
		EIEIO;
	}

	if (mcon->OutputDelay) {
		mmap->CommandValue = mcon->OutputDelay;
		mmap->Command = MttDrvrCommandSET_OUT_DELAY;
		EIEIO;
	}

	mmap->IrqEnable = mcon->EnabledInterrupts;
	EIEIO;
}

/*========================================================================*/
/* Get status                                                             */
/*========================================================================*/

static unsigned long GetStatus(MttDrvrModuleContext *mcon) {

	volatile MttDrvrModuleAddress *moad;        /* Module address, vector, level, ssch */
	volatile MttDrvrMap           *mmap;        /* Module Memory map */

	unsigned long          stat;

	moad  = &(mcon->Address);
	mmap  = (MttDrvrMap *) moad->VMEAddress;

	/* Read hardware status */

	stat = mmap->Status;

	/* OR in software status bits */

	if (mcon->BusError)  stat |= MttDrvrStatusBUS_FAULT;
	if (mcon->FlashOpen) stat |= MttDrvrStatusFLASH_OPEN;

	return stat;
}

/*========================================================================*/
/* Execute a module command                                               */
/*========================================================================*/

static unsigned long ModCommand(MttDrvrModuleContext *mcon, MttDrvrCommand cmnd, unsigned long value) {

	volatile MttDrvrModuleAddress *moad;  /* Module address, vector, level, ssch */
	volatile MttDrvrMap           *mmap;  /* Module Memory map */

	moad  = &(mcon->Address);
	mmap = (MttDrvrMap *) moad->VMEAddress;

	mmap->CommandValue = value;
	mmap->Command      = cmnd;
	EIEIO;
	return GetStatus(mcon);
}

/*========================================================================*/
/* Add a module to the driver, called per module from install             */
/*========================================================================*/

static int AddModule(mcon,index,flag)
MttDrvrModuleContext *mcon;        /* Module Context */
int index;                         /* Module index (0-based) */
int flag; {                        /* Check again */

	volatile MttDrvrModuleAddress *moad;        /* Module address, vector, level, ssch */
	unsigned long                  addr;        /* VME base address */
	unsigned long                  coco;        /* Completion code */

	struct pdparam_master param; void *vmeaddr; /* For CES PowerPC */

	/* Compute Virtual memory address as seen from system memory mapping */

	moad = &(mcon->Address);

	/* Check module has re-appeared after JTAG modification */

	if (!flag) {

		/* CES: build an address window (64 kbyte) for VME A16-D16 accesses */

		addr = (unsigned long) moad->JTGAddress;
		addr &= 0x0000ffff;                      /* A16 */

		param.iack   = 1;                /* no iack */
		param.rdpref = 0;                /* no VME read prefetch option */
		param.wrpost = 0;                /* no VME write posting option */
		param.swap   = 1;                /* VME auto swap option */
		param.dum[0] = VME_PG_SHARED;    /* window is sharable */
		param.dum[1] = 0;                /* XPC ADP-type */
		param.dum[2] = 0;                /* window is sharable */
		vmeaddr = (void *) find_controller(addr,                    /* Vme base address */
						(unsigned long) 0x10000,  /* Module address space */
						(unsigned long) 0x29,     /* Address modifier A16 */
						0,                        /* Offset */
						2,                        /* Size is D16 */
						&param);                  /* Parameter block */
		if (vmeaddr == (void *)(-1)) {
			cprintf("MttDrvr: find_controller: ERROR: Module:%d. JTAG Addr:%x\n",
				index+1,
				moad->JTGAddress);
			vmeaddr = 0;
		}
		moad->JTGAddress = (unsigned short *) vmeaddr;
	}

	/* CES: build an address window (64 kbyte) for VME A24-D32 accesses */

	addr = (unsigned long) moad->VMEAddress;
	if (addr == 0) addr = (unsigned long) moad->CopyAddress;
	addr &= 0x00ffffff;                      /* A24 */

	param.iack   = 1;                /* no iack */
	param.rdpref = 0;                /* no VME read prefetch option */
	param.wrpost = 0;                /* no VME write posting option */
	param.swap   = 1;                /* VME auto swap option */
	param.dum[0] = VME_PG_SHARED;    /* window is sharable */
	param.dum[1] = 0;                /* XPC ADP-type */
	param.dum[2] = 0;                /* window is sharable */
	vmeaddr = (void *) find_controller(addr,                    /* Vme base address */
					(unsigned long) 0x10000,  /* Module address space */
					(unsigned long) 0x39,     /* Address modifier A24 */
					0,                        /* Offset */
					4,                        /* Size is D32 */
					&param);                  /* Parameter block */
	if (vmeaddr == (void *)(-1)) {
		cprintf("MttDrvr: find_controller: ERROR: Module:%d. VME Addr:%x\n",
			index+1,
			moad->VMEAddress);
		pseterr(ENXIO);        /* No such device or address */

		if (moad->JTGAddress == 0) return 1;
		else                       return 2;
	}
	moad->VMEAddress = (unsigned long *) vmeaddr;

	/* Now set up the interrupt handler */

	coco = vme_intset((char *) (moad->InterruptVector), /* Vector */
			IntrHandler,                      /* Address of ISR */
			mcon,                             /* Parameter to pass */
			0);                               /* Don't save previous */
	if (coco < 0) {
		cprintf("MttDrvr: vme_intset: ERROR %d, MODULE %d\n",coco,index+1);
		pseterr(EFAULT);                                 /* Bad address */
		return 3;
	}

	Reset(mcon); /* Soft reset and initialize module */

	return 0;
}

/*========================================================================*/
/* Cancel a timeout in a safe way                                         */
/*========================================================================*/

static void CancelTimeout(t)
int *t; {

	int ps,v;

	disable(ps);
	{
		if ((v = *t)) {
			*t = 0;
			cancel_timeout(v);
		}
	}
	restore(ps);
}

/*========================================================================*/
/* Handel read timeouts                                                   */
/*========================================================================*/

static int ReadTimeout(ccon)
MttDrvrClientContext *ccon; {

	int ps;
	MttDrvrQueue *queue;

	disable(ps);
	{
		ccon->Timer = 0;

		sreset(&(ccon->Semaphore));
		queue = &(ccon->Queue);
		queue->Size   = 0;
		queue->RdPntr = 0;
		queue->WrPntr = 0;
		queue->Missed = 0;         /* ToDo: What to do with this info ? */
	}
	restore(ps);
	return 0;
}

/*========================================================================*/
/* Connect                                                                */
/*========================================================================*/

static int Connect(ccon, conx)
MttDrvrClientContext *ccon;
MttDrvrConnection    *conx; {

	volatile MttDrvrModuleAddress *moad;        /* Module address, vector, level, ssch */
	volatile MttDrvrMap           *mmap;        /* Module Memory map */
	volatile MttDrvrModuleContext *mcon;

	unsigned long ps, j, imsk, cmsk;

	if (conx->Module == 0) mcon = &(Wa->ModuleContexts[ccon->ModuleIndex]);
	else                   mcon = &(Wa->ModuleContexts[conx->Module -1]);

	moad  = &(mcon->Address);
	mmap  = (MttDrvrMap *) moad->VMEAddress;

	cmsk = 1 << ccon->ClientIndex;
	imsk = conx->ConMask;
	for (j=0; j<MttDrvrINTS; j++) {
		if (imsk & (1<<j)) {
			disable(ps);
			mcon->Connected[j] |= cmsk;
			mcon->EnabledInterrupts |= imsk;
			mmap->IrqEnable = mcon->EnabledInterrupts;
			restore(ps);
		}
	}
	return OK;
}

/*========================================================================*/
/* Disconnect                                                             */
/*========================================================================*/

void DisConnect(ccon, conx)
MttDrvrClientContext *ccon;
MttDrvrConnection    *conx; {

	volatile MttDrvrModuleAddress *moad;        /* Module address, vector, level, ssch */
	volatile MttDrvrMap           *mmap;        /* Module Memory map */
	volatile MttDrvrModuleContext *mcon;

	unsigned long ps, j, imsk, cmsk;

	if (conx->Module == 0)
		mcon = &(Wa->ModuleContexts[ccon->ModuleIndex]); /* Default module selected */
	else
		mcon = &(Wa->ModuleContexts[conx->Module -1]);

	moad = &(mcon->Address);
	mmap = (MttDrvrMap *) moad->VMEAddress;

	cmsk = 1 << ccon->ClientIndex;
	imsk = conx->ConMask;
	for (j=0; j<MttDrvrINTS; j++) {
		if (imsk & (1<<j)) {
			if (mcon->Connected[j] & cmsk) {
				mcon->Connected[j] &= ~cmsk;
				if (mcon->Connected[j] == 0) {
					disable(ps);
					mcon->EnabledInterrupts &= ~imsk;
					mmap->IrqEnable = mcon->EnabledInterrupts;
					restore(ps);
				}
			}
		}
	}
}

/*========================================================================*/
/* Disconnect all a clients conections, used in close                     */
/*========================================================================*/

static void DisConnectAll(MttDrvrClientContext *ccon) {

	MttDrvrConnection conx;
	int i;

	for (i=0; i<Wa->Modules; i++) {
		conx.ConMask = MttDrvrIntMASK;
		conx.Module  = i+1;
		DisConnect(ccon,&conx);
	}
}

/*========================================================================*/
/* Debug an IOCTL call                                                    */
/*========================================================================*/

static void DebugIoctl(ccon,cm,lav,mes)
MttDrvrClientContext  *ccon;
MttDrvrControlFunction cm;
long                    lav;
char                   *mes; {

	if (ccon->DebugOn) {
		if ((cm < 0) || (cm >=  MttDrvrLAST_IOCTL)) {
			cprintf("MttDrvr: Clnt: %d Pid: %d Bad IOCTL command: %d Arg: %d\n",
				(int) ccon->ClientIndex +1,
				(int) ccon->Pid,
				cm,
				(int) lav);
		} else {
			cprintf("MttDrvr: Clnt: %d Pid: %d Mod: %d IOCTL: %s Arg: %d[0x%x] %s\n",
				(int) ccon->ClientIndex +1,
				(int) ccon->Pid,
				(int) ccon->ModuleIndex +1,
				IoctlNames[cm],
				(int) lav,
				(int) lav,
				mes);
		}
	}
}

/***************************************************************************/
/* Interrupt handler                                                       */
/***************************************************************************/

void IntrHandler(mcon)
MttDrvrModuleContext *mcon; {

	volatile MttDrvrMap  *mmap;

	unsigned long         isrc, icnc, imsk, cmsk;
	MttDrvrClientContext *ccon;
	MttDrvrQueue         *queue;
	MttDrvrReadBuf        rb;
	unsigned int          i, j;

	mmap =  (MttDrvrMap *) mcon->Address.VMEAddress;

	isrc = mmap->IrqSource;
	EIEIO;

	rb.Time.MilliSecond = mmap->MilliSecond;
	EIEIO;
	rb.Time.Second      = mmap->Second;

	rb.Connection.Module = mcon->ModuleIndex+1;

	while (isrc) {

		for (i=0; i<MttDrvrINTS; i++) {
			imsk = isrc & (1<<i);
			icnc = mcon->Connected[i];
			if ((imsk) && (icnc)) {
				rb.Connection.ConMask = imsk;

				for (j=0; j<MttDrvrCLIENT_CONTEXTS; j++) {
					cmsk = icnc & (1<<j);
					if (cmsk) {
						ccon = &(Wa->ClientContexts[j]);
						queue = &(ccon->Queue);
						queue->Entries[queue->WrPntr] = rb;
						queue->WrPntr = (queue->WrPntr + 1) % MttDrvrQUEUE_SIZE;
						if (queue->Size < MttDrvrQUEUE_SIZE) {
							queue->Size++;
							ssignal(&(ccon->Semaphore));
						} else {
							queue->Missed++;
							queue->RdPntr = (queue->RdPntr + 1) % MttDrvrQUEUE_SIZE;
						}
					}
					if (!(icnc &= ~cmsk)) break;
				}
				if (!(isrc &= ~imsk)) break;
			}
		}
		isrc = mmap->IrqSource;
	}
}

/***************************************************************************/
/* INSTALL                                                                 */
/***************************************************************************/

char * MttDrvrInstall(info)
MttDrvrInfoTable * info; {      /* Driver info table */

	MttDrvrWorkingArea *wa;

	MttDrvrModuleContext          *mcon;
	volatile MttDrvrModuleAddress *moad;
	int                            i, erlv;

	/* Allocate the driver working area. */

	wa = (MttDrvrWorkingArea *) sysbrk(sizeof(MttDrvrWorkingArea));
	if (!wa) {
		cprintf("MttDrvrInstall: NOT ENOUGH MEMORY(WorkingArea)\n");
		pseterr(ENOMEM);          /* Not enough core */
		return((char *) SYSERR);
	}
	Wa = wa;                     /* Global working area pointer */

	/****************************************/
	/* Initialize the driver's working area */
	/* and add the modules ISR into LynxOS  */
	/****************************************/

	bzero((void *) wa,sizeof(MttDrvrWorkingArea));          /* Clear working area */

	for (i=0; i<info->Modules; i++) {
		mcon = &(wa->ModuleContexts[i]);
		moad = &mcon->Address;
		*moad = info->Addresses[i];
		mcon->ModuleIndex = i;
		erlv = AddModule(mcon,i,0);
		if (erlv == 0) {
			cprintf("MttDrvr: Module %d. VME Addr: %x JTAG Addr: %x Vect: %x Level: %x Installed OK\n",
				i+1,
				moad->VMEAddress,
				moad->JTGAddress,
				moad->InterruptVector,
				moad->InterruptLevel);
		} else if (erlv == 2) {
			moad->VMEAddress = 0;
			cprintf("MttDrvr: Module: %d WARNING: JTAG Only: JTAG Addr: %x\n",
				moad->JTGAddress);
		} else {
			moad->VMEAddress = 0;
			moad->JTGAddress = 0;
			cprintf("MttDrvr: Module: %d ERROR: Not Installed\n",i+1);
		}
	}
	wa->Modules = info->Modules;;

	cprintf("MttDrvr: Installed: %d MTT Modules in Total\n",wa->Modules);
	return((char*) wa);
}

/***************************************************************************/
/* UNINSTALL (Not suported)                                                */
/***************************************************************************/

int MttDrvrUninstall(wa)
MttDrvrWorkingArea * wa; {     /* Drivers working area pointer */

	cprintf("MttDrvr: UnInstalled\n");
	sysfree((void *) wa,sizeof(MttDrvrWorkingArea)); Wa = NULL;
	return OK;
}

/***************************************************************************/
/* OPEN                                                                    */
/***************************************************************************/

int MttDrvrOpen(wa, dnm, flp)
MttDrvrWorkingArea * wa;       /* Working area */
int dnm;                        /* Device number */
struct file * flp; {            /* File pointer */

	int cnum;                       /* Client number */
	MttDrvrClientContext * ccon;   /* Client context */

	/* We allow one client per minor device, we use the minor device */
	/* number as an index into the client contexts array. */

	cnum = minor(flp->dev) -1;
	if ((cnum < 0) || (cnum >= MttDrvrCLIENT_CONTEXTS)) {
		pseterr(EFAULT); /* EFAULT = "Bad address" */
		return SYSERR;
	}
	ccon = &(wa->ClientContexts[cnum]);

	/* If already open by someone else, give a permission denied error */

	if (ccon->InUse) {

		/* This next error is normal */

		pseterr(EBUSY); /* EBUSY = "Resource busy" File descriptor already open */
		return SYSERR;
	}

	/* Initialize a client context */

	bzero((void *) ccon,(long) sizeof(MttDrvrClientContext));
	ccon->ClientIndex = cnum;
	ccon->Timeout     = MttDrvrDEFAULT_TIMEOUT;
	ccon->InUse       = 1;
	ccon->Pid         = getpid();
	return OK;
}

/***************************************************************************/
/* CLOSE                                                                   */
/***************************************************************************/

int MttDrvrClose(wa, flp)
MttDrvrWorkingArea * wa;         /* Working area */
struct file * flp; {             /* File pointer */

	int cnum;                        /* Client number */
	MttDrvrClientContext     * ccon; /* Client context */

	/* We allow one client per minor device, we use the minor device */
	/* number as an index into the client contexts array.            */

	cnum = minor(flp->dev) -1;
	if ((cnum < 0) || (cnum >= MttDrvrCLIENT_CONTEXTS)) {
		pseterr(EFAULT); /* EFAULT = "Bad address" */
		return SYSERR;
	}
	ccon = &(Wa->ClientContexts[cnum]);

	CancelTimeout(&ccon->Timer);
	DisConnectAll(ccon);

	if (ccon->DebugOn)
		cprintf("MttDrvr: CLOSE client PID: %d\n",ccon->Pid);

	bzero((void *) ccon,sizeof(MttDrvrClientContext));

	return(OK);
}

/***************************************************************************/
/* IOCTL                                                                   */
/***************************************************************************/

int MttDrvrIoctl(wa, flp, cm, arg)
MttDrvrWorkingArea * wa;       /* Working area */
struct file * flp;             /* File pointer */
MttDrvrControlFunction cm;     /* IOCTL command */
char * arg; {                  /* Data for the IOCTL */

	MttDrvrModuleContext        *mcon;   /* Module context */
	MttDrvrClientContext        *ccon;   /* Client context */
	MttDrvrConnection           *conx;
	MttDrvrVersion              *ver;
	MttDrvrClientList           *cls;
	MttDrvrClientConnections    *ccn;
	MttDrvrRawIoBlock           *riob;
	MttDrvrModuleAddress        *moad;
	MttDrvrTime                 *mutc;
	MttDrvrTaskBuf              *tbuf;
	MttDrvrGlobalRegBuf         *gbuf;
	MttDrvrTaskRegBuf           *lbuf;
	MttDrvrProgramBuf           *ibuf;

	volatile MttDrvrTaskBlock   *tcbp;
	volatile MttDrvrMap         *mmap;
	volatile MttDrvrInstruction *iptr;

	int i, j, tnum, rnum;
	int cnum;                 /* Client number */
	long lav, *lap;           /* Long Value pointed to by Arg */
	unsigned short sav, sval; /* Short argument and for Jtag IO */
	unsigned long isze, msk;  /* Size of LongCopy, bit mask */
	int rcnt, wcnt;           /* Readable, Writable byte counts at arg address */

	unsigned long cmsk, tmsk, lmsk;

	/* Check argument contains a valid address for reading or writing. */
	/* We can not allow bus errors to occur inside the driver due to   */
	/* the caller providing a garbage address in "arg". So if arg is   */
	/* not null set "rcnt" and "wcnt" to contain the byte counts which */
	/* can be read or written to without error. */

	if (arg != NULL) {
		rcnt = rbounds((int)arg);       /* Number of readable bytes without error */
		wcnt = wbounds((int)arg);       /* Number of writable bytes without error */
		if (rcnt < sizeof(long)) {      /* We at least need to read one long */
			cprintf("MttDrvrIoctl: ILLEGAL NON NULL ARG POINTER, RCNT=%d/%d\n",
				rcnt,sizeof(long));
			pseterr(EINVAL);        /* Invalid argument */
			return SYSERR;
		}
		lav = *((long *) arg);       /* Long argument value */
		lap =   (long *) arg ;       /* Long argument pointer */
	} else {
		rcnt = 0; wcnt = 0; lav = 0; lap = NULL; /* Null arg = zero read/write counts */
	}
	sav = (unsigned short) lav;     /* Short argument value */

	/* We allow one client per minor device, we use the minor device */
	/* number as an index into the client contexts array. */

	cnum = minor(flp->dev) -1;
	if ((cnum < 0) || (cnum >= MttDrvrCLIENT_CONTEXTS)) {
		pseterr(ENODEV);          /* No such device */
		return SYSERR;
	}

	/* We can't control a file which is not open. */

	ccon = &(wa->ClientContexts[cnum]);
	if (ccon->InUse == 0) {
		cprintf("MttDrvrIoctl: DEVICE %2d IS NOT OPEN\n",cnum+1);
		pseterr(EBADF);           /* Bad file number */
		return SYSERR;
	}

	/* Set up some useful module pointers */

	mcon = &(wa->ModuleContexts[ccon->ModuleIndex]); /* Default module selected */
	moad = &(mcon->Address);
	mmap = (MttDrvrMap *) moad->VMEAddress;

	DebugIoctl(ccon,cm,lav,"Calling");

	/*************************************/
	/* Decode callers command and do it. */
	/*************************************/

	switch (cm) {

	case MttDrvrSET_SW_DEBUG:            /* Set driver debug mode */
		if (lav) ccon->DebugOn = 1;
		else     ccon->DebugOn = 0;
		DebugIoctl(ccon,cm,ccon->DebugOn,"SetDebug");
		return OK;

	case MttDrvrGET_SW_DEBUG:
		if (lap) {
			*lap = ccon->DebugOn;
			DebugIoctl(ccon,cm,ccon->DebugOn,"GetDebug");
			return OK;
		}
		break;

	case MttDrvrGET_VERSION:
		if (wcnt >= sizeof(MttDrvrVersion)) {
			if (PingModule(mcon) != OK)  return SYSERR; /* Bus Error */
			ver = (MttDrvrVersion *) arg;
			ver->VhdlVersion = 0;
			ver->VhdlVersion = GetVhdlVersion(mcon);
			ver->DrvrVersion = GetDrvrVersion();
			DebugIoctl(ccon,cm,ver->DrvrVersion,"GetDriverVersion");
			DebugIoctl(ccon,cm,ver->VhdlVersion,"GetVHDLVersion");
			return OK;
		}
		break;

	case MttDrvrSET_TIMEOUT:
		ccon->Timeout = lav;
		DebugIoctl(ccon,cm,lav,"SetTimeout");
		return OK;

	case MttDrvrGET_TIMEOUT:
		if (lap) {
			*lap = ccon->Timeout;
			DebugIoctl(ccon,cm,ccon->Timeout,"GetTimeout");
			return OK;
		}
		break;

	case MttDrvrSET_QUEUE_FLAG:          /* Set queuing capabiulities on off */
		if (lav) ccon->Queue.QueueOff = 1;
		else     ccon->Queue.QueueOff = 0;
		DebugIoctl(ccon,cm,ccon->Queue.QueueOff,"SetQFlag");
		return OK;

	case MttDrvrGET_QUEUE_FLAG:
		if (lap) {
			*lap = ccon->Queue.QueueOff;
			DebugIoctl(ccon,cm,ccon->Queue.QueueOff,"GetQFlag");
			return OK;
		}
		break;

	case MttDrvrGET_QUEUE_SIZE:
		if (lap) {
			*lap = ccon->Queue.Size;
			DebugIoctl(ccon,cm,ccon->Queue.Size,"GetQSize");
			return OK;
		}
		break;

	case MttDrvrGET_QUEUE_OVERFLOW:
		if (lap) {
			*lap = ccon->Queue.Missed;
			DebugIoctl(ccon,cm,ccon->Queue.Missed,"GetQOver");
			ccon->Queue.Missed = 0;
			return OK;
		}
		break;

	case MttDrvrSET_MODULE:
		if ((lav >= 1) && (lav <= Wa->Modules)) {
			ccon->ModuleIndex = lav -1;
			DebugIoctl(ccon,cm,ccon->ModuleIndex,"SetModuleIndex");
			return OK;
		}
		break;

	case MttDrvrGET_MODULE:
		if (lap) {
			*lap = ccon->ModuleIndex +1;
			DebugIoctl(ccon,cm,ccon->ModuleIndex,"GetModuleIndex");
			return OK;
		}
		break;

	case MttDrvrGET_MODULE_COUNT:
		if (lap) {
			*lap = Wa->Modules;
			DebugIoctl(ccon,cm,Wa->Modules,"GetModuleCount");
			return OK;
		}
		break;

	case MttDrvrGET_MODULE_ADDRESS:
		if (wcnt >= sizeof(MttDrvrModuleAddress)) {
			moad = (MttDrvrModuleAddress *) arg;
			*moad = mcon->Address;
			DebugIoctl(ccon,cm,(long) moad->VMEAddress,"GetModuleAddress");
			return OK;
		}
		break;

	case MttDrvrSET_MODULE_CABLE_ID:
		mcon->CableId = lav;
		DebugIoctl(ccon,cm,mcon->CableId,"SetCableId");
		return OK;

	case MttDrvrGET_MODULE_CABLE_ID:           /* Cables telegram ID */
		if (lap) {
			*lap = mcon->CableId;
			DebugIoctl(ccon,cm,mcon->CableId,"GetCableId");
			return OK;
		}
		break;

	case MttDrvrCONNECT:
		if (rcnt >= sizeof(MttDrvrConnection)) {
			conx = (MttDrvrConnection *) arg;
			DebugIoctl(ccon,cm,conx->ConMask,"Connect");
			if (conx->ConMask) return Connect(ccon, conx);
			DisConnectAll(ccon);
			return OK;
		}
		break;

	case MttDrvrGET_CLIENT_LIST:
		if (wcnt >= sizeof(MttDrvrClientList)) {
			cls = (MttDrvrClientList *) lap;
			bzero((void *) cls, sizeof(MttDrvrClientList));
			for (i=0; i<MttDrvrCLIENT_CONTEXTS; i++) {
				ccon = &(wa->ClientContexts[i]);
				if (ccon->InUse) {
					cls->Pid[cls->Size] = ccon->Pid;
					cls->Size++;
				}
			}
			DebugIoctl(ccon,cm,cls->Size,"ClientListSize");
			return OK;
		}
		break;

	case MttDrvrGET_CLIENT_CONNECTIONS:
		if (wcnt >= sizeof(MttDrvrClientConnections)) {
			ccn = (MttDrvrClientConnections *) arg;
			if (ccn->Pid == 0) ccn->Pid = ccon->Pid;
			ccn->Size = 0;

			cmsk = 0;
			for (j=0; j<MttDrvrCLIENT_CONTEXTS; j++) {
				ccon = &(wa->ClientContexts[j]);
				if (ccn->Pid == ccon->Pid) {
					cmsk = 1<<j;
					break;
				}
			}
			if (cmsk == 0) break;

			for (i=0; i<MttDrvrMODULE_CONTEXTS; i++) {
				mcon = &(Wa->ModuleContexts[i]);
				for (j=0; j<MttDrvrINTS; j++) {
					if (mcon->Connected[j] & cmsk) {
						ccn->Connections[ccn->Size].Module  = i+1;
						ccn->Connections[ccn->Size].ConMask = 1<<j;
						if (++(ccn->Size) >= MttDrvrCONNECTIONS) break;
					}
				}
			}
			ccon = &(wa->ClientContexts[cnum]);
			DebugIoctl(ccon,cm,ccn->Pid,"GetPidConnection");
			return OK;
		}
		break;

	case MttDrvrENABLE:
		if (lav) mcon->EnabledOutput = 1;
		else     mcon->EnabledOutput = 0;
		ModCommand(mcon,MttDrvrCommandENABLE,mcon->EnabledOutput);
		DebugIoctl(ccon,cm,mcon->EnabledOutput,"SetEnable");
		return OK;

	case MttDrvrRESET:
		Reset(mcon);
		DebugIoctl(ccon,cm,1,"Reset");
		return OK;

	case MttDrvrUTC_SENDING:
		if (lav) mcon->UtcSending = 1;
		else     mcon->UtcSending = 0;
		ModCommand(mcon,MttDrvrCommandUTC_SENDING_ON,mcon->UtcSending);
		DebugIoctl(ccon,cm,mcon->UtcSending,"SetUTCSending");
		return OK;

	case MttDrvrEXTERNAL_1KHZ:
		if (lav) mcon->External1KHZ = 1;
		else     mcon->External1KHZ = 0;
		ModCommand(mcon,MttDrvrCommandEXTERNAL_1KHZ,mcon->External1KHZ);
		DebugIoctl(ccon,cm,mcon->External1KHZ,"SetExternal1KHz");
		return OK;

	case MttDrvrSET_OUTPUT_DELAY:
		mcon->OutputDelay = lav;
		ModCommand(mcon,MttDrvrCommandSET_OUT_DELAY,mcon->OutputDelay);
		DebugIoctl(ccon,cm,mcon->OutputDelay,"SetOutputDelay");
		return OK;

	case MttDrvrGET_OUTPUT_DELAY:
		if (lap) {
			*lap = mcon->OutputDelay;
			DebugIoctl(ccon,cm,mcon->OutputDelay,"GetOutputDelay");
			return OK;
		}
		break;

	case MttDrvrSET_SYNC_PERIOD:
		if (lav) {
			mcon->SyncPeriod = lav;
			ModCommand(mcon,MttDrvrCommandSET_SYNC_PERIOD,mcon->SyncPeriod);
			DebugIoctl(ccon,cm,mcon->SyncPeriod,"SetSyncPeriod");
			return OK;
		}
		break;

	case MttDrvrSEND_EVENT:
		if (lav) {
			ModCommand(mcon,MttDrvrCommandSEND_EVENT,lav);
			DebugIoctl(ccon,cm,lav,"SendEvent");
			return OK;
		}
		break;

	case MttDrvrGET_SYNC_PERIOD:
		if (lap) {
			*lap = mcon->SyncPeriod;
			DebugIoctl(ccon,cm,mcon->SyncPeriod,"GetSyncPeriod");
			return OK;
		}
		break;

	case MttDrvrSET_UTC:
		if (rcnt >= sizeof(MttDrvrTime)) {
			mutc = (MttDrvrTime *) arg;
			GetSetUtc(mcon,mutc,1);
			DebugIoctl(ccon,cm,mutc->Second,"SetUtcTime");
			return OK;
		}
		break;

	case MttDrvrGET_UTC:
		if (wcnt >= sizeof(MttDrvrTime)) {
			mutc = (MttDrvrTime *) arg;
			GetSetUtc(mcon,mutc,0);
			DebugIoctl(ccon,cm,mutc->Second,"GetUtcTime");
			return OK;
		}
		break;

	case MttDrvrGET_STATUS:
		if (lap) {
			if (PingModule(mcon) != OK) return SYSERR; /* Bus Error */
			*lap = GetStatus(mcon);
			DebugIoctl(ccon,cm,*lap,"GetStatus");
			return OK;
		}
		break;

	case MttDrvrSTART_TASKS:
		lav &= MttDrvrTASK_MASK;
		if (lav) {
			for (i=0; i<MttDrvrTASKS; i++) {
				tmsk = 1<<i;
				if (lav & tmsk) {
					mmap->StopTasks = tmsk;
					tcbp = &(mmap->Tasks[i]);
					tcbp->Pc = mcon->Tasks[i].PcStart;
					mmap->StartTasks = tmsk;
				}
			}
			DebugIoctl(ccon,cm,lav,"StartedTasks");
			return OK;
		}
		break;

	case MttDrvrSTOP_TASKS:
		lav &= MttDrvrTASK_MASK;
		if (lav) {
			mmap->StopTasks = lav;
			DebugIoctl(ccon,cm,lav,"StoppedTasks");
			return OK;
		}
		break;

	case MttDrvrCONTINUE_TASKS:
		lav &= MttDrvrTASK_MASK;
		if (lav) {
			mmap->StartTasks = lav;
			DebugIoctl(ccon,cm,lav,"ContinuedTasks");
			return OK;
		}
		break;

	case MttDrvrGET_RUNNING_TASKS:
		if (lap) {
			tmsk = mmap->RunningTasks & MttDrvrTASK_MASK;
			*lap = tmsk;
			DebugIoctl(ccon,cm,tmsk,"RunningTasks");
			return OK;
		}
		break;

	case MttDrvrSET_TASK_CONTROL_BLOCK:
		if (wcnt >= sizeof(MttDrvrTaskBuf)) {
			tbuf = (MttDrvrTaskBuf *) arg;
			tnum = tbuf->Task;
			if ((tnum > 0) && (tnum <= MttDrvrTASKS)) {
				for (i=0; i<MttDrvrTBFbits; i++) {
					msk = 1 << i;
					if (msk & tbuf->Fields) {
						switch ((MttDrvrTBF) msk) {
						case MttDrvrTBFLoadAddress:
							mcon->Tasks[tnum -1].LoadAddress = tbuf->LoadAddress;
							break;

						case MttDrvrTBFInstructionCount:
							mcon->Tasks[tnum -1].InstructionCount = tbuf->InstructionCount;
							break;

						case MttDrvrTBFPcStart:
							mcon->Tasks[tnum -1].PcStart = tbuf->PcStart;
							break;

						case MttDrvrTBFPcOffset:
							mcon->Tasks[tnum -1].PcOffset = tbuf->ControlBlock.PcOffset;
							mmap->Tasks[tnum -1].PcOffset = tbuf->ControlBlock.PcOffset;
							break;

						case MttDrvrTBFPc:
							mcon->Tasks[tnum -1].Pc = tbuf->ControlBlock.Pc;
							mmap->Tasks[tnum -1].Pc = tbuf->ControlBlock.Pc;
							break;

						case MttDrvrTBFName:
							strncpy(mcon->Tasks[tnum -1].Name,
								tbuf->Name,
								MttDrvrNameSIZE -1);

						default: break;
						}
					}
				}
				return OK;
			}
		}
		break;

	case MttDrvrGET_TASK_CONTROL_BLOCK:
		if (wcnt >= sizeof(MttDrvrTaskBuf)) {
			tbuf = (MttDrvrTaskBuf *) arg;
			tnum = tbuf->Task;
			if ((tnum > 0) && (tnum <= MttDrvrTASKS)) {
				tcbp = &(mmap->Tasks[tnum -1]);
				tbuf->LoadAddress      = mcon->Tasks[tnum -1].LoadAddress;
				tbuf->InstructionCount = mcon->Tasks[tnum -1].InstructionCount;
				tbuf->PcStart          = mcon->Tasks[tnum -1].PcStart;
				strncpy(tbuf->Name,mcon->Tasks[tnum -1].Name,MttDrvrNameSIZE -1);
				LongCopy((unsigned long *) &(tbuf->ControlBlock),
					(unsigned long *) &(mmap->Tasks[tnum -1]),
					(unsigned long  ) sizeof(MttDrvrTaskBlock));
				tbuf->Fields = MttDrvrTBFAll;
				DebugIoctl(ccon,cm,tnum,"GetTaskControlBlock");
				return OK;
			}
		}
		break;

	case MttDrvrSET_GLOBAL_REG_VALUE:
		if (rcnt >= sizeof(MttDrvrGlobalRegBuf)) {
			gbuf = (MttDrvrGlobalRegBuf *) arg;
			rnum = gbuf->RegNum;
			if (rnum < MttDrvrGRAM_SIZE) {
				mmap->GlobalMem[rnum] = gbuf->RegVal;
				DebugIoctl(ccon,cm,rnum,"SetGlobalRegister");
				return OK;
			}
		}
		break;

	case MttDrvrGET_GLOBAL_REG_VALUE:
		if (wcnt >= sizeof(MttDrvrGlobalRegBuf)) {
			gbuf = (MttDrvrGlobalRegBuf *) arg;
			rnum = gbuf->RegNum;
			if (rnum < MttDrvrGRAM_SIZE) {
				gbuf->RegVal = mmap->GlobalMem[rnum];
				DebugIoctl(ccon,cm,rnum,"GetGlobalRegister");
				return OK;
			}
		}
		break;

	case MttDrvrSET_TASK_REG_VALUE:
		if (rcnt >= sizeof(MttDrvrTaskRegBuf)) {
			lbuf = (MttDrvrTaskRegBuf *) arg;
			tnum = lbuf->Task;
			if ((tnum > 0) && (tnum <= MttDrvrTASKS)) {
				DebugIoctl(ccon,cm,tnum,"SetRegistersTask");
				for (i=0; i<MttDrvrLRAM_SIZE; i++) {
					lmsk = 1<<i;
					if (lmsk & lbuf->RegMask) {
						mmap->LocalMem[tnum -1][i] = lbuf->RegVals[i];
						return OK;
					}
				}
			}
		}
		break;

	case MttDrvrGET_TASK_REG_VALUE:
		if (wcnt >= sizeof(MttDrvrTaskRegBuf)) {
			lbuf = (MttDrvrTaskRegBuf *) arg;
			tnum = lbuf->Task;
			if ((tnum > 0) && (tnum <= MttDrvrTASKS)) {
				DebugIoctl(ccon,cm,tnum,"GetRegistersTask");
				for (i=0; i<MttDrvrLRAM_SIZE; i++)
					lbuf->RegVals[i] = mmap->LocalMem[tnum -1][i];
				lbuf->RegMask = 0xFFFFFFFF;
				return OK;
			}
		}
		break;

	case MttDrvrSET_PROGRAM:
		if (rcnt >= sizeof(MttDrvrProgramBuf)) {
			ibuf = (MttDrvrProgramBuf *) arg;
			iptr = &(mmap->ProgramMem[ibuf->LoadAddress]);
			isze = ibuf->InstructionCount*sizeof(MttDrvrInstruction);
			LongCopy((unsigned long *) iptr,
				(unsigned long *) ibuf->Program,
				(unsigned long  ) isze);
			DebugIoctl(ccon,cm,ibuf->LoadAddress,"SetProgramAddr");
			return OK;
		}
		break;

	case MttDrvrGET_PROGRAM:            /* Read program code back from program memory */
		if (wcnt >= sizeof(MttDrvrProgramBuf)) {
			ibuf = (MttDrvrProgramBuf *) arg;
			iptr = &(mmap->ProgramMem[ibuf->LoadAddress]);
			isze = ibuf->InstructionCount*sizeof(MttDrvrInstruction);
			LongCopy((unsigned long *) ibuf->Program,
				(unsigned long *) iptr,
				(unsigned long  ) isze);
			DebugIoctl(ccon,cm,ibuf->LoadAddress,"GetProgramAddr");
			return OK;
		}
		break;

	case MttDrvrJTAG_OPEN:
		if (mcon->Address.JTGAddress != NULL) {
			mcon->FlashOpen = 1;
			return OK;
		}
		mcon->FlashOpen = 0;
		cprintf("mttdrvr: Can not open Jtag port: Hardware not present\n");
		break;

	case MttDrvrJTAG_READ_BYTE:
		if (mcon->FlashOpen) {
			sval = *(mcon->Address.JTGAddress);
			if (ccon->DebugOn >=2)
				cprintf("Jtag: Addr: %x Read %x\n",(int) mcon->Address.JTGAddress,(int) sval);
			*lap = (0x000000FF & (unsigned long) sval);
			return OK;
		}
		pseterr(EBUSY);                    /* Device busy, not opened */
		return SYSERR;

	case MttDrvrJTAG_WRITE_BYTE:         /* Up date configuration bit stream accross JTAG */
		if (mcon->FlashOpen) {
			sval = (short) lav;
			*(mcon->Address.JTGAddress) = sval;
			if (ccon->DebugOn >=2)
				cprintf("Jtag: Addr: %x Write %x\n",(int) mcon->Address.JTGAddress,(int) sval);
			return OK;
		}
		pseterr(EBUSY);                    /* Device busy, not opened */
		return SYSERR;

	case MttDrvrJTAG_CLOSE:
		mcon->FlashOpen = 0;
		Reset(mcon);
		return OK;

	case MttDrvrRAW_READ:
		if (wcnt >= sizeof(MttDrvrRawIoBlock)) {
			riob = (MttDrvrRawIoBlock *) arg;
			if ((riob->UserArray != NULL)
				&&  (wcnt > riob->Size * sizeof(unsigned long))) {
				return RawIo(mcon,riob,0,ccon->DebugOn);
			}
		}
		break;

	case MttDrvrRAW_WRITE:
		if (rcnt >= sizeof(MttDrvrRawIoBlock)) {
			riob = (MttDrvrRawIoBlock *) arg;
			if ((riob->UserArray != NULL)
				&&  (rcnt > riob->Size * sizeof(unsigned long))) {
				return RawIo(mcon,riob,1,ccon->DebugOn);
			}
		}
		break;

	default:
		break;
	}

	DebugIoctl(ccon,cm,lav,"Error return");

	/* EINVAL = "Invalid argument" */

	pseterr(EINVAL);                           /* Caller error */
	return SYSERR;
}

/***************************************************************************/
/* READ                                                                    */
/***************************************************************************/

int MttDrvrRead(wa, flp, u_buf, cnt)
MttDrvrWorkingArea * wa;       /* Working area */
struct file * flp;              /* File pointer */
char * u_buf;                   /* Users buffer */
int cnt; {                      /* Byte count in buffer */

	MttDrvrClientContext *ccon;    /* Client context */
	MttDrvrQueue         *queue;
	MttDrvrReadBuf       *rb;
	int                    cnum;    /* Client number */
	int                    wcnt;    /* Writable byte counts at arg address */
	int                    ps;

	if (u_buf != NULL) {
		wcnt = wbounds((int)u_buf);           /* Number of writable bytes without error */
		if (wcnt < sizeof(MttDrvrReadBuf)) {
			pseterr(EINVAL);
			return 0;
		}
	}

	cnum = minor(flp->dev) -1;
	ccon = &(wa->ClientContexts[cnum]);

	queue = &(ccon->Queue);
	if (queue->QueueOff) {
		disable(ps);
		{
			queue->Size   = 0;
			queue->RdPntr = 0;
			queue->WrPntr = 0;
			queue->Missed = 0;         /* ToDo: What to do with this info ? */
			sreset(&(ccon->Semaphore));
		}
		restore(ps);
	}

	if (ccon->Timeout) {
		ccon->Timer = timeout(ReadTimeout, (char *) ccon, ccon->Timeout);
		if (ccon->Timer < 0) {

			ccon->Timer = 0;

			/* EBUSY = "Device or resource busy" */

			pseterr(EBUSY);    /* No available timers */
			return 0;
		}
	}

	if (swait(&(ccon->Semaphore), SEM_SIGABORT)) {

		/* EINTR = "Interrupted system call" */

		pseterr(EINTR);   /* We have been signaled */
		return 0;
	}

	if (ccon->Timeout) {
		if (ccon->Timer) {
			CancelTimeout(&(ccon->Timer));
		} else {

			/* ETIME = "Timer expired */

			pseterr(ETIME);
			return 0;
		}
	}

	rb = (MttDrvrReadBuf *) u_buf;
	if (queue->Size) {
		disable(ps);
		{
			*rb = queue->Entries[queue->RdPntr];
			queue->RdPntr = (queue->RdPntr + 1) % MttDrvrQUEUE_SIZE;
			queue->Size--;
		}
		restore(ps);
		return sizeof(MttDrvrReadBuf);
	}

	pseterr(EINTR);
	return 0;
}

/***************************************************************************/
/* WRITE (Simulates Interrupts)                                            */
/***************************************************************************/

int MttDrvrWrite(wa, flp, u_buf, cnt)
MttDrvrWorkingArea * wa;       /* Working area */
struct file * flp;             /* File pointer */
char * u_buf;                  /* Users buffer */
int cnt; {                     /* Byte count in buffer */

	MttDrvrClientContext *ccon;
	MttDrvrModuleContext *mcon;
	MttDrvrQueue         *queue;
	MttDrvrConnection    *conx;
	MttDrvrReadBuf        rb;
	int                   rcnt;
	int                   ps;
	int                   i, j, midx;
	unsigned long         imsk, cmsk;

	if (u_buf != NULL) {
		rcnt = rbounds((int)u_buf);           /* Number of readable bytes without error */
		if (rcnt < sizeof(MttDrvrConnection)) {
			pseterr(EINVAL);
			return 0;
		}
	}
	conx = (MttDrvrConnection *) u_buf;

	midx = conx->Module -1;
	if ((midx<0) || (midx >= wa->Modules)) {
		pseterr(EINVAL);
		return 0;
	}
	mcon = &(wa->ModuleContexts[midx]);

	bzero((void *) &rb,sizeof(MttDrvrReadBuf));

	/* Place read buffer on connected clients queues */

	rb.Connection = *conx;
	GetSetUtc(mcon,&rb.Time,0);

	for (i=0; i<32; i++) {
		imsk = 1<<i;
		if (imsk & conx->ConMask) {
			cmsk = mcon->Connected[i];
			for (j=0; j<MttDrvrCLIENT_CONTEXTS; j++) {
				if (cmsk & (1<<j)) {
					ccon = &(Wa->ClientContexts[j]);
					queue = &(ccon->Queue);

					disable(ps); {
						queue->Entries[queue->WrPntr] = rb;
						queue->WrPntr = (queue->WrPntr + 1) % MttDrvrQUEUE_SIZE;
						if (queue->Size < MttDrvrQUEUE_SIZE) {
							queue->Size++;
							ssignal(&(ccon->Semaphore));
						} else {
							queue->Missed++;
							queue->RdPntr = (queue->RdPntr + 1) % MttDrvrQUEUE_SIZE;
						}
					} restore(ps);
				}
			}
		}
	}
	return sizeof(MttDrvrConnection);
}

/***************************************************************************/
/* SELECT                                                                  */
/***************************************************************************/

int MttDrvrSelect(wa, flp, wch, ffs)
MttDrvrWorkingArea * wa;       /* Working area */
struct file * flp;              /* File pointer */
int wch;                        /* Read/Write direction */
struct sel * ffs; {             /* Selection structurs */

	MttDrvrClientContext * ccon;
	int cnum;

	cnum = minor(flp->dev) -1;
	ccon = &(wa->ClientContexts[cnum]);

	if (wch == SREAD) {
		ffs->iosem = (int *) &(ccon->Semaphore); /* Watch out here I hope   */
		return OK;                               /* the system dosn't swait */
	}                                           /* the read does it too !! */

	pseterr(EACCES);         /* Permission denied */
	return SYSERR;
}

/*************************************************************/
/* Dynamic loading information for driver install routine.   */
/*************************************************************/

struct dldd entry_points = {
	MttDrvrOpen,    MttDrvrClose,
	MttDrvrRead,    MttDrvrWrite,
	MttDrvrSelect,  MttDrvrIoctl,
	MttDrvrInstall, MttDrvrUninstall
};
