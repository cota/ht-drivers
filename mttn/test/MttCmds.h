/* ****************************************************************************** */
/* MTT CTG Multi-Tasking CTG Test Program                                         */
/* Mon/25th/Sept/2006 Julian Lewis                                                */
/* ****************************************************************************** */

#ifdef __linux__
#include <sys/ipc.h>
#include <sys/shm.h>
#else  /* __Lynx__ */
#include <ipc.h>
#include <shm.h>
#endif

#include <tgm/tgm.h>
#include <tgv/tgv.h>
#include <ctrdrvr.h>
#include <mqueue.h>
#include <time.h>

extern int _TgmSemIsBlocked(const char *sname);
extern CfgGetTgmXXX *_TgmCfgGetTgmXXX(void);

#include <time.h>   /* ctime */

#include <lenval.h>
#include <micro.h>
#include <ports.h>
#include <libmtt.h>

#define LN 128

#define GREGS 224
#define LREGS 32
#define REGS (GREGS + LREGS)

#define UNSET 0x80000000

#define PsEQ 0x1 /* Last instruction result was Zero */
#define PsLT 0x2 /* Last instruction result was Less Than Zero */
#define PsGT 0x4 /* Last instruction result was Greater Than Zero */
#define PsCR 0x8 /* Last instruction result Set the Carry */

/* ======================================================================== */
/* Static memory declarations and initialization                            */
/* ======================================================================== */


static int module = 1;

static unsigned long tasknum = 1;
static unsigned long taskmsk = 1;
static char          taskname[MttDrvrNameSIZE];

static int xsvf_iDebugLevel = 0;

static char *editor = "e";

#ifdef __linux__
static char *defaultconfigpath = "/dsrc/drivers/mttn/test/mttconfig.linux";
#else
static char *defaultconfigpath = "/dsc/data/mtt/Mtt.conf";
#endif

#define FILE_MTT_OBJ "/tmp/Mtt.obj"

static char *configpath = NULL;
static char localconfigpath[LN];

static char path[LN];

static int yesno = 1;

static int connected = 0;

static TgmMachine tgmMch = TgmMACHINE_NONE;
static int  tgmNamesValid = 0;
static char tgmNames[TgmGROUPS_IN_TELEGRAM][TgmNAME_SIZE];

static char *int_names[MttDrvrINTS] = {

   "Int01    ",
   "Int02    ",
   "Int03    ",
   "Int04    ",
   "Int05    ",
   "Int06    ",
   "Int07    ",
   "Int08    ",
   "Int09    ",
   "Int10    ",
   "Int11    ",
   "Int12    ",
   "Int13    ",
   "Int14    ",
   "Int15    ",
   "Int16    ",

   "Reserver0",
   "No40MHz  ",
   "BadSync  ",
   "BadPPS   ",
   "BadOpCde ",
   "PrgHALT  ",
   "FrmMoved ",
   "FifoOver ",
   "PPS      ",
   "Synchro  ",

   "Reserved1",
   "Reserved2",

   "Software1",
   "Software2",
   "Software3",
   "Software4"
 };

static char *Statae[MttDrvrSTATAE] = {

   "Enabled",
   "UtcSet ",
   "AutoUtc",
   "No40MHz",
   "BadSync",
   "BadPPS ",
   "Ext1KHz",
   "NotUsed",
   "BusErr ",
   "FlshOpn"
 };

static char *TaskStatae[MttDrvrTaskSTATAE] = {
   "IlgOpCd ",
   "BadValu ",
   "BadRegN ",
   "Waiting ",
   "Stopped ",
   "Running "
 };

static char *EditMemHelp =

   "<number>[CR]      Write number in location\n"
   "[?][CR]           Print this help text\n"
   "[/]<location>[CR] Open location\n"
   "[CR]              Open next location\n"
   "[.][CR]           Exit edit command\n"
   "[x][CR]           Hexadecimal radix\n"
   "[d][CR]           Decimal radix\n";

typedef struct {
   char *Name;  /* Name of Raw Io location */
   int   Start; /* Start Address */
   int   End;   /* End address */
 } MemDsc;

#define MEMDSC 100
#define MEMMAX 4907
static MemDsc memdsc[MEMDSC] = {

   { "IrqSource ",0,0 },
   { "IrqEnable ",1,1 },
   { "IrqLevel  ",2,2 },
   { "VhdlDate  ",3,3 },
   { "Status    ",4,4 },
   { "CmdNumbr  ",5,5 },
   { "CmdValue  ",6,6 },
   { "Second    ",7,7 },
   { "MilliSec  ",8,8 },
   { "StrtTasks ",9,9 },
   { "StopTasks ",10,10 },
   { "RnngTasks ",11,11 },

   { "Tcb01-Pc  ",12,12 },
   { "Tcb01-PcOf",13,13 },
   { "Tcb01-Stat",14,14 },
   { "Tcb01-PsWd",15,15 },

   { "Tcb02-Pc  ",16,16 },
   { "Tcb02-PcOf",17,17 },
   { "Tcb02-Stat",18,18 },
   { "Tcb02-PsWd",19,19 },

   { "Tcb03-Pc  ",20,20 },
   { "Tcb03-PcOf",21,21 },
   { "Tcb03-Stat",22,22 },
   { "Tcb03-PsWd",23,23 },

   { "Tcb04-Pc  ",24,24 },
   { "Tcb04-PcOf",25,25 },
   { "Tcb04-Stat",26,26 },
   { "Tcb04-PsWd",27,27 },

   { "Tcb05-Pc  ",28,28 },
   { "Tcb05-PcOf",29,29 },
   { "Tcb05-Stat",30,30 },
   { "Tcb05-PsWd",31,31 },

   { "Tcb06-Pc  ",32,32 },
   { "Tcb06-PcOf",33,33 },
   { "Tcb06-Stat",34,34 },
   { "Tcb06-PsWd",35,35 },

   { "Tcb07-Pc  ",36,36 },
   { "Tcb07-PcOf",37,37 },
   { "Tcb07-Stat",38,38 },
   { "Tcb07-PsWd",39,39 },

   { "Tcb08-Pc  ",40,40 },
   { "Tcb08-PcOf",41,41 },
   { "Tcb08-Stat",42,42 },
   { "Tcb08-PsWd",43,43 },

   { "Tcb09-Pc  ",44,44 },
   { "Tcb09-PcOf",45,45 },
   { "Tcb09-Stat",46,46 },
   { "Tcb09-PsWd",47,47 },

   { "Tcb10-Pc  ",48,48 },
   { "Tcb10-PcOf",49,49 },
   { "Tcb10-Stat",50,50 },
   { "Tcb10-PsWd",51,51 },

   { "Tcb11-Pc  ",52,52 },
   { "Tcb11-PcOf",53,53 },
   { "Tcb11-Stat",54,54 },
   { "Tcb11-PsWd",55,55 },

   { "Tcb12-Pc  ",56,56 },
   { "Tcb12-PcOf",57,57 },
   { "Tcb12-Stat",58,58 },
   { "Tcb12-PsWd",59,59 },

   { "Tcb13-Pc  ",60,60 },
   { "Tcb13-PcOf",61,61 },
   { "Tcb13-Stat",62,62 },
   { "Tcb13-PsWd",63,63 },

   { "Tcb14-Pc  ",64,64 },
   { "Tcb14-PcOf",65,65 },
   { "Tcb14-Stat",66,66 },
   { "Tcb14-PsWd",67,67 },

   { "Tcb15-Pc  ",68,68 },
   { "Tcb15-PcOf",69,69 },
   { "Tcb15-Stat",70,70 },
   { "Tcb15-PsWd",71,71 },

   { "Tcb16-Pc  ",72,72 },
   { "Tcb16-PcOf",73,73 },
   { "Tcb16-Stat",74,74 },
   { "Tcb16-PsWd",75,75 },

   { "Global    ",76,293 },

   { "Msmr      ",294,294},
   { "Tsync     ",295,295},
   { "VmeP2     ",296,296},
   { "EvOut     ",297,297},
   { "MsFR      ",298,298},
   { "SyncFR    ",299,299},

   { "T01Local  ",300,331},
   { "T02Local  ",332,363},
   { "T03Local  ",364,395},
   { "T04Local  ",396,427},
   { "T05Local  ",428,459},
   { "T06Local  ",460,491},
   { "T07Local  ",492,523},
   { "T08Local  ",524,555},
   { "T09Local  ",556,587},
   { "T10Local  ",588,619},
   { "T11Local  ",620,651},
   { "T12Local  ",652,683},
   { "T13Local  ",684,715},
   { "T14Local  ",716,747},
   { "T15Local  ",748,779},
   { "T16Local  ",780,811},

   { "ProgMem   ",812,8192 }
 };

#define LRGNS 9
static char *LrgNames[LRGNS] = {
   "RUNC ","RMSK ","WMSK ","SSEC ",
   "SMSC ","WVME ","EVNT ","STAT ","TTYP "
 };

/* ======================================================================== */
/* Local routines used by commands                                          */
/* ======================================================================== */

#include <lenval.c>
#include <micro.c>
#include <ports.c>

/* ================================== */
/* Launch a task                      */

static void Launch(char *txt) {
pid_t child;

   if ((child = fork()) == 0) {
      if ((child = fork()) == 0) {
	 close(mtt);
	 system(txt);
	 exit (127);
      }
      exit (0);
   }
}

/* ================================== */
/* Get the name of a memory location  */

char *GetMemName(unsigned int n) {

int i;
MemDsc *md;

   if (n < MEMMAX) {
      for (i=0; i<MEMDSC; i++) {
	 md = &(memdsc[i]);
	 if ((n >= md->Start)
	 &&  (n <= md->End)) return md->Name;
      }
   }
   return "";
}

/* =========================== */
/* Set up telegram group names */

int SetupGroupNames() {

int i;
TgmGroupDescriptor desc;
TgmMachine  *mch;
TgmNetworkId nid;

   if (tgmNamesValid) return tgmNamesValid;

   nid = TgmGetDefaultNetworkId();
   mch = TgmGetAllOperMachinesInNetwork(nid);

   if (mch != NULL) {
      printf("On this network:");
      for (i=0; i<TgmMACHINES; i++) {
	 if (mch[i] == TgmMACHINE_NONE) break;
	 printf("%d/%s ",mch[i],TgmGetMachineName(mch[i]));
      }
      tgmMch = TgmNetworkIdToLastMachine(nid);

      printf("[Using:%s]\n",TgmGetMachineName(tgmMch));

      for (i=0; i<TgmLastGroupNumber(tgmMch); i++) {
	 if (TgmGetGroupDescriptor(tgmMch,i+1,&desc) == TgmSUCCESS) {
	    strcpy(tgmNames[i],desc.Name);
	    tgmNamesValid++;
	    continue;
	 }
	 printf("Error, can't get group descriptior\n");
	 tgmNamesValid = 0;
	 break;
      }
   }
   return tgmNamesValid;
}

/* ================================== */
/* Read object code binary from file  */

int ReadObject(FILE *objFile, ProgramBuf *code) {

Instruction *inst;
int i;
char ln[LN], *cp, *ep;

   if (fgets(ln,LN,objFile)) {
      cp = ln;
      code->LoadAddress      = strtoul(cp,&ep,0);
      cp = ep;
      code->InstructionCount = strtoul(cp,&ep,0);
   }
   inst = (Instruction *) malloc(sizeof(Instruction) * code->InstructionCount);
   code->Program = inst;

   for (i=0; i<code->InstructionCount; i++) {
      inst = &(code->Program[i]);
      bzero((void *) ln, LN);
      if (fgets(ln,LN,objFile)) {
	 cp = ln;
	 inst->Number = (unsigned short) strtoul(cp,&ep,0);
	 cp = ep;
	 inst->Src1   = strtoul(cp,&ep,0);
	 cp = ep;
	 inst->Src2   = (unsigned short) strtoul(cp,&ep,0);
	 cp = ep;
	 inst->Dest   = (unsigned short) strtoul(cp,&ep,0);
	 cp = ep;
	 inst->Crc    = (unsigned short) strtoul(cp,&ep,0);
      }
   }
   return 0;
}

/* ================================== */
/* Write object code binary to file   */

int WriteObject(ProgramBuf *code, FILE *objFile) {

Instruction *inst;
int i;

   fprintf(objFile,"0x%X 0x%X\n",
	   (int) code->LoadAddress,
	   (int) code->InstructionCount);

   for (i=0; i<code->InstructionCount; i++) {
      inst = &(code->Program[i]);
      fprintf(objFile,"0x%X 0x%X 0x%X 0x%X 0x%X\n",
	      (int) inst->Number,
	      (int) inst->Src1,
	      (int) inst->Src2,
	      (int) inst->Dest,
	      (int) inst->Crc);

   }
   return 0;
}

/* ============================== */
/* String to Register             */

unsigned long StringToReg(char *name) {

int i;
unsigned long rn, en, st, of;
char upr[MAX_REGISTER_STRING_LENGTH +1],
     *cp,
     *ep;

   cp = name;
   if (cp) {
      if (strlen(cp) <= MAX_REGISTER_STRING_LENGTH) {
	 StrToUpper(cp,upr);
	 for (i=0; i<REGNAMES; i++) {
	    if (strncmp(upr,Regs[i].Name,strlen(Regs[i].Name)) == 0) {

	       st = Regs[i].Start;
	       en = Regs[i].End;
	       of = Regs[i].Offset;

	       if (st < en) {
		  if (strlen(Regs[i].Name) < strlen(name)) {
		     cp = name + strlen(Regs[i].Name);
		     rn = strtoul(cp,&ep,0);
		     if (cp != ep) {
			if ((rn >= of)
			&&  (rn <= en - st + of)) {
			   return st + rn - of;
			}
		     }
		  }
	       } else return st;
	    }
	 }
      }
   }
   return 0;
}

/* ============================== */
/* Register To String             */

char *RegToString(int regnum) {
static char name[MAX_REGISTER_STRING_LENGTH + 10];
int rn, i;
RegisterDsc *reg;

   SetupGroupNames();

   for (i=0; i<REGNAMES; i++) {
      reg = &(Regs[i]);
      if ((regnum >= reg->Start) && (regnum <= reg->End)) {
	 rn = (regnum - reg->Start) + reg->Offset;
	 if (reg->Start < reg->End) {
	    if (regnum < tgmNamesValid)
	       sprintf(name,"%8s %s%1d",tgmNames[rn-1],reg->Name,rn);
	    else if ((regnum >= GREGS) && (rn <= LRGNS))
	       sprintf(name,"%s%s%1d",LrgNames[rn-1],reg->Name,rn);
	    else
	       sprintf(name,"%s%1d"                 ,reg->Name,rn);
	 } else sprintf(name,"%s"   ,reg->Name);
	 return name;
      }
   }
   return "???";
}

/* ============================== */
/* Edit local registers           */

void EditLocalRegs(int reg) {
char ln[LN];
char *cp, *ep, c;
unsigned long trv, val;
MttDrvrTaskRegBuf lrb;
int i;
long lrg[LREGS];

   while (1) {
      lrb.Task = tasknum;
      if (ioctl(mtt,MTT_IOCGTRVAL,&lrb) < 0) {
	 IErr("GET_TASK_REG_VALUE",NULL);
	 return;
      }
      for (i=0; i<LREGS; i++)
	 lrg[i] = lrb.RegVals[i];

      fprintf(stdout,"Tn:%02d Rg:%03d:%s 0x%x: ",
	      (int) tasknum,
		    reg,
		    RegToString(reg+GREGS),
	      (int) lrg[reg]);
      fflush(stdout);

      fgets(ln,LN,stdin); cp = ln; c = *cp;

      if (c == '.') return;

      if (c == '/') {
	 ep = ++cp;
	 trv = strtoul(cp,&ep,0);
	 if (cp != ep) {
	    if (trv < LREGS) reg = trv;
	    continue;
	 }
	 if (strlen(ln) > 1) {
	    trv = StringToReg(cp)-GREGS;
	    if ((trv >= 0) && (trv < LREGS)) reg = trv;
	    continue;
	 }
      }

      if (c == '\n') {
	 reg++;
	 if (reg >= LREGS) {
	    fprintf(stdout,"\n");
	    reg = 0;
	 }
	 continue;
      }

      ep = cp;
      val = strtoul(cp,&ep,0);
      if (cp != ep) {
	 lrg[reg] = val;
	 lrb.RegVals[reg] = lrg[reg];
	 lrb.RegMask = 1 << reg;

	 if (ioctl(mtt,MTT_IOCSTRVAL,&lrb) < 0) {
	    IErr("SET_TASK_REG_VALUE",NULL);
	    return;
	 }
	 continue;
      }
   }
}

/* ============================== */
/* Edit global registers          */

void EditGlobalRegs(int reg) {
char ln[LN];
char *cp, *ep, c;
unsigned long trv, val;
MttDrvrGlobalRegBuf grb;

   while (1) {

      grb.RegNum = reg;
      if (ioctl(mtt,MTT_IOCGGRVAL,&grb) < 0) {
	 IErr("GET_GLOBAL_REG_VALUE",NULL);
	 return;
      }

      fprintf(stdout,"%03d:%s 0x%x: ",reg,RegToString(reg),(int) grb.RegVal);
      fflush(stdout);

      fgets(ln,LN,stdin); cp = ln; c = *cp;

      if (c == '.') return;

      if (c == '/') {
	 ep = ++cp;
	 trv = strtoul(cp,&ep,0);
	 if (cp != ep) {
	    if (trv < GREGS) reg = trv;
	    continue;
	 }
	 if (strlen(ln) > 1) {
	    trv = StringToReg(cp);
	    if (trv < GREGS) reg = trv;
	    continue;
	 }
      }

      if (c == '\n') {
	 reg++;
	 if (reg >= GREGS) {
	    fprintf(stdout,"\n");
	    reg = 0;
	 }
	 continue;
      }

      ep = cp;
      val = strtoul(cp,&ep,0);
      if (cp != ep) {
	 grb.RegVal = val;
	 grb.RegNum = reg;
	 if (ioctl(mtt,MTT_IOCSGRVAL,&grb) < 0) {
	    IErr("SET_GLOBAL_REG_VALUE",NULL);
	    return;
	 }
	 continue;
      }
   }
}

/* ============================== */
/* Get a file configuration path  */

char *GetFile(char *name) {
FILE *gpath = NULL;
char txt[LN];
int i, j;

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

/* ======================================= */
/* Get file name extension                 */

typedef enum {CPP,ASM,OBJ,TBL,INVALID} FileType;

static char *extNames[] = {".cpp",".ass",".obj",".tbl"};

FileType GetFileType(char *path) {

int i, j;
char c, *cp, ext[5];

   for (i=strlen(path)-1; i>=0; i--) {
      cp = &(path[i]); c = *cp;
      if (c == '.') {
	 strncpy(ext,cp,4); ext[4] = 0;
	 for (j=0; j<INVALID; j++) {
	    if (strcmp(ext,extNames[j]) == 0) break;
	 }
	 return (FileType) j;
      }
   }
   return INVALID;
}

/* ======================================= */
/* Get directory name                      */

char *GetRootName(char *path) {

int i;
static char res[LN];

   if ((path != NULL)
   &&  (strlen(path) > 0)) {
      strcpy(res,path);
      for (i=strlen(res)-1; i>=0; i--) {
	 if (res[i] == '/') {
	    return &(res[i+1]);
	 }
      }
   }
   return res;
}

/* ======================================= */
/* Get directory name                      */

char *GetDirName(char *path) {

int i;
static char res[LN];

   if ((path != NULL)
   &&  (strlen(path) > 0)) {
      strcpy(res,path);
      for (i=strlen(res)-1; i>=0; i--) {
	 if (res[i] == '/') {
	    res[i+1] = 0;
	    return res;
	 }
      }
   }
   strcpy(res,"./");
   return res;
}

/* ======================================= */
/* Remove filename extension               */

char *RemoveExtension(char *path) {

int i;
static char res[LN];

   if ((path != NULL)
   &&  (strlen(path) > 0)) {
      strcpy(res,path);
      for (i=strlen(res)-1; i>=0; i--) {
	 if (res[i] == '.') {
	    res[i] = 0;
	    return res;
	 }
      }
   }
   return res;
}

/* ======================================= */
/* Processor Status to String              */
/* ======================================= */

char *PsToString(int ps) {
static char res[16];

   bzero(res,16);

   if (PsEQ & ps) strcat(res,"EQ.");
   else           strcat(res,"eq.");

   if (PsLT & ps) strcat(res,"LT.");
   else           strcat(res,"lt.");

   if (PsGT & ps) strcat(res,"GT.");
   else           strcat(res,"gt.");

   if (PsCR & ps) strcat(res,"CR");
   else           strcat(res,"cr");

   return res;
}

/* ======================================= */
/* Task status to string                   */
/* ======================================= */

char *TaskStatusToString(unsigned long tst) {
int i, msk;
static char res[48];

   bzero((void *) res, 48);
   for (i=0; i<MttDrvrTaskSTATAE; i++) {
      msk = 1 << i;
      if (msk & tst) strcat(res,TaskStatae[i]);
   }
   return res;
}

/* ======================================= */
/* Display a task buffer                   */

static int noblank = 0;

void DisplayTask(int tn, MttDrvrTBF tbf) {

int               i;
unsigned long     msk;
MttDrvrTaskBuf    tbuf;
MttDrvrTaskBlock *tcbp;

   tbuf.Task = tn;
   if (ioctl(mtt,MTT_IOCGTCB,&tbuf) < 0) {
      IErr("GET_TASK_CONTROL_BLOCK",NULL);
      return;
   }

   tcbp = &(tbuf.ControlBlock);

   for (i=0; i<MttDrvrTBFbits; i++) {
      msk = 1<< i;
      if (msk & tbf) {
	 switch ((MttDrvrTBF) msk) {
	    case MttDrvrTBFLoadAddress:
	       printf("Task: %02d LoadAddress: 0x%06x\n",
		      (int) tbuf.Task, (int) tbuf.LoadAddress);
	       break;

	    case MttDrvrTBFInstructionCount:
	       printf("Task: %02d InstructionCount: %d\n",
		      (int) tbuf.Task, (int) tbuf.InstructionCount);
	       break;

	    case MttDrvrTBFPcStart:
	       printf("Task: %02d PcStart: 0x%06X\n",
		      (int) tbuf.Task, (int) tbuf.PcStart);
	       break;

	    case MttDrvrTBFPcOffset:
	       printf("Task: %02d PcOffset: 0x%06X\n",
		      (int) tbuf.Task, (int) tcbp->PcOffset);
	       break;

	    case MttDrvrTBFPc:
	       printf("Task: %02d Pc: 0x%06X\n",
		      (int) tbuf.Task, (int) tcbp->Pc);
	       break;

	    case MttDrvrTBFName:
	       if ((strlen(tbuf.Name) == 0) && (noblank)) break;
	       printf("Task: %02d Name: ", (int) tbuf.Task);
	       if (strlen(tbuf.Name)) printf("%s\n",tbuf.Name);
	       else                   printf("NotSet\n");

	    default: break;
	 }
      }
   }
   if (tbf == MttDrvrTBFAll) {
      printf("Task: %02d Pswd: %s Stat: %s\n========\n",
	     (int) tbuf.Task,
	     PsToString(tcbp->ProcessorStatus),
	     TaskStatusToString(tcbp->TaskStatus));
   }
}

/* ======================================= */
/* Set task TCB parameter                  */

void SetTask(int tn, MttDrvrTBF tbf, unsigned long par) {

int               i;
unsigned long     msk;
MttDrvrTaskBuf    tbuf;
MttDrvrTaskBlock *tcbp;

   tbuf.Task   = tn;
   tbuf.Fields = tbf;
   tcbp = &(tbuf.ControlBlock);

   for (i=0; i<MttDrvrTBFbits; i++) {
      msk = 1<< i;
      if (msk & tbf) {
	 switch ((MttDrvrTBF) msk) {
	    case MttDrvrTBFLoadAddress:
	       tbuf.LoadAddress = par;
	       break;

	    case MttDrvrTBFInstructionCount:
	       tbuf.InstructionCount = par;
	       break;

	    case MttDrvrTBFPcStart:
	       tbuf.PcStart = par;
	       break;

	    case MttDrvrTBFPcOffset:
	       tcbp->PcOffset = par;
	       break;

	    case MttDrvrTBFPc:
	       tcbp->Pc = par;
	       break;

	    case MttDrvrTBFName:
	       strncpy(tbuf.Name,taskname,MttDrvrNameSIZE -1);

	    default: break;
	 }
      }
   }
   if (ioctl(mtt,MTT_IOCSTCB,&tbuf) < 0) {
      IErr("SET_TASK_CONTROL_BLOCK",NULL);
      return;
   }
}

/* ======================================= */
/* Print out an IOCTL driver error message */

static void IErr(char *name, int *value) {

   if (value != NULL)
      printf("MttDrvr: [%02d] ioctl=%s arg=%d :Error\n",
	     (int) mtt, name, (int) *value);
   else
      printf("MttDrvr: [%02d] ioctl=%s :Error\n",
	     (int) mtt, name);
   perror("MttDrvr");
}

/* ======================================= */
/* News                                    */

int News(int arg) {

char sys[LN], npt[LN];

   arg++;

   if (GetFile("mtt_news")) {
      strcpy(npt,path);
      sprintf(sys,"%s %s",GetFile(editor),npt);
      printf("\n%s\n",sys);
      system(sys);
      printf("\n");
   }
   return(arg);
}

/* ======================================= */
/* Get confirmation                        */

static int YesNo(char *question, char *name) {
int yn, c;

   if (yesno == 0) return 1;

   printf("%s: %s (Y/N):",question,name);
   yn = getchar(); c = getchar();
   if ((yn != (int) 'y') && (yn != 'Y')) return 0;
   return 1;
}

/* ====================================================================== */
/* Convert a MttDrvr time in milliseconds to a string routine.            */
/* Result is a pointer to a static string representing the time.          */
/*    the format is: Thu-18/Jan/2001 08:25:14.967                         */
/*                   day-dd/mon/yyyy hh:mm:ss.ddd                         */

volatile char *TimeToStr(MttDrvrTime *t) {

static char tbuf[LN];

char tmp[LN];
char *yr, *ti, *md, *mn, *dy;
unsigned int ms;

    bzero((void *) tbuf, LN);
    bzero((void *) tmp, LN);

    if (t->Second) {

	ctime_r (&t->Second, tmp); /* Day Mon DD HH:MM:SS YYYY\n\0 */

        tmp[3] = 0;
        dy = &(tmp[0]);
        tmp[7] = 0;
        mn = &(tmp[4]);
        tmp[10] = 0;
        md = &(tmp[8]);
        if (md[0] == ' ')
            md[0] = '0';
        tmp[19] = 0;
        ti = &(tmp[11]);
        tmp[24] = 0;
        yr = &(tmp[20]);

	if (t->MilliSecond) {
	   ms = t->MilliSecond;
	   sprintf (tbuf, "%s-%s/%s/%s %s.%03d", dy, md, mn, yr, ti, ms);
	} else
	   sprintf (tbuf, "%s-%s/%s/%s %s"  , dy, md, mn, yr, ti);

    }
    else {
	sprintf (tbuf, "--- Zero ---");
    }
    return (tbuf);
}

/* ==================================== */
/* Ctr driver time to string conversion */

volatile char *CtrTimeToStr(CtrDrvrTime *t, int hpflag) {

static char tbuf[128];

char tmp[128];
char *yr, *ti, *md, *mn, *dy;
double ms;
double fms;

    bzero((void *) tbuf, 128);
    bzero((void *) tmp, 128);

    if (t->Second) {

	ctime_r (&t->Second, tmp); /* Day Mon DD HH:MM:SS YYYY\n\0 */

        tmp[3] = 0;
        dy = &(tmp[0]);
        tmp[7] = 0;
        mn = &(tmp[4]);
        tmp[10] = 0;
        md = &(tmp[8]);
        if (md[0] == ' ')
            md[0] = '0';
        tmp[19] = 0;
        ti = &(tmp[11]);
        tmp[24] = 0;
        yr = &(tmp[20]);

	if ((t->TicksHPTDC) && (hpflag)) {
	   fms  = ((double) t->TicksHPTDC) / 0.256;
	   ms = fms;
	   sprintf (tbuf, "%s.%010.0f", ti, ms);
	} else
	   sprintf (tbuf, "%s-%s/%s/%s %s"  , dy, md, mn, yr, ti);

    }
    else {
	sprintf (tbuf, "--- Zero ---");
    }
    return (tbuf);
}

/* =================================== */
/* Convert HPTDC ticks to nano seconds */

static unsigned long HptdcToNano(unsigned long hptdc) {
double fns;
unsigned long ins;

   fns = ((double) hptdc) / 2.56;
   ins = (unsigned long) fns;
   return ins;
}

/* ============================================ */
/* Subtract times and return a float in seconds */

double CtrTimeDiff(CtrDrvrCTime *lc, CtrDrvrCTime *rc) {

double s, n, d;
unsigned long nl, nr;
CtrDrvrTime *l;
CtrDrvrTime *r;

   l = (&lc->Time);
   r = (&rc->Time);

   s = (double) (l->Second - r->Second);

   nl = HptdcToNano(l->TicksHPTDC);
   nr = HptdcToNano(r->TicksHPTDC);

   if (nr > nl) {
      nl += 1000000000;
      s -= 1;
   }
   n = (unsigned long) ((nl - nr)/1000)*1000;

   d = s + (n / 1000000000.0);

   if (d < 0.001) {
      if (lc->CTrain != rc->CTrain) d = 0.001;
      else                          d = 0.000;
   }
   return d;
}

/* ============================== */
/* Convert a status into a string */

char *StatusToStr(unsigned long stat) {
int i, msk;
static char res[48];

   bzero((void *) res, 48);
   sprintf(res,"0x%08X\n",(int) stat);
   for (i=0; i<MttDrvrSTATAE; i++) {
      msk = 1 << i;
      strcat(res,Statae[i]);
      if (msk & stat) strcat(res,"-1\n");
      else            strcat(res,"-0\n");
   }
   return res;
}

/* ===================================== */
/* Download a new VHDL bitstream routine */

int JtagLoader(char *fname) {

int cc;
MttDrvrVersion version;
MttDrvrTime t;

   jtag_input = fopen(fname,"r");
   if (jtag_input) {
      if (ioctl(mtt,SkelDrvrIoctlJTAG_OPEN,NULL)) {
	 IErr("JTAG_OPEN",NULL);
	 return 1;
      }

      cc = xsvfExecute(); /* Play the xsvf file */
      printf("\n");
      if (cc) printf("Jtag: xsvfExecute: ReturnCode: %d Error\n",cc);
      else    printf("Jtag: xsvfExecute: ReturnCode: %d All OK\n",cc);
      fclose(jtag_input);

      sleep(5); /* Wait for hardware to reconfigure */

      if (ioctl(mtt,SkelDrvrIoctlJTAG_CLOSE,NULL) < 0) {
	 IErr("JTAG_CLOSE",NULL);
	 return 1;
      }

      bzero((void *) &version, sizeof(MttDrvrVersion));
      if (ioctl(mtt,MTT_IOCGVERSION,&version) < 0) {
	 IErr("GET_VERSION",NULL);
	 return 1;
      }
      t.Second = version.VhdlVersion;
      printf("New VHDL bit-stream loaded, Version: [%u] %s\n",
       (int) version.VhdlVersion,
	     TimeToStr(&t));
   } else {
      perror("fopen");
      printf("Error: Could not open the file: %s for reading\n",fname);
      return 1;
   }
   return cc;
}

/* ================= */
/* Raw memory editor */

void EditMemory(int address) {

MttDrvrRawIoBlock iob;
unsigned long array[2];
unsigned long addr, *data;
char c, *cp, str[LN];
int n, radix, nadr;

   data = array;
   addr = address;
   radix = 16;
   c = '\n';

   while (1) {

      iob.Size = 1;
      iob.Offset = addr;
      iob.UserArray = array;

      if (ioctl(mtt,MTT_IOCRAW_READ,&iob) < 0) {
	 IErr("RAW_READ",NULL);
	 break;
      }

      if (radix == 16) {
	 if (c=='\n') printf("%s:0x%04x: 0x%04x ",GetMemName(addr),(int) addr,(int) *data);
      } else {
	 if (c=='\n') printf("%s:%04d: %5d ",GetMemName(addr),(int) addr,(int) *data);
      }

      c = (char) getchar();

      if (c == '/') {
	 bzero((void *) str, LN); n = 0;
	 while ((c != '\n') && (n < LN)) c = str[n++] = (char) getchar();
	 nadr = strtoul(str,&cp,radix);
	 if (cp != str) addr = nadr;
      }

      else if (c == 'x')  { radix = 16; addr--; continue; }
      else if (c == 'd')  { radix = 10; addr--; continue; }
      else if (c == '.')  { c = getchar(); printf("\n"); break; }
      else if (c == '\n') { addr++; }
      else if (c == '?')  {
	 printf("%s\n",EditMemHelp);
	 continue;
      }

      else {
	 bzero((void *) str, LN); n = 0;
	 str[n++] = c;
	 while ((c != '\n') && (n < LN)) c = str[n++] = (char) getchar();
	 *data = strtoul(str,&cp,radix);
	 if (cp != str) {
	    iob.Size = 1;
	    if (ioctl(mtt,MTT_IOCRAW_WRITE,&iob) < 0) {
	       IErr("RAW_WRITE",NULL);
	       break;
	    }
	 }
      }
   }
}

/* =============================================================== */
/* This hash algorithum is needed for handling shared memory keys. */

int GetKey(char *name) {
int i, key;

   key = 0;
   if (name != NULL)
      for (i=0; i<strlen(name); i++)
	 key = (key << 1) + (int) name[i];
   return(key);
}


/* ================== */
/* Mttlog definitions */

static char log_name[LN];

#define LOG_NAME "MttLog.log"
#define LOG_ENTRIES 512

typedef struct {
   time_t Time;
   char Text[LN];
 } LogEntry;

typedef struct {
   int NextEntry;
   LogEntry Entry[LOG_ENTRIES];
 } LogEntries;

/* ================================== */

static LogEntries *event_log = NULL;

LogEntries *GetLogTable() {

key_t smemky;
unsigned smemid;

   if (event_log) return event_log;

   smemky = GetKey(LOG_NAME);
   smemid = shmget(smemky,sizeof(LogEntries),0666);
   if (smemid == -1) {
      if (errno == ENOENT) {
	 smemid = shmget(smemky,sizeof(LogEntries),IPC_CREAT | 0666);
	 if (smemid != -1) {
	    event_log = (LogEntries *) shmat(smemid,(char *) 0,0);
	    if ((int) event_log != -1) {
	       bzero((void *) event_log, sizeof(LogEntries));
	       return event_log;
	    }
	 }
      }
   } else {
      event_log = (LogEntries *) shmat(smemid,(char *) 0,0);
      if ((int) event_log != -1) return event_log;
   }
   return NULL;
}

/* ===============================================================*/
/* Commands used in the test program.                            */
/* ===============================================================*/

/* ======================================= */
/* Launch video displays                   */

int LaunchVideo(int arg) {

char txt[LN];

   arg++;

   if (_TgmSemIsBlocked(_TgmCfgGetTgmXXX ()->semName) ==0) {
      if (YesNo("Task get_tgm_tim is not running: ","Launch it") == 0) {
	 printf("Command aborted\n");
	 return arg;
      }
      sprintf(txt,"get_tgm_tim &");
      printf("Launching: %s\n",txt);
      system(txt);
   }

   GetFile("genvid");
   sprintf(txt,"%s",path);
   printf("Launching: %s\n\n",txt);
   Launch(txt);
   return arg;
}

/* ======================================= */
/* Change editor program used in program   */

int ChangeEditor(int arg) {
static int eflg = 0;

   arg++;
   if (eflg++ > 4) eflg = 1;

   if      (eflg == 1) editor = "e";
   else if (eflg == 2) editor = "emacs";
   else if (eflg == 3) editor = "nedit";
   else if (eflg == 4) editor = "vi";

   printf("Editor: %s\n",GetFile(editor));
   return arg;
}

/* ======================================= */
/* Change configuration file location      */

int ChangeDirectory(int arg) {
ArgVal   *v;
AtomType  at;
char txt[LN], fname[LN], *cp;
int n, earg;

   arg++;
   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;
   if ((v->Type != Close)
   &&  (v->Type != Terminator)) {

      bzero((void *) fname, LN);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));

      strcpy(localconfigpath,fname);
      strcat(localconfigpath,"/mtttest.config");
      if (YesNo("Change mtttest config to:",localconfigpath))
	 configpath = localconfigpath;
      else
	 configpath = NULL;
   }

   cp = GetFile(editor);
   sprintf(txt,"%s %s",cp,configpath);
   printf("\n%s\n",txt);
   system(txt);
   printf("\n");
   return(earg);
}

/* ======================================= */
/* Get/Set the software debug level        */

int GetSetSwDeb(int arg) {
ArgVal   *v;
AtomType  at;
SkelDrvrDebug debug;

   /* configure the debug level for the current process */
   debug.ClientPid = 0;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) debug.DebugFlag = v->Number;
      else           debug.DebugFlag = 0;
      if (ioctl(mtt,SkelDrvrIoctlSET_DEBUG,&debug) < 0) {
         IErr("SET_DEBUG",(int *)&debug.DebugFlag);
	 return arg;
      }
   }

   if (ioctl(mtt,SkelDrvrIoctlGET_DEBUG,&debug) < 0) {
      IErr("GET_DEBUG",NULL);
      return arg;
   }
   if (debug.DebugFlag)
      printf("SoftwareDebug: Level:%d Enabled\n",debug.DebugFlag);
   else
      printf("SoftwareDebug: Level:%d Disabled\n",debug.DebugFlag);

   return arg;
}

/* ======================================= */
/* Get all version information             */

int GetVersion(int arg) {

MttDrvrVersion version;
MttDrvrTime t;

   arg++;

   bzero((void *) &version, sizeof(MttDrvrVersion));
   if (ioctl(mtt,MTT_IOCGVERSION,&version) < 0) {
      IErr("GET_VERSION",NULL);
      return arg;
   }

   t.Second = version.VhdlVersion;
   t.MilliSecond = 0;
   printf("VHDL Compiled: [%u] %s\n",(int) t.Second,TimeToStr(&t));

   t.Second = version.DrvrVersion;
   t.MilliSecond = 0;
   printf("Drvr Compiled: [%u] %s\n",(int) t.Second,TimeToStr(&t));

#ifdef COMPILE_TIME
   t.Second = COMPILE_TIME;
   t.MilliSecond = 0;
   printf("mtttest Compiled: [%u] %s\n",(int) t.Second,TimeToStr(&t));
#else
   printf("mtttest Compiled: %s %s\n", __DATE__, __TIME__);
#endif
   return arg;
}

/* ======================================= */
/* Get/Set driver client queueing          */

int GetSetQue(int arg) {
ArgVal   *v;
AtomType  at;
long qflag, qsize, qover;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      qflag = v->Number;

      if (ioctl(mtt,SkelDrvrIoctlSET_QUEUE_FLAG,&qflag) < 0) {
	 IErr("SET_QUEUE_FLAG",(int *) &qflag);
	 return arg;
      }
   }
   qflag = -1;
   if (ioctl(mtt,SkelDrvrIoctlGET_QUEUE_FLAG,&qflag) < 0) {
      IErr("GET_QUEUE_FLAG",NULL);
      return arg;
   }
   if (ioctl(mtt,SkelDrvrIoctlGET_QUEUE_SIZE,&qsize) < 0) {
      IErr("GET_QUEUE_SIZE",NULL);
      return arg;
   }
   if (ioctl(mtt,SkelDrvrIoctlGET_QUEUE_OVERFLOW,&qover) < 0) {
      IErr("GET_QUEUE_FLAG",NULL);
      return arg;
   }

   if (qflag==1) printf("NoQueueFlag: Set,   Queuing is Off\n");
   else        { printf("NoQueueFlag: ReSet, Queuing is On\n");
		 printf("QueueSize: %d\n",    (int) qsize);
		 printf("QueueOverflow: %d\n",(int) qover);
	       }
   return arg;
}

static void print_interrupt_config(void)
{
	MttDrvrRawIoBlock	ioblock;
	unsigned long		val;

	ioblock.UserArray	= &val;
	ioblock.Offset		= MTT_INTR / sizeof(uint32_t);
	ioblock.Size		= 1;

	if (ioctl(mtt, MTT_IOCRAW_READ, &ioblock) < 0) {
		IErr("RAW_READ", NULL);
		return;
	}

	printf("Intr. Vector:\t0x%02lx\n", val & 0xff);
	printf("Intr. Level: \t%ld\n", (val >> 8) & 0x1f);
}

/* ======================================= */
/* Select the working module               */

int GetSetModule(int arg) {
ArgVal   *v;
AtomType  at;
int mod, cnt, cbl;
SkelDrvrMaps maps;
TgvName cblnam;
TgmMachine mch;
int wrc;
int i;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) {
	 mod = v->Number;
	 if (ioctl(mtt,SkelDrvrIoctlSET_MODULE,&mod) < 0) {
	    IErr("SET_MODULE",&mod);
	    return arg;
	 }
      }
      module = mod;
   }

   if (ioctl(mtt,SkelDrvrIoctlGET_MODULE_COUNT,&cnt) < 0) {
      IErr("GET_MODULE_COUNT",NULL);
      return arg;
   }

   for (mod=1; mod<=cnt; mod++) {
      if (ioctl(mtt,SkelDrvrIoctlSET_MODULE,&mod) < 0) {
	 IErr("SET_MODULE",&mod);
	 return arg;
      }
      if (ioctl(mtt,SkelDrvrIoctlGET_MODULE_MAPS,&maps) < 0) {
	 IErr("GET_MODULE_MAPS",NULL);
	 return arg;
      }
      if (ioctl(mtt,MTT_IOCGCABLEID,&cbl) < 0) {
	 IErr("GET_MODULE_CABLE_ID",NULL);
	 return arg;
      }
      printf("Mod:%d ",(int) mod);

	if (!maps.Mappings) {
		printf("No mappings for module #%d\n", mod);
		goto out;
	}
	printf("\nSpace#\t\tMapped\n"
		"------\t\t------\n");
	for (i = 0; i < maps.Mappings; i++) {
		printf("0x%x\t\t0x%lx\n", maps.Maps[i].SpaceNumber,
			maps.Maps[i].Mapped);
	}
	print_interrupt_config();

      wrc = 0;
      if (TgvGetCableName(cbl,&cblnam)) {
	 mch = TgvTgvToTgmMachine(TgvFirstMachineForCableId(cbl));
	 printf("Cable:%s Tgm:%s",cblnam,TgmGetMachineName(mch));
      } else {
	 wrc = cbl;
	 printf("Cable:Wrong:%d",cbl);
      }
      if (mod == module) printf("<==");
      printf("\n");

      if (wrc) printf("ERROR: CONFIGURATION: Cable:%d Incompatible TGM NETWORK setting\n", wrc);
   }
out:
   ioctl(mtt,SkelDrvrIoctlSET_MODULE,&module);

   return arg;
}

/* ======================================= */
/* Select the next module                  */

int NextModule(int arg) {
int cnt;

   arg++;

   ioctl(mtt,SkelDrvrIoctlGET_MODULE_COUNT,&cnt);
   if (module >= cnt) {
      module = 1;
      ioctl(mtt,SkelDrvrIoctlSET_MODULE,&module);
   } else {
      module ++;
      ioctl(mtt,SkelDrvrIoctlSET_MODULE,&module);
   }

   return arg;
}

/* ======================================= */
/* Select the previous module              */

int PrevModule(int arg) {
int cnt;

   arg++;

   ioctl(mtt,SkelDrvrIoctlGET_MODULE_COUNT,&cnt);
   if (module > 1) {
      module --;
      ioctl(mtt,SkelDrvrIoctlSET_MODULE,&module);
   } else {
      module = cnt;
      ioctl(mtt,SkelDrvrIoctlSET_MODULE,&module);
   }

   return arg;
}

/* ======================================= */
/* Set the module cable ID                 */

int GetSetCableId(int arg) {
ArgVal   *v;
AtomType  at;

int module, id;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) {
	 id = v->Number;
	 if (ioctl(mtt,MTT_IOCSCABLEID,&id) < 0) {
	    IErr("SET_MODULE_CABLE_ID",&id);
	    return arg;
	 }
      }
   }

   module = -1;
   if (ioctl(mtt,SkelDrvrIoctlGET_MODULE,&module) < 0) {
      IErr("GET_MODULE",NULL);
      return arg;
   }

   id = -1;
   if (ioctl(mtt,MTT_IOCGCABLEID,&id) < 0) {
      IErr("GET_MODULE_CABLE_ID",NULL);
      return arg;
   }

   printf("Module: %d ID: %d",module,id);
   if (id) printf("\n"); else printf(": Not Set\n");

   return arg;
}

/* ======================================= */
/* Set wait timeout in read                */

int GetSetTmo(int arg) {
ArgVal   *v;
AtomType  at;
int timeout;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      timeout = v->Number;
      if (ioctl(mtt,SkelDrvrIoctlSET_TIMEOUT,&timeout) < 0) {
	 IErr("SET_TIMEOUT",&timeout);
	 return arg;
      }
   }
   timeout = -1;
   if (ioctl(mtt,SkelDrvrIoctlGET_TIMEOUT,&timeout) < 0) {
      IErr("GET_TIMEOUT",NULL);
      return arg;
   }
   if (timeout > 0)
      printf("Timeout: [%d] Enabled\n",timeout);
   else
      printf("Timeout: [%d] Disabled\n",timeout);

   return arg;
}

/* ======================================= */
/* Connect and wait for an interrupt       */

int WaitInterrupt(int arg) {
ArgVal   *v;
AtomType  at;
SkelDrvrConnection con;
SkelDrvrReadBuf rbf;
MttDrvrTime mtt_time;
int i, cc, qflag, qsize, interrupt, cnt;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Operator) {
      if (v->OId == OprNOOP) {
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;

	 for (i=0; i<MttDrvrINTS; i++) {
	    printf("%s: 0x%08X ",int_names[i],(1<<i));
	    if (((i+1)%4) == 0)  printf("\n");
	 }
	 printf("\n");
	 return arg;
      }
   }
   interrupt = 0;
   if (at == Numeric) {                 /* Hardware mask ? */
      arg++;

      if (v->Number) {
	 interrupt    = v->Number;
	 con.Module   = module;
	 con.ConMask  = interrupt;
	 if (ioctl(mtt,SkelDrvrIoctlCONNECT,&con) < 0) {
	    IErr("CONNECT",&interrupt);
	    return arg;
	 }
	 connected = 1;
      } else {
	 con.Module   = 0;
	 con.ConMask  = 0;
	 if (ioctl(mtt,SkelDrvrIoctlCONNECT,&con) < 0) {
	    IErr("CONNECT",&interrupt);
	    return arg;
	 }
	 connected = 0;
	 printf("Disconnected from all interrupts\n");
	 return arg;
      }
   }

   if (connected == 0) {
      printf("WaitInterrupt: Error: No connections to wait for\n");
      return arg;
   }

   cnt = 0;
   do {
      cc = read(mtt,&rbf,sizeof(SkelDrvrReadBuf));
      if (cc <= 0) {
	 printf("Time out or Interrupted call\n");
	 return arg;
      }
      if ((interrupt == 0) || (rbf.Connection.ConMask & interrupt)) break;
      if (cnt++ > 64) {
	 printf("Aborted wait loop after 64 iterations\n");
	 return arg;
      }
   } while (True);

   if (ioctl(mtt,SkelDrvrIoctlGET_QUEUE_FLAG,&qflag) < 0) {
      IErr("GET_QUEUE_FLAG",NULL);
      return arg;
   }
   if (ioctl(mtt,SkelDrvrIoctlGET_QUEUE_SIZE,&qsize) < 0) {
      IErr("GET_QUEUE_SIZE",NULL);
      return arg;
   }

   /* convert from SkelTime to MttTime */
   bzero((void *)&mtt_time, sizeof(MttDrvrTime));
   mtt_time.Second	= rbf.Time.Second;
   mtt_time.MilliSecond	= rbf.Time.NanoSecond * 1000;

   printf("Mod[%d] Int[0x%06x] Time[%s]",
    (int) rbf.Connection.Module,
    (int) rbf.Connection.ConMask,
	  TimeToStr(&mtt_time));

   for (i=0; i<MttDrvrINTS; i++) {
      if ((1<<i) & rbf.Connection.ConMask)
	 printf(" %s",int_names[i]);
   }

   if ((qflag == 0) && (qsize != 0)) printf(" Queued: %d",(int) qsize);

   printf("\n");

   return arg;
}

/* ===============================================================*/

int SimulateInterrupt(int arg) { /* msk */
ArgVal   *v;
AtomType  at;

SkelDrvrConnection con;
SkelDrvrReadBuf wbf;
int i, cc;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Operator) {
      if (v->OId == OprNOOP) {
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;

	 for (i=0; i<MttDrvrINTS; i++) {
	    printf("%s: 0x%04X ",int_names[i],(1<<i));
	    if ((i%4) == 0)  printf("\n");
	 }
	 printf("\n");
	 return arg;
      }
   }

   if (at == Numeric) {
      arg++;

      if (v->Number) {
	 con.Module   = module;
	 con.ConMask  = v->Number;
      }
   }
   wbf.Connection = con;
   cc = write(mtt,&wbf,sizeof(SkelDrvrReadBuf));
   return arg;
}

/* ======================================= */
/* List client connections                 */

int ListClients(int arg) {

SkelDrvrClientConnections cons;
SkelDrvrConnection *con;
SkelDrvrClientList clist;
int i, j, k, pid, mypid;

   arg++;

   mypid = getpid();

   if (ioctl(mtt,SkelDrvrIoctlGET_CLIENT_LIST,&clist) < 0) {
      IErr("GET_CLIENT_LIST",NULL);
      return arg;
   }

   for (i=0; i<clist.Size; i++) {
      pid = clist.Pid[i];

      bzero((void *) &cons, sizeof(SkelDrvrClientConnections));
      cons.Pid = pid;

      if (ioctl(mtt,SkelDrvrIoctlGET_CLIENT_CONNECTIONS,&cons) < 0) {
	 IErr("GET_CLIENT_CONNECTIONS",NULL);
	 return arg;
      }

      if (pid == mypid) printf("Pid: %4d (mtttest) ",pid);
      else              printf("Pid: %4d ",pid);

      if (cons.Size) {
	 for (j=0; j<cons.Size; j++) {
	    con = &(cons.Connections[j]);
	    printf("\nPid: %4d ",pid);
	    printf("[Mod:%d Mask:0x%08X",(int) con->Module,(int) con->ConMask);
	    for (k=0; k<MttDrvrINTS; k++) {
	       if ((1<<k) & con->ConMask) printf(" %s",int_names[k]);
	    }
	    printf("]");
	 }
      } else printf("No connections");
      printf("\n");
   }
   return arg;
}

/* ======================================= */
/* Enable/Disable event output to GMT      */

int GetSetEnable(int arg) {
ArgVal   *v;
AtomType  at;
unsigned long enb, stat;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      enb = v->Number;
      if (ioctl(mtt,MTT_IOCSOUT_ENABLE,&enb) < 0) {
	 printf("Error: Cant enable/disable event output\n");
	 IErr("ENABLE",NULL);
	 return arg;
      }
   }
   if (ioctl(mtt,MTT_IOCGSTATUS,&stat) < 0) {
      printf("Error: Cant read status from module: %d\n",(int) module);
      IErr("GET_STATUS",NULL);
      return arg;
   }
   printf("Module: %d Event output: ",(int) module);
   if (stat & MttDrvrStatusENABLED) printf("ENABLED\n");
   else                             printf("DISABLED\n");

   return arg;
}

/* ======================================= */
/* Reset the module                        */

int Reset(int arg) {

int module;

   arg++;

   if (ioctl(mtt,SkelDrvrIoctlRESET,NULL) < 0) {
      IErr("RESET",NULL);
      return arg;
   }

   module = -1;
   if (ioctl(mtt,SkelDrvrIoctlGET_MODULE,&module) < 0) {
      IErr("GET_MODULE",NULL);
      return arg;
   }

   printf("Reset Module: %d\n",module);

   return arg;
}

/* ======================================= */
/* Select automatic UTC sending            */

int GetSetUtcSending(int arg) {
ArgVal   *v;
AtomType  at;
uint32_t autc, stat;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      autc = v->Number;
      if (ioctl(mtt,MTT_IOCSUTC_SENDING,&autc) < 0) {
	 printf("Error: Cant set Auto UTC sending\n");
	 IErr("UTC_SENDING",NULL);
	 return arg;
      }
   }
   if (ioctl(mtt,MTT_IOCGSTATUS,&stat) < 0) {
      printf("Error: Cant read status from module: %d\n",(int) module);
      IErr("GET_STATUS",NULL);
      return arg;
   }
   printf("Module: %d Auto UTC Sending: ",(int) module);
   if (stat & MttDrvrStatusUTC_SENDING_ON) printf("ENABLED\n");
   else                                    printf("DISABLED\n");

   return arg;
}

/* ======================================= */
/* Select external 1KHz source             */

int GetSetExt1Khz(int arg) {
ArgVal   *v;
AtomType  at;
uint32_t xkhz, stat;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      xkhz = v->Number;
      if (ioctl(mtt,MTT_IOCSEXT_1KHZ,&xkhz) < 0) {
	 printf("Error: Cant set External 1Khz\n");
	 IErr("EXTERNAL_1KHZ",NULL);
	 return arg;
      }
   }
   if (ioctl(mtt,MTT_IOCGSTATUS,&stat) < 0) {
      printf("Error: Cant read status from module: %d\n",(int) module);
      IErr("GET_STATUS",NULL);
      return arg;
   }
   printf("Module: %d External 1KHz: ",(int) module);
   if (stat & MttDrvrStatusEXTERNAL_1KHZ) printf("ENABLED\n");
   else                                   printf("DISABLED\n");

   return arg;
}

/* ======================================= */
/* Edit the output delay value             */

int GetSetOutDelay(int arg) {
ArgVal   *v;
AtomType  at;
uint32_t outd;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      outd = v->Number;
      if (ioctl(mtt,MTT_IOCSOUT_DELAY,&outd) < 0) {
	 printf("Error: Cant set Event Output Delay\n");
	 IErr("SET_OUTPUT_DELAY",NULL);
	 return arg;
      }
   }
   if (ioctl(mtt,MTT_IOCGOUT_DELAY,&outd) < 0) {
      printf("Error: Cant get Event Output Delay\n");
      IErr("GET_OUTPUT_DELAY",NULL);
      return arg;
   }

   printf("Module: %d Output Delay: %d (%dus)\n",
	  module,
	  (int) outd,
	  (int) ((outd*25)/1000));
   return arg;
}

/* ======================================= */
/* Edit the sync period                    */

int GetSetSyncPeriod(int arg) {
ArgVal   *v;
AtomType  at;
uint32_t synp;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      synp = v->Number;
      if (ioctl(mtt,MTT_IOCSSYNC_PERIOD,&synp) < 0) {
	 printf("Error: Cant set Sync Period\n");
	 IErr("SET_SYNC_PERIOD",NULL);
	 return arg;
      }
   }
   if (ioctl(mtt,MTT_IOCGSYNC_PERIOD,&synp) < 0) {
      printf("Error: Cant get Sync Period\n");
      IErr("GET_SYNC_PERIOD",NULL);
      return arg;
   }

   printf("Module: %d Sync Period: %dms\n",
	  module,
	  (int) synp);
   return arg;
}

/* ======================================= */
/* Set the UTC time                        */

int GetSetUtc(int arg) {
ArgVal   *v;
AtomType  at;

time_t tim;
MttDrvrTime  t;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;

      if (v->Number == 1) {
	 if (time(&tim) > 0) {
	    t.Second = tim+1; t.MilliSecond = 0;
	    printf("UTC: Set time: %s\n",TimeToStr(&t));
	    if (ioctl(mtt,MTT_IOCSUTC,&t) < 0) {
	       IErr("SET_UTC",NULL);
	       return arg;
	    }
	    sleep(1);
	 }
      }
   }

   if (ioctl(mtt,MTT_IOCGUTC,&t) < 0) {
      IErr("GET_UTC",NULL);
      return arg;
   }

   printf("UTC: %d.%03d %s \n",(int) t.Second, (int) t.MilliSecond, TimeToStr(&t));

   return arg;
}

/* ======================================= */
/* Print the module status                 */

int GetStatus(int arg) {

uint32_t stat;

   arg++;
   if (ioctl(mtt,MTT_IOCGSTATUS,&stat) < 0) {
      IErr("GET_STATUS",NULL);
      return arg;
   }
   printf("Mod:%d Status:%s\n",module,StatusToStr(stat));
   return arg;
}

/* ======================================= */
/* Send out an event frame now             */

int SendEvent(int arg) {
ArgVal   *v;
AtomType  at;
MttDrvrEvent event;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      event.Frame = v->Number;
      event.Priority = 0;
      if (ioctl(mtt,MTT_IOCSSEND_EVENT,&event) < 0) {
	 printf("Error: Cant send frame: 0x%08X\n", event.Frame);
	 IErr("SEND_EVENT", &event.Frame);
	 return arg;
      }
      printf("Sent: 0x%08X\n", event.Frame);
      return arg;
   }
   printf("No frame to send\n");
   return arg;
}


/* ======================================= */
/* Get/Set the working task                */

int GetSetTask(int arg) {
ArgVal   *v;
AtomType  at;
int tn;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      tn = v->Number;
      if ((tn < 1) || (tn > MttDrvrTASKS)) {
	 printf("Illegal task number: %d Range is [1..%d]\n",
		(int) tn,
		(int) MttDrvrTASKS);
	 return arg;
      }
      tasknum = tn;
      taskmsk = 1 << (tn -1);
   }
   DisplayTask(tasknum, MttDrvrTBFName);
   return arg;
}

/* ======================================= */
/* Select the next task                    */

int NextTask(int arg) {
int tn;

   arg++;

   tn = tasknum + 1;
   if (tn > MttDrvrTASKS) tn = 1;
   tasknum = tn;
   taskmsk = 1 << (tn -1);
   DisplayTask(tn, MttDrvrTBFName);
   return arg;
}

/* ======================================= */
/* Select the previous task                */

int PrevTask(int arg) {
int tn;

   arg++;

   tn = tasknum - 1;
   if (tn < 1) tn = MttDrvrTASKS;
   tasknum = tn;
   taskmsk = 1 << (tn -1);
   DisplayTask(tn, MttDrvrTBFName);
   return arg;
}

/* ======================================= */
/* Start tasks                             */

int StartTasks(int arg) {
ArgVal   *v;
AtomType  at;

int i;
uint32_t tmsk, rmsk, msk;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   if (at == Numeric) {
      arg++;
      tmsk = v->Number;
   } else tmsk = taskmsk;

   printf("Starting tasks: 0x%X [",(int) tmsk);
   for (i=0; i<MttDrvrTASKS; i++) {
      msk = 1 << i;
      if (msk & tmsk) printf("%d ",i+1);
   }
   printf("]\n");

   if (ioctl(mtt,MTT_IOCGTASKS_CURR,&rmsk) < 0) {
      IErr("GET_RUNNING_TASKS",NULL);
      return arg;
   }

   rmsk = rmsk & tmsk;
   if (rmsk) {
      printf("Tasks: ");
      for (i=0; i<MttDrvrTASKS; i++) {
	 msk = 1 << i;
	 if (msk & tmsk) printf("%d ",i+1);
      }
      printf("Already running: Error\n");
      return arg;
   }

   if (ioctl(mtt,MTT_IOCSTASKS_START,&tmsk) < 0) {
      IErr("START_TASKS",NULL);
      return arg;
   }

   printf("Started OK\n");
   return arg;
}

/* ======================================= */
/* Stop tasks                              */

int StopTasks(int arg) {
ArgVal   *v;
AtomType  at;

int i;
uint32_t tmsk, msk;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   if (at == Numeric) {
      arg++;
      tmsk = v->Number;
   } else tmsk = taskmsk;

   printf("Stopping tasks: 0x%X [",(int) tmsk);
   for (i=0; i<MttDrvrTASKS; i++) {
      msk = 1 << i;
      if (msk & tmsk) printf("%d ",i+1);
   }
   printf("]\n");

   if (ioctl(mtt,MTT_IOCSTASKS_STOP,&tmsk) < 0) {
      IErr("STOP_TASKS",NULL);
      return arg;
   }

   printf("Stopped OK\n");
   return arg;
}

/* ======================================= */
/* Continue tasks                          */

int ContTask(int arg) {
ArgVal   *v;
AtomType  at;

int i;
uint32_t tmsk, msk;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   if (at == Numeric) {
      arg++;
      tmsk = v->Number;
   } else tmsk = taskmsk;

   printf("Continue tasks: 0x%X [",(int) tmsk);
   for (i=0; i<MttDrvrTASKS; i++) {
      msk = 1 << i;
      if (msk & tmsk) printf("%d ",i+1);
   }
   printf("]\n");

   if (ioctl(mtt,MTT_IOCSTASKS_CONT,&tmsk) < 0) {
      IErr("CONTINUE_TASKS",NULL);
      return arg;
   }

   printf("Continued: WARNING: The task status is WAITING\n");
   printf("You must set the PC to a no-wait instruction\n");
   printf("Wait instructions can not be restarted\n");
   return arg;
}

/* ======================================= */
/* Get the list of running tasks           */

int GetRunningTasks(int arg) {

int i;
uint32_t rmsk, msk;
int cnt;

   arg++;

   if (ioctl(mtt,MTT_IOCGTASKS_CURR,&rmsk) < 0) {
      IErr("GET_RUNNING_TASKS",NULL);
      return arg;
   }

   printf("Loaded tasks:\n");

   noblank = 1;
   for (i=0; i<MttDrvrTASKS; i++) {
      DisplayTask(i+1, MttDrvrTBFName);
   }
   noblank = 0;
   printf("\n");

   cnt = 0;
   printf("Runninging tasks: RunMask: 0x%X\n",(int) rmsk);
   for (i=0; i<MttDrvrTASKS; i++) {
      msk = 1 << i;
      if (msk & rmsk) {
	 DisplayTask(i+1, MttDrvrTBFName);
	 cnt++;
      }
   }

   printf("%d Running tasks\n",cnt);

   return arg;
}

/* ======================================= */
/* Display tasks TCB               */

int GetTcb(int arg) {
ArgVal   *v;
AtomType  at;

unsigned long tn;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   tn = UNSET;
   if (at == Numeric) {
      arg++;
      tn = v->Number;
   }
   if (tn == UNSET) tn = tasknum;

   DisplayTask(tn,MttDrvrTBFAll);

   return arg;
}

/* ======================================= */
/* Set the working tasks load address      */

int GetSetLa(int arg) {
ArgVal   *v;
AtomType  at;

unsigned long la;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   la = UNSET;
   if (at == Numeric) {
      arg++;
      la = v->Number;
   }
   if (la != UNSET) SetTask(tasknum, MttDrvrTBFLoadAddress, la);

   DisplayTask(tasknum, MttDrvrTBFLoadAddress);

   return arg;
}

/* ======================================= */
/* Set the working tasks instruction count */

int GetSetIc(int arg) {
ArgVal   *v;
AtomType  at;

unsigned long ic;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   ic = UNSET;
   if (at == Numeric) {
      arg++;
      ic = v->Number;
   }
   if (ic != UNSET) SetTask(tasknum, MttDrvrTBFInstructionCount, ic);

   DisplayTask(tasknum, MttDrvrTBFInstructionCount);

   return arg;
}

/* ======================================= */
/* Set the working tasks PC Start address  */

int GetSetPs(int arg) {
ArgVal   *v;
AtomType  at;

unsigned long ps;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   ps = UNSET;
   if (at == Numeric) {
      arg++;
      ps = v->Number;
   }
   if (ps != UNSET) SetTask(tasknum, MttDrvrTBFPcStart, ps);

   DisplayTask(tasknum, MttDrvrTBFPcStart);

   return arg;
}

/* ======================================= */
/* Set the working tasks PC Offset         */

int GetSetPo(int arg) {
ArgVal   *v;
AtomType  at;

unsigned long po;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   po = UNSET;
   if (at == Numeric) {
      arg++;
      po = v->Number;
   }
   if (po != UNSET) SetTask(tasknum, MttDrvrTBFPcOffset, po);

   DisplayTask(tasknum, MttDrvrTBFPcOffset);

   return arg;
}

/* ======================================= */
/* Get/Set working tasks PC                */

int GetSetPc(int arg) {
ArgVal   *v;
AtomType  at;

unsigned long pc;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   pc = UNSET;
   if (at == Numeric) {
      arg++;
      pc = v->Number;
   }
   if (pc != UNSET) SetTask(tasknum, MttDrvrTBFPc, pc);

   DisplayTask(tasknum, MttDrvrTBFPc);

   return arg;
}

/* ======================================= */
/* Get/Set task name                       */

int GetSetTaskName(int arg) {
ArgVal   *v;
AtomType  at;
char *cp;
int n, earg;

   arg++;
   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;
   if ((v->Type != Close)
   &&  (v->Type != Terminator)) {

      bzero((void *) taskname, MttDrvrNameSIZE);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    taskname[n++] = *cp;
	 taskname[n] = 0;
	 if (n >= MttDrvrNameSIZE) break;
	 cp++;
      } while ((at != Close) && (at != Terminator));
      if (strcmp(taskname,"clear") == 0) bzero((void *) taskname,MttDrvrNameSIZE);
      SetTask(tasknum, MttDrvrTBFName, 0);
   }
   DisplayTask(tasknum, MttDrvrTBFName);

   return earg;
}

/* ======================================= */
/* Edit the working tasks local register   */

int EditLreg(int arg) {
ArgVal   *v;
AtomType  at;

unsigned long rgn;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   if (at == Numeric) {
      arg++;
      rgn = v->Number;
      if (rgn >= LREGS) {
	 printf("Bad local register number: %d\n",(int) rgn);
	 return arg;
      }
   } else rgn = 0;

   EditLocalRegs(rgn);

   return arg;
}

/* ======================================= */
/* Edit global registers                   */

int EditGreg(int arg) {
ArgVal   *v;
AtomType  at;

unsigned long rgn;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   if (at == Numeric) {
      arg++;
      rgn = v->Number;
      if (rgn >= GREGS) {
	 printf("Bad Global register number: %d\n",(int) rgn);
	 return arg;
      }
   } else rgn = 0;

   EditGlobalRegs(rgn);

   return arg;
}

/* ======================================= */
/* Read program from working module        */

int GetProgram(int arg) {
ArgVal   *v;
AtomType  at;

char fname[LN];
ProgramBuf pbf;
unsigned long la, ic;
FILE *fp;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   la = 0;
   if (at == Numeric) {

      la = v->Number;

      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }

   ic = 100;
   if (at == Numeric) {

      ic = v->Number;

      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }

   bzero((void *) fname, LN);
   strcpy(fname,FILE_MTT_OBJ);
   fp = fopen(fname,"w");
   if (fp == NULL) {
      printf("Can't open file: %s for write\n",fname);
      return arg;
   }

   pbf.LoadAddress = la;
   pbf.InstructionCount = ic;
   pbf.Program = (Instruction *) malloc(ic * sizeof(Instruction));
   if (pbf.Program == NULL) {
      printf("Can't allocate enough memory to hold program: %d instructions long\n",
	     (int) ic);
      fclose(fp);
      return arg;
   }

   if (ioctl(mtt,MTT_IOCGPROGRAM,&pbf) < 0) {
      IErr("GET_PROGRAM",NULL);
      fclose(fp);
      free(pbf.Program);
      return arg;
   }
   WriteObject(&pbf,fp);

   printf("Read program from hardware and write to file: %s\n",fname);
   printf("Read %d Instructions from hardware address: 0x%X OK\n",
	  (int) ic,
	  (int) la);

   fclose(fp);
   free(pbf.Program);

   return arg;
}

/* ======================================= */
/* Write program to working module         */

int SetProgram(int arg) {
ArgVal   *v;
AtomType  at;

char fname[LN], *cp;
unsigned long la, tn, relo;
ProgramBuf pbf;
FILE *fp;
MttDrvrTaskBlock *tcbp;
MttDrvrTaskBuf tbuf;
FileType ft;
int n, earg, laset;

   arg++;

   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;

   tn = tasknum;
   la = (tn -1) * 0x100;

   if (at == Numeric) {

      tn = v->Number;
      if ((tn < 1) || (tn > MttDrvrTASKS)) {
	 printf("Illegal task number: %d\n", (int) tn);
	 return arg;
      }

      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }

   laset = 0;
   if (at == Numeric) {

      la = v->Number;
      laset = 1;

      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }

   if ((v->Type == Close)
   ||  (v->Type == Terminator)) {

      strcpy(fname,FILE_MTT_OBJ);

   } else {

      bzero((void *) fname, LN);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));
   }

   ft = GetFileType(fname);
   if (ft != OBJ) {
      printf("The file: %s is not an object file\n",fname);
      return earg;
   }

   fp = fopen(fname,"r");
   if (fp == NULL) {
      printf("Can't open file: %s for read\n",fname);
      return earg;
   }

   ReadObject(fp,&pbf);

   if (pbf.LoadAddress == 0) {
      relo = 1;
      pbf.LoadAddress = la;
   } else {
      relo = 0;
      if (laset) {
	 printf("Object file is absolute: Load address not allowed\n");
	 fclose(fp);
	 free(pbf.Program);
	 return earg;
      }
   }

   if (ioctl(mtt,MTT_IOCSPROGRAM,&pbf) < 0) {
      IErr("SET_PROGRAM",NULL);
      fclose(fp);
      free(pbf.Program);
      return earg;
   }

   printf("Read program from file: %s and write to hardware\n",fname);
   printf("Written %d Instructions to hardware address: 0x%X OK\n",
	  (int) pbf.InstructionCount,
	  (int) pbf.LoadAddress);

   fclose(fp);

   if ((tn >= 1) && (tn <= MttDrvrTASKS)) {
      tbuf.Task = tn;
      tbuf.Fields = MttDrvrTBFAll;
      tbuf.LoadAddress = pbf.LoadAddress;
      tbuf.InstructionCount = pbf.InstructionCount;
      if (relo) tbuf.PcStart = 0;
      else      tbuf.PcStart = pbf.LoadAddress;
      tcbp = &(tbuf.ControlBlock);
      if (relo) tcbp->Pc = 0;                 /* Relocatable */
      else      tcbp->Pc = pbf.LoadAddress;   /* Absolute */
      if (relo) tcbp->PcOffset = pbf.LoadAddress;
      else      tcbp->PcOffset = 0;
      if (strlen(fname) == 0) sprintf(tbuf.Name,"Task-%02d.obj",(int) tn);
      else                    sprintf(tbuf.Name,"%s",GetRootName(fname));
      if (ioctl(mtt,MTT_IOCSTCB,&tbuf) < 0) {
	 IErr("SET_TASK_CONTROL_BLOCK",NULL);
	 return earg;
      }
      printf("Task: %02d Set up as follows:\n",(int) tn);
      DisplayTask(tn, MttDrvrTBFAll);
   }

   free(pbf.Program);

   return earg;
}

/* ======================================= */
/* Read a task program from the hardware   */

int GetTaskProgram(int arg) {
ArgVal   *v;
AtomType  at;

char fname[LN];
unsigned long la, ic, tn;
ProgramBuf pbf;
FILE *fp;
MttDrvrTaskBlock *tcbp;
MttDrvrTaskBuf tbuf;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   tn = UNSET;          /* See if a task needs to be set up */
   if (at == Numeric) {

      tn = v->Number;

      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }
   if (tn == UNSET) tn = tasknum; /* Default is current working task */

   strcpy(fname,FILE_MTT_OBJ);
   fp = fopen(fname,"w");
   if (fp == NULL) {
      printf("Can't open file: %s for write\n",fname);
      return arg;
   }

   tcbp = &(tbuf.ControlBlock);

   if ((tn >= 1) && (tn <= MttDrvrTASKS)) {
      tbuf.Task = tn;
      if (ioctl(mtt,MTT_IOCGTCB,&tbuf) < 0) {
	 IErr("GET_TASK_CONTROL_BLOCK",NULL);
	 return arg;
      }
      la = tbuf.LoadAddress;
      pbf.LoadAddress = la;
      ic = tbuf.InstructionCount;
      if (ic == 0) {
	 printf("Task: %d is empty, no program loaded\n",(int) tn);
	 fclose(fp);
	 return arg;
      }
      pbf.InstructionCount = ic;
      pbf.Program = (Instruction *) malloc(ic * sizeof(Instruction));
      if (pbf.Program == NULL) {
	 printf("Can't allocate enough memory to hold program: %d instructions long\n",
		(int) ic);
	 fclose(fp);
	 return arg;
      }

      if (ioctl(mtt,MTT_IOCGPROGRAM,&pbf) < 0) {
	 IErr("GET_PROGRAM",NULL);
	 fclose(fp);
	 free(pbf.Program);
	 return arg;
      }
      WriteObject(&pbf,fp);

      printf("Read program from hardware and write to file: %s\n",fname);
      printf("Read %d Instructions from hardware address: 0x%X OK\n",
	     (int) ic,
	     (int) la);

   } else printf("Bad task number: %d\n",(int) tn);

   fclose(fp);
   free(pbf.Program);

   return arg;
}

/* ======================================= */
/* Edit and assemble program               */

int Asm(int arg) {
ArgVal   *v;
AtomType  at;
char fname[LN], cmd[LN], tmp[LN], *cp;
int n, earg;
FileType ft;

   arg++;

   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;

   if ((v->Type == Close)
   ||  (v->Type == Terminator)) {

      bzero((void *) fname, LN);
      if (GetFile("Mtt.cpp") == NULL) {
	 printf("Can not find file: Mtt.cpp\n");
	 return earg;
      }
      strcpy(fname,path);

   } else {

      bzero((void *) fname, LN);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));
   }

   ft = GetFileType(fname);

   if ((ft == CPP) || (ft == ASM)) {
      strcpy(tmp,GetFile(editor));
      sprintf(cmd,"%s %s",
	      tmp,
	      fname);
      printf("%s\n",cmd);
      system(cmd);
   }

   if (ft == CPP) {
      strcpy(tmp,GetFile("cpp"));
      sprintf(cmd,"%s -I%s %s -o %s.ass",
	      tmp,
	      GetDirName(fname),
	      fname,
	      RemoveExtension(fname));
      printf("%s\n",cmd);
      system(cmd);
   }
   if ((ft == CPP) || (ft == ASM)) {
      strcpy(tmp,GetFile("asm"));
      sprintf(cmd,"%s %s.ass > %s.lst",
	      tmp,
	      RemoveExtension(fname),
	      RemoveExtension(fname));
      printf("%s\n",cmd);
      system(cmd);
      sprintf(cmd,"cat %s.lst",
	      RemoveExtension(fname));
      printf("%s\n",cmd);
      system(cmd);
      return earg;
   }
   printf("Bad source file name:%s\n",fname);
   return earg;
}

/* ======================================= */
/* Disassemble program                     */

int DisAsm(int arg) {
ArgVal   *v;
AtomType  at;
char fname[LN], cmd[LN], tmp[LN], *cp;
int n, earg;
FileType ft;

   arg++;

   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;

   if ((v->Type == Close)
   ||  (v->Type == Terminator)) {

      strcpy(fname,FILE_MTT_OBJ);

   } else {

      bzero((void *) fname, LN);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));
   }

   ft = GetFileType(fname);

   if (ft == OBJ) {
      strcpy(tmp,GetFile("asm"));
      sprintf(cmd,"%s %s.obj > %s.dsa",
	      tmp,
	      RemoveExtension(fname),
	      RemoveExtension(fname));
      printf("%s\n",cmd);
      system(cmd);

      strcpy(tmp,GetFile(editor));
      sprintf(cmd,"%s %s.dsa",
	      tmp,
	      RemoveExtension(fname));
      printf("%s\n",cmd);
      system(cmd);
      return earg;
   }
   printf("Bad object file name:%s\n",fname);
   return earg;
}

/* ======================================= */
/* Emulate program                         */

int EmuProg(int arg) {
ArgVal   *v;
AtomType  at;
char fname[LN], cmd[LN], tmp[LN], *cp;
int n, earg;
FileType ft;

   arg++;

   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;

   if ((v->Type == Close)
   ||  (v->Type == Terminator)) {

      strcpy(fname,FILE_MTT_OBJ);

   } else {

      bzero((void *) fname, LN);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));
   }

   ft = GetFileType(fname);

   if (ft == OBJ) {
      strcpy(tmp,GetFile("emu"));
      sprintf(cmd,"%s %s.obj",
	      tmp,
	      RemoveExtension(fname));
      printf("%s\n",cmd);
      system(cmd);
      return earg;
   }
   printf("Bad object file name:%s\n",fname);
   return earg;
}

/* ======================================= */
/* Filter an event table                   */

#define TS 0x100
#define MAXE 4
#define HGAP 10
#define SDMP 8
static int ctr = 0;
static int tblerrs = 0;

/* Reading the CTR event history takes around 1000 us, in which time the write pointer moves */
/* This means that I have to throw away HGAP entries after the index to ensure that I look */
/* in linear time and don't cross the divide between newest and oldest by mistake. This is */
/* very tricky !! */

double dmod(double a1, double a2) {
   if (a1 > a2) return a1 - a2;
   if (a2 > a1) return a2 - a1;
   return 0.0;
}

int FilterTable(int arg) {
ArgVal   *v;
AtomType  at;
char fname[LN], tmp[LN], enm[TS][LN], *cp;
int i, j, k, n, fc, fn, hi, kk, lok, loj, earg, ecn, strt;
FileType ft;
FILE *tbf = NULL;
unsigned long sec[TS], msc[TS], pay[TS], frm[TS], ers[TS], fnd[TS];
CtrDrvrEventHistory ehsb;
CtrDrvrCTime *tzro, *tdif;
double mdif, ddif;

   arg++;

   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;

   if ((v->Type == Close)
   ||  (v->Type == Terminator)) {

      bzero((void *) fname, LN);
      if (GetFile("Mtt.tbl") == NULL) {
	 printf("Can not find file: Mtt.tbl\n");
	 return earg;
      }
      strcpy(fname,path);

   } else {

      bzero((void *) fname, LN);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));
   }

   ft = GetFileType(fname);

   if (ft == TBL) {
      for (i=0; i<TS; i++) {
	 sec[i] = 0;
	 msc[i] = 0;
	 pay[i] = 0;
	 frm[i] = 0;
	 ers[i] = 0;
	 fnd[i] = 0;
	 enm[i][0] = 0;
      }
      fc = 0;

      tbf = fopen(fname,"r");
      if (tbf == NULL) {
	 printf("Can't open: %s for reading\n",fname);
	 perror(fname);
	 return earg;
      }

      while (fgets(tmp,LN,tbf)) {
	 if (isdigit((int) tmp[0])) {
	    sscanf(tmp,"%u %u %s %x",
			(int  *) &(sec[fc]),
			(int  *) &(msc[fc]),
			(char *) &(enm[fc][0]),
			(int  *) &(pay[fc]));
	    frm[fc] = TgvGetFrameForName((TgvName *) enm[fc]);
	    if (frm) {
	       frm[fc] &= 0xFFFF0000;
	       pay[fc] &= 0x0000FFFF;
	       frm[fc]= frm[fc] | pay[fc];
	       fc++;
	    }
	 }
      }
      fclose(tbf);

      for (i=0; i<fc; i++) {

	 printf("%02d: %02d.%03d 0x%08X %s\n",
		(i+1),
		(int) sec[i],
		(int) msc[i],
		(int) frm[i],
		      enm[i]);
      }
      printf("=== End Table: %s  ===\n\n",fname);
      printf("=== Results of searching CTR event history ===\n\n");

      if (ctr == 0) {
	 for (i = 1; i <= CtrDrvrCLIENT_CONTEXTS; i++) {
	    sprintf(tmp,"/dev/ctr.%1d",i);
	    if ((fn = open(tmp,O_RDWR,0)) > 0) {
	       ctr = fn;
	       break;
	    }
	 }
      }

      if (ctr == 0) {
	 printf("Error: Can't open the CTR driver\n");
	 perror("CTR Drvr");
	 return earg;
      }

      bzero((void *) &ehsb, sizeof(CtrDrvrEventHistory));
      if (ioctl(ctr,CtrDrvrREAD_EVENT_HISTORY,&ehsb) < 0) {
	 printf("Error: Can't read CTR Event History\n");
	 perror("CTR Drvr");
	 return earg;
      }

      /* Get hardware index and move it forward by HGAP entries */

      hi = ehsb.Index;
      if ((hi<0) || (hi>CtrDrvrHISTORY_TABLE_SIZE)) {
	 printf("Error: CTR-Hardware: Bad history index: %d ==> 0\n",hi);
	 hi = 0;
      }
      hi += HGAP; if (hi >= CtrDrvrHISTORY_TABLE_SIZE) hi -= CtrDrvrHISTORY_TABLE_SIZE;

      /* Now scan the history ignoring HGAP entries */

      loj = 0; lok = 0; strt= 0; tzro = NULL; tdif = NULL; ecn = 0; n = 0;
      for (i=0; i<CtrDrvrHISTORY_TABLE_SIZE - HGAP; i++) {  /* Race condition = start later */
	 j = hi + i;
	 if (j >= CtrDrvrHISTORY_TABLE_SIZE) j -=  CtrDrvrHISTORY_TABLE_SIZE;

	 for (k=0; k<fc; k++) {
	    if (ehsb.Entries[j].Frame.Long == frm[k]) {
	       if (tdif == NULL) tdif = &(ehsb.Entries[j].CTime);
	       if (tzro == NULL) tzro = &(ehsb.Entries[j].CTime);
	       fnd[k]++;
	       if (k == 0) {
		  printf("\n=== Start ===\n");
		  strt++;
		  tzro = &(ehsb.Entries[j].CTime);
	       }
	       mdif = (double) (sec[k]) + (double) ((double) (msc[k]) / 1000.000);
	       ddif = dmod(mdif, CtrTimeDiff(&(ehsb.Entries[j].CTime),tdif));
	       if ((strt < 1) || (fc < 2)) {
		  printf(".. ");
	       } else if (ddif < 0.001) {
		  printf("OK ");
		  lok = k;
		  loj = j;
	       } else {
		  ecn++;
		  if (k > lok+1) {
		     printf("** %02d: Sequnce 0x%08x %s\n",lok+2,(int) frm[lok+1],enm[lok+1]);
		     if ((ecn < MAXE) && (YesNo("Dump Event history around",enm[lok+1]))) {
			printf("HistoryIndex: %d (+%d)\n", (int) hi, HGAP);
			printf("Error  Index: %d\n", (int) j);
			kk = loj - SDMP; if (kk < 0) kk = loj;
			for (; kk < j + SDMP; kk++) {
			   printf("%04d: 0x%08X C%04d",
				  kk,
				  (int) ehsb.Entries[kk].Frame.Long,
				  (int) ehsb.Entries[kk].CTime.CTrain);
			   tmp[0] = 0;
			   TgvGetNameForFrame(ehsb.Entries[kk].Frame.Long, (TgvName *) tmp);
			   if (strlen(tmp)) printf(" %s",tmp);
			   printf("\n");
			}
			if (YesNo("Continue","") == 0) return earg;
		     }
		  }
		  ers[k]++;
		  if (ecn < MAXE) printf("*****: Error: Wrong by: %2.3f\n",ddif);
		  printf("** ");
	       }
	       printf("%02d: %2.3f %2.3f 0x%08X C%04d %s %s\n",
		      (int) (k + 1),
		      CtrTimeDiff(&(ehsb.Entries[j].CTime),tdif),
		      CtrTimeDiff(&(ehsb.Entries[j].CTime),tzro),
		      (int) frm[k],
		      (int) ehsb.Entries[j].CTime.CTrain,
		      CtrTimeToStr(&(ehsb.Entries[j].CTime.Time),1),
		      enm[k]);

	       tdif = &(ehsb.Entries[j].CTime);
	       break;
	    }
	 }
      }
      printf("\n");
      for (k=0; k<fc; k++) {
	 if (fnd[k] == 0) {
	    printf(".. %02d: Missing 0x%08x %s\n",(int) (k+1), (int) frm[k], enm[k]);
	 }
	 if (ers[k]) {
	    printf("ER %02d: BadTime 0x%08x %s\n",(int) (k+1), (int) frm[k], enm[k]);
	 }
      }
      if (ecn) {
	 printf("Error(s) were found, count: %d\n",ecn);
	 tblerrs += ecn;
      }
      if (tblerrs) {
	 printf("Total errors since starting mtttest: %d\n",tblerrs);
      }
      printf("\n");
      return earg;
   }
   printf("Bad event table file name:%s\n",fname);
   return earg;
}

/* ================================================================ */
/* Get host configuration character                                 */

char MttLibGetConfgChar(int n) {

char *cp, fname[LN], ln[LN];
FILE *fp;

   cp = GetFile("Mtt.hostconfig");
   if (cp) {
      strcpy(fname,cp);
      fp = fopen(fname,"r");
      if (fp) {
	 cp = fgets(ln,LN,fp);
	 fclose(fp);
	 if (n<strlen(ln)) return ln[n];
      }
   }
   return '?';
}

/* ======================================= */
/* Compile an event table                  */

int CompileTable(int arg) {
ArgVal   *v;
AtomType  at;
char fname[LN], cmd[LN], tmp[LN], *cp;
int n, earg;
FileType ft;

   arg++;

   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;

   if ((v->Type == Close)
   ||  (v->Type == Terminator)) {

      bzero((void *) fname, LN);
      if (GetFile("Mtt.tbl") == NULL) {
	 printf("Can not find file: Mtt.tbl\n");
	 return earg;
      }
      strcpy(fname,path);

   } else {

      bzero((void *) fname, LN);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));
   }

   ft = GetFileType(fname);

   if (ft == TBL) {
      strcpy(tmp,GetFile(editor));
      sprintf(cmd,"%s %s",
	      tmp,
	      fname);
      printf("%s\n",cmd);
      system(cmd);

      strcpy(tmp,GetFile("cmt"));
      sprintf(cmd,"%s %s",
	      tmp,
	      fname);
      printf("%s\n",cmd);
      system(cmd);

      return earg;
   }
   printf("Bad event table file name:%s\n",fname);
   return earg;
}

/* ======================================= */
/* Reload VHDL into module                 */

int ReloadJtag(int arg) {
ArgVal   *v;
AtomType  at;
char fname[LN], *cp;
int n, earg;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      xsvf_iDebugLevel = v->Number;
   } else xsvf_iDebugLevel = 1;

   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;
   if ((v->Type == Close)
   ||  (v->Type == Terminator)) {
      strcpy(fname,"/usr/local/drivers/mttn/Mtt.xsvf");
   } else {

      bzero((void *) fname, LN);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));
   }

   if (YesNo("ReloadJtag: VHDL-Compiled BitStream",fname )) {
      JtagLoader(fname);
   } else printf("ABORTED\n");
   return earg;
}

/* ======================================= */
/* Raw memory access                       */

int MemIo(int arg) {
ArgVal   *v;
AtomType  at;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      EditMemory(v->Number);
   } else
      EditMemory(0);

   return arg;
}


/* ======================================= */
/* Run ctrtest program                     */

int CtrTest(int arg) {

char *cp, cmd[LN];

   arg++;
   cp = GetFile("ctrtest");
   if (cp) {
      sprintf(cmd,"xterm 2>/dev/null -e %s",cp);
      printf("Launching: %s\n",cmd);
      Launch(cmd);
   }
   return arg;
}

/* ======================================= */
/* Run timtest program                     */

int TimTest(int arg) {

char *cp, cmd[LN];

   arg++;
   cp = GetFile("timtest");
   if (cp) {
      sprintf(cmd,"xterm 2>/dev/null -e %s",cp);
      printf("Launching: %s\n",cmd);
      Launch(cmd);
   }
   return arg;
}

/* ============================= */
/* List files                    */

int ListFiles(int arg) {

   arg++;
   system("ls -l");
   return arg;
}


/* ========================= */
/* Make a CTF table          */

int MakeCtfTable(int arg) {
ArgVal   *v;
AtomType  at;
FILE *fp;
unsigned long prod, meas, intv;
char tnam[LN];
int i;

   prod = 0;
   meas = 0;
   intv = 0;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      prod = v->Number;

      v = &(vals[arg]);
      at = v->Type;
      if (at == Numeric) {
	 arg++;
	 meas = v->Number;

	 v = &(vals[arg]);
	 at = v->Type;
	 if (at == Numeric) {
	    arg++;
	    intv = v->Number;
	 }
      }
   }

   if (intv>120) {
      printf("Error:Interval:%d Out of range[1..120]\n",(int) intv);
      return arg;
   }
   if ((meas == 0) && (prod == 0)) {
      printf("Error:No production or measure\n");
      return arg;
   }
   if (meas == 0) printf("Production only table\n");
   if (prod == 0) printf("Measure only table\n");

   sprintf(tnam,"%s",GetFile("Mtt.tbl"));
   fp = fopen(tnam,"w");
   if (fp == NULL) {
      printf("Can't open file: %s for write\n",log_name);
      perror("Open error");
      return arg;
   }
   if (prod) {
      fprintf(fp,"0 10 PROD 0x%04X\n",(int) prod);
      if (meas) fprintf(fp,"0 00 MEAS 0x%04X\n",(int) meas);
   } else fprintf(fp,"0 10 MEAS 0x%04X\n",(int) meas);
   for (i=1; i<intv; i++) {
      if (prod) {
	 fprintf(fp,"0 10 PROD 0x0000\n");
	 if (meas) fprintf(fp,"0 00 MEAS 0x0000\n");
      } else fprintf(fp,"0 10 MEAS 0x0000\n");
   }
   fclose(fp);

   printf("Written: PROD:0x%04X MEAS:0x%04X INTERVAL:%d to %s OK\n",
	  (int) prod,(int) meas,(int) intv,tnam);
   return arg;
}

/* ======================================= */
/* Save running tasks and registers        */

int SaveRunningTasks(int arg) {
ArgVal   *v;
AtomType  at;
char fname[LN], cmd[LN], *cp;
int i, j, n, earg;
unsigned long lr;
uint32_t msk, rmsk;
MttDrvrTaskBuf    tbuf;
MttDrvrTaskRegBuf lrb;
MttDrvrGlobalRegBuf grb;

FILE *fp;

   arg++;

   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;

   if ((v->Type == Close)
   ||  (v->Type == Terminator)) {

      bzero((void *) fname, LN);
      if (GetFile("Mtt.tasks") == NULL) {
	 printf("Can not find file: Mtt.tasks\n");
	 return earg;
      }
      strcpy(fname,path);

   } else {

      bzero((void *) fname, LN);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));
   }

   fp = fopen(fname,"w");
   if (fp == NULL) {
      printf("Can't open file: %s for write\n",fname);
      perror("Open error");
      return earg;
   }

   if (ioctl(mtt,MTT_IOCGTASKS_CURR,&rmsk) < 0) {
      IErr("GET_RUNNING_TASKS",NULL);
      return earg;
   }

   for (i=0; i<MttDrvrTASKS; i++) {
      msk = 1 << i;
      if (msk & rmsk) {
	 tbuf.Task = i+1;
	 if (ioctl(mtt,MTT_IOCGTCB,&tbuf) < 0) {
	    IErr("GET_TASK_CONTROL_BLOCK",NULL);
	    return earg;
	 }

	 fprintf(fp,"%s",tbuf.Name);
	 lrb.Task = tbuf.Task;
	 if (ioctl(mtt,MTT_IOCGTRVAL,&lrb) < 0) {
	    IErr("GET_TASK_REG_VALUE",NULL);
	    return earg;
	 }
	 for (j=31; j>=1; j--) {
	    lr = lrb.RegVals[j];
	    if (lr) fprintf(fp," LR%d 0x%X",j,(int) lr);
	 }
	 if (strcmp(tbuf.Name,"tgm.obj") == 0) {
	    for (j=0; j<32; j++) {
	       grb.RegNum = j;
	       if (ioctl(mtt,MTT_IOCGGRVAL,&grb) < 0) {
		  IErr("GET_GLOBAL_REG_VALUE",NULL);
		  break;
	       }
	       if (grb.RegVal) {
		  fprintf(fp," GR%d 0x%X",j+1,(int) grb.RegVal);
	       }
	    }
	 }
	 fprintf(fp,"\n");
      }
   }
   fclose(fp);

   sprintf(cmd,"more %s",
	   fname);
   printf("%s\n",cmd);
   system(cmd);

   printf("Tasks and local registers: Saved:%s\n",fname);

   return earg;
}

