/**************************************************************************/
/* MTT Library routines to support FESA class                             */
/* Fri 20th October 2006                                                  */
/* Julian Lewis AB/CO/HT                                                  */
/**************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>        /* Error numbers */
#include <sys/file.h>
#include <a.out.h>
#include <ctype.h>
#include <mqueue.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

#include <mttdrvr.h>
#include "libmtt.h"

/* ================================================================ */
/* Globals                                                          */

#define LN 128
#define MS100 100000

static int mtt = 0; /* File handle on the module */
static char objdir[LN]; /* Object directory */
static MttDrvrInt connected = 0; /* Connected interrupts */
static int noqueueflag = 0; /* Queueing ON */
static int timeout = 200; /* 2 Seconds */
static int first_task = 1; /* First task number TCB */
static int last_task = MttLibTASKS; /* Last task number */
static int max_size = MttLibTASK_SIZE; /* Max task size allowed */

static char *errstrings[MttLibERRORS] = {

/*  MttLibErrorNONE,   */"No error, all OK",
/*  MttLibErrorINIT,   */"The MTT library has not been initialized",
/*  MttLibErrorOPEN,   */"Unable to open the MTT driver",
/*  MttLibErrorIO,     */"IO error, see errno",
/*  MttLibErrorSYS,    */"Operating system error, see errno",
/*  MttLibErrorPATH,   */"Bad path name syntax",
/*  MttLibErrorFILE,   */"Could not find file, see errno",
/*  MttLibErrorREAD,   */"Can not read file, or file empty",
/*  MttLibErrorNOMEM,  */"Not enough memory",
/*  MttLibErrorNORELO, */"Task object binary is not relocatable",
/*  MttLibErrorTOOBIG, */"Task size is too big",
/*  MttLibErrorEMPTY,  */"Object file is empty",
/*  MttLibErrorFULL,   */"No more tasks can be loaded, full up",
/*  MttLibErrorNOLOAD, */"No such task is loaded",
/*  MttLibErrorISLOAD, */"Task is already loaded",
/*  MttLibErrorNAME,   */"Illegal task name",
/*  MttLibErrorLREG,   */"No such local register",
/*  MttLibErrorGREG,   */"No such global register"

};

/* =========================== */
/* Open MTT driver file handel */
static int MttOpen() {

	char fnm[32];
	int i, fn;

	for (i = 1; i <= SkelDrvrCLIENT_CONTEXTS; i++) {
		sprintf(fnm, "/dev/%s.%1d", "mtt", i);
		if ((fn = open(fnm, O_RDWR, 0)) > 0)
			return (fn);
	};
	return (0);
}

/* =========================== */

void MttLibUsleep(int dly) {
	struct timespec rqtp, rmtp; /* 'nanosleep' time structure */
	rqtp.tv_sec = 0;
	rqtp.tv_nsec = dly * 1000;
	nanosleep(&rqtp, &rmtp);
}

/* ============================= */
/* Get a file configuration path */

#if 0
#ifdef __linux__
static char *defaultconfigpath = "/dsrc/drivers/mtt/test/mttconfig.linux";
#else
static char *defaultconfigpath = "/dsc/data/mtt/Mtt.conf";
#endif

static char *configpath = NULL;

char *MttLibGetFile(char *name) {
	FILE *gpath = NULL;
	char txt[LN];
	int i, j;

	static char path[LN];

	if (configpath) {
		gpath = fopen(configpath,"r");
		if (gpath == NULL) {
			configpath = NULL;
		}
	}

	if (configpath == NULL) {
		configpath = defaultconfigpath;
		gpath = fopen(configpath,"r");
		if (gpath == NULL) {
			configpath = NULL;
			sprintf(path,"./%s",name);
			return path;
		}
	}

	bzero((void *) path,LN);

	while (1) {
		if (fgets(txt,LN,gpath) == NULL) break;
		if (strncmp(name,txt,strlen(name)) == 0) {
			for (i=strlen(name); i<strlen(txt); i++) {
				if (txt[i] != ' ') break;
			}
			j= 0;
			while ((txt[i] != ' ') && (txt[i] != 0) && (txt[i] != '\n')) {
				path[j] = txt[i];
				j++; i++;
			}
			strcat(path,name);
			fclose(gpath);
			return path;
		}
	}
	fclose(gpath);
	return NULL;
}
#endif

/* ================================== */
/* Read object code binary from file  */

MttLibError MttLibReadObject(FILE *objFile, ProgramBuf *code) {

	Instruction *inst;
	int i;
	char ln[LN], *cp, *ep;

	if (fgets(ln, LN, objFile)) {
		cp = ln;
		code->LoadAddress = strtoul(cp, &ep, 0);
		cp = ep;
		code->InstructionCount = strtoul(cp, &ep, 0);
	} else
		return MttLibErrorREAD;

	inst = (Instruction *) malloc(sizeof(Instruction) * code->InstructionCount);
	if (inst == NULL)
		return MttLibErrorNOMEM;

	code->Program = inst;

	for (i = 0; i < code->InstructionCount; i++) {
		inst = &(code->Program[i]);
		bzero((void *) ln, LN);
		if (fgets(ln, LN, objFile)) {
			cp = ln;
			inst->Number = (unsigned short) strtoul(cp, &ep, 0);
			cp = ep;
			inst->Src1 = strtoul(cp, &ep, 0);
			cp = ep;
			inst->Src2 = (unsigned short) strtoul(cp, &ep, 0);
			cp = ep;
			inst->Dest = (unsigned short) strtoul(cp, &ep, 0);
			cp = ep;
			inst->Crc = (unsigned short) strtoul(cp, &ep, 0);
		}
	}
	return MttLibErrorNONE;
}

/* =================================== */
/* String to upper case until a space  */

static void StrToUpper(inp, out)
	char *inp;char *out; {

	int i;
	char *cp;

	for (i = 0; i < strlen(inp); i++) {
		cp = &(inp[i]);
		if (*cp == ' ')
			break;
		if ((*cp >= 'a') && (*cp <= 'z'))
			*cp -= ' ';
		out[i] = *cp;
	}
	out[i] = 0;
}

/* ============================== */
/* String to Register             */

unsigned long MttLibStringToReg(char *name, MttLibRegType *lorg) {

	int i;
	unsigned long rn, en, st, of, lg, rs;
	char upr[MAX_REGISTER_STRING_LENGTH + 1], *cp, *ep;

	cp = name;
	if (cp) {
		StrToUpper(cp, upr);
		if (strlen(upr) <= MAX_REGISTER_STRING_LENGTH) {
			for (i = 0; i < REGNAMES; i++) {
				if (strncmp(upr, Regs[i].Name, strlen(Regs[i].Name)) == 0) {

					st = Regs[i].Start;
					en = Regs[i].End;
					of = Regs[i].Offset;
					lg = Regs[i].LorG;

					if (st < en) {
						if (strlen(Regs[i].Name) < strlen(name)) {
							cp = name + strlen(Regs[i].Name);
							rn = strtoul(cp, &ep, 0);
							if (cp != ep) {
								if ((rn >= of) && (rn <= en - st + of)) {
									*lorg = lg;
									rs = st + rn - of;
									if (lg == LOCAL_REG)
										rs -= GLOBALS;
									return rs;
								}
							}
						}
					} else
						return st;
				}
			}
		}
	}
	return 0;
}

/* ============================== */
/* Register To String             */

char *MttLibRegToString(int regnum, MttLibRegType lorg) {

	static char name[MAX_REGISTER_STRING_LENGTH];
	int rn, i;
	RegisterDsc *reg;

	if (lorg == MttLibRegTypeLOCAL)
		regnum += GLOBALS;

	for (i = 0; i < REGNAMES; i++) {
		reg = &(Regs[i]);
		if ((regnum >= reg->Start) && (regnum <= reg->End)) {
			rn = (regnum - reg->Start) + reg->Offset;
			if (reg->Start < reg->End)
				sprintf(name, "%s%1d", reg->Name, rn);
			else
				sprintf(name, "%s", reg->Name);
			return name;
		}
	}
	return "???";
}

/* ================================================================ */
/* Initialize the MTT library. This routine opens the driver, and   */
/* initializes the MTT module. The supplied path is where the       */
/* library will look to find the compiled event table object files. */
/* Notice we don't pass event tables through reflective memory.     */
/* The path name must end with the character '/' and be less than   */
/* LN (128) characters long. If Path is NULL the default is path is */
/* used as defined in libmtt.h DEFAULT_OBJECT_PATH                  */

int sysldr = 0;

MttLibError MttLibInit(char *path) {

	uint32_t enb, stat, autc, msk;
	time_t tim;
	MttDrvrTime t;

	if (path == NULL)
		strncpy(objdir, MttLibDEFAULT_OBJECT_PATH, LN);
	else {
		if (strlen(path) >= 1) {
			strncpy(objdir, path, LN);
			if ( path[strlen(objdir) - 1] != '/')
				strcat(objdir,"/");
		} else
			return MttLibErrorPATH;
	}
	if (mtt != 0) {
		close(mtt);
		mtt = 0;
	}

	mtt = MttOpen();
	if (mtt == 0)
		return MttLibErrorOPEN;

	if (ioctl(mtt, MTT_IOCGSTATUS, &stat) < 0) {
		mtt = 0;
		return MttLibErrorIO;
	}

	if ((sysldr) || ((stat & MttDrvrStatusENABLED) == 0)) {
		enb = 1;
		if (ioctl(mtt, MTT_IOCSOUT_ENABLE, &enb) < 0) {
			close(mtt);
			mtt = 0;
			return MttLibErrorIO;
		}
	}

	if ((sysldr) || ((stat & MttDrvrStatusUTC_SET) == 0)) {
		msk = MttLibWait(MttDrvrIntPPS, 0, 200);
		if (msk & MttDrvrIntPPS) {
			MttLibUsleep(MS100);
			if (time(&tim) > 0) {
				t.Second = tim + 1;
				t.MilliSecond = 0;
				if (ioctl(mtt, MTT_IOCSUTC, &t.Second) < 0) {
					close(mtt);
					mtt = 0;
					return MttLibErrorIO;
				}
			} else {
				close(mtt);
				mtt = 0;
				return MttLibErrorSYS;
			}
		} else {
			close(mtt);
			mtt = 0;
			return MttLibErrorIO;
		}
	}

	if ((sysldr) || ((stat & MttDrvrStatusUTC_SENDING_ON) == 0)) {
		autc = 1;
		if (ioctl(mtt, MTT_IOCSUTC_SENDING, &autc) < 0) {
			close(mtt);
			mtt = 0;
			return MttLibErrorIO;
		}
	}
	return MttLibErrorNONE;
}

/* ================================================================ */
/* Get a string for an MttLibError code                             */

char *MttLibErrorToString(MttLibError err) {

	if (err >= MttLibERRORS)
		return "No such error code";
	return errstrings[err];
}

/* ================================================================ */
/* Load task program object into MTT program memory                 */

MttLibError MttLibLoadTaskObject(char *name, ProgramBuf *pbf) {

	int i;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn, ftn, tcnt, tval;
	uint32_t tmsk;

	if (mtt == 0)
		return MttLibErrorINIT;

	if ((name == NULL) || (strlen(name) == 0) || (strlen(name)
			>= MttLibMAX_NAME_SIZE) || (strcmp(name, "ALL") == 0) || (isalpha(
			(int) name[0]) == 0))
		return MttLibErrorNAME;

	for (i = 1; i < strlen(name); i++) {
		if ((name[i] != '_') && (name[i] != '.') && (isalnum((int) name[i])
				== 0))
			return MttLibErrorNAME;
	}
	if (strlen(name) < MttLibMIN_NAME_SIZE)
		return MttLibErrorNAME;

	if (pbf->LoadAddress != 0)
		return MttLibErrorNORELO;
	if (pbf->InstructionCount == 0)
		return MttLibErrorEMPTY;

	tcnt = (pbf->InstructionCount / (max_size + 1)) + 1;
	tval = 0;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if (strncmp(tbuf.Name, name, MttLibMAX_NAME_SIZE) == 0)
			return MttLibErrorISLOAD;
	}
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if (strlen(tbuf.Name) == 0) {
			tmsk = 1 << i;
			ioctl(mtt, MTT_IOCSTASKS_STOP, &tmsk);
			if (tval == 0)
				ftn = tn;
			tval++;
			if (tval >= tcnt)
				break;
		} else
			tval = 0;
	}
	if (tval == 0)
		return MttLibErrorNOMEM;

	pbf->LoadAddress = max_size * (tn - 1);
	if (ioctl(mtt, MTT_IOCSPROGRAM, pbf) < 0)
		return MttLibErrorIO;

	for (i = ftn; i < ftn + tval; i++) {
		tbuf.Task = i;
		tbuf.Fields = MttDrvrTBFAll;
		tbuf.LoadAddress = pbf->LoadAddress;
		tbuf.InstructionCount = pbf->InstructionCount;
		tbuf.PcStart = 0;
		tcbp->Pc = tbuf.PcStart;
		tcbp->PcOffset = tbuf.LoadAddress;

		strncpy(tbuf.Name, name, MttLibMAX_NAME_SIZE);
		if (ioctl(mtt, MTT_IOCSTCB, &tbuf) < 0)
			return MttLibErrorIO;
	}
	return MttLibErrorNONE;
}

/* ================================================================ */
/* Load/UnLoad a compiled table object from the path specified in   */
/* the initialization routine into a spare task slot if one exists. */
/* Memory handling may result in running tasks being moved around.  */
/* Task names must be alpha numeric strings with the first char a   */
/* letter and less than MttLibMAX_NAME_SIZE characters.             */
/* The special name "ALL" can be used in calls to the UnLoad        */
/* function to force all tasks and any zombies to be unloaded.      */

MttLibError MttLibLoadTask(char *name) {

	int i;
	char fname[LN];
	FILE *fhand;
	ProgramBuf pbf;
	MttLibError err;

	if (mtt == 0)
		return MttLibErrorINIT;

	if ((name == NULL) || (strlen(name) == 0) || (strlen(name)
			>= MttLibMAX_NAME_SIZE) || (strcmp(name, "ALL") == 0) || (isalpha(
			(int) name[0]) == 0))
		return MttLibErrorNAME;
	for (i = 1; i < strlen(name); i++) {
		if ((name[i] != '_') && (name[i] != '.') && (isalnum((int) name[i])
				== 0))
			return MttLibErrorNAME;
	}
	if (strlen(name) < MttLibMIN_NAME_SIZE)
		return MttLibErrorNAME;

	sprintf(fname, "%s%s", objdir, name);
	fhand = fopen(fname, "r");
	if (fhand == NULL)
		return MttLibErrorFILE;
	err = MttLibReadObject(fhand, &pbf);
	fclose(fhand);
	if (err)
		return err;

	err = MttLibLoadTaskObject(name, &pbf);
	free(pbf.Program);
	return err;
}

/* ================================================================ */

MttLibError MttLibUnloadTask(char *name) {

	int i, all, err;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn;
	uint32_t tmsk;

	if (mtt == 0)
		return MttLibErrorINIT;

	if (strcmp(name, "ALL") == 0)
		all = 1;
	else
		all = 0;

	err = 0;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if ((all) || (strcmp(name, tbuf.Name) == 0)) {
			tmsk = 1 << i;
			err += ioctl(mtt, MTT_IOCSTASKS_STOP, &tmsk);
			bzero((void *) tbuf.Name, MttLibMAX_NAME_SIZE);
			err += ioctl(mtt, MTT_IOCSTCB, &tbuf);
		}
	}
	if ((err < 0) && (all == 0))
		return MttLibErrorNOLOAD;
	return MttLibErrorNONE;
}

/* ================================================================ */
/* Here the names pointer should be able to store MttLibTABLES char */
/* pointers. Loaded tasks reside in MTT memory, and are running if  */
/* they are not stopped. Do not confuse this state with the run and */
/* wait bit masks in the event table task logic.                    */

MttLibError MttLibGetLoadedTasks(MttLibName *names) {

	int i;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn;

	if (mtt == 0)
		return MttLibErrorINIT;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if (strlen(tbuf.Name))
			strncpy(names[i], tbuf.Name, MttLibMAX_NAME_SIZE);
		else
			names[i][0] = 0;
	}
	return MttLibErrorNONE;
}

/* ================================================================ */

MttLibError MttLibGetRunningTasks(MttLibName *names) {

	int i;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn;

	if (mtt == 0)
		return MttLibErrorINIT;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if ((strlen(tbuf.Name))
				&& (tcbp->TaskStatus & MttDrvrTaskStatusRUNNING))
			strncpy(names[i], tbuf.Name, MttLibMAX_NAME_SIZE);
	}
	return MttLibErrorNONE;
}

/* ================================================================ */
/* Each task running an event table has MttLibLocakREGISTERS that   */
/* can be used to contain task parameters such as the run count.    */
/* The caller must know what these parameters mean for event tasks. */

MttLibError MttLibSetTaskRegister(char *name, MttLibLocalRegister treg,
		unsigned long val) {

	int i;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn;
	MttDrvrTaskRegBuf lrb;

	if (mtt == 0)
		return MttLibErrorINIT;

	if (treg >= MttLibLocalREGISTERS)
		return MttLibErrorLREG;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if (strcmp(name, tbuf.Name) == 0) {
			lrb.Task = tn;
			lrb.RegMask = 1 << treg;
			lrb.RegVals[treg] = val;
			if (ioctl(mtt, MTT_IOCSTRVAL, &lrb) < 0)
				return MttLibErrorIO;
			return MttLibErrorNONE;
		}
	}
	return MttLibErrorNOLOAD;
}

/* ================================================================ */

MttLibError MttLibGetTaskRegister(char *name, MttLibLocalRegister treg,
		unsigned long *val) {
	int i;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn;
	MttDrvrTaskRegBuf lrb;

	if (mtt == 0)
		return MttLibErrorINIT;

	if (treg >= MttLibLocalREGISTERS)
		return MttLibErrorLREG;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if (strcmp(name, tbuf.Name) == 0) {
			lrb.Task = tn;
			lrb.RegMask = 1 << treg;
			if (ioctl(mtt, MTT_IOCGTRVAL, &lrb) < 0)
				return MttLibErrorIO;
			*val = lrb.RegVals[treg];
			return MttLibErrorNONE;
		}
	}
	return MttLibErrorNOLOAD;
}

/* ================================================================ */
/* Tasks running event tables can be started, stopped or continued. */
/* Starting a task loads its program counter with its start address */
/* Stopping a task stops it dead, Continuing a task continues from  */
/* the last program counter value without reloading it.             */

MttLibError MttLibStartTask(char *name) {

	int i;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn;
	uint32_t tmsk;

	if (mtt == 0)
		return MttLibErrorINIT;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if (strcmp(name, tbuf.Name) == 0) {
			tmsk = 1 << i;
			if (ioctl(mtt, MTT_IOCSTASKS_START, &tmsk) < 0)
				return MttLibErrorIO;
			return MttLibErrorNONE;
		}
	}
	return MttLibErrorNOLOAD;
}

/* ================================================================ */

MttLibError MttLibStopTask(char *name) {

	int i;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn;
	uint32_t tmsk;

	if (mtt == 0)
		return MttLibErrorINIT;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if (strcmp(name, tbuf.Name) == 0) {
			tmsk = 1 << i;
			if (ioctl(mtt, MTT_IOCSTASKS_STOP, &tmsk) < 0)
				return MttLibErrorIO;
			return MttLibErrorNONE;
		}
	}
	return MttLibErrorNOLOAD;
}

/* ================================================================ */

MttLibError MttLibContinueTask(char *name) {

	int i;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn;
	uint32_t tmsk;

	if (mtt == 0)
		return MttLibErrorINIT;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if (strcmp(name, tbuf.Name) == 0) {
			tmsk = 1 << i;
			if (ioctl(mtt, MTT_IOCSTASKS_CONT, &tmsk) < 0)
				return MttLibErrorIO;
			return MttLibErrorNONE;
		}
	}
	return MttLibErrorNOLOAD;
}

/* ================================================================ */
/* The task status enumeration is defined in the driver includes.   */
/* By including mttdrvr.h then mtthard.h is also included, and in   */
/* this hardware description file MtttDrvrTaskStatus is defined.    */
/* The task status has values like Running, Stopped, Waiting etc    */

MttDrvrTaskStatus MttLibGetTaskStatus(char *name) {

	int i;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn;

	if (mtt == 0)
		return 0;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if (strcmp(name, tbuf.Name) == 0)
			return tcbp->TaskStatus;
	}
	return 0;
}

static MttLibError __send_event(int frame, int priority)
{
	MttDrvrEvent event;

	if (mtt == 0)
		return MttLibErrorINIT;

	event.Frame	= frame;
	event.Priority	= priority;
	if (ioctl(mtt, MTT_IOCSSEND_EVENT, &event) < 0)
		return MttLibErrorIO;

	return MttLibErrorNONE;
}

/* ================================================================ */
/* Calling this routine sends out the specified frame immediately.  */
/* The event will be sent in the high priority queue                */

MttLibError MttLibSendEvent(unsigned long frame) {

	return __send_event(frame, 0);
}

MttLibError MttLibSendEventPrio(unsigned long frame, int priority)
{
	return __send_event(frame, priority);
}

/* ================================================================ */
/* This routine allows you to connect to MTT interrupts and wait    */
/* for them to arrive. You supply a mask defined in mtthard.h that  */
/* defines which interrupts you want to wait for. The possible      */
/* interrupts are error conditions, task interrupts, the PPS, SYNC, */
/* and software triggered events. The noqueue flag kills the queue  */
/* if it has a non zero value. Tmo is the timeout in 10ms units.    */
/* A tmo value of zero means no timeout, hence wait indefinatley.   */

MttDrvrInt MttLibWait(MttDrvrInt mask, int noqueue, int tmo) {

	SkelDrvrConnection con;
	SkelDrvrReadBuf rbf;
	int cc;

	if (mtt == 0)
		return 0;

	if ((mask & connected) != mask) {
		con.Module = 1;
		con.ConMask = mask;
		if (ioctl(mtt, SkelDrvrIoctlCONNECT, &con) < 0)
			return 0;
		connected |= mask;
	}
	if (noqueue != noqueueflag) {
		if (ioctl(mtt, SkelDrvrIoctlSET_QUEUE_FLAG, &noqueue) < 0)
			return MttLibErrorIO;
		noqueueflag = noqueue;
	}

	if (tmo != timeout) {
		if (ioctl(mtt, SkelDrvrIoctlSET_TIMEOUT, &timeout) < 0)
			return MttLibErrorIO;
		timeout = tmo;
	}

	while (1) {
		cc = read(mtt, &rbf, sizeof(SkelDrvrReadBuf));
		if (cc <= 0)
			return 0;
		if (mask == 0)
			return rbf.Connection.ConMask;
		if (mask & rbf.Connection.ConMask)
			return rbf.Connection.ConMask;
	}
	return 0;
}

/* ================================================================ */
/* The Mtt module status can be read to check its functioning.      */
/* MttDrvrStatus is defined in mtthard.h.                           */

MttDrvrStatus MttLibGetStatus() {

	uint32_t stat;

	if (mtt == 0)
		return 0;

	if (ioctl(mtt, MTT_IOCGSTATUS, &stat) < 0)
		return 0;
	return (MttDrvrStatus) stat;
}

/* ================================================================ */
/* Global Mtt module registers contain stuff like telegrams, UTC    */
/* and other stuff. The exact meaning of these registers will       */
/* depend completely on the tasks running in the Mtt module. The    */
/* caller needs to be aware of the register usage conventions.      */

MttLibError MttSetGlobalRegister(MttLibGlobalRegister greg, unsigned long val) {

	MttDrvrGlobalRegBuf grb;

	if (mtt == 0)
		return MttLibErrorINIT;

	if (greg >= MttLibGlobalREGISTERS)
		return MttLibErrorGREG;

	grb.RegNum = greg;
	grb.RegVal = val;
	if (ioctl(mtt, MTT_IOCSGRVAL, &grb) < 0)
		return MttLibErrorIO;

	return MttLibErrorNONE;
}

/* ================================================================ */

MttLibError MttGetGlobalRegister(MttLibGlobalRegister greg, unsigned long *val) {

	MttDrvrGlobalRegBuf grb;

	if (mtt == 0)
		return MttLibErrorINIT;

	if (greg >= MttLibGlobalREGISTERS)
		return MttLibErrorGREG;

	grb.RegNum = greg;
	if (ioctl(mtt, MTT_IOCGGRVAL, &grb) < 0)
		return MttLibErrorIO;
	*val = grb.RegVal;

	return MttLibErrorNONE;
}

/* ================================================================ */
/* This routine returns the mtt driver file handle so that direct   */
/* client ioctl calls can be made.                                  */

int MttLibGetHandle() {
	return mtt;
}

/* ================================================================ */
/* Set task range.                                                  */
/* Never call this routine unless the task size is too small.       */

MttLibError MttLibSetTaskRange(unsigned int first, unsigned int last,
		unsigned int isize) {

	if ((first < 1) || (first > MttDrvrTASKS) || (last <= first) || (last
			> MttDrvrTASKS))
		return MttLibErrorNOLOAD;

	first_task = first; /* First task number 1..MttDrvrTASKS */
	last_task = last; /* Last  task number 1..MttDrvrTASKS */
	max_size = isize; /* Number of instructions per task */

	return MttLibErrorNONE;
}
/* ================================================================ */
/* Get task range.                                                  */

void MttLibGetTaskRange(unsigned int *first, unsigned int *last,
		unsigned int *isize) {

	*first = first_task;
	*last = last_task;
	*isize = max_size;
}

/* ================================================================ */
/* Get the TCB number for a given named task, 1..16. If the task is */
/* not found the routine returns zero.                              */

int MttLibGetTcbNum(char *name) {

	int i;
	MttDrvrTaskBuf tbuf;
	MttDrvrTaskBlock *tcbp;
	unsigned long tn;

	if (mtt == 0)
		return 0;

	tcbp = &(tbuf.ControlBlock);
	for (i = first_task - 1; i < last_task; i++) {
		tn = i + 1;
		tbuf.Task = tn;
		if (ioctl(mtt, MTT_IOCGTCB, &tbuf) < 0)
			return MttLibErrorIO;
		if (strcmp(name, tbuf.Name) == 0)
			return tn;
	}
	return 0;
}

#if 0
/* ================================================================ */
/* Get host configuration character                                 */

char MttLibGetConfgChar(int n) {

	char *cp, fname[LN], ln[LN];
	FILE *fp;

	cp = MttLibGetFile("Mtt.hostconfig");
	if (cp) {
		strcpy(fname, cp);
		fp = fopen(fname, "r");
		if (fp) {
			cp = fgets(ln, LN, fp);
			fclose(fp);
			if (n < strlen(ln))
				return ln[n];
		}
	}
	return '?';
}

/* ================================================================ */
/* Get host configuration ID (M, A or B)                            */

char MttLibGetHostId() {
	return MttLibGetConfgChar(0);
}

/* ================================================================= */
/* Compile an event table by sending a message to the table compiler */
/* server message queue. The resulting object file is transfered to  */
/* the LHC MTGs accross reflective memory. This routine avoids the   */
/* need for issuing system calls within complex multithreaded FESA   */
/* classes. The table must be loaded as usual and completion must    */
/* be checked as usual by looking at the response xmem tables.       */
/* If the return code is an IO error this probably means the cmtsrv  */
/* task is not running. A SYS error indicates a message queue error. */



/* See libxmem, I had to use xmem to check for update of the object  */
/* table. This delays CompileTable until the object is loaded in to  */
/* the reflective memory table.                                      */
/* I can't put xmem calls here as others use this library.           */

MttLibError MttLibCompileTable(char *name) {

	ssize_t ql;
	mqd_t q;
	int terr;

	q = mq_open("/tmp/cmtsrv",(O_WRONLY | O_NONBLOCK));
	if (q == (mqd_t) -1) return MttLibErrorIO;

	ql = mq_send(q,name,MttLibMAX_NAME_SIZE,NULL);
	terr = errno;
	mq_close(q);
	if (q == (mqd_t) -1) {
		if (terr == EAGAIN) return MttLibErrorIO;
		else return MttLibErrorSYS;
	}
	return MttLibErrorNONE;
}
#endif
