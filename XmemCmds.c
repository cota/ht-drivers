/* ****************************************************************************** */
/* VMIC-5565 Reflective memory test program using the XMEM device driver.         */
/* Julian Lewis Tue 18th October 2004. First Version implemented.                 */
/* ****************************************************************************** */

#include <stdint.h>
#include "iofield.h"

#include "../driver/vmic5565_layout.h"
#include "vmic5565.c"
#include "../../include/plx9656_layout.h"
#include "plx9656.c"
#include "../driver/xmemDrvr.h"
#include <sys/mman.h>

static int module = 1;
static char *editor = "e";
static const char *intr_names[XmemDrvrIntrSOURCES] = {
   "IntOne",
   "IntTwo",
   "SegmentUpdate",
   "ResetRequest",
   NULL,
   NULL,
   "RogueClobber",
   "PendingInit",
   "DataError",
   "RxAlmostFull",
   "RxOverFlow",
   "LostSync",
   "ParityInhibit",
   "ParityError",
   "SoftWakeup",
   NULL,
   NULL };

static char *lnams[] = {"","Byte","Word","","Long"};

#define ONE_PAGE 4096
static unsigned char seg_page_buf[ONE_PAGE+1];

unsigned long segment = 1;
XmemDrvrSegTable seg_tab;
XmemDrvrNodeTable node_tab;

/**************************************************************************/
#ifdef __linux__
//#define XMEM_PATH "/dsrc/drivers/xmem/test/
#define XMEM_PATH "/acc/src/dsc/drivers/coht/xmem/test/"
#else
#define XMEM_PATH "/dsc/data/xmem/"
#endif

#define SEG_TABLE_NAME "Xmem.segs"
#define NODE_TABLE_NAME "Xmem.nodes"
#define CONFIG_FILE_NAME "Xmem.conf"

char *defaultconfigpath = XMEM_PATH CONFIG_FILE_NAME;
char *configpath = NULL;
char localconfigpath[128];  /* After a CD */
extern char *configfiles;
extern char *build_fullpath(const char *path, const char *filename);

static char path[128];

/**************************************************************************/
/* Chop memory into blocks for partial transfer                           */

typedef enum { FIRST, MIDDLE, LAST, IO_PARTS } IoParts;
typedef struct {
   unsigned int Pages;
   XmemDrvrSegIoDesc IoDesc[IO_PARTS];
} SegIoBlocks;

static SegIoBlocks sgiob;

static SegIoBlocks *ChopSegment(XmemDrvrSegIoDesc *sgio) {

int i;
unsigned long flen, llen, pags;

   bzero((void *) &sgiob, sizeof(SegIoBlocks));

   flen = ONE_PAGE - (sgio->Offset % ONE_PAGE);
   if (flen > sgio->Size) {
      flen = sgio->Size;
      llen = 0;
      pags = 0;
   } else {
      llen = (sgio->Size - flen) % ONE_PAGE;
      pags = (sgio->Size - flen - llen) / ONE_PAGE;
   }

   for (i=FIRST; i<IO_PARTS; i++) {
      sgiob.IoDesc[i].Module    = sgio->Module;
      sgiob.IoDesc[i].Id        = sgio->Id;
      sgiob.IoDesc[i].UserArray = sgio->UserArray;
   }

   sgiob.IoDesc[FIRST].Size     = flen;
   sgiob.IoDesc[FIRST].Offset   = sgio->Offset;

   if (pags) {
      sgiob.IoDesc[MIDDLE].Size    = ONE_PAGE;
      sgiob.IoDesc[MIDDLE].Offset  = flen;
      sgiob.Pages = pags;
   }

   if (llen) {
      sgiob.IoDesc[LAST].Size      = llen;
      sgiob.IoDesc[LAST].Offset    = ONE_PAGE * (pags +1);
      sgiob.IoDesc[LAST].UpdateFlg = sgio->UpdateFlg;
   }
   return &sgiob;
}

/**************************************************************************/
/* Look in the config file to get paths to data needed by this program.   */
/* If no config path has been set up by the user, then ...                */
/* First try the current working directory, if not found try TEMP, and if */
/* still not found try CONF                                               */


char *GetFile(char *name) {
	FILE *gpath = NULL;
	char txt[128];
	int i, j;

	if (configfiles && (!strcmp(name, NODE_TABLE_NAME) ||
				!strcmp(name, SEG_TABLE_NAME))) {
		strncpy(path, build_fullpath(configfiles, name), 128);
		return path;
	}

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

   bzero((void *) path,128);

   while (1) {
      if (fgets(txt,128,gpath) == NULL) break;
      if (strncmp(name,txt,strlen(name)) == 0) {
	 for (i=strlen(name); i<strlen(txt); i++) {
	    if (txt[i] != ' ') break;
	 }
	 j= 0;
	 while ((txt[i] != ' ') && (txt[i] != 0)) {
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

/**************************************************************************/
/* This routine handles getting File Names specified optionally on the    */
/* command line where an unusual file is specified not handled in GetFile */

char *GetFileName(int *arg) {

ArgVal   *v;
AtomType  at;
char     *cp;
int       n, earg;

   /* Search for the terminator of the filename or command */

   for (earg=*arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   n = 0;
   bzero((void *) path,128);

   v = &(vals[*arg]);
   at = v->Type;
   if ((v->Type != Close)
   &&  (v->Type != Terminator)) {

      bzero((void *) path, 128);

      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    path[n++] = *cp;
	 path[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));
   }
   if (n) {
      *arg = earg;
      return path;
   }
   return NULL;
}

/**************************************************************************/
/* Get a Node name                                                        */

char *GetNodeName(unsigned long nid) {

int i;
unsigned long msk;

   for (i=0; i<XmemDrvrNODES; i++) {
      msk = 1 << i;
      if (msk & node_tab.Used) {
	 if (nid == node_tab.Descriptors[i].Id) {
	    return node_tab.Descriptors[i].Name;
	 }
      }
   }
   return "..";
}

/**************************************************************************/
/* Print out driver IOCTL error messages.                                 */

static void IErr(char *name, int *value) {

   if (value != NULL)
      printf("XmemDrvr: [%02d] ioctl=%s arg=%d :Error\n",
	     (int) xmem, name, (int) *value);
   else
      printf("XmemDrvr: [%02d] ioctl=%s :Error\n",
	     (int) xmem, name);
   perror("XmemDrvr");
}

/**************************************************************************/

static int yesno=1;

static int YesNo(char *question, char *name) {
int yn, c;

   if (yesno == 0) return 1;

   printf("%s: %s\n",question,name);
   printf("Continue (Y/N):"); yn = getchar(); c = getchar();
   if ((yn != (int) 'y') && (yn != 'Y')) return 0;
   return 1;
}

/*****************************************************************/
/* Commands used in the test program.                            */
/*****************************************************************/

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

/*****************************************************************/
/* Select a VMIC-5565 module.                                    */

int Module(int arg) {
ArgVal   *v;
AtomType  at;
int mod, cnt;
XmemDrvrModuleDescriptor mdesc;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) {
	 mod = v->Number;
	 if (ioctl(xmem,XmemDrvrSET_MODULE,&mod) < 0) {
	    IErr("SET_MODULE",&mod);
	    return arg;
	 }
      }
      module = mod;
   }

   if (ioctl(xmem,XmemDrvrGET_MODULE_COUNT,&cnt) < 0) {
      IErr("GET_MODULE_COUNT",NULL);
      return arg;
   }

   for (mod=1; mod<=cnt; mod++) {
      ioctl(xmem,XmemDrvrSET_MODULE,&mod);
      ioctl(xmem,XmemDrvrGET_MODULE_DESCRIPTOR,&mdesc);
      printf("Module:%d Slot:%d Node:%s/%d Rfm:0x%08X Ram:0x%08X",
	     (int) mdesc.Module,
	     (int) mdesc.PciSlot,
		   GetNodeName(1 << (mdesc.NodeId -1)),
	     (int) mdesc.NodeId,
	     (int) mdesc.Map,
	     (int) mdesc.SDRam);
      if (mod == module) printf(" <<==");
      printf("\n");
   }
   ioctl(xmem,XmemDrvrSET_MODULE,&module);

   return arg;
}

/*****************************************************************/

int NextModule(int arg) {
int cnt;

   arg++;

   ioctl(xmem,XmemDrvrGET_MODULE_COUNT,&cnt);
   if (module >= cnt) {
      module = 1;
      ioctl(xmem,XmemDrvrSET_MODULE,&module);
   } else {
      module ++;
      ioctl(xmem,XmemDrvrSET_MODULE,&module);
   }

   return arg;
}

/*****************************************************************/

int PrevModule(int arg) {
int cnt;

   arg++;

   ioctl(xmem,XmemDrvrGET_MODULE_COUNT,&cnt);
   if (module > 1) {
      module --;
      ioctl(xmem,XmemDrvrSET_MODULE,&module);
   } else {
      module = cnt;
      ioctl(xmem,XmemDrvrSET_MODULE,&module);
   }

   return arg;
}

/*****************************************************************/

int SwDeb(int arg) { /* Arg!=0 => On, else Off */
ArgVal   *v;
AtomType  at;
int debug;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) debug = v->Number;
      else           debug = 0;
      if (ioctl(xmem,XmemDrvrSET_SW_DEBUG,&debug) < 0) {
	 IErr("SET_SW_DEBUG",&debug);
	 return arg;
      }
   }
   debug = -1;
   if (ioctl(xmem,XmemDrvrGET_SW_DEBUG,&debug) < 0) {
      IErr("GET_SW_DEBUG",NULL);
      return arg;
   }
   if (debug > 0) {
      if (debug & XmemDrvrDebugDEBUG)  printf("SoftwareDebug: Enabled\n");
      if (debug & XmemDrvrDebugTRACE)  printf("SoftwareTrace: Enabled\n");
      if (debug & XmemDrvrDebugMODULE) printf("SoftwareModul: Enabled\n");
				       printf("Debug mask: [0x%X]\n",debug);
   } else printf("Debug: Disabled\n");

   return arg;
}

/*****************************************************************/

int GetSetTmo(int arg) { /* Arg=0 => Timeouts disabled, else Arg = Timeout */
ArgVal   *v;
AtomType  at;
int timeout;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      timeout = v->Number;
      if (ioctl(xmem,XmemDrvrSET_TIMEOUT,&timeout) < 0) {
	 IErr("SET_TIMEOUT",&timeout);
	 return arg;
      }
   }
   timeout = -1;
   if (ioctl(xmem,XmemDrvrGET_TIMEOUT,&timeout) < 0) {
      IErr("GET_TIMEOUT",NULL);
      return arg;
   }
   if (timeout > 0)
      printf("Timeout (ms): [%d] Enabled\n",timeout);
   else
      printf("Timeout (ms): [%d] Disabled\n",timeout);

   return arg;
}

/*****************************************************************/

IoField *GetField(int address, IoFieldType ft) {

static IoField fd;

int i, sz, msk, namok;
IoField *fp, *rp;

   switch (ft) {
      case IoFieldTypePLX_CONFIG:
	 fp = PlxConfigs;
	 sz = PlxCONFIG_REGS;
	 break;

      case IoFieldTypePLX_LOCAL:
	 fp = PlxLocals;
	 sz = PlxLOCAL_REGS;
	 break;

      case IoFieldTypeRFM_REGS:
	 fp = RfmRegs;
	 sz = VmicRFM_REGISTERS;
	 break;

      case IoFieldTypeVMIC_SDRAM:
	 fd.FieldSize = 4;
	 fd.Offset = address;
	 namok = 0;
	 if (seg_tab.Used) {
	    for (i=0; i<XmemDrvrSEGMENTS; i++) {
	       msk = 1 << i;
	       if (seg_tab.Used & msk) {
		  if ((address >= (int) seg_tab.Descriptors[i].Address)
		  &&  (address <  (int) seg_tab.Descriptors[i].Address
				    +   seg_tab.Descriptors[i].Size)) {
		     sprintf(fd.Name,"%s[%dL]",
			     seg_tab.Descriptors[i].Name,
			     (address - (int) seg_tab.Descriptors[i].Address)/sizeof(long));
		     namok = 1;
		     break;
		  }
	       }
	    }
	 }
	 if (namok == 0) sprintf(fd.Name,"Long[%d]",(int) fd.Offset/sizeof(long));
	 sprintf(fd.Comment,"Address: 0x%06x",address);
	 return &fd;

      default:
	 return NULL;
   }

   for (i=0; i<sz; i++) {
      rp = &(fp[i]);
      if (address <= rp->Offset) return rp;
   }
   return NULL;
}

/*****************************************************************/

void EditMemory(int address, IoFieldType ft) {

XmemDrvrRawIoBlock iob;
unsigned long array[2];
unsigned long addr, *data;
char c, *cp, str[128];
int n, radix, nadr, maxadr;

IoField *fp;

   printf("EditMemory: [?]: Comment, [/]: Open, [CR]: Next, [.]: Exit [x]: Hex, [d]: Dec\n\n");

   data = array;
   addr = address;
   radix = 16;
   c = '\n';

   while (1) {
      fp = GetField(addr,ft);
      iob.Items = 1;
      iob.Size = fp->FieldSize;
      addr = iob.Offset = fp->Offset;
      iob.UserArray = array;

      switch (ft) {
	 case IoFieldTypePLX_CONFIG:
	    if (ioctl(xmem,XmemDrvrCONFIG_READ,&iob) < 0) IErr("CONFIG_READ",(int *) &addr);
	    maxadr = PlxCONFIG_MAX_ADR;
	    break;

	 case IoFieldTypePLX_LOCAL:
	    if (ioctl(xmem,XmemDrvrLOCAL_READ,&iob) < 0) IErr("LOCAL_READ",(int *) &addr);
	    maxadr = PlxLOCAL_MAX_ADR;
	    break;

	 case IoFieldTypeRFM_REGS:
	    if (ioctl(xmem,XmemDrvrRFM_READ,&iob) < 0) IErr("RFM_READ",(int *) &addr);
	    maxadr = VmicRFM_MAX_ADR;
	    break;

	 case IoFieldTypeVMIC_SDRAM:
	    if (ioctl(xmem,XmemDrvrRAW_READ,&iob) < 0) IErr("RAW_READ",(int *) &addr);
	    maxadr = VmicSDRAM_MAX_ADR;
	    break;

	 default:
	    return;
      }

      printf("%s\tSize:%d ",fp->Name,(int) fp->FieldSize);
      if (radix == 16) {
	 if (c=='\n') printf("Addr:0x%04x: 0x%04x ",(int) addr,(int) *data);
      } else {
	 if (c=='\n') printf("Addr:%04d: %5d ",(int) addr,(int) *data);
      }

      c = (char) getchar();

      if (c == '/') {
	 bzero((void *) str, 128); n = 0;
	 while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();
	 nadr = strtoul(str,&cp,radix);
	 if (cp != str) addr = nadr;
      }
      else if (c == 'x')  {radix = 16; addr>0 ? addr-- : 1; continue; }
      else if (c == 'd')  {radix = 10; addr>0 ? addr-- : 1; continue; }
      else if (c == '.')  { c = getchar(); printf("\n"); break; }
      else if (c == '\n') {
	 addr += fp->FieldSize;
	 if (addr > maxadr) {
	    addr = 0;
	    printf("\n===\n");
	 }
      }
      else if (c == '?')  { if (fp) printf("%s\n",fp->Comment); }

      else {
	 bzero((void *) str, 128); n = 0;
	 str[n++] = c;
	 while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();
	 *data = strtoul(str,&cp,radix);
	 if (cp != str) {

	    switch (ft) {
	       case IoFieldTypePLX_CONFIG:
		  if (ioctl(xmem,XmemDrvrCONFIG_WRITE,&iob) < 0) IErr("CONFIG_WRITE",(int *) &addr);
		  break;

	       case IoFieldTypePLX_LOCAL:
		  if (ioctl(xmem,XmemDrvrLOCAL_WRITE,&iob) < 0) IErr("LOCAL_WRITE",(int *) &addr);
		  break;

	       case IoFieldTypeRFM_REGS:
		  if (ioctl(xmem,XmemDrvrRFM_WRITE,&iob) < 0) IErr("RFM_WRITE",(int *) &addr);
		  break;

	       case IoFieldTypeVMIC_SDRAM:
		  if (ioctl(xmem,XmemDrvrRAW_WRITE,&iob) < 0) IErr("RAW_WRITE",(int *) &addr);
		  break;

	       default:
		  return;
	    }
	 }
      }
   }
}

/*****************************************************************/

int RfmIo(int arg) { /* Address */
ArgVal   *v;
AtomType  at;

   printf("VmicRfmIo: Address space: BAR2: VMIC registers\n");

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   ioctl(xmem,XmemDrvrRFM_OPEN,NULL);
   if (at == Numeric) {
      arg++;
      EditMemory(v->Number,IoFieldTypeRFM_REGS);
   } else
      EditMemory(0,IoFieldTypeRFM_REGS);
   ioctl(xmem,XmemDrvrRFM_CLOSE,NULL);

   return arg;
}

/*****************************************************************/

int RawIo(int arg) { /* Address */
ArgVal   *v;
AtomType  at;

   printf("SdramIo: Address space: BAR3: VMIC SDRAM Reflective memory direct access\n");

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   ioctl(xmem,XmemDrvrRAW_OPEN,NULL);
   if (at == Numeric) {
      arg++;
      EditMemory(v->Number,IoFieldTypeVMIC_SDRAM);
   } else
      EditMemory(0,IoFieldTypeVMIC_SDRAM);
   ioctl(xmem,XmemDrvrRAW_CLOSE,NULL);

   return arg;
}

/*****************************************************************/

int ConfigIo(int arg) { /* Address */
ArgVal   *v;
AtomType  at;

   printf("ConfigIo: Address space: PLX9656 Configuration Registers\n");

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   ioctl(xmem,XmemDrvrCONFIG_OPEN,NULL);
   if (at == Numeric) {
      arg++;
      EditMemory(v->Number,IoFieldTypePLX_CONFIG);
   } else
      EditMemory(0,IoFieldTypePLX_CONFIG);
   ioctl(xmem,XmemDrvrCONFIG_CLOSE,NULL);

   return arg;
}

/*****************************************************************/

int LocalIo(int arg) { /* Address */
ArgVal   *v;
AtomType  at;

   printf("LocalIo: Address space: BAR0: PLX9656 Local Configuration Registers\n");

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   ioctl(xmem,XmemDrvrLOCAL_OPEN,NULL);
   if (at == Numeric) {
      arg++;
      EditMemory(v->Number,IoFieldTypePLX_LOCAL);
   } else
      EditMemory(0,IoFieldTypePLX_LOCAL);
   ioctl(xmem,XmemDrvrLOCAL_CLOSE,NULL);

   return arg;
}

/*****************************************************************/

int GetSetQue(int arg) { /* Arg=Flag */
ArgVal   *v;
AtomType  at;
long qflag, qsize, qover;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      qflag = v->Number;

      if (ioctl(xmem,XmemDrvrSET_QUEUE_FLAG,&qflag) < 0) {
	 IErr("SET_QUEUE_FLAG",(int *) &qflag);
	 return arg;
      }
   }
   qflag = -1;
   if (ioctl(xmem,XmemDrvrGET_QUEUE_FLAG,&qflag) < 0) {
      IErr("GET_QUEUE_FLAG",NULL);
      return arg;
   }
   if (ioctl(xmem,XmemDrvrGET_QUEUE_SIZE,&qsize) < 0) {
      IErr("GET_QUEUE_SIZE",NULL);
      return arg;
   }
   if (ioctl(xmem,XmemDrvrGET_QUEUE_OVERFLOW,&qover) < 0) {
      IErr("GET_QUEUE_OVERFLOW",NULL);
      return arg;
   }

   if (qflag==1) printf("NoQueueFlag: Set,   Queuing is Off\n");
   else        { printf("NoQueueFlag: ReSet, Queuing is On\n");
		 printf("QueueSize: %d\n",    (int) qsize);
		 printf("QueueOverflow: %d\n",(int) qover);
	       }
   return arg;
}

/*****************************************************************/

int ListClients(int arg) {

XmemDrvrClientConnections cons;
XmemDrvrConnection *con;
XmemDrvrClientList clist;
int i, j, k, pid, mypid;

   arg++;

   mypid = getpid();

   if (ioctl(xmem,XmemDrvrGET_CLIENT_LIST,&clist) < 0) {
      IErr("GET_CLIENT_LIST",NULL);
      return arg;
   }

   for (i=0; i<clist.Size; i++) {
      pid = clist.Pid[i];

      bzero((void *) &cons, sizeof(XmemDrvrClientConnections));
      cons.Pid = pid;

      if (ioctl(xmem,XmemDrvrGET_CLIENT_CONNECTIONS,&cons) < 0) {
	 IErr("GET_CLIENT_CONNECTIONS",NULL);
	 return arg;
      }

      if (pid == mypid) printf("Pid: %d (xmemtest) ",pid);
      else              printf("Pid: %d ",pid);

      if (cons.Size) {
	 for (j=0; j<cons.Size; j++) {
	    con = &(cons.Connections[j]);
	    printf("[Mod:%d Mask:0x%X]:",(int) con->Module,(int) con->Mask);
	    for (k=0; k<XmemDrvrIntrSOURCES; k++) {
	       if ((1<<k) & con->Mask) {
		  if (intr_names[k] != NULL)
		     printf(" %s",intr_names[k]);
		  else
		     printf(" Bit%02d",k);
	       }
	    }
	 }
      } else printf("No connections");
      printf("\n");
   }
   return arg;
}

/*****************************************************************/

int Reset(int arg) {

int module;

   arg++;

   if (ioctl(xmem,XmemDrvrRESET,NULL) < 0) {
      IErr("RESET",NULL);
      return arg;
   }

   module = -1;
   if (ioctl(xmem,XmemDrvrGET_MODULE,&module) < 0) {
      IErr("GET_MODULE",NULL);
      return arg;
   }

   printf("Reset Module: %d\n",module);

   return arg;
}

/*****************************************************************/

#define STAT_STR_SIZE 512

char *StatusToStr(unsigned long stat) {

static char res[STAT_STR_SIZE];

   bzero((void *) res,STAT_STR_SIZE);

   if (XmemDrvrScrLED_ON            & stat) strcat(res," [LedOn]\n");
   else                                     strcat(res," [LedOff]\n");
   if (XmemDrvrScrTR_OFF            & stat) strcat(res," [TransmitterOff]\n");
   else                                     strcat(res," [TransmitterOn]\n");
   if (XmemDrvrScrDARK_ON           & stat) strcat(res," [DarkFillOn]\n");
   else                                     strcat(res," [DarkFillOff]\n");
   if (XmemDrvrScrLOOP_ON           & stat) strcat(res," [LoopBackTestModeOn]\n");
   if (XmemDrvrScrPARITY_ON         & stat) strcat(res," [ParityCheckOn]\n");
   else                                     strcat(res," [ParityCheckOff]\n");
   if (XmemDrvrScrREDUNDANT_ON      & stat) strcat(res," [RedundantModeOn]\n");
   if (XmemDrvrScrROGUE_MASTER_1_ON & stat) strcat(res," [RogueMasterOne]\n");
   if (XmemDrvrScrROGUE_MASTER_0_ON & stat) strcat(res," [RogueMasterZero]\n");
   if (XmemDrvrScr128MB             & stat) strcat(res," [128MbRAM]\n");
   else                                     strcat(res," [64MbRAM]\n");
   if (XmemDrvrScrTX_EMPTY          & stat) strcat(res," [XmitEmpty]\n");
   if (XmemDrvrScrTX_ALMOST_FULL    & stat) strcat(res," [XmitAlmostFull]\n");
   if (XmemDrvrScrRX_OVERFLOW       & stat) strcat(res," [RcvrFull]\n");
   if (XmemDrvrScrRX_ALMOST_FULL    & stat) strcat(res," [RcvrAlmostFull]\n");
   if (XmemDrvrScrSYNC_LOST         & stat) strcat(res," [SyncLost]\n");
   else                                     strcat(res," [SyncOk]\n");
   if (XmemDrvrScrSIGNAL_DETECT     & stat) strcat(res," [SignalDetected]\n");
   else                                     strcat(res," [NoSignalPresent]\n");
   if (XmemDrvrScrDATA_LOST         & stat) strcat(res," [DataBad]\n");
   else                                     strcat(res," [DataOk]\n");
   if (XmemDrvrScrOWN_DATA          & stat) strcat(res," [OwnDataDetected]\n");
   else                                     strcat(res," [OwnDataNotSet]\n");

   return res;
}

/*****************************************************************/

volatile char *TimeToStr(time_t tod) {

static char tbuf[128];

char tmp[128];
char *yr, *ti, *md, *mn, *dy;

    bzero((void *) tbuf, 128);
    bzero((void *) tmp, 128);

    if (tod) {
	ctime_r (&tod, tmp);

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

	sprintf (tbuf, "%s-%s/%s/%s %s"  , dy, md, mn, yr, ti);

    } else sprintf(tbuf, "--- Zero ---");

    return (tbuf);
}

/*****************************************************************/

int GetVersion(int arg) {

XmemDrvrVersion version;
time_t tod;

   arg++;

   bzero((void *) &version, sizeof(XmemDrvrVersion));
   if (ioctl(xmem,XmemDrvrGET_VERSION,&version) < 0) {
      IErr("GET_VERSION",NULL);
      return arg;
   }

   tod = version.DriverVersion;
   printf("Drvr Compiled: [%u] %s\n",(int) tod,TimeToStr(tod));
   printf("Board Revision: %d ID: %d\n", (int) version.BoardRevision,(int) version.BoardId);

   printf("%s Compiled: %s %s\n", "xmemtest", __DATE__, __TIME__);

   return arg;
}

/*****************************************************************/

static int connected = 0;

int WaitInterrupt(int arg) { /* msk */
ArgVal   *v;
AtomType  at;

XmemDrvrConnection con;
XmemDrvrReadBuf rbf;
int i, cc, qflag, qsize, nowait, interrupt, cnt, bit;
unsigned long nid;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   nowait = 0;
   if (at == Operator) {
      if (v->OId == OprNOOP) {
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;

	 printf("WaitInterrupt: [-] Optional No Wait, Connect only\n");
	 printf("WaitInterrupt: Sources are:\n");
	 for (i=0; i<XmemDrvrIntrSOURCES; i++) {
	    if (intr_names[i] != NULL) {
	       printf("0x%04X %s\n",(1<<i),intr_names[i]);
	    }
	 }
	 return arg;
      }
      if (v->OId == OprMI) {    /* A - means don't wait, just connect */
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;

	 nowait = 1;
      }
   }

   interrupt = 0;
   if (at == Numeric) {                 /* Hardware mask ? */
      arg++;

      if (v->Number) {
	 interrupt  = v->Number;
	 con.Module = module;
	 con.Mask   = interrupt;
	 if (ioctl(xmem,XmemDrvrCONNECT,&con) < 0) {
	    IErr("CONNECT",&interrupt);
	    return arg;
	 }
	 connected = 1;
      } else {
	 con.Module = 0;
	 con.Mask   = 0;
	 if (ioctl(xmem,XmemDrvrDISCONNECT,&con) < 0) {
	    IErr("DISCONNECT",&interrupt);
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

   if (nowait) {
      printf("WaitInterrupt: Connect: No Wait: OK\n");
      return arg;
   }

   cnt = 0;
   do {
      cc = read(xmem,&rbf,sizeof(XmemDrvrReadBuf));
      if (cc <= 0) {
	 printf("Time out or Interrupted call\n");
	 return arg;
      }
      if (interrupt != 0) {
	 if (rbf.Mask & interrupt) break;
      } else break;
      if (cnt++ > 64) {
	 printf("Aborted wait loop after 64 iterations\n");
	 return arg;
      }
   } while (True);

   if (ioctl(xmem,XmemDrvrGET_QUEUE_FLAG,&qflag) < 0) {
      IErr("GET_QUEUE_FLAG",NULL);
      return arg;
   }

   if (ioctl(xmem,XmemDrvrGET_QUEUE_SIZE,&qsize) < 0) {
      IErr("GET_QUEUE_SIZE",NULL);
      return arg;
   }

   printf("Mod[%d] Int[0x%X]",
	  (int) rbf.Module,
	  (int) rbf.Mask);

   for (i=0; i<XmemDrvrIntrSOURCES; i++) {
      bit = 1<<i;
      if (bit & rbf.Mask) {

	 if (intr_names[i] != NULL) printf("[%s",intr_names[i]);
	 else                       printf("[Bit%02d", i);

	 if (bit == XmemDrvrIntrPENDING_INIT) {
	    nid = rbf.NodeId[XmemDrvrIntIdxPENDING_INIT];
	    printf(" Nd:%s/%d Dt:%d", GetNodeName(1 << (nid -1)), (int) nid,
		  (int) rbf.NdData[XmemDrvrIntIdxPENDING_INIT]);

	 } else if (bit == XmemDrvrIntrSEGMENT_UPDATE) {
	    nid = rbf.NodeId[XmemDrvrIntIdxSEGMENT_UPDATE];
	    printf(" Nd:%s/%d Sg:0x%X", GetNodeName(1 << (nid -1)), (int) nid,
		  (int) rbf.NdData[XmemDrvrIntIdxSEGMENT_UPDATE]);

	 } else if (bit == XmemDrvrIntrINT_1) {
	    nid = rbf.NodeId[XmemDrvrIntIdxINT_1];
	    printf(" Nd:%s/%d Dt:%d", GetNodeName(1 << (nid -1)), (int) nid,
		  (int) rbf.NdData[XmemDrvrIntIdxINT_1]);

	 } else if (bit == XmemDrvrIntrINT_2) {
	    nid = rbf.NodeId[XmemDrvrIntIdxINT_2];
	    printf(" Nd:%s/%d Dt:%d", GetNodeName(1 << (nid -1)), (int) nid,
		  (int) rbf.NdData[XmemDrvrIntIdxINT_2]);
	 } else if (bit == XmemDrvrIntrSOFTWAKEUP) {
		 nid = rbf.NodeId[XmemDrvrIntIdxSOFTWAKEUP];
		 printf(" Nd:%s/ Dt:%d", GetNodeName(1 << (nid - 1)),
			 (int)rbf.NdData[XmemDrvrIntIdxSOFTWAKEUP]);
	 }
	 printf("]");
      }
   }

   if (qflag == 0) {
      if (qsize) printf(" Queued: %d",(int) qsize);
   }
   printf("\n");

   return arg;
}

/*****************************************************************/

#define VCMDS 6
#define VCMD_BITS 0xF8000001

char *vcmd_hlp[VCMDS] = { "StatusLED-On","Transmitter-Off","DarkFillingMode-On","LoopBackMode-On","ParityCheck-On", "OwnDataLED-On" };
int   vcmd_bit[VCMDS] = {  0x80000000,    0x40000000,       0x20000000,          0x10000000,       0x08000000,       0x00000001 };

int SetCommand(int arg) { /* Command */
ArgVal   *v;
AtomType  at;

int i;
unsigned long stat;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Operator) {
      if (v->OId == OprNOOP) {
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;
	 printf("Commands are:\n");
	 for (i=0; i<VCMDS; i++) {
	    printf("0x%08x %s\n",vcmd_bit[i],vcmd_hlp[i]);
	 }
      }
      return arg;
   }

   if (at == Numeric) {
      arg++;
      stat = v->Number & VCMD_BITS;
      if (ioctl(xmem,XmemDrvrSET_COMMAND,&stat) < 0) {
	 IErr("SET_COMMAND",(int *) &stat);
	 return arg;
      }
   }

   if (ioctl(xmem,XmemDrvrGET_STATUS,&stat) < 0) {
      IErr("GET_STATUS",NULL);
      return arg;
   }
   printf("Mod:%d Status:0x%08x\n%s",module,(int) stat, StatusToStr(stat));

   return arg;
}

/*****************************************************************/
/* Show the node network                                         */

int ShowNetwork(int arg) {

int i;
unsigned long nodes, msk;

   arg++;

   if (ioctl(xmem,XmemDrvrGET_NODES,&nodes) < 0) {
      IErr("GET_NODES",NULL);
      return arg;
   }
   if (nodes) {
      printf("ShowNetwork: Running node Mask: 0x%02X\n",(int) nodes);
      if (node_tab.Used == 0) {
	 for (i=0; i<XmemDrvrNODES; i++) {
	    msk = 1 << i;
	    if (msk & nodes) printf("Node:%02d Id:0x%08X :UP\n",i+1,(int) msk);
	 }
      } else {
	 for (i=0; i<XmemDrvrNODES; i++) {
	    msk = 1 << i;
	    if (msk & node_tab.Used) {
	       printf("Node:%02d Name:%8s Id:0x%08X ",
		       i+1,
		       node_tab.Descriptors[i].Name,
		 (int) node_tab.Descriptors[i].Id);

	       if (msk & nodes) printf(":OK\n");
	       else             printf(":..\n");
	    }
	 }
      }
   } else printf("ShowNetwork: Network not setup\n");

   return arg;
}

/*****************************************************************/
/* Write segment table to the hardware                           */

int WriteSegTable(int arg) {

   arg++;

   if (seg_tab.Used) {
      if (ioctl(xmem,XmemDrvrSET_SEGMENT_TABLE,&seg_tab) < 0) {
	 IErr("SET_SEGMENT_TABLE",(int *) &seg_tab.Used);
	 return arg;
      }
      printf("SegmentTable: Written to driver: Used: 0x%08x\n",(int) seg_tab.Used);
   } else printf("SegmentTable: Empty: Not written to hardware\n");

   return arg;
}

/*****************************************************************/

int ReadSegTable(int arg) {

   arg++;

   if (ioctl(xmem,XmemDrvrGET_SEGMENT_TABLE,&seg_tab) < 0) {
      IErr("GET_SEGMENT_TABLE",NULL);
      return arg;
   }
   if (seg_tab.Used) printf("SegmentTable: Read from driver: Used: 0x%08x\n",(int) seg_tab.Used);
   else              printf("SegmentTable: Read back is empty\n");
   return arg;
}

/*****************************************************************/

int ShowSegTable(int arg) {

int i, adr, segcnt;

   arg++;

   adr = segcnt = 0;

   if (ioctl(xmem,XmemDrvrGET_SEGMENT_TABLE,&seg_tab) < 0) {
      IErr("GET_SEGMENT_TABLE",NULL);
      return arg;
   }

   if (seg_tab.Used == 0) {
      if (YesNo("SegmentTable: Empty:","Set it to the default")) {
	 seg_tab.Used = 0xFFFFFFFF;
	 for (i=0; i<XmemDrvrSEGMENTS; i++) {
	    sprintf(seg_tab.Descriptors[i].Name,"Seg_%02d",i+1);
	    seg_tab.Descriptors[i].Id = 1<<i;
	    seg_tab.Descriptors[i].Size = 0x200000;
	    seg_tab.Descriptors[i].Address = (char *) adr;
	    adr += 0x200000;
	    seg_tab.Descriptors[i].Nodes = 0xFFFFFFFF;
	    seg_tab.Descriptors[i].User  = 3;
	 }
	 if (ioctl(xmem,XmemDrvrSET_SEGMENT_TABLE,&seg_tab) < 0) {
	    IErr("SET_SEGMENT_TABLE",(int *) &seg_tab.Used);
	    return arg;
	 }
      }
   }
   for (i=0; i<XmemDrvrSEGMENTS; i++) {
      if (seg_tab.Used & (1<<i)) {
	 segcnt++;
	 printf("%02d { %s Id 0x%08x Sz 0x%x Addr 0x%07x Nodes 0x%08x User 0x%08x }\n",
		(i+1),
		      seg_tab.Descriptors[i].Name,
		(int) seg_tab.Descriptors[i].Id,
		(int) seg_tab.Descriptors[i].Size,
		(int) seg_tab.Descriptors[i].Address,
		(int) seg_tab.Descriptors[i].Nodes,
		(int) seg_tab.Descriptors[i].User);
      }
   }
   printf("Segments in use: %d\n",segcnt);
   return arg;
}

/*****************************************************************/

int EditSegTable(int arg) {

char *cp;
char txt[128];

   arg++;

   sprintf(txt,"%s ",GetFile(editor));

   cp = GetFileName(&arg);
   if (cp == NULL) {
      cp = GetFile(SEG_TABLE_NAME);
      if (cp == NULL) {
	 printf("Couldn't locate file: %s\n",SEG_TABLE_NAME);
	 return arg;
      }
   }

   strcat(txt,cp);
   printf("\n%s\n",txt);
   if (YesNo("Execute:",txt)) {
      system(txt);
      printf("\n");
   } else printf("Aborted\n");
   return arg;
}

/*****************************************************************/

int WriteSegTableFile(int arg) {

int i, segcnt;
FILE *fp;
char *cp;

   arg++;

   cp = GetFileName(&arg);
   if (cp == NULL) {
      cp = GetFile(SEG_TABLE_NAME);
      if (cp == NULL) {
	 printf("Couldn't locate file: %s\n",SEG_TABLE_NAME);
	 return arg;
      }
   }
   segcnt = 0;

   fp = fopen(cp,"w");
   if (fp) {
      for (i=0; i<XmemDrvrSEGMENTS; i++) {
	 if (seg_tab.Used & (1<<i)) {
	    segcnt++;
	    fprintf(fp,"{ %s 0x%08x 0x%x 0x%07x 0x%08x 0x%08x }\n",
			  seg_tab.Descriptors[i].Name,
		    (int) seg_tab.Descriptors[i].Id,
		    (int) seg_tab.Descriptors[i].Size,
		    (int) seg_tab.Descriptors[i].Address,
		    (int) seg_tab.Descriptors[i].Nodes,
		    (int) seg_tab.Descriptors[i].User);
	 }
      }

      printf("Written: %d segments to: %s\n",segcnt,cp);

   } else printf("Could't open file: %s for write\n",cp);
   return arg;
}

/*****************************************************************/

int ReadNodeTableFile(int arg) {

int i, fc, nodecnt;
FILE *fp;
char *cp;

   arg++;

   cp = GetFileName(&arg);
   if (cp == NULL) {
      cp = GetFile(NODE_TABLE_NAME);
      if (cp == NULL) {
	 printf("Couldn't locate file: %s\n",NODE_TABLE_NAME);
	 return arg;
      }
   }
   nodecnt = 0;

   fp = fopen(cp,"r");
   if (fp) {
      node_tab.Used = 0;
      for (i=0; i<XmemDrvrNODES; i++) {
	 fc = fscanf(fp,"{ %s 0x%x }\n",
		    (char *) &node_tab.Descriptors[i].Name[0],
		    (int  *) &node_tab.Descriptors[i].Id);

	 if (fc == EOF) break;
	 if (fc != 2) {
	    printf("Syntax error in input at line: %d\n",i+1);
	    break;
	 }
	 node_tab.Used |= node_tab.Descriptors[i].Id;
	 nodecnt++;
      }

      printf("Read: %d Node descriptors from: %s\n",nodecnt,cp);

   } else printf("Could't open file: %s for read\n",cp);
   return arg;
}

/*****************************************************************/

int ReadSegTableFile(int arg) {
int i, fc, segcnt;
FILE *fp;
char *cp;

   arg++;

   cp = GetFileName(&arg);
   if (cp == NULL) {
      cp = GetFile(SEG_TABLE_NAME);
      if (cp == NULL) {
	 printf("Couldn't locate file: %s\n",SEG_TABLE_NAME);
	 return arg;
      }
   }
   segcnt = 0;
   fp = fopen(cp,"r");
   if (fp) {
      seg_tab.Used = 0;
      for (i=0; i<XmemDrvrSEGMENTS; i++) {
	 fc = fscanf(fp,"{ %s 0x%x 0x%x 0x%x 0x%x 0x%x }\n",
		    (char *) &seg_tab.Descriptors[i].Name[0],
		    (int  *) &seg_tab.Descriptors[i].Id,
		    (int  *) &seg_tab.Descriptors[i].Size,
		    (int  *) &seg_tab.Descriptors[i].Address,
		    (int  *) &seg_tab.Descriptors[i].Nodes,
		    (int  *) &seg_tab.Descriptors[i].User);

	 if (fc == EOF) break;
	 if (fc != 6) {
	    printf("Syntax error in input at line: %d\n",i+1);
	    break;
	 }
	 seg_tab.Used |= seg_tab.Descriptors[i].Id;
	 segcnt++;
      }

      printf("Read: %d Segment descriptors from: %s\n",segcnt,cp);

   } else printf("Could't open file: %s for read\n",cp);
   return arg;
}

/*****************************************************************/

int WriteSegFile(int arg) {

FILE *fp;
char *cp;
char txt[128], snam[16];
XmemDrvrSegIoDesc sgio, *sgiop;
unsigned long msk;
int i, j, cnt;

   arg++;

   sprintf(txt,"%s ",GetFile(editor));
   sprintf(snam,"XmemSeg%02d.bin",(int) segment);

   cp = GetFileName(&arg);
   if (cp == NULL) {
      cp = GetFile(snam);
      if (cp == NULL) {
	 printf("Couldn't locate file: %s\n",snam);
	 return arg;
      }
   }

   msk = 1 << (segment -1);
   if (seg_tab.Used & msk) {
      i = segment -1;
      printf("CurrentSegment: %02d { %s Id 0x%08x Sz 0x%x Addr 0x%07x Nodes 0x%08x User 0x%08x }\n",
	     (i+1),
		   seg_tab.Descriptors[i].Name,
	     (int) seg_tab.Descriptors[i].Id,
	     (int) seg_tab.Descriptors[i].Size,
	     (int) seg_tab.Descriptors[i].Address,
	     (int) seg_tab.Descriptors[i].Nodes,
	     (int) seg_tab.Descriptors[i].User);
      if (YesNo("Write this segment to",cp)) {
	 fp = fopen(cp,"w");
	 if (fp) {

	    sgio.Module    = module;
	    sgio.Id        = msk;
	    sgio.Offset    = 0;
	    sgio.Size      = seg_tab.Descriptors[segment -1].Size;
	    sgio.UserArray = (unsigned char *) seg_page_buf;
	    sgio.UpdateFlg = 0;

	    ChopSegment(&sgio);

	    for (i=FIRST; i<IO_PARTS; i++) {
	       sgiop = &(sgiob.IoDesc[i]);

	       if (i==MIDDLE) cnt = sgiob.Pages;
	       else           cnt = 1;

	       for (j=0; j<cnt; j++) {
		  if (sgiop->Size) {
		     bzero((void *) seg_page_buf, ONE_PAGE);
		     if (ioctl(xmem,XmemDrvrREAD_SEGMENT,sgiop) < 0) {
			IErr("READ_SEGMENT",NULL);
			break;
		     }
		     if (fwrite(seg_page_buf,sgiop->Size,1,fp) <= 0) {
			printf("Failed to write to file: %s\n",cp);
			break;
		     }
		     sgiop->Offset += sgiop->Size;
		  }
	       }
	    }
	    fclose(fp);
	    printf("Written: %d Pages to file: %s\n",(int) sgiob.Pages + 1,cp);
	 } else printf("Can't open: %s for write\n",cp);
      }
   } else printf("Segment: %d not in use\n",(int) segment);
   return arg;
}

/*****************************************************************/

int ReadSegFile(int arg) {

FILE *fp;
char *cp;
char txt[128], snam[16];
XmemDrvrSegIoDesc sgio, *sgiop;
XmemDrvrSendBuf msb;
unsigned long msk;
int i, j, cnt;

   arg++;

   sprintf(txt,"%s ",GetFile(editor));
   sprintf(snam,"XmemSeg%02d.bin",(int) segment);

   cp = GetFileName(&arg);
   if (cp == NULL) {
      cp = GetFile(snam);
      if (cp == NULL) {
	 printf("Couldn't locate file: %s\n",snam);
	 return arg;
      }
   }

   msk = 1 << (segment -1);

   i = segment -1;

   if (seg_tab.Used & msk) {
      printf("CurrentSegment: %02d { %s Id 0x%08x Sz 0x%x Addr 0x%07x Nodes 0x%08x User 0x%x }\n",
	     (i+1),
		   seg_tab.Descriptors[i].Name,
	     (int) seg_tab.Descriptors[i].Id,
	     (int) seg_tab.Descriptors[i].Size,
	     (int) seg_tab.Descriptors[i].Address,
	     (int) seg_tab.Descriptors[i].Nodes,
	     (int) seg_tab.Descriptors[i].User);
   } else {
      printf("Segment: %d Not in use\n",i+1);
      return arg;
   }

   if (YesNo("OverWrite this segment from",cp)) {
      fp = fopen(cp,"r");
      if (fp) {
	 sgio.Module    = module;
	 sgio.Id        = msk;
	 sgio.Offset    = 0;
	 sgio.Size      = seg_tab.Descriptors[segment -1].Size;
	 sgio.UserArray = (unsigned char *) seg_page_buf;
	 sgio.UpdateFlg = 0;

	 ChopSegment(&sgio);

	 for (i=FIRST; i<IO_PARTS; i++) {
	    sgiop = &(sgiob.IoDesc[i]);

	    if (i==MIDDLE) cnt = sgiob.Pages;
	    else           cnt = 1;

	    for (j=0; j<cnt; j++) {
	       if (sgiop->Size) {
		  bzero((void *) seg_page_buf, ONE_PAGE);
		  if (fread(seg_page_buf,sgiop->Size,1,fp) <= 0) {
		     printf("Failed to read from file: %s\n",cp);
		     break;
		  }
		  if (ioctl(xmem,XmemDrvrWRITE_SEGMENT,sgiop) < 0) {
		     IErr("WRITE_SEGMENT",NULL);
		     break;
		  }
		  sgiop->Offset += sgiop->Size;
	       }
	    }
	 }

	 fclose(fp);

	 /* Broadcast a SEGMENT_UPDATE message, I have turned off the */
	 /* automatic broadcast by setting UpdateFlg to zero. */

	 bzero((void *) &msb, sizeof(XmemDrvrSendBuf));
	 msb.Module = module;
	 msb.InterruptType = XmemDrvrNicBROADCAST | XmemDrvrNicSEGMENT_UPDATE;
	 msb.Data = msk;
	 if (ioctl(xmem,XmemDrvrSEND_INTERRUPT,&msb) < 0) {
	    IErr("SEND_INTERRUPT",(int *) &msb.InterruptType);
	    return arg;
	 }

	 printf("Read: %d Pages from file: %s\n",(int) sgiob.Pages + 1,cp);
      } else printf("Can't open: %s for read\n",cp);
   }
   return arg;
}

/*****************************************************************/

int SegIoDma(int arg) {

XmemDrvrSegIoDesc sgio;
unsigned long msk;
int i;

   arg++;

   msk = 1 << (segment -1);
   if (seg_tab.Used & msk) {
      i = segment -1;

      sgio.Module    = module;
      sgio.Id        = msk;
      sgio.Offset    = 0;
      sgio.Size      = seg_tab.Descriptors[segment -1].Size;
      sgio.UserArray = (unsigned char *) malloc(sgio.Size);
      sgio.UpdateFlg = 0;

      if (sgio.UserArray) {
		mlock((void *)sgio.UserArray, sgio.Size);

		printf("Reading:%d bytes (%f MB) via DMA:",(int) sgio.Size,
			(double)sgio.Size/(1024*1024));

		if (ioctl(xmem,XmemDrvrREAD_SEGMENT,&sgio) < 0)
			IErr("READ_SEGMENT",NULL);
		else
			printf("OK\n");

		printf("Writing:%d bytes via DMA:",(int) sgio.Size);

		if (ioctl(xmem,XmemDrvrWRITE_SEGMENT,&sgio) < 0)
			IErr("WRITE_SEGMENT",NULL);
		else
			printf("OK\n");

		munlock((void *)sgio.UserArray, sgio.Size);
		free((unsigned char *) sgio.UserArray);

      } else printf("Can't allocate:%d bytes\n",(int) sgio.Size);
   } else printf("Segment:%d not in use\n",(int) segment);
   return arg;
}

/*****************************************************************/

int Segment(int arg) {
ArgVal   *v;
AtomType  at;
int seg, msk;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   seg = segment;
   msk = 1 << (seg -1);
   if (at == Numeric) {
      arg++;
      seg = v->Number;
      if ((seg >= 1) && (seg <= XmemDrvrSEGMENTS)) {
	 segment = seg;
	 msk = 1 << (seg -1);
	 if ((seg_tab.Used & msk) == 0) {
	    printf("Warning: Segment: %d Is not defined\n",(int) segment);
	 }
      }
   }
   printf("Segment: %d Mask Bit: 0x%08x\n",(int) segment, msk);
   return arg;
}

/*****************************************************************/

int NextSegment(int arg) {

   arg++;

   if (segment < XmemDrvrSEGMENTS) {
      segment++;
   } else {
      segment = 1;
   }
   return arg;
}

/*****************************************************************/

int PrevSegment(int arg) {

   arg++;

   if (segment > 1) {
      segment--;
   } else {
      segment = XmemDrvrSEGMENTS;
   }
   return arg;
}

/*****************************************************************/

#define MSG_TYPES 5
char *msg_types[MSG_TYPES] = { "RqReset", "Int1", "Int2", "SegUpdate", "Initialized" };

#define MSG_CASTS 3
char *msg_casts[MSG_CASTS] = { "BroadCast", "MultiCast", "UniCast" };

int SendMessage(int arg) {  /* msg_type, msg_cast, data, [destination] */
ArgVal   *v;
AtomType  at;
int i;
unsigned long mtyp, mcst, mdat, mdst;

XmemDrvrSendBuf msbf;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Operator) {
      if (v->OId == OprNOOP) {
	 printf("SendMessage: MsgType MsgCast [<data>] [<destinations>]\n");

	 printf("MsgType:");
	 for (i=0; i<MSG_TYPES; i++) printf(" [%1d/%s]",i,msg_types[i]);
	 printf("\n");

	 printf("MsgCast:");
	 for (i=0; i<MSG_CASTS; i++) printf(" [%1d/%s]",i,msg_casts[i]);
	 printf("\n");

	 return arg;
      }
   }

   mtyp = 0;
   if (at == Numeric) {
      i = v->Number;
      if (i > 4) {
	 printf("Message Type parameter: %d out of range [0..4]\n", i);
	 return arg;
      }
      if      (i == 0) mtyp = XmemDrvrNicREQUEST_RESET;
      else if (i == 1) mtyp = XmemDrvrNicINT_1;
      else if (i == 2) mtyp = XmemDrvrNicINT_2;
      else if (i == 3) mtyp = XmemDrvrNicSEGMENT_UPDATE;
      else if (i == 4) mtyp = XmemDrvrNicINITIALIZED;

      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }

   mcst = 0;
   if (at == Numeric) {
      i = v->Number;
      if (i > 2) {
	 printf(" Message Cast parameter: %d out of range [0..2]\n", i);
	 return arg;
      }
      if      (i == 0) mcst = XmemDrvrNicBROADCAST;
      else if (i == 1) mcst = XmemDrvrNicMULTICAST;
      else if (i == 2) mcst = XmemDrvrNicUNICAST;

      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }

   mdat = 0;
   if (at == Numeric) {
      mdat = v->Number;

      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }

   mdst = 0;
   if (at == Numeric) {
      mdst = v->Number;

      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }


   bzero((void *) &msbf, sizeof(XmemDrvrSendBuf));
   msbf.Module = module;
   if (mcst == XmemDrvrNicUNICAST) msbf.UnicastNodeId = mdst;
   else                            msbf.MulticastMask = mdst;
   msbf.InterruptType = mtyp | mcst;
   msbf.Data = mdat;

   if (ioctl(xmem,XmemDrvrSEND_INTERRUPT,&msbf) < 0) {
      IErr("SEND_INTERRUPT",(int *) &msbf.InterruptType);
      return arg;
   }

   printf("Message sent OK\n");

   return arg;
}

/* send a softwake with contents: nodeid, data */
int SendSoftWakeup(int arg)
{
	XmemDrvrWriteBuf	wbuf;
	ArgVal			*v;
	AtomType		at;
	int			err;

	arg++;
	v = &vals[arg];
	at = v->Type;
	if (at == Operator && v->OId == OprNOOP) {
		printf("SendMessage: NodeId Data\n");
		return arg;
	}

	wbuf.Module	= 1;
	wbuf.Data	= 1;
	wbuf.NodeId	= 1;

	err = write(xmem, &wbuf, sizeof(XmemDrvrWriteBuf));
	if (err <= 0) {
		printf("Err when sending SoftWakeUp\n");
		return arg;
	}

	return arg;
}

/*****************************************************************/

int CheckSeg(int arg) {
unsigned long smsk, msk;
int i;

   arg++;

   if (ioctl(xmem,XmemDrvrCHECK_SEGMENT,&smsk) < 0) {
      IErr("CHECK_SEGMENT",NULL);
      return arg;
   }

   if (smsk) {
      printf("Updated Segments Mask: 0x%08X\n",(int) smsk);
      printf("Updated Segments:");
      for (i=0; i<XmemDrvrSEGMENTS; i++) {
	 msk = 1<<i;
	 if (msk & smsk) {
	    if (seg_tab.Used & msk) printf(" %s",seg_tab.Descriptors[i].Name);
	    else                    printf(" UnUsedSeg:%d",i+1);
	 }
      }
      printf("\n");
   } else printf("No updated segments found\n");

   return arg;
}

/*****************************************************************/

int FlushSegs(int arg) {
ArgVal   *v;
AtomType  at;
unsigned long smsk;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   smsk = 0xFFFFFFFF;
   if (at == Numeric) {
      smsk = v->Number;
      arg++;
   }

   if (ioctl(xmem,XmemDrvrFLUSH_SEGMENTS,&smsk) < 0) {
      IErr("FLUSH_SEGMENTS",(int *) &smsk);
      return arg;
   }
   printf("Flushed Segments Mask: 0x%08X\n",(int) smsk);

   return arg;
}

/*****************************************************************/

int EditSeg(int arg) {  /* Offset */
ArgVal   *v;
AtomType  at;
int msk, address;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   msk = 1 << (segment -1);
   if (seg_tab.Used & msk) {
      address = (int) seg_tab.Descriptors[segment -1].Address;
      if (at == Numeric) {
	 arg++;
	 address += v->Number;
      }

      ioctl(xmem,XmemDrvrRAW_OPEN,NULL);
      EditMemory(address,IoFieldTypeVMIC_SDRAM);
      ioctl(xmem,XmemDrvrRAW_CLOSE,NULL);

   } else printf("Segment: %d is not defined\n",(int) segment);

   return arg;
}

/*****************************************************************/

int DmaThreshold(int arg) {  /* Dma threshold */
ArgVal   *v;
AtomType  at;

unsigned long thresh;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Numeric) {
      arg++;
      thresh = v->Number;
      if (ioctl(xmem,XmemDrvrSET_DMA_THRESHOLD,&thresh) < 0) {
	 IErr("SET_DMA_THRESHOLD",(int *) &thresh);
	 return arg;
      }
   }

   thresh = -1;
   if (ioctl(xmem,XmemDrvrGET_DMA_THRESHOLD,&thresh) < 0) {
      IErr("GET_DMA_THRESHOLD",NULL);
      return arg;
   }

   printf("DmaThreshold: %d 0x%x (%d Pages)\n",(int) thresh,(int) thresh,(int) thresh/1024);

   return arg;
}

/*****************************************************************/

int SetSegPattern(int arg) {  /* Pattern, offset, size */
ArgVal   *v;
AtomType  at;

unsigned long patn, msk;
XmemDrvrSegIoDesc sgio, *sgiop;
XmemDrvrSendBuf msb;
int i, j, cnt, ofst, ssze;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Numeric) {
      arg++;
      patn = v->Number;

      memset(seg_page_buf, (char)patn, ONE_PAGE);

      v = &(vals[arg]);
      at = v->Type;

      ofst = 0;
      ssze = seg_tab.Descriptors[segment -1].Size;

      if (at == Numeric) {
	 arg++;
	 if (v->Number < ssze) ofst = v->Number;
	 ssze -= ofst;

	 v = &(vals[arg]);
	 at = v->Type;

	 if (at == Numeric) {
	    arg++;
	    if (v->Number < ssze) ssze = v->Number;
	 }
      }

      sgio.Module    = module;
      sgio.Id        = 1 << (segment -1);;
      sgio.Offset    = ofst;
      sgio.Size      = ssze;
      sgio.UserArray = seg_page_buf;
      sgio.UpdateFlg = 0;

      ChopSegment(&sgio);

      msk = 1 << (segment -1);
      if (seg_tab.Used & msk) {
	 for (i=FIRST; i<IO_PARTS; i++) {

	    sgiop = &(sgiob.IoDesc[i]);

	    if (i==MIDDLE) cnt = sgiob.Pages;
	    else           cnt = 1;

	    for (j=0; j<cnt; j++) {
	       if (sgiop->Size) {
		  if (ioctl(xmem,XmemDrvrWRITE_SEGMENT,sgiop) < 0) {
		     IErr("WRITE_SEGMENT",(int *) &i);
		     return arg;
		  }
		  sgiop->Offset += sgiop->Size;
	       } else break;
	    }
	 }

	 /* Broadcast a SEGMENT_UPDATE message, I have turned off the */
	 /* automatic broadcast by setting UpdateFlg to zero. */

	 bzero((void *) &msb, sizeof(XmemDrvrSendBuf));
	 msb.Module = module;
	 msb.InterruptType = XmemDrvrNicBROADCAST | XmemDrvrNicSEGMENT_UPDATE;
	 msb.Data = msk;
	 if (ioctl(xmem,XmemDrvrSEND_INTERRUPT,&msb) < 0) {
	    IErr("SEND_INTERRUPT",(int *) &msb.InterruptType);
	    return arg;
	 }
	 printf("Written: %d blocks of: %d to segment: %d OK\n",(int) sgiob.Pages + 1
							       ,(int) patn
							       ,(int) segment);
      } else printf("Segment: %d Not in use\n",(int) segment);
   } else printf("No fill value supplied\n");
   return arg;
}

/*****************************************************************/

int SearchPattern(int arg) {  /* Pattern */
ArgVal   *v;
AtomType  at;

unsigned long pat, adr, val, fndcnt;
XmemDrvrSegIoDesc sgio, *sgiop;
int i, j, k, len, cnt, found;
char c;
unsigned short w;
unsigned long l;
OprId opr;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   len = 1;
   if (at == Alpha) {
      if ((v->Text[0] == 'b') || (v->Text[0] == 'B')) len = 1;
      if ((v->Text[0] == 'w') || (v->Text[0] == 'W')) len = 2;
      if ((v->Text[0] == 'l') || (v->Text[0] == 'L')) len = 4;
      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }

   opr = OprEQ;
   if (at == Operator) {
      opr = v->OId;
      arg++;
      v = &(vals[arg]);
      at = v->Type;
      if (opr == OprNOOP) {
	 printf("Search Segment: [B|W|L] [<Operator>] <Pattern>\n");
	 printf("Where [B|W|L]      Optional Pattern match size: Byte|Word|Long\n");
	 printf("Where <Pattern>    Required Byte|Word|Long value to be matched\n");
	 printf("Where [<Operator>] Optional Match operator can be:\n");
	 for (i=OprNE; i<OprAS; i++)
	    printf("%s\t%s\n",oprs[i].Name,oprs[i].Help);
	 return arg;
      }
   }

   if (at == Numeric) {
      arg++;
      pat = v->Number;

      fndcnt = 0;
      printf("Segment: %02d Pattern:(%s)%d=0x%x {%s} Size:%s:\n",
	     (int) segment,oprs[(int) opr].Name,(int) pat,(int) pat,oprs[(int) opr].Help,lnams[len]);

      sgio.Module    = module;
      sgio.Id        = 1 << (segment -1);
      sgio.Offset    = 0;
      sgio.Size      = seg_tab.Descriptors[segment -1].Size;
      sgio.UserArray = (unsigned char *) seg_page_buf;
      sgio.UpdateFlg = 0;

      ChopSegment(&sgio);

      for (i=FIRST; i<IO_PARTS; i++) {
	 sgiop = &(sgiob.IoDesc[i]);

	 if (i==MIDDLE) cnt = sgiob.Pages;
	 else           cnt = 1;

	 for (j=0; j<cnt; j++) {
	    if (sgiop->Size) {
	       bzero((void *) seg_page_buf, ONE_PAGE);
	       if (ioctl(xmem,XmemDrvrREAD_SEGMENT,sgiop) < 0) {
		  IErr("READ_SEGMENT",NULL);
		  break;
	       }

	       found = 0;
	       seg_page_buf[ONE_PAGE] = (char) 0;

	       if (len == 1) {
		  for (k=0; k<sgiop->Size; k++) {
		     val = c = seg_page_buf[k];  pat &= 0xFF;
		     switch(opr) {
			case OprNE:
			   if (pat != c) found = 1;
			break;

			case OprEQ:
			   if (pat == c) found = 1;
			break;

			case OprGT:
			   if (c > pat) found = 1;
			break;

			case OprGE:
			   if (c >= pat) found = 1;
			break;

			case OprLT:
			   if (c < pat) found = 1;
			break;

			case OprLE:
			   if (c <= pat) found = 1;
			break;

			default:
			   printf("Illegal operator:%s %s\n",
				  oprs[(int) opr].Name,
				  oprs[(int) opr].Help);
			   return arg;
		     }
		     if (found) {
			found = 0;
			adr = k + sgiop->Offset;
			printf("0x%06X/0x%02x ",(int) adr,(int) val);
			if ((++fndcnt % 8) == 0) printf("\n");
			if (fndcnt > 64) {
			   printf("\n... Aborting search: Too many matches\n");
			   return arg;
			}
		     }
		  }
	       }
	       if (len == 2) {
		  for (k=0; k<sgiop->Size/2; k++) {
		     val = w = ((unsigned short *) seg_page_buf)[k]; pat &= 0xFFFF;
		     switch(opr) {
			case OprNE:
			   if (pat != w) found = 1;
			break;

			case OprEQ:
			   if (pat == w) found = 1;
			break;

			case OprGT:
			   if (w > pat) found = 1;
			break;

			case OprGE:
			   if (w >= pat) found = 1;
			break;

			case OprLT:
			   if (w < pat) found = 1;
			break;

			case OprLE:
			   if (w <= pat) found = 1;
			break;

			default:
			   printf("Illegal operator:%s %s\n",
				  oprs[(int) opr].Name,
				  oprs[(int) opr].Help);
			   return arg;
		     }
		     if (found) {
			found = 0;
			adr = k*2 + sgiop->Offset;
			printf("0x%06X/0x%04x ",(int) adr,(int) val);
			if ((++fndcnt % 6) == 0) printf("\n");
			if (fndcnt > 64) {
			   printf("\n... Aborting search: Too many matches\n");
			   return arg;
			}
		     }
		  }
	       }
	       if (len == 4) {
		  for (k=0; k<sgiop->Size/4; k++) {
		     val = l = ((unsigned long *) seg_page_buf)[k];
		     switch(opr) {
			case OprNE:
			   if (pat != l) found = 1;
			break;

			case OprEQ:
			   if (pat == l) found = 1;
			break;

			case OprGT:
			   if (l > pat) found = 1;
			break;

			case OprGE:
			   if (l >= pat) found = 1;
			break;

			case OprLT:
			   if (l < pat) found = 1;
			break;

			case OprLE:
			   if (l <= pat) found = 1;
			break;

			default:
			   printf("Illegal operator:%s %s\n",
				  oprs[(int) opr].Name,
				  oprs[(int) opr].Help);
			   return arg;
		     }
		     if (found) {
			found = 0;
			adr = k*4 + sgiop->Offset;
			printf("0x%06X/0x%08x ",(int) adr,(int) val);
			if ((++fndcnt % 4) == 0) printf("\n");
			if (fndcnt > 64) {
			   printf("\n... Aborting search: Too many matches\n");
			   return arg;
			}
		     }
		  }
	       }
	       sgiop->Offset += sgiop->Size;
	    }
	 }
      }
      if (fndcnt) {
	 printf("\nFound:%d Pattern:(%s)%d Size:%s Segment:%d Pages:%d\n",
	       (int) fndcnt,
		     oprs[(int) opr].Name,
	       (int) pat,lnams[len],
	       (int) segment,
	       (int) sgiob.Pages +1);
      } else {
	 printf("\nPattern:%d Not found\n",(int) pat);
      }
   } else
      printf("No search patter specified\n");
   return arg;
}
