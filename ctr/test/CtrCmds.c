/* ****************************************************************************** */
/* Controls Timing Receiver CTR PMC/PCI Versions test program.                    */
/* Defining PS_VER (PS Version) causes the program to use the Tgm and Tgv libs    */
/* for proper telegram decoding and name space handling.                          */
/* Julian Lewis Tue 23rd March 2004 Final                                         */
/* ****************************************************************************** */

#ifdef PS_VER
#include <tgm/tgm.h>
#include <tgv/tgv.h>
extern int _TgmSemIsBlocked(const char *sname);
extern CfgGetTgmXXX *_TgmCfgGetTgmXXX(void);
#endif

#include <time.h>   /* ctime */
#include <sys/types.h>
#include <sys/stat.h>

/* Jtag public code */

int xsvf_iDebugLevel = 0;

#ifdef CTR_PCI
#include <plx9030.h>
#include <plx9030.c>
#endif

#include <lenval.h>
#include <micro.h>
#include <ports.h>

#include <lenval.c>
#include <micro.c>
#include <ports.c>

#include <hptdc.c>

#ifdef CTR_PCI
#define CHANNELS 4
#endif

#ifdef CTR_PMC
#define CHANNELS 4
#endif

#ifdef CTR_VME
#define CHANNELS 8
#endif

#ifndef CHANNELS
#define CHANNELS 8
#endif

#ifdef CTR_PCI
static Plx9030Eprom eeprom;
#endif

static int module = 1;
static int channel = CtrDrvrCounter1;

static char *editor = "e";

static char *TriggerConditionNames[CtrDrvrTriggerCONDITIONS] = {"*", "=", "&" };
static char *CounterStart         [CtrDrvrCounterSTARTS]     = {"Nor", "Ext1", "Ext2", "Chnd", "Self", "Remt", "Pps", "Chnd+Stop"};
static char *CounterMode          [CtrDrvrCounterMODES]      = {"Once", "Mult", "Brst", "Mult+Brst"};
static char *CounterClock         [CtrDrvrCounterCLOCKS]     = {"1KHz", "10MHz", "40MHz", "Ext1", "Ext2", "Chnd" };
static char *CounterOnZero        [4]                        = {"NoOut", "Bus", "Out", "BusOut" };
static char *Statae               [CtrDrvrSTATAE]            = {"Gmt", "Pll", "XCk1", "XCk2", "Slf", "Enb", "Tdc", "CtrI", "CtrP", "CtrV", "Int", "Bus" };
static char *int_names            [CtrDrvrInterruptSOURCES]  = {"Cntr0","Cntr1","Cntr2","Cntr3","Cntr4","Cntr5","Cntr6","Cntr7","Cntr8",
								"PllIt","GmtEv","OneHz","OneKHz","Match" };
static char *mbits                [12]                       = {"????","Cnt1","Cnt2","Cnt3","Cnt4","Cnt5","Cnt6","Cnt7","Cnt8","40Mz","Ext1","Ext2"};
static char *rmbits               [CtrDrvrREMOTES]           = {"Load","Stop","Start","Out","Bus" };

static char *ioStatae             [CtrDrvrIoSTATAE]          = {"Eng","XIO","V1","V2","XSt1","XSt2","XCk1","XCk2","Out1","Out2","Out3","Out4","Out5","Out6","Out7","Out8","IdOK","DbHs","PllUtc","XMem","Tpr"};

/**************************************************************************/

char *defaultconfigpath = "/usr/local/drivers/ctr/ctrtest.config";

char *configpath = NULL;
char localconfigpath[128];  /* After a CD */

/**************************************************************************/

static char path[128];

char *GetFile(char *name) {
FILE *gpath = NULL;
char txt[128];
int i, j;

   if (configpath) {
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = NULL;
      }
   }

   if (configpath == NULL) {
      configpath = "./ctrtest.config";
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = "/dsc/local/data/ctrtest.config";
	 gpath = fopen(configpath,"r");
	 if (gpath == NULL) {
	    configpath = defaultconfigpath;
	    gpath = fopen(configpath,"r");
	    if (gpath == NULL) {
	       configpath = NULL;
	       sprintf(path,"./%s",name);
	       return path;
	    }
	 }
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

static void IErr(char *name, int *value) {

   if (value != NULL)
      printf("CtrDrvr: [%02d] ioctl=%s arg=%d :Error\n",
	     (int) ctr, name, (int) *value);
   else
      printf("CtrDrvr: [%02d] ioctl=%s :Error\n",
	     (int) ctr, name);
   perror("CtrDrvr");
}

/*****************************************************************/
/* News                                                          */

int News(int arg) {

char sys[128], npt[128];

   arg++;

   if (GetFile("ctr_news")) {
      strcpy(npt,path);
      sprintf(sys,"%s %s",GetFile(editor),npt);
      printf("\n%s\n",sys);
      system(sys);
      printf("\n");
   }
   return(arg);
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

/**************************************************************************/
/* Launch a task                                                          */

static void Launch(char *txt) {
pid_t child;

   if ((child = fork()) == 0) {
      if ((child = fork()) == 0) {
	 close(ctr);
	 system(txt);
	 exit (127);
      }
      exit (0);
   }
}

/**************************************************************************/

static unsigned int HptdcToNano(unsigned int hptdc) {
double fns;
unsigned int ins;

   fns = ((double) hptdc) / 2.56;
   ins = (unsigned int) fns;
   return ins;
}

/**************************************************************************/
/* Subtract times and return a float in seconds                           */

double TimeDiff(CtrDrvrTime *l, CtrDrvrTime *r) {

double s, n;
unsigned int nl, nr;

   s = (double) (l->Second - r->Second);

   nl = HptdcToNano(l->TicksHPTDC);
   nr = HptdcToNano(r->TicksHPTDC);

   if (nr > nl) {
      nl += 1000000000;
      s -= 1;
   }
   n = (unsigned int) ((nl - nr)/1000)*1000;

   return s + (n / 1000000000.0);
}

/**************************************************************************/
/* Convert a CtrDrvr time in milliseconds to a string routine.            */
/* Result is a pointer to a static string representing the time.          */
/*    the format is: Thu-18/Jan/2001 08:25:14.967                         */
/*                   day-dd/mon/yyyy hh:mm:ss.ddd                         */

volatile char *TimeToStr(CtrDrvrTime *t, int hpflag) {

static char tbuf[128];

char tmp[128];
char *yr, *ti, *md, *mn, *dy;
double ms;
double fms;

    bzero((void *) tbuf, 128);
    bzero((void *) tmp, 128);

    if (t->Second) {

	ctime_r ((time_t *) &t->Second, tmp);  /* Day Mon DD HH:MM:SS YYYY\n\0 */

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
	   sprintf (tbuf, "%s-%s/%s/%s %s.%010.0f", dy, md, mn, yr, ti, ms);
	} else
	   sprintf (tbuf, "%s-%s/%s/%s %s"  , dy, md, mn, yr, ti);

    }
    else {
	sprintf (tbuf, "--- Zero ---");
    }
    return (tbuf);
}

/**************************************************************************/
/* Convert a ptim to its name                                             */

typedef struct {
   unsigned int Eqp;
   char Name[32];
   char Comment[64];
   unsigned int Flg;
 } PtmNames;

static PtmNames ptm_names[128];
static char ptm_name_txt[32];
static char ptm_comment_txt[64];
static int ptm_names_size = 0;

char *GetPtmName(unsigned int eqp, int pfl) {

char *cp, *ep;
int i;

   if (ptm_names_size == 0) {
      GetFile("ltim.obnames");
      inp = fopen(path,"r");
      if (inp) {
	 while (fgets(ptm_name_txt,128,inp) != NULL) {
	    cp = ep = ptm_name_txt;
	    ptm_names[ptm_names_size].Eqp = strtoul(cp,&ep,0);
	    if (cp == ep) continue;
	    if ((*ep != 0) && (*ep != '\n')) cp = ep +1;
	    ptm_names[ptm_names_size].Flg = strtoul(cp,&ep,0);
	    if (cp != ep) ep++;

	    cp = index(ep,':');
	    if (cp) { *cp = '\0'; cp++; }

	    if (cp) {
	       for (i=strlen(cp); i>=0; i--) {
		  if (cp[i] == '\n') {
		     cp[i] = 0;
		     break;
		  }
	       }
	       if (strcmp(cp,"It's a SPS device") == 0) strcpy(ptm_names[ptm_names_size].Comment," -");
	       else                                     strcpy(ptm_names[ptm_names_size].Comment,cp);
	    }
	    if (ep) {
	       for (i=strlen(ep); i>=0; i--) {
		  if (ep[i] == '\n') {
		     ep[i] = 0;
		     break;
		  }
	       }
	       strcpy(ptm_names[ptm_names_size].Name,ep);
	    }

	    if (++ptm_names_size >= 128) break;
	 }
	 fclose(inp);
      }
   }

   for (i=0; i<ptm_names_size; i++) {
      if (ptm_names[i].Eqp == eqp) {
	 sprintf(ptm_comment_txt,"%s",ptm_names[i].Comment);
	 if (pfl) {
	    sprintf(ptm_name_txt,"%s",ptm_names[i].Name);
	 } else {
	    if (ptm_names[i].Flg) {
	       sprintf(ptm_name_txt,"%04d:RtAqn:%s",
				    (int) eqp,
					  ptm_names[i].Name);
	    } else {
	       sprintf(ptm_name_txt,"%04d:%s",
				    (int) eqp,
					  ptm_names[i].Name);
	    }
	 }
	 return ptm_name_txt;
      }
   }

   sprintf(ptm_name_txt,"%04d",(int) eqp);
   return ptm_name_txt;
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

int ChangeDirectory(int arg) {
ArgVal   *v;
AtomType  at;
char txt[128], fname[128], *cp;
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

      bzero((void *) fname, 128);

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
      strcat(localconfigpath,"/ctrtest.config");
      if (YesNo("Change ctrtest config to:",localconfigpath))
	 configpath = localconfigpath;
      else
	 configpath = NULL;
   }

   cp = GetFile(editor);
   sprintf(txt,"%s %s",cp,configpath);
   printf("\n%s\n",txt);
   system(txt);
   printf("\n");
   return(arg);
}

/*****************************************************************/

int Module(int arg) {
ArgVal   *v;
AtomType  at;
int mod, cnt, cbl;
CtrDrvrModuleAddress moad;

#ifdef PS_VER
TgvName cblnam;
TgmMachine mch;
int wrc;
#endif

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) {
	 mod = v->Number;
	 if (ioctl(ctr,CtrDrvrSET_MODULE,&mod) < 0) {
	    IErr("SET_MODULE",&mod);
	    return arg;
	 }
      }
      module = mod;
   }

   if (ioctl(ctr,CtrDrvrGET_MODULE_COUNT,&cnt) < 0) {
      IErr("GET_MODULE_COUNT",NULL);
      return arg;
   }

   for (mod=1; mod<=cnt; mod++) {
      if (ioctl(ctr,CtrDrvrSET_MODULE,&mod) < 0) {
	 IErr("SET_MODULE",&mod);
	 return arg;
      }
      if (ioctl(ctr,CtrDrvrGET_MODULE_DESCRIPTOR,&moad) < 0) {
	 IErr("GET_MODULE_DESCRIPTOR",NULL);
	 return arg;
      }
      if (ioctl(ctr,CtrDrvrGET_CABLE_ID,&cbl) < 0) {
	 IErr("GET_CABLE_ID",NULL);
	 return arg;
      }
      printf("Mod:%d ",(int) mod);

#ifdef CTR_PCI
      if (moad.ModuleType == CtrDrvrModuleTypeCTR)
	 printf("Typ:CTR PCI ");
      else
	 printf("Typ:PLX9030 ");

      printf("Vendor:0x%04x Device:0x%04x PciSlot:%d Fpga:0x%X Plx:0x%X ",
	     (int) moad.VendorId,
	     (int) moad.DeviceId,
	     (int) moad.PciSlot,
	     (int) moad.MemoryMap & 0x00FFFFFF,
	     (int) moad.LocalMap  & 0x00FFFFFF);
#endif

#ifdef CTR_VME
      printf("VME:0x%08x JTAG:0x%08x Vec:0x%02x Lvl:%1d ",
	     (int) moad.VMEAddress,
	     (int) moad.JTGAddress,
	     (int) moad.InterruptVector,
	     (int) moad.InterruptLevel);
#endif

#ifdef PS_VER
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
#else
      printf("Cable:%d\n",cbl);
#endif

   }
   ioctl(ctr,CtrDrvrSET_MODULE,&module);

   return arg;
}

/*****************************************************************/

int NextModule(int arg) {
int cnt;

   arg++;

   ioctl(ctr,CtrDrvrGET_MODULE_COUNT,&cnt);
   if (module >= cnt) {
      module = 1;
      ioctl(ctr,CtrDrvrSET_MODULE,&module);
   } else {
      module ++;
      ioctl(ctr,CtrDrvrSET_MODULE,&module);
   }

   return arg;
}

/*****************************************************************/

int NextChannel(int arg) {

   arg++;

   channel++;
   if (channel > CHANNELS) channel = CtrDrvrCounter1;

   return arg;
}

/*****************************************************************/

int PrevModule(int arg) {
int cnt;

   arg++;

   ioctl(ctr,CtrDrvrGET_MODULE_COUNT,&cnt);
   if (module > 1) {
      module --;
      ioctl(ctr,CtrDrvrSET_MODULE,&module);
   } else {
      module = cnt;
      ioctl(ctr,CtrDrvrSET_MODULE,&module);
   }

   return arg;
}

/*****************************************************************/

int PrevChannel(int arg) {

   arg++;

   channel--;
   if (channel < CtrDrvrCounter1) channel = CHANNELS;

   return arg;
}

/*****************************************************************/

int GetSetChannel(int arg) {
ArgVal   *v;
AtomType  at;
int ch;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      ch = v->Number;
      if ((ch >= CtrDrvrCounter1) && (ch <= CHANNELS)) {
	 channel = ch;
      } else {
	 printf("Error: Illegal counter number: %d\n",ch);
      }
   }
   printf("Active Counter: %d\n",channel);
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
      if (ioctl(ctr,CtrDrvrSET_SW_DEBUG,&debug) < 0) {
	 IErr("SET_SW_DEBUG",&debug);
	 return arg;
      }
   }
   debug = -1;
   if (ioctl(ctr,CtrDrvrGET_SW_DEBUG,&debug) < 0) {
      IErr("GET_SW_DEBUG",NULL);
      return arg;
   }
   if (debug > 0)
      printf("SoftwareDebug: [%d] Enabled\n",debug);
   else
      printf("SoftwareDebug: [%d] Disabled\n",debug);

   return arg;
}

/*****************************************************************/

#ifdef CTR_VME
int GetSetOByte(int arg) { /* Output Byte on P2 0..8 */
ArgVal   *v;
AtomType  at;
int obyte;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      obyte = v->Number;
      if (ioctl(ctr,CtrDrvrSET_OUTPUT_BYTE,&obyte) < 0) {
	 IErr("SET_OUTPUT_BYTE",&obyte);
	 return arg;
      }
   }
   obyte = -1;
   if (ioctl(ctr,CtrDrvrGET_OUTPUT_BYTE,&obyte) < 0) {
      IErr("GET_OUTPUT_BYTE",NULL);
      return arg;
   }
   if (obyte > 0)
      printf("Output Byte on P2 is: VME[%d] Enabled\n",obyte);
   else
      printf("Output Byte on P2 is: VME[0] Disabled\n");

   return arg;
}
#endif

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
      if (ioctl(ctr,CtrDrvrSET_TIMEOUT,&timeout) < 0) {
	 IErr("SET_TIMEOUT",&timeout);
	 return arg;
      }
   }
   timeout = -1;
   if (ioctl(ctr,CtrDrvrGET_TIMEOUT,&timeout) < 0) {
      IErr("GET_TIMEOUT",NULL);
      return arg;
   }
   if (timeout > 0)
      printf("Timeout: [%d] Enabled\n",timeout);
   else
      printf("Timeout: [%d] Disabled\n",timeout);

   return arg;
}

/*****************************************************************/

#ifdef CTR_PCI
Plx9030Field *GetField(int address, int space) {

int i, sz;
Plx9030Field *fp, *rp;

	if (space == 1) { fp = Configs; sz = Plx9030CONFIG_REGS; }
   else if (space == 2) { fp = Locals;  sz = Plx9030LOCAL_REGS;  }
   else if (space == 3) { fp = Eprom;   sz = Plx9030EPROM_REGS;  }
   else return NULL;

   for (i=0; i<sz; i++) {
      rp = &(fp[i]);
      if (address <= rp->Offset) return rp;
   }
   return NULL;
}
#endif

/*****************************************************************/

void EditMemory(int address, int space) {

CtrDrvrRawIoBlock iob;
unsigned int array[2];
unsigned int addr, *data;
char c, *cp, str[128];
int n, radix, nadr;

#ifdef CTR_PCI
Plx9030Field *fp;
#endif

   printf("EditMemory: [?]: Comment, [/]: Open, [CR]: Next, [.]: Exit [x]: Hex, [d]: Dec\n\n");

   data = array;
   addr = address;
   radix = 16;
   c = '\n';

   while (1) {

      iob.Size = 1;
      iob.Offset = addr;
      iob.UserArray = array;

      if (space == 0) {
	 if (ioctl(ctr,CtrDrvrRAW_READ,&iob) < 0) {
	    IErr("RAW_READ",NULL);
	    break;
	 }

#ifdef CTR_PCI
      } else if (space == 1) {
	 fp = GetField(addr,1);
	 if (fp) {
	    printf("%s\t[%d] ",fp->Name,(int) fp->FieldSize);
	    iob.Size = fp->FieldSize;
	    addr = iob.Offset = fp->Offset;
	 } else return;

	 if (ioctl(ctr,CtrDrvrPLX9030_CONFIG_READ,&iob) < 0) {
	    IErr("PLX9030_CONFIG_READ",NULL);
	    break;
	 }
      } else if (space == 2) {
	 fp = GetField(addr,2);
	 if (fp) {
	    printf("%s\t[%d] ",fp->Name,(int) fp->FieldSize);
	    iob.Size = fp->FieldSize;
	    addr = iob.Offset = fp->Offset;
	 } else return;
	 if (ioctl(ctr,CtrDrvrPLX9030_LOCAL_READ,&iob) < 0) {
	    IErr("PLX9030_LOCAL_READ",NULL);
	    break;
	 }
      } else if (space == 3) {
	 fp = GetField(addr,3);
	 if (fp) {
	    printf("%s\t[%d] ",fp->Name,(int) fp->FieldSize);
	    iob.Size = fp->FieldSize;
	    addr = iob.Offset = fp->Offset;
	    iob.UserArray[0] = eeprom[addr/2];
	 } else return;
#endif

      } else return;

      if (radix == 16) {
	 if (c=='\n') printf("0x%04x: 0x%04x ",(int) addr,(int) *data);
      } else {
	 if (c=='\n') printf("%04d: %5d ",(int) addr,(int) *data);
      }

      c = (char) getchar();

      if (c == '/') {
	 bzero((void *) str, 128); n = 0;
	 while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();
	 nadr = strtoul(str,&cp,radix);
	 if (cp != str) addr = nadr;
      }

      else if (c == 'x')  {radix = 16; addr--; continue; }
      else if (c == 'd')  {radix = 10; addr--; continue; }
      else if (c == '.')  { c = getchar(); printf("\n"); break; }
      else if (c == 'q')  { c = getchar(); printf("\n"); break; }
      else if (c == '\n') { if (space == 0) addr++;

#ifdef CTR_PCI
			    else addr += fp->FieldSize;
#endif
			  }
#ifdef CTR_PCI
      else if (c == '?')  { if (fp) printf("%s\n",fp->Comment); }
#endif

      else {
	 bzero((void *) str, 128); n = 0;
	 str[n++] = c;
	 while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();
	 *data = strtoul(str,&cp,radix);
	 if (cp != str) {
	    if (space == 0) {
	       iob.Size = 1;
	       if (ioctl(ctr,CtrDrvrRAW_WRITE,&iob) < 0) {
		  IErr("RAW_WRITE",NULL);
		  break;
	       }

#ifdef CTR_PCI
	    } else if (space == 1) {
	       if (ioctl(ctr,CtrDrvrPLX9030_CONFIG_WRITE,&iob) < 0) {
		  IErr("PLX9030_CONFIG_WRITE",NULL);
		  break;
	       }
	    } else if (space == 2) {
	       if (ioctl(ctr,CtrDrvrPLX9030_LOCAL_WRITE,&iob) < 0) {
		  IErr("PLX9030_LOCAL_WRITE",NULL);
		  break;
	       }
	    } else if (space == 3) {
	       eeprom[(iob.Offset)/2] = iob.UserArray[0];
#endif

	    } else return;
	 }
      }
   }
}

/*****************************************************************/

int MemIo(int arg) { /* Address */
ArgVal   *v;
AtomType  at;

   printf("Raw Io: Address space: BAR2: Ctr hardware space\n");

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      EditMemory(v->Number,0);
   } else
      EditMemory(0,0);

   return arg;
}

/*****************************************************************/

#ifdef CTR_PCI
int ConfigIo(int arg) { /* Address */
ArgVal   *v;
AtomType  at;

   printf("Raw Io: Address space: PLX9030 Configuration Registers\n");
   ioctl(ctr,CtrDrvrPLX9030_CONFIG_OPEN,NULL);

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      EditMemory(v->Number,1);
   } else
      EditMemory(0,1);

   ioctl(ctr,CtrDrvrPLX9030_CONFIG_CLOSE,NULL);

   return arg;
}
#endif

/*****************************************************************/

#ifdef CTR_PCI
int LocalIo(int arg) { /* Address */
ArgVal   *v;
AtomType  at;

   printf("Raw Io: Address space: BAR0: PLX9030 Local Configuration Registers\n");
   ioctl(ctr,CtrDrvrPLX9030_LOCAL_OPEN,NULL);

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      EditMemory(v->Number,2);
   } else
      EditMemory(0,2);

   ioctl(ctr,CtrDrvrPLX9030_LOCAL_CLOSE,NULL);

   return arg;
}
#endif

/*****************************************************************/

#ifdef CTR_PCI
int EpromEdit(int arg) { /* Address */
ArgVal   *v;
AtomType  at;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      EditMemory(v->Number,3);
   } else
      EditMemory(0,3);

   return arg;
}
#endif

/*****************************************************************/

#ifdef CTR_PCI
int EpromRead(int arg) {

CtrDrvrRawIoBlock iob;
unsigned int array[2];
Plx9030Field *fp;
int i;

   arg++;

   printf("EpromRead: 93LC56B EEPROM from the CTR PCI card\n");


   ioctl(ctr,CtrDrvr93LC56B_EEPROM_OPEN,NULL);

   for (i=0; i<Plx9030EPROM_REGS; i++) {

      fp = &(Eprom[i]);
      iob.Size         = fp->FieldSize;
      iob.Offset       = fp->Offset;
      iob.UserArray    = array;
      iob.UserArray[0] = 0;

      if (ioctl(ctr,CtrDrvr93LC56B_EEPROM_READ,&iob) < 0) {
	 IErr("93LC56B_EEPROM_WRITE",NULL);
	 break;
      }
      eeprom[i] = (unsigned short) iob.UserArray[0];
   }

   ioctl(ctr,CtrDrvr93LC56B_EEPROM_CLOSE,NULL);

   if (i>=Plx9030EPROM_REGS) printf("EpromRead: Done\n");

   return arg;
}
#endif

/*****************************************************************/

#ifdef CTR_PCI
int EpromWrite(int arg) {

CtrDrvrRawIoBlock iob;
unsigned int array[2];
Plx9030Field *fp;
int i;

   arg++;

   if (YesNo("EpromWrite: Overwrite CTR flash","93LC56B EEPROM") == 0)
      return arg;

   iob.UserArray = array;

   ioctl(ctr,CtrDrvr93LC56B_EEPROM_OPEN,NULL);

   for (i=0; i<Plx9030EPROM_REGS; i++) {

      fp = &(Eprom[i]);
      iob.Size         = fp->FieldSize;
      iob.Offset       = fp->Offset;
      iob.UserArray[0] = (unsigned int) eeprom[i];

      if (ioctl(ctr,CtrDrvr93LC56B_EEPROM_WRITE,&iob) < 0) {
	 IErr("93LC56B_EEPROM_WRITE",NULL);
	 break;
      }
   }

   ioctl(ctr,CtrDrvr93LC56B_EEPROM_CLOSE,NULL);

   if (i>=Plx9030EPROM_REGS) printf("EpromWrite: Done\n");

   return arg;
}
#endif

/*****************************************************************/

#ifdef CTR_PCI
int EpromReadFile(int arg) {
ArgVal   *v;
AtomType  at;
char txt[128], fname[128], *cp, *ep;
int n, earg;
int i;

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

      GetFile("Ctr.eprom");
      strcpy(fname,path);

   } else {

      bzero((void *) fname, 128);

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

   if (YesNo("EpromReadFile: Read: 93lc56B EEPROM memory image from: ",fname) == 0)
      return earg;

   inp = fopen(fname,"r");
   if (inp) {
      for (i=0; i<Plx9030EPROM_REGS; i++) {
	 if (fgets(txt,128,inp) == NULL) break;
	 cp = ep = txt;
	 eeprom[i] = strtoul(cp,&ep,0);
	 if (cp == ep) i--;
      }
      fclose(inp);
      sprintf(txt,"cat %s\n",fname); system(txt);
      printf("EpromReadFile: Read: %d Lines: OK\n",i);
   } else {
      perror("fopen");
      printf("Error: Could not open the file: %s for reading\n",fname);
   }
   return earg;
}
#endif

/*****************************************************************/

#ifdef CTR_PCI
int EpromWriteFile(int arg) {
ArgVal   *v;
AtomType  at;
char txt[128], fname[128], *cp;
int n, earg;
int i;

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

      GetFile("Ctr.eprom");
      strcpy(fname,path);

   } else {

      bzero((void *) fname, 128);

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

   if (YesNo("EpromReadFile: Write: 93lc56B EEPROM memory image to: ",fname) == 0)
      return earg;

   inp = fopen(fname,"w");
   if (inp) {
      for (i=0; i<Plx9030EPROM_REGS; i++) {
	 fprintf(inp,
		 "0x%04X Addr:0x%02X : %s%s\n",
		 eeprom[i],
	   (int) Eprom[i].Offset,
		 Eprom[i].Comment,
		 Eprom[i].Name);
      }
      fclose(inp);
      sprintf(txt,"cat %s\n",fname); system(txt);
      printf("EpromReadFile: Written: %d Lines: OK\n",i);

   } else {
      perror("fopen");
      printf("Error: Could not open the file: %s for writing\n",fname);
   }
   return earg;
}
#endif

/*****************************************************************/

#ifdef CTR_PCI
int EpromErase(int arg) {

   arg++;

   if (YesNo("EpromErase: Wipe CTR flash","Revert to Plx9030 defaults") == 0)
      return arg;

   ioctl(ctr,CtrDrvr93LC56B_EEPROM_OPEN,NULL);
   if (ioctl(ctr,CtrDrvr93LC56B_EEPROM_ERASE,NULL) < 0)
      IErr("93LC56B_EEPROM_ERASE",NULL);
   ioctl(ctr,CtrDrvr93LC56B_EEPROM_CLOSE,NULL);

   return arg;
}
#endif

/*****************************************************************/

#ifdef CTR_PCI
int PlxReConfig(int arg) {

   arg++;

   if (YesNo("PlxReConfig: Reconfigure CTR from","93LC56B EEPROM") == 0)
      return arg;

   if (ioctl(ctr,CtrDrvrPLX9030_RECONFIGURE,NULL) < 0)
      IErr("PLX9030_RECONFIGURE",NULL);

   return arg;
}
#endif

/*****************************************************************/

#ifdef CTR_PCI
int Remap(int arg) {

   arg++;

#ifdef __linux__
   printf("Not available on Linux platforms\n");
   return arg;

#else
   if (YesNo("Remap: Rebuild PCI tree","Reinstall CTR driver") == 0)
      return arg;

   if (ioctl(ctr,CtrDrvrREMAP,NULL) < 0)
      IErr("REMAP",NULL);

   printf("Remap: Done, you must restart the test program\n");
   printf("Remap: Bye\n");
   exit(0);
#endif

}
#endif

/*****************************************************************/

int JtagLoader(char *fname) {

int cc;
CtrDrvrVersion version;
CtrDrvrTime t;

   inp = fopen(fname,"r");
   if (inp) {
      if (ioctl(ctr,CtrDrvrJTAG_OPEN,NULL)) {
	 IErr("JTAG_OPEN",NULL);
	 return 1;
      }

      cc = xsvfExecute(); /* Play the xsvf file */
      printf("\n");
      if (cc) printf("Jtag: xsvfExecute: ReturnCode: %d Error\n",cc);
      else    printf("Jtag: xsvfExecute: ReturnCode: %d All OK\n",cc);
      fclose(inp);

      if (ioctl(ctr,CtrDrvrJTAG_CLOSE,NULL) < 0) {
	 IErr("JTAG_CLOSE",NULL);
	 return 1;
      }

      sleep(5); /* Wait for hardware to reconfigure */

      bzero((void *) &version, sizeof(CtrDrvrVersion));
      if (ioctl(ctr,CtrDrvrGET_VERSION,&version) < 0) {
	 IErr("GET_VERSION",NULL);
	 return 1;
      }
      t.Second = version.VhdlVersion;
      printf("New VHDL bit-stream loaded, Version: [%u] %s\n",
       (int) version.VhdlVersion,
	     TimeToStr(&t,0));
   } else {
      perror("fopen");
      printf("Error: Could not open the file: %s for reading\n",fname);
      return 1;
   }
   return cc;
}

/*****************************************************************/
/* Force reload of all installed modeules                        */

int ReloadAllJtag(int arg) {
ArgVal   *v;
AtomType  at;
CtrDrvrVersion version;
CtrDrvrTime t;
unsigned int vhdlver;
char fname[128], vname[128], txt[128], *cp, *ep;
char host[49];
CtrDrvrHardwareType ht, cht;
int m, cnt, update;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   update = 0;
   if (at == Alpha) {
      if ((v->Text[0] == 'U') || (v->Text[0] == 'u')) { /* Update VHDL > */
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;
	 update = 1;
      }
   }

   cht = CtrDrvrHardwareTypeNONE;
   bzero((void *) vname, 128);
   bzero((void *) host,49);
   gethostname(host,48);
   bzero((void *) &t, sizeof(CtrDrvrTime));

   version.HardwareType = CtrDrvrHardwareTypeNONE;
   if (ioctl(ctr,CtrDrvrGET_VERSION,&version) >= 0) {
      cht = version.HardwareType;

      if (version.HardwareType == CtrDrvrHardwareTypeCTRP) {
	 printf("Detected: Hardware Type: CTRP PMC (3 Channel)\n");
	 if (GetFile("Ctrp.xsvf") == NULL) {
	    printf("Can not find file: Ctrp.xsvf: PMC VHDL version\n");
	    return arg;
	 }
	 strcpy(vname,path);
      }

      if (version.HardwareType == CtrDrvrHardwareTypeCTRI) {
	 printf("Detected: Hardware Type: CTRI PCI (4 Channel)\n");
	 if (GetFile("Ctri.xsvf") == NULL) {
	    printf("Can not find file: Ctri.xsvf: PCI VHDL version\n");
	    return arg;
	 }
	 strcpy(vname,path);
      }

      if (version.HardwareType == CtrDrvrHardwareTypeCTRV) {
	 printf("Detected: Hardware Type: CTRV VME (8 Channel)\n");
	 if (GetFile("Ctrv.xsvf") == NULL) {
	    printf("Can not find file: Ctrv.xsvf: VME VHDL version\n");
	    return arg;
	 }
	 strcpy(vname,path);
      }
   }

#ifdef CTR_PCI
   if (strlen(vname) == 0) {
      printf("WARNING: ASSUMING: Hardware Type: CTRI PCI (4 Channel)\n");
      printf("WARNING: If you are not SURE of the actual hardware type, ABORT\n");
      printf("WARNING: Loading the wrong VHDL will DAMAGE the module !\n");
      cht = CtrDrvrHardwareTypeCTRI;
      if (GetFile("Ctri.xsvf") == NULL) {
	 printf("Can not find file: Ctri.xsvf\n");
	 return arg;
      }
      strcpy(vname,path);
      if (!(YesNo("ReloadJtag for CTRI (PCI): VHDL-Compiled BitStream",vname ))) {
	 printf("ABORTED\n");
	 return arg;
      }
   }
#endif

#ifdef CTR_PMC
   if (strlen(vname) == 0) {
      printf("WARNING: ASSUMING: Hardware Type: CTRP PMC (3 Channel)\n");
      printf("WARNING: If you are not SURE of the actual hardware type, ABORT\n");
      printf("WARNING: Loading the wrong VHDL will DAMAGE the module !\n");
      cht = CtrDrvrHardwareTypeCTRP;
      if (GetFile("Ctrp.xsvf") == NULL) {
	 printf("Can not find file: Ctrp.xsvf\n");
	 return arg;
      }
      strcpy(vname,path);
      if (!(YesNo("ReloadJtag for CTRP (PMC): VHDL-Compiled BitStream",vname ))) {
	 printf("ABORTED\n");
	 return arg;
      }
   }
#endif

#ifdef CTR_VME
   if (strlen(vname) == 0) {
      printf("WARNING: ASSUMING: Hardware Type: CTRV VME (8 Channel)\n");
      printf("WARNING: If you are not SURE of the actual hardware type, ABORT\n");
      printf("WARNING: Loading the wrong VHDL will DAMAGE the module !\n");
      cht = CtrDrvrHardwareTypeCTRV;
      if (GetFile("Ctrv.xsvf") == NULL) {
	 printf("Can not find file: Ctrv.xsvf\n");
	 return arg;
      }
      strcpy(vname,path);
      if (!(YesNo("ReloadJtag for CTRV (VME): VHDL-Compiled BitStream",vname ))) {
	 printf("ABORTED\n");
	 return arg;
      }
   }
#endif

   if (strlen(vname) == 0) {
      printf("Configuration error: Wrong compilation options: -D[CTR_PCI/CTR_PMC/CTR_VME]\n");
      return arg;
   }

   if (GetFile("Vhdl.versions") == NULL) {
      printf("Can not find the file: Vhdl.versions\n");
      return arg;
   }
   strcpy(fname,path);
   inp = fopen(fname,"r");
   if (inp) {
      for (ht=CtrDrvrHardwareTypeNONE; ht<CtrDrvrHardwareTYPES; ht++) {
	 if (fgets(txt,128,inp) != NULL) {
	    if (ht == cht) {
	       printf("%s:%s",fname,txt);
	       cp = ep = txt;
	       vhdlver = strtoul(cp,&ep,10);
	       break;
	    }
	 }
      }
      fclose(inp);
   } else {
      perror("fopen");
      printf("Could not open the file: %s for reading\n",fname);
      return arg;
   }
   if ((cht == CtrDrvrHardwareTypeNONE)
   ||  (cht != ht)) {
      printf("Syntax error in file:%s\n",fname);
      return arg;
   }

   if (ioctl(ctr,CtrDrvrGET_MODULE_COUNT,&cnt) < 0) {
      IErr("GET_MODULE_COUNT",NULL);
      return arg;
   }

   for (m=1; m<=cnt; m++) {

      GetFile("CtrVhdlUpdateRequired");
      sprintf(fname,"%s_%s.%02d",path,host,(int) module);
      sprintf(txt,"rm -f %s",fname);
      system(txt);

      if (ioctl(ctr,CtrDrvrSET_MODULE,&m) < 0) {
	 IErr("SET_MODULE",&m);
	 return arg;
      }

      bzero((void *) &version, sizeof(CtrDrvrVersion));
      if (ioctl(ctr,CtrDrvrGET_VERSION,&version) < 0) {
	 IErr("GET_VERSION",NULL);
	 return arg;
      }

      if (vhdlver == version.VhdlVersion) {
	 printf("Module:%d VHDL version:%d correct and up to date\n",
		(int) m,
		(int) vhdlver);
	 continue;
      }

      if (vhdlver > version.VhdlVersion) {

	 sprintf(txt,"Ctr: Host:%s Module:%d OLD VHDL VERSION:%d Should be:%d",
		 host,
		(int) m,
		(int) version.VhdlVersion,
		(int) vhdlver);

	 if (update) {
	    printf("WARNING: %s RELOADING...\n",txt);
	    if (JtagLoader(vname)) printf("ERROR");
	    else                   printf("OK");
	    printf("\n\n");
	 } else {
	    fprintf(stderr,"WARNING: %s VHDL UPDATE REQUIRED\n",txt);
	    umask(0);
	    inp = fopen(fname,"w");
	    if (inp) {
	       fprintf(inp,"WARNING: %s VHDL UPDATE REQUIRED\n",txt);
	       t.Second = version.VhdlVersion;
	       fprintf(inp,"Old version date:[%d] = %s\n",(int) t.Second,TimeToStr(&t,0));
	       t.Second = vhdlver;
	       fprintf(inp,"New version date:[%d] = %s\n",(int) t.Second,TimeToStr(&t,0));
	       fclose(inp);
	    }
	 }
	 continue;
      }

      if (vhdlver < version.VhdlVersion) {
	 printf("Module:%d VHDL version:%d is more recent than disc copy:%d SKIPING\n",
		(int) m,
		(int) version.VhdlVersion,
		(int) vhdlver);
	 continue;
      }
   }

   if (ioctl(ctr,CtrDrvrSET_MODULE,&module) < 0) {
      IErr("SET_MODULE",&module);
   }
   return arg;
}

/*****************************************************************/

int ReloadJtag(int arg) {
ArgVal   *v;
AtomType  at;
char fname[128], txt[128], *cp;
char host[49];
int n, earg;
CtrDrvrVersion version;

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

      bzero((void *) fname, 128);
      version.HardwareType = CtrDrvrHardwareTypeNONE;
      if (ioctl(ctr,CtrDrvrGET_VERSION,&version) >= 0) {

	 if (version.HardwareType == CtrDrvrHardwareTypeCTRP) {
	    printf("Detected: Hardware Type: CTRP PMC (3 Channel)\n");
	    if (GetFile("Ctrp.xsvf") == NULL) {
	       printf("Can not find file: Ctrp.xsvf: PMC VHDL version\n");
	       return arg;
	    }
	    strcpy(fname,path);
	 }

	 if (version.HardwareType == CtrDrvrHardwareTypeCTRI) {
	    printf("Detected: Hardware Type: CTRI PCI (4 Channel)\n");
	    if (GetFile("Ctri.xsvf") == NULL) {
	       printf("Can not find file: Ctri.xsvf: PCI VHDL version\n");
	       return arg;
	    }
	    strcpy(fname,path);
	 }

	 if (version.HardwareType == CtrDrvrHardwareTypeCTRV) {
	    printf("Detected: Hardware Type: CTRV VME (8 Channel)\n");
	    if (GetFile("Ctrv.xsvf") == NULL) {
	       printf("Can not find file: Ctrv.xsvf: VME VHDL version\n");
	       return arg;
	    }
	    strcpy(fname,path);
	 }
      }

#ifdef CTR_PCI
      if (strlen(fname) == 0) {
	 printf("WARNING: ASSUMING: Hardware Type: CTRI PCI (4 Channel)\n");
	 printf("WARNING: If you are not SURE of the actual hardware type, ABORT\n");
	 printf("WARNING: Loading the wrong VHDL will DAMAGE the module !\n");
	 if (GetFile("Ctri.xsvf") == NULL) {
	    printf("Can not find file: Ctri.xsvf\n");
	    return arg;
	 }
	 strcpy(fname,path);
      }
#endif

#ifdef CTR_PMC
      if (strlen(fname) == 0) {
	 printf("WARNING: ASSUMING: Hardware Type: CTRP PMC (3 Channel)\n");
	 printf("WARNING: If you are not SURE of the actual hardware type, ABORT\n");
	 printf("WARNING: Loading the wrong VHDL will DAMAGE the module !\n");
	 if (GetFile("Ctrp.xsvf") == NULL) {
	    printf("Can not find file: Ctrp.xsvf\n");
	    return arg;
	 }
	 strcpy(fname,path);
      }
#endif

#ifdef CTR_VME
      if (strlen(fname) == 0) {
	 printf("WARNING: ASSUMING: Hardware Type: CTRV VME (8 Channel)\n");
	 printf("WARNING: If you are not SURE of the actual hardware type, ABORT\n");
	 printf("WARNING: Loading the wrong VHDL will DAMAGE the module !\n");
	 if (GetFile("Ctrv.xsvf") == NULL) {
	    printf("Can not find file: Ctrv.xsvf\n");
	    return arg;
	 }
	 strcpy(fname,path);
      }
#endif

      if (strlen(fname) == 0) {
	 printf("Configuration error: Wrong compilation options: -D[CTR_PCI/CTR_PMC/CTR_VME]\n");
	 printf("Manual filename specification required\n");
	 return arg;
      }

   } else {

      bzero((void *) fname, 128);

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
      gethostname(host,48);
      GetFile("CtrVhdlUpdateRequired");
      sprintf(fname,"%s_%s.%02d",path,host,(int) module);
      sprintf(txt,"rm -f %s",fname);
      system(txt);
   } else printf("ABORTED\n");
   return earg;
}

/*****************************************************************/

int GetSetQue(int arg) { /* Arg=Flag */
ArgVal   *v;
AtomType  at;
int qflag, qsize, qover;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      qflag = v->Number;

      if (ioctl(ctr,CtrDrvrSET_QUEUE_FLAG,&qflag) < 0) {
	 IErr("SET_QUEUE_FLAG",(int *) &qflag);
	 return arg;
      }
   }
   qflag = -1;
   if (ioctl(ctr,CtrDrvrGET_QUEUE_FLAG,&qflag) < 0) {
      IErr("GET_QUEUE_FLAG",NULL);
      return arg;
   }
   if (ioctl(ctr,CtrDrvrGET_QUEUE_SIZE,&qsize) < 0) {
      IErr("GET_QUEUE_SIZE",NULL);
      return arg;
   }
   if (ioctl(ctr,CtrDrvrGET_QUEUE_OVERFLOW,&qover) < 0) {
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

/*****************************************************************/

int GetSetCableId(int arg) {
ArgVal   *v;
AtomType  at;

unsigned int cid;

   arg++;

   if (ioctl(ctr,CtrDrvrSET_MODULE,&module) < 0) {
      IErr("SET_MODULE",NULL);
      return arg;
   }

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      cid = v->Number;

      if (ioctl(ctr,CtrDrvrSET_CABLE_ID,&cid) < 0) {
	 IErr("SET_CABLE_ID",(int *) &cid);
	 return arg;
      }
   }

   cid = -1;
   if (ioctl(ctr,CtrDrvrGET_CABLE_ID,&cid) < 0) {
      IErr("GET_CABLE_ID",NULL);
      return arg;
   }

   printf("Module: %d ID: %d",module, (int) cid);
   if (cid) printf("\n"); else printf(": Not Set\n");

   return arg;
}

/*****************************************************************/

int ListClients(int arg) {

CtrDrvrClientConnections cons;
CtrDrvrConnection *con;
CtrDrvrClientList clist;
int i, j, k, pid, mypid, omod, nmod;

   arg++;

   mypid = getpid();

   if (ioctl(ctr,CtrDrvrGET_CLIENT_LIST,&clist) < 0) {
      IErr("GET_CLIENT_LIST",NULL);
      return arg;
   }

   for (i=0; i<clist.Size; i++) {
      pid = clist.Pid[i];

      bzero((void *) &cons, sizeof(CtrDrvrClientConnections));
      cons.Pid = pid;

      if (ioctl(ctr,CtrDrvrGET_CLIENT_CONNECTIONS,&cons) < 0) {
	 IErr("GET_CLIENT_CONNECTIONS",NULL);
	 return arg;
      }

      if (pid == mypid) printf("Pid: %4d (ctrtest) ",pid);
      else              printf("Pid: %4d ",pid);

      if (cons.Size) {
	 for (j=0; j<cons.Size; j++) {
	    con = &(cons.Connections[j]);
	    omod = nmod;
	    nmod = con->Module;
	    if ((j) && (omod != nmod)) printf("\nPid: %4d ",pid);
	    if (con->EqpClass == CtrDrvrConnectionClassHARD) {
	       printf("[Mod:%d Mask:0x%X Hard:",(int) con->Module,(int) con->EqpNum);
	       for (k=0; k<CtrDrvrInterruptSOURCES; k++) {
		  if ((1<<k) & con->EqpNum) printf(" %s",int_names[k]);
	       }
	       printf("]");
	    } else if (con->EqpClass == CtrDrvrConnectionClassPTIM) {
	       printf("[Mod:%d Ptim:%s]",(int) con->Module,GetPtmName(con->EqpNum,1));
	       if ((j & 1) && (j+1 != cons.Size)) printf("\nPid: %4d ",pid);
	    } else {
	       printf("[Mod:%d Ctim:%d]",(int) con->Module,(int) con->EqpNum);
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

   if (ioctl(ctr,CtrDrvrRESET,NULL) < 0) {
      IErr("RESET",NULL);
      return arg;
   }

   module = -1;
   if (ioctl(ctr,CtrDrvrGET_MODULE,&module) < 0) {
      IErr("GET_MODULE",NULL);
      return arg;
   }

   printf("Reset Module: %d\n",module);

   return arg;
}

/*****************************************************************/

char *StatusToStr(unsigned int stat) {
int i, msk;
static char res[48];

   bzero((void *) res, 48);
   for (i=0; i<CtrDrvrSTATAE; i++) {
      msk = 1 << i;
      strcat(res,Statae[i]);
      if (msk & stat) strcat(res,"-OK ");
      else            strcat(res,"-ER ");
   }
   return res;
}

/*****************************************************************/

char *IoStatusToStr(unsigned int stat) {
int i, msk;
static char res[128];

   bzero((void *) res, 128);
   for (i=0; i<CtrDrvrIoSTATAE; i++) {
      msk = 1 << i;
      if (msk & stat) {
	 strcat(res,ioStatae[i]);
	 strcat(res," ");
      }
   }
   return res;
}

/*****************************************************************/

int GetStatus(int arg) {

unsigned int stat;
CtrDrvrModuleStats mstat;

   arg++;
   if (ioctl(ctr,CtrDrvrGET_STATUS,&stat) < 0) {
      IErr("GET_STATUS",NULL);
      return arg;
   }
   printf("Mod:%d Status:%s\n",module,StatusToStr(stat));

   if (ioctl(ctr,CtrDrvrGET_IO_STATUS,&stat) <0) {
      IErr("GET_IO_STATUS",NULL);
      return arg;
   }
   printf("Mod: %d IoStatus:%s\n",module,IoStatusToStr(stat));

   if (stat & CtrDrvrIOStatusExtendedMemory) {
      if (ioctl(ctr,CtrDrvrGET_MODULE_STATS,&mstat) < 0) {
	 IErr("GET_MODULE_STATS",NULL);
	 return arg;
      }

      printf("PllErrorThreshold    :%d\n",(int) mstat.PllErrorThreshold);
      printf("PllDacLowPassValue   :%d\n",(int) mstat.PllDacLowPassValue);
      printf("PllDacCICConstant    :%d\n",(int) mstat.PllDacCICConstant);
      printf("PllMonitorCICConstant:%d\n",(int) mstat.PllMonitorCICConstant);
      printf("PhaseDCM             :%d\n",(int) mstat.PhaseDCM);
      printf("UtcPllPhaseError     :%d\n",(int) mstat.UtcPllPhaseError);
      printf("Temperature          :%d\n",(int) mstat.Temperature);
      printf("MsMissedErrs         :%d\n",(int) mstat.MsMissedErrs);
      printf("LastMsMissed         :%s\n",TimeToStr(&(mstat.LastMsMissed),1));
      printf("PllErrors            :%d\n",(int) mstat.PllErrors);
      printf("LastPllError         :%s\n",TimeToStr(&(mstat.LastPllError),1));
      printf("MissedFrames         :%d\n",(int) mstat.MissedFrames);
      printf("LastFrameMissed      :%s\n",TimeToStr(&(mstat.LastFrameMissed),1));
      printf("BadReceptionCycles   :%d\n",(int) mstat.BadReceptionCycles);
      printf("ReceivedFrames       :%d\n",(int) mstat.ReceivedFrames);
      printf("SentFramesEvent      :%d\n",(int) mstat.SentFramesEvent);
      printf("UtcPllErrs           :%d\n",(int) mstat.UtcPllErrs);
      printf("LastExt1Start        :%s\n",TimeToStr(&(mstat.LastExt1Start),1));

   }

   return arg;
}

/*****************************************************************/

unsigned long GetHptdcVersion() {

CtrDrvrHptdcIoBuf hpio;
HptdcRegisterVlaue hpver, hpverbk;

   hpver.Value    = HptdcVERSION;
   hpver.StartBit = 0;
   hpver.EndBit   = 31;

   hpio.Size = 1;
   hpio.Pflg = 0;               /* No Parity */
   hpio.Rflg = 0;               /* No state reset */
   hpio.Cmd  = HptdcCmdIDCODE;
   hpio.Wreg = &hpver;
   hpio.Rreg = &hpverbk;

   if (ioctl(ctr,CtrDrvrHPTDC_OPEN,NULL) < 0) {
      IErr("HPTDC_OPEN",NULL);
      return 0;
   }
   if (ioctl(ctr,CtrDrvrHPTDC_IO,&(hpio)) < 0) {
      IErr("HPTDC_IO",NULL);
      return 0;
   }
   ioctl(ctr,CtrDrvrHPTDC_CLOSE,NULL);

   return hpverbk.Value;
}

/*****************************************************************/

int GetVersion(int arg) {
CtrDrvrVersion version;

int hptdc;
CtrDrvrTime t;
unsigned long hpver, vhdlver;
char fname[128], txt[128], *cp, *ep;
CtrDrvrHardwareType ht;

   arg++;

   bzero((void *) &version, sizeof(CtrDrvrVersion));
   if (ioctl(ctr,CtrDrvrGET_VERSION,&version) < 0) {
      IErr("GET_VERSION",NULL);
      return arg;
   }

   t.Second = version.VhdlVersion;
   t.TicksHPTDC = 0;
   printf("VHDL Compiled: [%u] %s\n",(int) t.Second,TimeToStr(&t,0));

   t.Second = version.DrvrVersion;
   t.TicksHPTDC = 0;
   printf("Drvr Compiled: [%u] %s\n",(int) t.Second,TimeToStr(&t,0));

   printf("%s Compiled: %s %s\n", "ctrtest", __DATE__, __TIME__);

   hptdc = 0;
   hpver = GetHptdcVersion();
   if (hpver == HptdcVERSION) {
      hptdc = 1;
      printf("HPTDC Chip Version: 0x%08X Version: 1.3: OK\n",(int) hpver);
   } else if ((hpver == 0xFFFFFFFF) || (hpver == 0))
      printf("HPTDC Chip Version: 0x%08X No HPTDC chip installed\n",(int) hpver);
   else
      printf("HPTDC Chip version: 0x%08X Should be: 0x%08X: Bad version number: ERROR\n",
	     (int) hpver,
	     (int) HptdcVERSION);

   if (version.HardwareType == CtrDrvrHardwareTypeCTRP) printf("Hardware Type: CTRP PMC (3 Channel)\n");
   if (version.HardwareType == CtrDrvrHardwareTypeCTRI) printf("Hardware Type: CTRI PCI (4 Channel)\n");
   if (version.HardwareType == CtrDrvrHardwareTypeCTRV) printf("Hardware Type: CTRV VME (8 Channel)\n");

   if (GetFile("Vhdl.versions") == NULL) return arg;
   strcpy(fname,path);
   inp = fopen(fname,"r");

   if (inp) {
      for (ht=CtrDrvrHardwareTypeNONE; ht<CtrDrvrHardwareTYPES; ht++) {
	 if (fgets(txt,128,inp) != NULL) {
	    if (ht == version.HardwareType) {
	       printf("%s:%s",fname,txt);
	       cp = ep = txt;
	       vhdlver = strtoul(cp,&ep,10);
	       if ((vhdlver) && (vhdlver == version.VhdlVersion)) {
		  printf("VHDL version:%d is correct and up to date\n",
			 (int) vhdlver);
	       } else {
		  if (vhdlver > version.VhdlVersion) {
		     printf("VHDL version:%d is older than disc copy:%d ERROR\n",
			    (int) version.VhdlVersion,
			    (int) vhdlver);
		  } else {
		     printf("VHDL version:%d is more recent than disc copy:%d\n",
			    (int) version.VhdlVersion,
			    (int) vhdlver);
		  }
	       }
	       break;
	    }
	 } else {
	    printf("File:%s contains garbage\n",fname);
	    break;
	 }
      }
      fclose(inp);
   } else {
      perror("fopen");
      printf("Could not open the file: %s for reading\n",fname);
   }

   return arg;
}

/*****************************************************************/

int GetSetUtc(int arg) {
ArgVal   *v;
AtomType  at;

time_t tim;
CtrDrvrCTime ct;
CtrDrvrTime  *t;

   t = &ct.Time;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;

      if (v->Number == 1) {
	 if (time(&tim) > 0) {
	    t->Second = tim; t->TicksHPTDC = 0;
	    printf("UTC: Set time: %s\n",TimeToStr(t,0));
	    if (ioctl(ctr,CtrDrvrSET_UTC,&t->Second) < 0) {
	       IErr("SET_UTC",NULL);
	       return arg;
	    }
	    sleep(1);
	 }
      }
   }

   if (ioctl(ctr,CtrDrvrGET_UTC,&ct) < 0) {
      IErr("GET_UTC",NULL);
      return arg;
   }

   printf("UTC: %s C:%04d\n",TimeToStr(t,1),(int) ct.CTrain);

   return arg;
}

/*****************************************************************/

static int connected = 0;

int WaitInterrupt(int arg) { /* msk */
ArgVal   *v;
AtomType  at;

CtrDrvrConnection con;
CtrDrvrReadBuf rbf;
int i, cc, qflag, qsize, interrupt, cnt, msk;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Operator) {
      if (v->OId == OprNOOP) {
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;

	 for (i=0; i<CtrDrvrInterruptSOURCES; i++) {
	    printf("%s: 0x%04X ",int_names[i],(1<<i));
	    if ((i%4) == 0)  printf("\n");
	 }
	 printf("\nP<ptim object> C<ctim object>\n");
	 return arg;
      }
   }
   interrupt = 0;
   if (at == Numeric) {                 /* Hardware mask ? */
      arg++;

      if (v->Number) {
	 interrupt    = v->Number;
	 con.Module   = module;
	 con.EqpNum   = interrupt;
	 con.EqpClass = CtrDrvrConnectionClassHARD;
	 if (ioctl(ctr,CtrDrvrCONNECT,&con) < 0) {
	    IErr("CONNECT",&interrupt);
	    return arg;
	 }
	 connected = 1;
      } else {
	 con.Module   = 0;
	 con.EqpNum   = 0;
	 if (ioctl(ctr,CtrDrvrDISCONNECT,&con) < 0) {
	    IErr("DISCONNECT",&interrupt);
	    return arg;
	 }
	 connected = 0;
	 printf("Disconnected from all interrupts\n");
	 return arg;
      }
   }

   if (at == Alpha) {
      if ((v->Text[0] == 'P') || (v->Text[0] == 'p')) { /* Ptim equipment ? */
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;
	 if (at == Numeric) {
	    interrupt    = v->Number;
	    con.Module   = 0;
	    con.EqpNum   = interrupt;
	    con.EqpClass = CtrDrvrConnectionClassPTIM;
	    if (ioctl(ctr,CtrDrvrCONNECT,&con) < 0) {
	       IErr("CONNECT",&interrupt);
	       return arg;
	    }
	    connected = 1;
	 }
      } else if ((v->Text[0] == 'C') || (v->Text[0] == 'c')) { /* Ctim equipment ? */
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;
	 if (at == Numeric) {
	    interrupt    = v->Number;
	    con.Module   = module;
	    con.EqpNum   = interrupt;
	    con.EqpClass = CtrDrvrConnectionClassCTIM;
	    if (ioctl(ctr,CtrDrvrCONNECT,&con) < 0) {
	       IErr("CONNECT",&interrupt);
	       return arg;
	    }
	    connected = 1;
	 }
      }
   }

   if (connected == 0) {
      printf("WaitInterrupt: Error: No connections to wait for\n");
      return arg;
   }

   cnt = 0;
   do {
      cc = read(ctr,&rbf,sizeof(CtrDrvrReadBuf));
      if (cc <= 0) {
	 printf("Time out or Interrupted call\n");
	 return arg;
      }
      if (interrupt) {
	 if (con.EqpClass == CtrDrvrConnectionClassHARD) {
	    msk = 1 << rbf.InterruptNumber;
	    if (interrupt & msk) break;
	 } else {
	    if ((interrupt == rbf.Connection.EqpNum)
	    &&  (con.EqpClass == rbf.Connection.EqpClass)) break;
	 }
      } else break;
      if (cnt++ > 64) {
	 printf("Aborted wait loop after 64 iterations\n");
	 return arg;
      }
   } while (True);

   if (ioctl(ctr,CtrDrvrGET_QUEUE_FLAG,&qflag) < 0) {
      IErr("GET_QUEUE_FLAG",NULL);
      return arg;
   }
   if (ioctl(ctr,CtrDrvrGET_QUEUE_SIZE,&qsize) < 0) {
      IErr("GET_QUEUE_SIZE",NULL);
      return arg;
   }

   if (rbf.Connection.EqpClass == CtrDrvrConnectionClassHARD) {
      printf("Mod[%d] Int[0x%03x %d] Time[%d:%s]",
       (int) rbf.Connection.Module,
       (int) rbf.Connection.EqpNum,
       (int) rbf.InterruptNumber,
       (int) rbf.OnZeroTime.Time.Second,
	     TimeToStr(&rbf.OnZeroTime.Time,1));

      for (i=0; i<CtrDrvrInterruptSOURCES; i++) {
	 if ((1<<i) & rbf.Connection.EqpNum) {
	    printf(" %s",int_names[i]);
	 }
      }

   } else if (rbf.Connection.EqpClass == CtrDrvrConnectionClassPTIM) {
      printf("Mod[%d] PTIM[%s] Cntr[%d] Frme[0x%08X] Time[%s]",
       (int) rbf.Connection.Module,
	     GetPtmName(rbf.Connection.EqpNum,0),
       (int) rbf.InterruptNumber,
       (int) rbf.Frame.Long,
	     TimeToStr(&rbf.OnZeroTime.Time,1));
   } else {
      printf("Mod[%d] CTIM[%d] INum[%d] Frme[0x%08X] Time[%s]",
       (int) rbf.Connection.Module,
       (int) rbf.Connection.EqpNum,
       (int) rbf.InterruptNumber,
       (int) rbf.Frame.Long,
	     TimeToStr(&rbf.StartTime.Time,1));
   }

   if (qflag == 0) {
      if (qsize) printf(" Queued: %d",(int) qsize);
   }
   printf(" Hptdc: 0x%08X",(unsigned int) rbf.OnZeroTime.Time.TicksHPTDC);
   printf("\n");

   return arg;
}

/*****************************************************************/

int GetLatency(int arg) {
ArgVal   *v;
AtomType  at;

CtrDrvrCTime ct;
CtrDrvrConnection con;
CtrDrvrReadBuf rbf;
int i, cc, sec, qflag, loop;
double lat, avr, sum, maxt, mint;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   loop = 5;
   if (at == Numeric) {
      arg++;
      if (v->Number) loop = v->Number;
   }
   if (connected) {
      printf("Error: You must first disconnect from all interrupts (wi 0)\n");
      return arg;
   }
   if (ioctl(ctr,CtrDrvrGET_QUEUE_FLAG,&qflag) < 0) {
      IErr("GET_QUEUE_FLAG",NULL);
      return arg;
   }
   if (qflag == 0) {
      printf("Error: Queueing must be off (qf 1)\n");
      return arg;
   }

   con.Module = 0;
   con.EqpNum = CtrDrvrInterruptMaskPPS;
   con.EqpClass = CtrDrvrConnectionClassHARD;
   if (ioctl(ctr,CtrDrvrCONNECT,&con) < 0) {
      IErr("CONNECT",(int *) &con.EqpNum);
      return arg;
   }

   for (i=0; i<loop; i++) {

      cc = read(ctr,&rbf,sizeof(CtrDrvrReadBuf));
      if (ioctl(ctr,CtrDrvrGET_UTC,&ct) < 0) {
	 IErr("GET_UTC",NULL);
	 return arg;
      }
      if (cc <= 0) {
	 printf("Time out or Interrupted call\n");
	 return arg;
      }

      if (rbf.Connection.EqpClass == CtrDrvrConnectionClassHARD) {

	 printf("Time of 1PPS Interrupt: %s\n",TimeToStr(&rbf.OnZeroTime.Time,1));
	 printf("Time of call to driver: %s\n",TimeToStr(&ct.Time,1));

	 sec = ct.Time.Second - rbf.OnZeroTime.Time.Second;
	 lat = (double) (ct.Time.TicksHPTDC - rbf.OnZeroTime.Time.TicksHPTDC) / 0.256;
	 if (i) {
	    sum += lat;
	    if (lat < mint) mint = lat;
	    if (lat > maxt) maxt = lat;
	 } else sum = maxt = mint = lat;
	 avr  = sum / ((double) i+1);
	 printf("Latency of driver call: %d.%010.0f Seconds\n",sec,lat);
	 printf("Average time: %03d call: 0.%010.0f Seconds\n\n",i,avr);
	 printf("Max and Min: [0.%010.0f -  0.%010.0f]\n\n",maxt,mint);
      }
   }
   return arg;
}

/*****************************************************************/

#define ACT_STR_LEN 132

static int err_in_act = 0;
static CtrDrvrAction act;
static char act_str[ACT_STR_LEN];

char *ActionToStr(int anum) {

CtrDrvrTrigger              *trg;
CtrDrvrCounterConfiguration *cnf;
CtrDrvrTgmGroup             *grp;
char *cp = act_str;

   bzero((void *) &act, sizeof(CtrDrvrAction));
   bzero((void *) &act_str,ACT_STR_LEN);

   err_in_act = 0;
   act.TriggerNumber = anum;
   trg = &(act.Trigger);
   cnf = &(act.Config);
   grp = &(trg->Group);

   if (ioctl(ctr,CtrDrvrGET_ACTION,&act) < 0) {
      IErr("GET_ACTION",&anum);
      return NULL;
   }

   sprintf(cp,"[%d]",anum);
   cp = &act_str[strlen(act_str)];

   if      (act.EqpClass == CtrDrvrConnectionClassCTIM) sprintf(cp,"Ctm:");
   else if (act.EqpClass == CtrDrvrConnectionClassPTIM) sprintf(cp,"Ptm:");
   else                                                 return NULL;
   cp = &act_str[strlen(act_str)];

   sprintf(cp,"%d",(int) act.EqpNum);
   cp = &act_str[strlen(act_str)];

   sprintf(cp,"[Ctm:%d(%02x:%02x:%04x)]",
	  (int) trg->Ctim,
	  (int) trg->Frame.Struct.Header,
	  (int) trg->Frame.Struct.Code,
	  (int) trg->Frame.Struct.Value);
   cp = &act_str[strlen(act_str)];

   if (trg->TriggerCondition != CtrDrvrTriggerConditionNO_CHECK) {
      if (trg->Machine >= CtrDrvrMachineMACHINES) {
	 printf("Error: In action trigger: Illegal machine: %d\n",(int) trg->Machine);
	 trg->Machine = 0;
	 err_in_act++;
      }
      if (trg->TriggerCondition >= CtrDrvrTriggerCONDITIONS) {
	 printf("Error: In action trigger: Illegal trigger condition: %d\n",(int) trg->TriggerCondition);
	 trg->TriggerCondition = 0;
	 err_in_act++;
      }
      sprintf(cp,"[%s %2d%s0x%X]",
	     MachineNames[trg->Machine],
	     (int) grp->GroupNumber,
	     TriggerConditionNames[trg->TriggerCondition],
	     (int) grp->GroupValue);
      cp = &act_str[strlen(act_str)];
   }
   if (cnf->Start >= CtrDrvrCounterSTARTS) {
      printf("Error: In action config: Illegal counter start: %d\n",(int) cnf->Start);
      cnf->Start = 0;
      err_in_act++;
   }
   if (cnf->Mode >= CtrDrvrCounterMODES) {
      printf("Error: In action config: Illegal counter mode: %d\n",(int) cnf->Mode);
      cnf->Mode = 0;
      err_in_act++;
   }
   if (cnf->Clock >= CtrDrvrCounterCLOCKS) {
      printf("Error: In action config: Illegal counter clock: %d\n",(int) cnf->Clock);
      cnf->Clock = 0;
      err_in_act++;
   }
   if (cnf->OnZero >= 4) {
      printf("Error: In action config: Illegal counter onzero: %d\n",(int) cnf->OnZero);
      cnf->OnZero = 0;
      err_in_act++;
   }

   sprintf(cp,"[Chn:%d Str:%s Mde:%s Clk:%s %d#%d %s]",
	  trg->Counter,
	  CounterStart[cnf->Start],
	  CounterMode[cnf->Mode],
	  CounterClock[cnf->Clock],
	  (int) cnf->Delay,
	  (int) cnf->PulsWidth,
	  CounterOnZero[cnf->OnZero]);
   cp = &act_str[strlen(act_str)];

   return act_str;
}

/*****************************************************************/

int SimulateInterrupt(int arg) { /* msk */
ArgVal   *v;
AtomType  at;

CtrDrvrConnection con;
CtrDrvrWriteBuf wbf;
CtrDrvrPtimBinding ob;
int i, cc, trignum;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Operator) {
      if (v->OId == OprNOOP) {
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;

	 for (i=0; i<CtrDrvrInterruptSOURCES; i++) {
	    printf("%s: 0x%04X ",int_names[i],(1<<i));
	    if ((i%4) == 0)  printf("\n");
	 }
	 printf("\nT<trig number> P<ptim object> C<ctim object>\n");
	 return arg;
      }
   }

   trignum = 0;

   if (at == Numeric) {                 /* Hardware mask ? */
      arg++;

      if (v->Number) {
	 con.Module   = module;
	 con.EqpNum   = v->Number;
	 con.EqpClass = CtrDrvrConnectionClassHARD;
      }
   } else if (at == Alpha) {
      if ((v->Text[0] == 'P') || (v->Text[0] == 'p')) { /* Ptim equipment ? */
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;
	 if (at == Numeric) {
	    ob.EqpNum = v->Number;
	    ob.ModuleIndex = 0;
	    ob.StartIndex = 0;
	    ob.Counter = 0;
	    ob.Size = 0;
	    if (ioctl(ctr,CtrDrvrGET_PTIM_BINDING,&ob) < 0) {
	       printf("Error: Cant get Binding for PTIM object: %d\n",(int) ob.EqpNum);
	       IErr("GET_PTIM_BINDING",NULL);
	       return arg;
	    }
	    con.Module   = ob.ModuleIndex + 1;
	    con.EqpNum   = ob.EqpNum;
	    con.EqpClass = CtrDrvrConnectionClassPTIM;
	 }
      } else if ((v->Text[0] == 'C') || (v->Text[0] == 'c')) { /* Ctim equipment ? */
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;
	 if (at == Numeric) {
	    con.Module   = 1;
	    con.EqpNum   = v->Number;
	    con.EqpClass = CtrDrvrConnectionClassCTIM;
	 }
      } else if ((v->Text[0] == 'T') || (v->Text[0] == 't')) { /* Trigger number */
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;
	 if (at == Numeric) {
	    trignum    = v->Number;
	    act.TriggerNumber = trignum;
	    if (ioctl(ctr,CtrDrvrGET_ACTION,&act) < 0) {
	       IErr("GET_ACTION",&trignum);
	       return arg;
	    }
	    ob.EqpNum = act.EqpNum;
	    ob.ModuleIndex = 0;
	    ob.StartIndex = 0;
	    ob.Counter = 0;
	    ob.Size = 0;
	    if (ioctl(ctr,CtrDrvrGET_PTIM_BINDING,&ob) < 0) {
	       printf("Error: Cant get Binding for PTIM object: %d\n",(int) ob.EqpNum);
	       IErr("GET_PTIM_BINDING",NULL);
	       return arg;
	    }
	    con.Module   = ob.ModuleIndex + 1;
	    con.EqpNum   = act.EqpNum;
	    con.EqpClass = act.EqpClass;
	 }
      }
   }
   wbf.TriggerNumber = trignum;
   wbf.Connection = con;
   cc = write(ctr,&wbf,sizeof(CtrDrvrReadBuf));
   return arg;
}

/*****************************************************************/

static char *eact_help =

"<CrLf>                 Next action           \n"
"/<Action Number>       Open action for edit  \n"
"?                      Print this help text  \n"
".                      Exit from the editor  \n"
"i<CTIM>                Change CTIM trigger number  CTIM                                        \n"
"f<Frame>               Change frame                Frame                                       \n"
"t<Trigger Condition>   Change trigger condition    */=/&                                       \n"
"m<Trigger Machine>     Change trigger machine      LHC/SPS/[CPS|SCT]/[PSB|FCT]/LEI/ADE         \n"
"n<Trigger Group>       Change trigger group number Group Number                                \n"
"g<Trigger group value> Change trigger group value  Group Value                                 \n"
"s<Start>               Change Start                1KHz/Ext1/Ext2/Chnd/Self/Remt/Pps/Chnd+Stop \n"
"k<Clock>               Change Clock                1KHz/10MHz/40MHz/Ext1/Ext2/Chnd             \n"
"o<Mode>                Change Mode                 Once/Mult/Brst/Mult+Brst                    \n"
"w<Pulse Width>         Change Pulse Width          Pulse Width                                 \n"
"v<Delay>               Change Delay                Delay                                       \n"
"e<enable>              Enable or Disable output    1=Enable/0=Disable                          \n";

static CtrDrvrCtimObjects ctimo;

void EditActions(int start, int size, int mod) {

CtrDrvrTrigger              *trg;
CtrDrvrCounterConfiguration *cnf;
CtrDrvrTgmGroup             *grp;

char c, *cp, *ep, txt[128];
int i, n, wbk, ok;
unsigned long nadr, anum;
unsigned long eqp, frm, grn, grv, plw, dly, trc, mch, str, clk, mde, enb;

   if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&mod);

   anum = start;

   act.TriggerNumber = anum;
   trg = &(act.Trigger);
   cnf = &(act.Config);
   grp = &(trg->Group);

   if (ioctl(ctr,CtrDrvrLIST_CTIM_OBJECTS,&ctimo) < 0) {
      IErr("LIST_CTIM_OBJECTS",NULL);
      if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
      return;
   }

   while (1) {
      if (ActionToStr(anum)) {
	 printf("%s : ",act_str); fflush(stdout);

	 bzero((void *) txt, 128); n = 0; c = 0;
	 cp = ep = txt; wbk = 0;
	 while ((c != '\n') && (n < 128)) c = txt[n++] = (char) getchar();

	 while (*cp != 0) {
	    switch (*cp++) {

	       case '\n':
		  if (wbk || err_in_act) {
		     wbk = 0; err_in_act = 0;
		     if (ioctl(ctr,CtrDrvrSET_ACTION,&act) < 0) {
			IErr("SET_ACTION",(int *) &anum);
			if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
			return;
		     }
		  } else if (n==1) {
		     anum++;
		     if (anum >= start+size) {
			anum = start;
			printf("\n");
		     }
		  }
	       break;

	       case '/':
		  nadr = strtoul(cp,&ep,0);
		  if (cp != ep) cp = ep;
		  if ((nadr<start) || (nadr>=start+size)) nadr = start;
		  anum = nadr;
	       break;

	       case '?':
		  printf("%s\n",eact_help);
	       break;

	       case 'q':
	       case '.':
		  if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		  return;

	       case 'i':
		  eqp = strtoul(cp,&ep,0); cp = ep;
		  if (eqp) {
		     ok = 0;
		     for (i=0; i<ctimo.Size; i++) {
			if (eqp == ctimo.Objects[i].EqpNum) {
			   trg->Ctim  = eqp;
			   trg->Frame = ctimo.Objects[i].Frame;
			   wbk = ok = 1;
			   break;
			}
		     }
		     if (ok) break;
		  }
		  printf("Error: No such CTIM equipment: %d\n",(int) eqp);
		  if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		  return;

	       case 'f':
		  frm = strtoul(cp,&ep,0); cp = ep;
		  if (frm) {
		     ok = 0;
		     for (i=0; i<ctimo.Size; i++) {
			if ((frm & 0xFFFF0000) == (ctimo.Objects[i].Frame.Long & 0xFFFF0000)) {
			   trg->Ctim  = ctimo.Objects[i].EqpNum;
			   trg->Frame.Long = frm;
			   wbk = ok = 1;
			   break;
			}
		     }
		     if (ok) break;
		  }
		  printf("Error: No CTIM equipment has Frame: %d\n",(int) frm);
		  if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		  return;

	       case 't':
		  trc = strtoul(cp,&ep,0); cp = ep;
		  if (trc < CtrDrvrTriggerCONDITIONS) {
		     trg->TriggerCondition = trc;
		     wbk = 1;
		     break;
		  }
		  printf("Error: No such Trigger Condition: %d\n",(int) trc);
		  if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		  return;

	       case 'm':
		  if (trg->TriggerCondition != CtrDrvrTriggerConditionNO_CHECK) {
		     mch = strtoul(cp,&ep,0); cp = ep;
		     if (mch < CtrDrvrMachineMACHINES) {
			trg->Machine = mch;
			wbk = 1;
			break;
		     }
		     printf("Error: No such Machine: %d\n",(int) mch);
		     if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		     return;
		  }
		  printf("Error: Telegram checking is Off\n");
		  if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		  return;

	       case 'n':
		  if (trg->TriggerCondition != CtrDrvrTriggerConditionNO_CHECK) {
		     grn = strtoul(cp,&ep,0); cp = ep;
		     if ((grn > 0) && (grn <= CtrDrvrTgmGROUP_VALUES)) {
			grp->GroupNumber = grn;
			wbk = 1;
			break;
		     }
		     printf("Error: Group Number: %d Out of Range\n",(int) grn);
		     if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		     return;
		  }
		  printf("Error: Telegram checking is Off\n");
		  if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		  return;

	       case 'g':
		  if (trg->TriggerCondition != CtrDrvrTriggerConditionNO_CHECK) {
		     grv = strtoul(cp,&ep,0); cp = ep;
		     grp->GroupValue = grv;
		     wbk = 1;
		     break;
		  }
		  printf("Error: Telegram checking is Off\n");
		  if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		  return;

	       case 's':
		  str = strtoul(cp,&ep,0); cp = ep;
		  if (str < CtrDrvrCounterSTARTS) {
		     cnf->Start = str;
		     wbk = 1;
		     break;
		  }
		  printf("Error: No such counter Start: %d\n",(int) str);
		  if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		  return;

	       case 'k':
		  clk = strtoul(cp,&ep,0); cp = ep;
		  if (clk<=CtrDrvrCounterCLOCKS) {
		     cnf->Clock = clk;
		     wbk = 1;
		     break;
		  }
		  printf("Error: No such counter Clock: %d\n",(int) clk);
		  if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		  return;

	       case 'o':
		  mde = strtoul(cp,&ep,0); cp = ep;
		  if (mde<=CtrDrvrCounterMODES) {
		     cnf->Mode = mde;
		     wbk = 1;
		     break;
		  }
		  printf("Error: No such counter Mode: %d\n",(int) mde);
		  if (mod) ioctl(ctr,CtrDrvrSET_MODULE,&module);
		  return;

	       case 'w':
		  plw = strtoul(cp,&ep,0); cp = ep;
		  cnf->PulsWidth = plw;
		  wbk = 1;
	       break;

	       case 'v':
		  dly = strtoul(cp,&ep,0); cp = ep;
		  cnf->Delay = dly;
		  wbk = 1;
	       break;

	       case 'e':
		  enb = strtoul(cp,&ep,0); cp = ep;
		  if (enb) cnf->OnZero |=  CtrDrvrCounterOnZeroOUT;
		  else     cnf->OnZero &= ~CtrDrvrCounterOnZeroOUT;
		  wbk = 1;
	       break;

	       default: break;
	    }
	 }
      } else break;
   }
}

/*****************************************************************/

int GetSetActions(int arg) { /* action num, number of actions */
ArgVal   *v;
AtomType  at;

int anum, asze;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   anum = 1;
   asze = CtrDrvrRamTableSIZE;

   if (at == Numeric) {
      arg++;
      anum = v->Number;

      v = &(vals[arg]);
      at = v->Type;
      if (at == Numeric) {
	 arg++;
	 asze = v->Number;
      }
   }
   EditActions(anum,asze,0);
   return arg;
}

/*****************************************************************/

static char *ectim_help =

"<CrLf>        Next CTIM equipment   \n"
"/<Index>      Go to entry Index     \n"
"?             Print this help text  \n"
".             Exit from the editor  \n"
"f<Frame>      Change CTIM Frame     \n"
"x             Delete CTIM equipment \n"
"y<Id>,<Frame> Create CTIM equipment \n";

void EditCtim(int id) {

CtrDrvrCtimBinding ob;
char c, *cp, *ep, str[128];
int n, i, ix, nadr;

#ifdef PS_VER
char comment[128];
#endif

   if (id) {
      ix = -1;
      for (i=0; i<ctimo.Size; i++) {
	 if (id == ctimo.Objects[i].EqpNum) {
	    ix = i;
	    break;
	 }
      }
      if (ix < 0) {
	 printf("Error: No such CTIM equipment: %d\n",id);
	 return;
      }
   } else ix = 0;

   i = ix;

   while (1) {

      bzero((void *) &ctimo, sizeof(CtrDrvrCtimObjects));
      if (ioctl(ctr,CtrDrvrLIST_CTIM_OBJECTS,&ctimo) < 0) {
	 printf("Error: Cant get CTIM Object Bindings\n");
	 IErr("LIST_CTIM_OBJECTS",NULL);
	 return;
      }

      if (ctimo.Size) {

#ifdef PS_VER
	 bzero((void *) comment,128);
	 if (ctimo.Objects[i].Frame.Long == 0x0100FFFF)
	    strcpy(comment,"Millisecond C-Event with wildcard");
	 else
	    TgvFrameExplanation(ctimo.Objects[i].Frame.Long,comment);
	 printf("[%d]Ctm:%d Fr:0x%08X ;%s\t: ",i,
		(int) ctimo.Objects[i].EqpNum,
		(int) ctimo.Objects[i].Frame.Long,
		      comment);
#else
	 printf("[%d]Ctm:%d Fr:0x%08X : ",i,
		(int) ctimo.Objects[i].EqpNum,
		(int) ctimo.Objects[i].Frame.Long);
#endif

      } else
	 printf(">>>Ctm:None defined : ");
      fflush(stdout);

      bzero((void *) str, 128); n = 0; c = 0;
      cp = ep = str;
      while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();

      while (*cp != 0) {

	 switch (*cp++) {

	    case '\n':
	       if (n<=1) {
		  i++;
		  if (i>=ctimo.Size) {
		     i = ix;
		     printf("\n");
		  }
	       }
	    break;

	    case '/':
	       i = ix;
	       nadr = strtoul(cp,&ep,0);
	       if (cp != ep) {
		  if (nadr < ctimo.Size) i = nadr;
		  cp = ep;
	       }
	    break;

	    case 'q':
	    case '.': return;

	    case '?':
	       printf("%s\n",ectim_help);
	    break;

	    case 'f':
	       ob.EqpNum = ctimo.Objects[i].EqpNum;
	       ob.Frame.Long = strtoul(cp,&ep,0);
	       if (cp == ep) break;
	       cp = ep;
	       if (ioctl(ctr,CtrDrvrCHANGE_CTIM_FRAME,&(ob)) < 0) {
		  printf("Error: Cant create CTIM object: %d\n",(int) ob.EqpNum);
		  IErr("CHANGE_CTIM_FRAME",NULL);
		  return;
	       }
	       printf(">>>Ctm:%d Fr:0x%08X : Frame changed\n",
		      (int) ob.EqpNum,
		      (int) ob.Frame.Long);
	    break;

	    case 'x':
	       if (ioctl(ctr,CtrDrvrDESTROY_CTIM_OBJECT,&(ctimo.Objects[i])) < 0) {
		  printf("Error: Cant destroy CTIM object: %d\n",(int) ctimo.Objects[i].EqpNum);
		  IErr("DESTROY_CTIM_OBJECT",NULL);
		  return;
	       }
	       printf(">>>Ctm:%d Fr:0x%08X : Destroyed\n",
		      (int) ctimo.Objects[i].EqpNum,
		      (int) ctimo.Objects[i].Frame.Long);
	       if (ix>=(ctimo.Size-1)) ix=0;
	       i = ix;
	    break;

	    case 'y':
	       ob.EqpNum = strtoul(cp,&ep,0);
	       if (cp == ep) break;
	       cp = ep;
	       ob.Frame.Long = strtoul(cp,&ep,0);
	       if (cp == ep) break;
	       cp = ep;
	       if (ioctl(ctr,CtrDrvrCREATE_CTIM_OBJECT,&(ob)) < 0) {
		  printf("Error: Cant create CTIM object: %d\n",(int) ob.EqpNum);
		  IErr("CREATE_CTIM_OBJECT",NULL);
		  return;
	       }
	       printf(">>>Ctm:%d Fr:0x%08X : Created\n",
		      (int) ob.EqpNum,
		      (int) ob.Frame.Long);
	       i = ctimo.Size;
	    break;

	    default: ;
	 }
      }
   }
}

/*****************************************************************/

static char *eptim_help =

"<CrLf>              Next PTIM equipment   \n"
"/<Index>            Go to entry Index     \n"
"?                   Print this help text  \n"
".                   Exit from the editor  \n"
"a                   Edit actions          \n"
"x                   Delete PTIM equipment \n"
"y<Id>,<Cntr>,<Size> Create PTIM equipment \n";

void EditPtim(int id) {

CtrDrvrPtimObjects ptimo;
CtrDrvrPtimBinding ob;
CtrDrvrTrigger *trg;
CtrDrvrCounterConfiguration *cnf;

char c, *cp, *ep, str[128];
int n, i, j, ix, nadr;

   if (id) {
      ix = -1;
      for (i=0; i<ptimo.Size; i++) {
	 if (id == ptimo.Objects[i].EqpNum) {
	    ix = i;
	    break;
	 }
      }
      if (ix < 0) {
	 printf("Error: No such PTIM object: %d\n",id);
	 return;
      }
   } else ix = 0;

   i = ix;

   while (1) {

      bzero((void *) &ptimo, sizeof(CtrDrvrPtimObjects));
      if (ioctl(ctr,CtrDrvrLIST_PTIM_OBJECTS,&ptimo) < 0) {
	 printf("Error: Cant get PTIM Object Bindings\n");
	 IErr("LIST_PTIM_OBJECTS",NULL);
	 return;
      }

      if (ptimo.Size)
	 printf("[%d]Ptm:%s Mo:%d Ch:%d Sz:%d St:%d : ",
		      i+1,
		      GetPtmName(ptimo.Objects[i].EqpNum,0),
		(int) ptimo.Objects[i].ModuleIndex +1,
		(int) ptimo.Objects[i].Counter,
		(int) ptimo.Objects[i].Size,
		(int) ptimo.Objects[i].StartIndex  +1);
      else
	 printf(">>>Ptm:None defined : ");
      fflush(stdout);

      bzero((void *) str, 128); n = 0; c = 0;
      cp = ep = str;
      while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();

      while (*cp != 0) {

	 bzero((void *) &ptimo, sizeof(CtrDrvrPtimObjects));
	 if (ioctl(ctr,CtrDrvrLIST_PTIM_OBJECTS,&ptimo) < 0) {
	    printf("Error: Cant get PTIM Object Bindings\n");
	    IErr("LIST_PTIM_OBJECTS",NULL);
	    return;
	 }

	 switch (*cp++) {

	    case '\n':
	       if (n<=1) {
		  i++;
		  if (i>=ptimo.Size) {
		     i = ix;
		     printf("\n");
		  }
	       }
	    break;

	    case '/':
	       i = ix;
	       nadr = strtoul(cp,&ep,0);
	       if (cp != ep) {
		  if (nadr < ptimo.Size) i = nadr;
		  cp = ep;
	       }
	    break;

	    case 'q':
	    case '.': return;

	    case '?':
	       printf("%s\n",eptim_help);
	    break;

	    case 'a':
	       ob.EqpNum = ptimo.Objects[i].EqpNum;
	       ob.ModuleIndex = 0;
	       ob.StartIndex = 0;
	       ob.Counter = 0;
	       ob.Size = 0;
	       if (ioctl(ctr,CtrDrvrGET_PTIM_BINDING,&ob) < 0) {
		  printf("Error: Cant get Binding for PTIM object: %d\n",(int) ob.EqpNum);
		  IErr("GET_PTIM_BINDING",NULL);
		  return;
	       }
	       EditActions(ob.StartIndex+1,ob.Size,ptimo.Objects[i].ModuleIndex+1);
	    break;

	    case 'x':
	       ob.EqpNum = ptimo.Objects[i].EqpNum;
	       ob.ModuleIndex = module-1;
	       ob.StartIndex = 0;
	       ob.Counter = 0;
	       ob.Size = 0;
	       if (ioctl(ctr,CtrDrvrGET_PTIM_BINDING,&ob) < 0) {
		  printf("Error: Cant get Binding for PTIM object: %d\n",(int) ob.EqpNum);
		  IErr("GET_PTIM_BINDING",NULL);
		  return;
	       }
	       if (ioctl(ctr,CtrDrvrDESTROY_PTIM_OBJECT,&(ob)) < 0) {
		  printf("Error: Cant destroy PTIM object: %d\n",(int) ptimo.Objects[i].EqpNum);
		  IErr("DESTROY_PTIM_OBJECT",NULL);
		  return;
	       }
	       printf(">>>Ptm:%d Destroyed\n",
		      (int) ptimo.Objects[i].EqpNum);
	       if (ix>=(ptimo.Size-1)) ix=0;
	       i = ix;
	    break;

	    case 'y':
	       ob.ModuleIndex = module-1;
	       ob.StartIndex = 0;
	       ob.EqpNum = strtoul(cp,&ep,0);
	       if (cp == ep) break;
	       cp = ep;
	       ob.Counter = strtoul(cp,&ep,0);
	       if (cp == ep) break;
	       if ((ob.Counter < CtrDrvrCounter1)
	       ||  (ob.Counter > CHANNELS)) {
		  printf("Illegal Counter:%d Range is[%d..%d]\n",
			 (int) ob.Counter,
			 (int) CtrDrvrCounter1,
			 (int) CHANNELS);
		  return;
	       }
	       cp = ep;
	       ob.Size = strtoul(cp,&ep,0);
	       if (cp == ep) break;
	       cp = ep;

	       if (ioctl(ctr,CtrDrvrCREATE_PTIM_OBJECT,&(ob)) < 0) {
		  printf("Error: Cant create PTIM object: %d\n",(int) ob.EqpNum);
		  IErr("CREATE_PTIM_OBJECT",NULL);
		  return;
	       }
	       if (ioctl(ctr,CtrDrvrGET_PTIM_BINDING,&(ob)) < 0) {
		  printf("Error: Cant read PTIM object: %d\n",(int) ob.EqpNum);
		  IErr("GET_PTIM_BINDING",NULL);
		  return;
	       }
	       printf(">>>Ptm:%d Mo:%d Ch:%d Sz:%d St:%d Created\n",
		      (int) ob.EqpNum,
		      (int) ob.ModuleIndex +1,
		      (int) ob.Counter,
		      (int) ob.Size,
		      (int) ob.StartIndex  +1);

	       trg = &(act.Trigger);
	       cnf = &(act.Config);

	       act.EqpClass = CtrDrvrConnectionClassPTIM;
	       act.EqpNum = ob.EqpNum;

	       trg->Counter = ob.Counter;
	       trg->Ctim = 107;
	       trg->Frame.Long = 0x34070000;
	       trg->TriggerCondition = CtrDrvrTriggerConditionNO_CHECK;
	       trg->Machine = CtrDrvrMachineCPS;
	       trg->Group.GroupNumber = 1;
	       trg->Group.GroupValue = 24;

	       cnf->Start = CtrDrvrCounterStartNORMAL;
	       cnf->Mode = CtrDrvrCounterModeNORMAL;
	       cnf->Clock = CtrDrvrCounterClock1KHZ;
	       cnf->Delay = 1;
	       cnf->PulsWidth = 400;
	       cnf->OnZero = CtrDrvrCounterOnZeroOUT;

	       for (j=ob.StartIndex; j<ob.StartIndex+ob.Size; j++) {
		  act.TriggerNumber = j+1;
		  if (ioctl(ctr,CtrDrvrSET_ACTION,&act) < 0) {
		     printf("Error: Cant initialize PTIM object\n");
		     IErr("SET_ACTION",&i);
		     return;
		  }
	       }
	    break;

	    default: ;
	 }
      }
   }
}

/*****************************************************************/

int GetSetOutMask(int arg) { /* Counter, Output Mask */
ArgVal   *v;
AtomType  at;

CtrDrvrCounterMaskBuf cmsb;
unsigned long cntr, omsk;
int i;

   cntr = channel;
   cmsb.Counter = cntr;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      omsk = v->Number;
      if ((omsk & 0xFFE) != omsk) {
	 printf("Error: Output Mask: 0x%X Illegal\n",(int) omsk);
	 return arg;
      }
      ioctl(ctr,CtrDrvrGET_OUT_MASK,&cmsb);
      cmsb.Mask = omsk;

      v = &(vals[arg]);
      at = v->Type;
      if (at == Numeric) {
	 arg++;
	 if (v->Number == 1) cmsb.Polarity = CtrDrvrPolarityTTL_BAR;
	 else                cmsb.Polarity = CtrDrvrPolarityTTL;
      }

      if (ioctl(ctr,CtrDrvrSET_OUT_MASK,&cmsb) < 0) {
	 printf("Error: Cant set Mask: 0x%X for Counter: %d\n",(int) cmsb.Mask,(int) cmsb.Counter);
	 IErr("SET_OUT_MASK",NULL);
	 return arg;
      }
   }
   if (ioctl(ctr,CtrDrvrGET_OUT_MASK,&cmsb) < 0) {
      printf("Error: Cant Get Mask for Counter: %d\n",(int) cmsb.Counter);
      IErr("GET_OUT_MASK",NULL);
      return arg;
   }
   for (i=1; i<12; i++) printf("%04X ",(1<<i));
   printf("\n");
   for (i=1; i<12; i++) printf("%s ",mbits[i]);
   printf("\n");
   printf("\n");

   omsk = 1 << cntr;
   printf("Counter: %d [0x%X] Output Mask: 0x%X\n",(int) cmsb.Counter,(int) omsk,(int) cmsb.Mask);
   for (i=1; i<12; i++) {
      if ((1<<i) & cmsb.Mask) printf("%s ",mbits[i]);
      else                    printf(".... ");
   }
   printf("\n");
   if (cmsb.Polarity == CtrDrvrPolarityTTL_BAR) printf("Polarity: -Ve TTL-BAR (Negative Logic)\n");
   else                                         printf("Polarity: +Ve TTL (Posative Logic)\n");
   if ((omsk & cmsb.Mask) == 0) printf("Warning: [0x%X] Bit is not set in Mask\n",(int) omsk);
   return arg;
}

/*****************************************************************/

int GetSetRemote(int arg) { /* Remote flag */
ArgVal   *v;
AtomType  at;

CtrdrvrRemoteCommandBuf crmb;
unsigned long cntr, rflg;

   cntr = channel;
   crmb.Counter = cntr;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      rflg = v->Number;
      if (rflg) crmb.Remote = 1;
      else      crmb.Remote = 0;
      if (ioctl(ctr,CtrDrvrSET_REMOTE,&crmb) < 0) {
	 printf("Error: Cant set Counter: %d to Remote state: %d\n",
		(int) crmb.Counter,
		(int) crmb.Remote);
	 IErr("SET_REMOTE",NULL);
	 return arg;
      }
   }
   if (ioctl(ctr,CtrDrvrGET_REMOTE,&crmb) < 0) {
      printf("Error: Cant Get Remote state for Counter: %d\n",(int) crmb.Counter);
      IErr("GET_REMOTE",NULL);
      return arg;
   }
   if (crmb.Remote)
      printf("Counter: %d Remote controlled from host\n",(int) crmb.Counter);
   else
      printf("Counter: %d Local control from CTR card\n",(int) crmb.Counter);

   return arg;
}

/*****************************************************************/

char *RemoteToString(unsigned long rem) {

static char res[32];

int i, msk;

   bzero((void *) res,32);
   for (i=0; i<CtrDrvrREMOTES; i++) {
      msk = 1 << i;
      if (msk & rem) { strcat(res,rmbits[i]); strcat(res," "); }
      else             strcat(res,".. ");
   }
   return res;
}

/*****************************************************************/

int SetRemoteCmd(int arg) { /* Counter, Remote command */
ArgVal   *v;
AtomType  at;

CtrdrvrRemoteCommandBuf crmb;
unsigned long cntr, rcmd;
int i ,msk;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Operator) {
      if (v->OId == OprNOOP) {
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;
	 printf("Remote bits: ");
	 for (i=0; i<CtrDrvrREMOTES; i++) {
	    msk = 1 << i;
	    printf("0x%02x=%s ",msk,rmbits[i]);
	 }
	 printf("\n");
      }
      return arg;
   }

   cntr = channel;
   crmb.Counter = cntr;
   if (ioctl(ctr,CtrDrvrGET_REMOTE,&crmb) < 0) {
      printf("Error: Cant Get Remote state for Counter: %d\n",(int) crmb.Counter);
      IErr("GET_REMOTE",NULL);
      return arg;
   }

   if (crmb.Remote) {
      v = &(vals[arg]);
      at = v->Type;
      if (at == Numeric) {
	 arg++;
	 rcmd = v->Number;
	 if (rcmd) {
	    crmb.Remote = rcmd;
	    if (ioctl(ctr,CtrDrvrREMOTE,&crmb) < 0) {
	       printf("Error: Cant send: 0x%X to Counter: %d\n",
		      (int) crmb.Remote,
		      (int) crmb.Counter);
	       IErr("REMOTE",NULL);
	       return arg;
	    }
	    printf("Send: ");
	    for (i=0; i<CtrDrvrREMOTES; i++) {
	       msk = 1 << i;
	       if (rcmd & msk) printf("%s ",rmbits[i]);
	       else            printf("..");
	     }
	    printf(" : To counter: %d :Done\n", (int) crmb.Counter);
	    return arg;
	 } else printf("Error: Zero command value\n");
      } else printf("Error: No command field\n");
   } else printf("Error: Counter: %d Not under Remote host control\n",(int) crmb.Counter);
   return arg;
}

/*****************************************************************/

int SetDebHis(int arg) {
ArgVal   *v;
AtomType  at;

unsigned long stat;
int debh;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   debh = 0;
   if (at == Numeric) {
      arg++;
      debh = v->Number;
      if (ioctl(ctr,CtrDrvrSET_DEBUG_HISTORY,&debh) < 0) {
	 IErr("SET_DEBUG_HISTORY",NULL);
	 return arg;
      }
   }
   if (ioctl(ctr,CtrDrvrGET_IO_STATUS,&stat) <0) {
      IErr("GET_IO_STATUS",NULL);
      return arg;
   }
   printf("Mod: %d IoStatus:%s\n",module,IoStatusToStr(stat));
   return arg;
}


/*****************************************************************/

int SetBrutalPll(int arg) {
ArgVal   *v;
AtomType  at;

unsigned long stat;
int brtpll;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   brtpll = 0;
   if (at == Numeric) {
      arg++;
      brtpll = v->Number;
      if (ioctl(ctr,CtrDrvrSET_BRUTAL_PLL,&brtpll) < 0) {
	 IErr("SET_BRUTAL_PLL",NULL);
	 return arg;
      }
   }
   if (ioctl(ctr,CtrDrvrGET_IO_STATUS,&stat) <0) {
      IErr("GET_IO_STATUS",NULL);
      return arg;
   }
   printf("Mod: %d IoStatus:%s\n",module,IoStatusToStr(stat));
   return arg;
}

/*****************************************************************/

int GetSetPtim(int arg) { /* PTIM ID */
ArgVal   *v;
AtomType  at;

int ptim;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   ptim = 0;
   if (at == Numeric) {
      arg++;
      ptim = v->Number;
   }
   EditPtim(ptim);
   return arg;
}

/*****************************************************************/

int GetSetCtim(int arg) { /* CTIM ID */
ArgVal   *v;
AtomType  at;

int ctim;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   ctim = 0;
   if (at == Numeric) {
      arg++;
      ctim = v->Number;
   }
   EditCtim(ctim);
   return arg;
}

/*****************************************************************/

char *HrValueToString(HptdcRegisterVlaue *hrv) {
int i, len, msk;
static char res[33];

   for (i=0; i<32; i++) res[i] = ' ';
   res[32] = 0;
   len = hrv->EndBit - hrv->StartBit;
   if (len > 31) len = 31;
   for (i=0; i<=len; i++) {
      msk = 1 << (len - i);
      if (msk & hrv->Value) res[i] = '1';
      else                  res[i] = '0';
   }
   return res;
}

/*****************************************************************/

static char *ehptdc_help =

"<CrLf>   Next register \n"
"/<Index> Open register \n"
"?        Print help    \n"
".        Exit          \n"
"r<Radix> Change Radix  \n"
"v<Value> Change bits   \n";

void EditHptdcRegs(HptdcRegisterVlaue *hrv, HptdcName *names, int fields, int ix) {

char c, *cp, *ep, str[128];
int n, i, nadr, radix, valu;

   i = ix; radix = 16;

   while (1) {

      printf("%03d:[%03d..%03d]/[0x%08X/(Bin)%s] %s: ",
	     i,
	     (int) hrv[i].StartBit,
	     (int) hrv[i].EndBit,
	     (int) hrv[i].Value,
	     HrValueToString(&(hrv[i])),
	     names[i]);
      fflush(stdout);

      bzero((void *) str, 128); n = 0; c = 0;
      cp = ep = str;
      while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();

      while (*cp != 0) {

	 switch (*cp++) {

	    case '\n':
	       if (n<=1) {
		  i++;
		  if (i>=fields) { i = ix; printf("\n"); }
	       }
	    break;

	    case '/':
	       i = ix;
	       nadr = strtoul(cp,&ep,0);
	       if (cp != ep) {
		  if (nadr < fields) i = nadr;
		  cp = ep;
	       }
	    break;

	    case 'q':
	    case '.': return;

	    case '?':
	       printf("%s\n",ehptdc_help);
	    break;

	    case 'r':
	       radix = strtoul(cp,&ep,0);
	       if (cp != ep) {
		  if ((radix <= 1)
		  ||  (radix > 16)) radix = 16;
		  cp = ep;
	       } else radix = 16;
	       printf(">>>Radix: %d (In Decimal)\n",radix);
	    break;

	    case 'v':
	       valu = strtoul(cp,&ep,radix);
	       if (cp != ep) {
		  cp = ep;
		  hrv[i].Value = valu;
	       }
	    break;

	    default: ;
	 }
      }
   }
}

/*****************************************************************/

int HptdcSetupEdit(int arg) {
ArgVal   *v;
AtomType  at;
int ix;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   ix = 0;
   if (at == Numeric) {
      arg++;
      ix = v->Number;
      if (ix >= HptdcSetupREGISTERS) ix = 0;
   }
   EditHptdcRegs(hptdcSetup, hptdcSetupNames, HptdcSetupREGISTERS, ix);

   return arg;
}

/*****************************************************************/

int HptdcSetupIo(int arg) {

CtrDrvrHptdcIoBuf hpio;
int i, diffs;

   arg++;

   if (hptdc_setup_loaded) {
      hpio.Size = HptdcSetupREGISTERS;
      hpio.Pflg = 1;                    /* Parity On */
      hpio.Rflg = 1;                    /* Reset, we are loading a new setup */
      hpio.Cmd  = HptdcCmdSETUP;
      hpio.Wreg = hptdcSetup;
      hpio.Rreg = hptdcSetupRbk;

      if (ioctl(ctr,CtrDrvrHPTDC_OPEN,NULL) < 0) {
	 printf("Cant open HPTDC JTAG interface\n");
	 IErr("HPTDC_OPEN",NULL);
	 return arg;
      }
      if (ioctl(ctr,CtrDrvrHPTDC_IO,&(hpio)) < 0) {
	 printf("Cant access HPTDC setup registers on chip\n");
	 IErr("HPTDC_IO",NULL);
	 return arg;
      }
      ioctl(ctr,CtrDrvrHPTDC_CLOSE,NULL);
      diffs = 0;
      for (i=0; i<HptdcSetupREGISTERS; i++) {
	 if (hptdcSetup[i].Value != hptdcSetupRbk[i].Value) {
	    printf("%03d:(%03d..%03d)[New:%X Old:%X]\t%s\n",
		   i,
		   (int) hptdcSetup[i].StartBit,
		   (int) hptdcSetup[i].EndBit,
		   (int) hptdcSetup[i].Value,
		   (int) hptdcSetupRbk[i].Value,
		   hptdcSetupNames[i]);
		   diffs++;
	 }
      }
      if (diffs)
	 printf("NewValues: %d written\n",diffs);
      else
	 printf("NewValues: none\n");
      return arg;
   }
   printf("No setup registers loaded\n");
   return arg;
}

/*****************************************************************/

int ReadSetup(char *fname) {
AtomType  at;
char txt[128], *cp, *ep;
int i, j;
HptdcRegisterVlaue *hrv;

   inp = fopen(fname,"r");
   if (inp) {
      i = 0; while (1) {
	 if (fgets(txt,128,inp) == NULL) break;
	 cp = ep = txt;

	 if ((strlen(txt) > 0)
	 &&  ((txt[0] != '/') && (txt[0] != '\n') && (txt[0] != ' '))) {
	    hrv = &(hptdcSetup[i]);
	    hrv->Value = strtoul(cp,&ep,2);
	    if (cp != ep) {
	       cp = ep;
	       hrv->EndBit = strtoul(cp,&ep,10);
	       if (cp != ep) {
		  cp = ep;
		  hrv->StartBit = strtoul(cp,&ep,10);
		  if (cp != ep) {
		     cp = ep;
		     while(*cp == ' ') cp++;
		     strcpy(hptdcSetupNames[i],cp);
		     for (j=0; j<strlen(hptdcSetupNames[i]); j++) {
			at = atomhash[(int) hptdcSetupNames[i][j]];
			if ((at == Seperator)
			||  (at == Terminator)) {
			   hptdcSetupNames[i][j] = 0;
			   break;
			}
		     }
		     i++;
		     if (i >= HptdcSetupREGISTERS) break;
		     else                          continue;
		  }
	       }
	    }
	    printf("Syntax error in: %s at line: %d\n",fname,i+1);
	    return 0;
	 }
      }
      printf("Read: %d Setup register values from: %s\n",i,fname);
      fclose(inp); inp = NULL;
      hptdc_setup_loaded = 1;
      return 1;
   } else {
      perror("fopen");
      printf("Could not open the file: %s for reading\n",fname);
   }
   return 0;
}

/*****************************************************************/

int HptdcSetupReadFile(int arg) {
ArgVal   *v;
AtomType  at;
char fname[128], *cp;
int n, earg;

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

      GetFile("Ctr.hptdc.setup");
      strcpy(fname,path);

   } else {

      bzero((void *) fname, 128);

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

   if (YesNo("HptdcSetupReadFile: Read: HPTDC Setup from: ",fname) == 0)
      return earg;

   ReadSetup(fname);
   return earg;
}

/*****************************************************************/

int HptdcSetupWriteFile(int arg) {
ArgVal   *v;
AtomType  at;
char fname[128], *cp;
int n, earg;
int i;
HptdcRegisterVlaue *hrv;

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

      GetFile("Ctr.hptdc.setup");
      strcpy(fname,path);

   } else {

      bzero((void *) fname, 128);

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

   if (YesNo("HptdcWriteReadFile: Write: HPTDC Setup to: ",fname) == 0)
      return earg;

   inp = fopen(fname,"w");
   if (inp) {
      for (i=0; i<HptdcSetupREGISTERS; i++) {
	 hrv = &(hptdcSetup[i]);
	 fprintf(inp,"%s %3d %3d %s\n",
		 HrValueToString(hrv),
		 hrv->EndBit,
		 hrv->StartBit,
		 hptdcSetupNames[i]);
      }
      printf("Written: %d Setup register values to: %s\n",i,fname);
      fclose(inp); inp = NULL;

   } else {
      perror("fopen");
      printf("Could not open the file: %s for write\n",fname);
   }
   return earg;
}

/*****************************************************************/

int HptdcStatusRead(int arg) {

CtrDrvrHptdcIoBuf hpio;

   arg++;

   hpio.Size = HptdcStatusREGISTERS;
   hpio.Pflg = 0;                       /* Parity off */
   hpio.Rflg = 0;                       /* No state reset */
   hpio.Cmd  = HptdcCmdSTATUS;
   hpio.Wreg = hptdcStatus;
   hpio.Rreg = hptdcStatus;

   if (ioctl(ctr,CtrDrvrHPTDC_OPEN,NULL) < 0) {
      printf("Cant open HPTDC JTAG interface\n");
      IErr("HPTDC_OPEN",NULL);
      return arg;
   }
   if (ioctl(ctr,CtrDrvrHPTDC_IO,&(hpio)) < 0) {
      printf("Cant access HPTDC status registers on chip\n");
      IErr("HPTDC_IO",NULL);
      return arg;
   }
   ioctl(ctr,CtrDrvrHPTDC_CLOSE,NULL);
   EditHptdcRegs(hptdcStatus, hptdcStatusNames, HptdcStatusREGISTERS, 0);

   return arg;
}

/*****************************************************************/

int HptdcControlEdit(int arg) {
ArgVal   *v;
AtomType  at;

int ix;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   ix = 0;
   if (at == Numeric) {
      arg++;
      ix = v->Number;
      if (ix >= HptdcControlREGISTERS) ix = 0;
   }
   EditHptdcRegs(hptdcControl, hptdcControlNames, HptdcControlREGISTERS, ix);

   return arg;
}

/*****************************************************************/

int HptdcControlIo(int arg) {

CtrDrvrHptdcIoBuf hpio;
int i, diffs;

   arg++;

   if (hptdc_control_loaded) {
      hpio.Size = HptdcControlREGISTERS;
      hpio.Pflg = 1;                        /* Parity on */
      hpio.Rflg = 0;                        /* No state reset */
      hpio.Cmd  = HptdcCmdCONTROL;
      hpio.Wreg = hptdcControl;
      hpio.Rreg = hptdcControlRbk;

      if (ioctl(ctr,CtrDrvrHPTDC_OPEN,NULL) < 0) {
	 printf("Cant open HPTDC JTAG interface\n");
	 IErr("HPTDC_OPEN",NULL);
	 return arg;
      }
      if (ioctl(ctr,CtrDrvrHPTDC_IO,&(hpio)) < 0) {
	 printf("Cant access HPTDC control registers on chip\n");
	 IErr("HPTDC_IO",NULL);
	 return arg;
      }
      ioctl(ctr,CtrDrvrHPTDC_CLOSE,NULL);
      diffs = 0;
      for (i=0; i<HptdcControlREGISTERS; i++) {
	 if (hptdcControl[i].Value != hptdcControlRbk[i].Value) {
	    printf("%03d:(%03d..%03d)[New:%X Old:%X]\t%s\n",
		   i,
		   (int) hptdcControl[i].StartBit,
		   (int) hptdcControl[i].EndBit,
		   (int) hptdcControl[i].Value,
		   (int) hptdcControlRbk[i].Value,
		   hptdcControlNames[i]);
		   diffs++;
	 }
      }
      if (diffs)
	 printf("NewValues: %d written\n",diffs);
      else
	 printf("NewValues: none\n");
      return arg;
   }
   printf("No control registers have been loaded\n");
   return arg;
}

/*****************************************************************/

int ReadControl(char *fname) {
char txt[128], *cp, *ep;
AtomType  at;
int i, j;
HptdcRegisterVlaue *hrv;

   inp = fopen(fname,"r");
   if (inp) {
      i = 0; while (1) {
	 if (fgets(txt,128,inp) == NULL) break;
	 cp = ep = txt;

	 if ((strlen(txt) > 0)
	 &&  ((txt[0] != '/') && (txt[0] != '\n') && (txt[0] != ' '))) {
	    hrv = &(hptdcControl[i]);
	    hrv->Value = strtoul(cp,&ep,2);
	    if (cp != ep) {
	       cp = ep;
	       hrv->EndBit = strtoul(cp,&ep,10);
	       if (cp != ep) {
		  cp = ep;
		  hrv->StartBit = strtoul(cp,&ep,10);
		  if (cp != ep) {
		     cp = ep;
		     while(*cp == ' ') cp++;
		     strcpy(hptdcControlNames[i],cp);
		     for (j=0; j<strlen(hptdcControlNames[i]); j++) {
			at = atomhash[(int) hptdcControlNames[i][j]];
			if ((at == Seperator)
			||  (at == Terminator)) {
			   hptdcControlNames[i][j] = 0;
			   break;
			}
		     }
		     i++;
		     if (i >= HptdcControlREGISTERS) break;
		     else                            continue;
		  }
	       }
	    }
	    printf("Syntax error in: %s at line: %d\n",fname,i+1);
	    return 0;
	 }
      }
      printf("Read: %d Control register values from: %s\n",i,fname);
      fclose(inp); inp = NULL;
      hptdc_control_loaded = 1;
      return 1;

   } else {
      perror("fopen");
      printf("Could not open the file: %s for reading\n",fname);
   }
   return 0;
}

/*****************************************************************/

int HptdcControlReadFile(int arg) {
ArgVal   *v;
AtomType  at;
char fname[128], *cp;
int n, earg;

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

      GetFile("Ctr.hptdc.control");
      strcpy(fname,path);

   } else {

      bzero((void *) fname, 128);

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

   if (YesNo("HptdcControlReadFile: Read: HPTDC Control from: ",fname) == 0)
      return earg;

   ReadControl(fname);
   return earg;
}

/*****************************************************************/

int HptdcControlWriteFile(int arg) {
ArgVal   *v;
AtomType  at;
char fname[128], *cp;
int n, earg;
int i;
HptdcRegisterVlaue *hrv;

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

      GetFile("Ctr.hptdc.control");
      strcpy(fname,path);

   } else {

      bzero((void *) fname, 128);

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

   if (YesNo("HptdcControlWriteFile: Write: HPTDC Control to: ",fname) == 0)
      return earg;

   inp = fopen(fname,"w");
   if (inp) {
      for (i=0; i<HptdcControlREGISTERS; i++) {
	 hrv = &(hptdcControl[i]);
	 fprintf(inp,"%s %3d %3d %s\n",
		 HrValueToString(hrv),
		 hrv->EndBit,
		 hrv->StartBit,
		 hptdcControlNames[i]);
      }
      printf("Written: %d Control register values to: %s\n",i,fname);
      fclose(inp); inp = NULL;

   } else {
      perror("fopen");
      printf("Could not open the file: %s for write\n",fname);
   }
   return earg;
}

/*****************************************************************/

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
      if (ioctl(ctr,CtrDrvrENABLE,&enb) < 0) {
	 printf("Error: Cant enable/disable event reception\n");
	 IErr("ENABLE",NULL);
	 return arg;
      }
   }
   if (ioctl(ctr,CtrDrvrGET_STATUS,&stat) < 0) {
      printf("Error: Cant read status from module: %d\n",(int) module);
      IErr("GET_STATUS",NULL);
      return arg;
   }
   printf("Module: %d Event reception: ",(int) module);
   if (stat & CtrDrvrStatusENABLED) printf("ENABLED\n");
   else                             printf("DISABLED\n");

   return arg;
}

/*****************************************************************/

int GetSetInputDelay(int arg) {
ArgVal   *v;
AtomType  at;
unsigned long dly;
float us;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      dly = v->Number;
      if (ioctl(ctr,CtrDrvrSET_INPUT_DELAY,&dly) < 0) {
	 printf("Error: Cant set input delay to: %d\n",(int) dly);
	 IErr("SET_INPUT_DELAY",NULL);
	 return arg;
      }
   }
   if (ioctl(ctr,CtrDrvrGET_INPUT_DELAY,&dly) < 0) {
      printf("Error: Cant get input delay from module: %d\n",(int) module);
      IErr("GET_INPUT_DELAY",NULL);
      return arg;
   }
   us = (25.00 * ((float) dly))/1000.00;
   printf("Module: %d Input Delay: %d = %fus\n",(int) module,(int) dly, us);

   return arg;
}

/*****************************************************************/

static char *ecnf_help =

"<CrLf>                 Next counter          \n"
"/<Action Number>       Open counter for edit \n"
"?                      Print this help text  \n"
".                      Exit from the editor  \n"
"s<Start>               Change Start                1KHz/Ext1/Ext2/Chnd/Self/Remt/Pps/Chnd+Stop \n"
"k<Clock>               Change Clock                1KHz/10MHz/40MHz/Ext1/Ext2/Chnd             \n"
"o<Mode>                Change Mode                 Once/Mult/Brst/Mult+Brst                    \n"
"w<Pulse Width>         Change Pulse Width          Pulse Width                                 \n"
"v<Delay>               Change Delay                Delay                                       \n"
"e<Enable>              Enable or Disable output    1=Enable/0=Disable                          \n"
"b<Bus>                 Enable businterrupts        1=Interrupt/0=No interrupt                  \n";

void EditConfig(unsigned long cntr) {

CtrDrvrCounterConfiguration *cnf;
CtrDrvrCounterConfigurationBuf cnfb;
CtrdrvrRemoteCommandBuf crmb;

char c, *cp, *ep, txt[128];
int n, wbk;
unsigned long plw, dly, str, clk, mde, enb, bus;

   cnf = &cnfb.Config;

   while (1) {
      crmb.Counter = cntr;
      if (ioctl(ctr,CtrDrvrGET_REMOTE,&crmb) < 0) {
	 printf("Error: Cant Get Remote state for Counter: %d\n",(int) crmb.Counter);
	 IErr("GET_REMOTE",NULL);
	 return;
      }
      if (crmb.Remote) {
	 cnfb.Counter = cntr;
	 if (ioctl(ctr,CtrDrvrGET_CONFIG,&cnfb) < 0) {
	    printf("Error: Cant Get configuration for Counter: %d\n",(int) cntr);
	    IErr("GET_CONFIG",NULL);
	    return;
	 }
	 printf("[Ch:%d] St:%s Mo:%s Ck:%s %d(%d) %s : ",
		 (int) cntr,
		 CounterStart[cnf->Start],
		 CounterMode[cnf->Mode],
		 CounterClock[cnf->Clock],
		 (int) cnf->Delay,
		 (int) cnf->PulsWidth,
		 CounterOnZero[cnf->OnZero]);
	 fflush(stdout);

	 bzero((void *) txt, 128); n = 0; c = 0;
	 cp = ep = txt; wbk = 0;
	 while ((c != '\n') && (n < 128)) c = txt[n++] = (char) getchar();

	 while (*cp != 0) {
	    switch (*cp++) {

	       case '\n':
		  if (wbk) {
		     wbk = 0;
		     if (ioctl(ctr,CtrDrvrSET_CONFIG,&cnfb) < 0) {
			printf("Error: Cant set counter: %d configuration\n",(int) cntr);
			IErr("SET_CONFIG",(int *) &cntr);
			return;
		     }
		     crmb.Remote = CtrDrvrRemoteLOAD;
		     if (ioctl(ctr,CtrDrvrREMOTE,&crmb) < 0) {
			IErr("REMOTE",(int *) &cntr);
			return;
		     }
		  } else if (n==1) {
		     cntr++;
		     if (cntr > CHANNELS) {
			cntr = CtrDrvrCounter1;
			printf("\n");
		     }
		  }
	       break;

	       case '/':
		  cntr = strtoul(cp,&ep,0);
		  if (cp != ep) cp = ep;
		  if ((cntr<CtrDrvrCounter1) || (cntr>CHANNELS)) cntr = CtrDrvrCounter1;
	       break;

	       case '?':
		  printf("%s\n",ecnf_help);
	       break;

	       case 'q':
	       case '.': return;

	       case 's':
		  str = strtoul(cp,&ep,0); cp = ep;
		  if (str < CtrDrvrCounterSTARTS) {
		     cnf->Start = str;
		     wbk = 1;
		     break;
		  }
		  printf("Error: No such counter Start: %d\n",(int) str);
		  return;

	       case 'k':
		  clk = strtoul(cp,&ep,0); cp = ep;
		  if (clk<=CtrDrvrCounterCLOCKS) {
		     cnf->Clock = clk;
		     wbk = 1;
		     break;
		  }
		  printf("Error: No such counter Clock: %d\n",(int) clk);
		  return;

	       case 'o':
		  mde = strtoul(cp,&ep,0); cp = ep;
		  if (mde<=CtrDrvrCounterMODES) {
		     cnf->Mode = mde;
		     wbk = 1;
		     break;
		  }
		  printf("Error: No such counter Mode: %d\n",(int) mde);
		  return;

	       case 'w':
		  plw = strtoul(cp,&ep,0); cp = ep;
		  cnf->PulsWidth = plw;
		  wbk = 1;
	       break;

	       case 'v':
		  dly = strtoul(cp,&ep,0); cp = ep;
		  cnf->Delay = dly;
		  wbk = 1;
	       break;

	       case 'b':
		  bus = strtoul(cp,&ep,0); cp = ep;
		  if (bus) cnf->OnZero |=  CtrDrvrCounterOnZeroBUS;
		  else     cnf->OnZero &= ~CtrDrvrCounterOnZeroBUS;
		  wbk = 1;
	       break;

	       case 'e':
		  enb = strtoul(cp,&ep,0); cp = ep;
		  if (enb) cnf->OnZero |=  CtrDrvrCounterOnZeroOUT;
		  else     cnf->OnZero &= ~CtrDrvrCounterOnZeroOUT;
		  wbk = 1;
	       break;

	       default: break;
	    }
	 }
      } else {
	 printf("Counter: %d Not in Remote\n",(int) cntr);
	 cntr++;
	 if (cntr > CHANNELS) return;
      }
   }
}

/*****************************************************************/

int GetSetConfig(int arg) {
unsigned long cntr;

   arg++;
   cntr = 1;
   EditConfig(cntr);
   return arg;
}

/*****************************************************************/

int GetCounterHistory(int arg) {
unsigned long cntr;
CtrDrvrCounterHistoryBuf chsb;

   arg++;

   cntr = channel;
   chsb.Counter = cntr;
   if (ioctl(ctr,CtrDrvrGET_COUNTER_HISTORY,&chsb) < 0) {
      printf("Error: Cant Get History for Counter: %d\n",(int) chsb.Counter);
      IErr("GET_COUNTER_HISTORY",NULL);
      return arg;
   }

   printf("[Ch:%d] Act:%d Fr:0x%X\n",
	  (int) cntr,
	  (int) chsb.History.Index,
	  (int) chsb.History.Frame.Long);

   printf("Loaded : C%04d-%s [0x%08X - 0x%08X]\n",
	  (int) chsb.History.TriggerTime.CTrain,
	  TimeToStr(&(chsb.History.TriggerTime.Time),1),
	  (int) chsb.History.TriggerTime.Time.Second,
	  (int) chsb.History.TriggerTime.Time.TicksHPTDC);

   printf("Started: C%04d-%s [0x%08X - 0x%08X]\n",
	  (int) chsb.History.StartTime.CTrain,
	  TimeToStr(&(chsb.History.StartTime.Time),1),
	  (int) chsb.History.StartTime.Time.Second,
	  (int) chsb.History.StartTime.Time.TicksHPTDC);

   printf("Output : C%04d-%s [0x%08X - 0x%08X]\n",
	  (int) chsb.History.OnZeroTime.CTrain,
	  TimeToStr(&(chsb.History.OnZeroTime.Time),1),
	  (int) chsb.History.OnZeroTime.Time.Second,
	  (int) chsb.History.OnZeroTime.Time.TicksHPTDC);

   return arg;
}

/*****************************************************************/

int GetSetDac(int arg) { /* DAC Value */
ArgVal   *v;
AtomType  at;

CtrDrvrPll plb;
float aspn;
double ph, er;
int sign = 1;

   arg++;

   v = &(vals[arg]);
   at = v->Type;

   if (at == Operator) {
      if (v->OId == OprMI) {
	 sign = -1;
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;
      }
   }

   if (at == Numeric) {

      if (ioctl(ctr,CtrDrvrGET_PLL,&plb) < 0) {
	 IErr("GET_PLL",NULL);
	 return arg;
      }
      arg++;
      plb.Dac = v->Number * sign;

      if (ioctl(ctr,CtrDrvrSET_PLL,&plb) < 0) {
	 IErr("SET_PLL",NULL);
	 return arg;
      }
      if (ioctl(ctr,CtrDrvrSET_PLL,&plb) < 0) {
	 IErr("SET_PLL",NULL);
	 return arg;
      }
   }

   sleep(1);

   if (ioctl(ctr,CtrDrvrGET_PLL,&plb) < 0) {
      IErr("GET_PLL",NULL);
      return arg;
   }
   if (ioctl(ctr,CtrDrvrGET_PLL_ASYNC_PERIOD,&aspn) < 0) {
      IErr("GET_PLL_ASYNC_PERIOD",NULL);
      return arg;
   }
   ph = aspn * ((double) (int) plb.Phase / (double) (int) plb.NumAverage);
   er = aspn * ((double) (int) plb.Error / (double) (int) plb.NumAverage);

   printf("Error      : [0x%08x] %d\n",(int) plb.Error     ,(int) plb.Error);
   printf("Integrator : [0x%08x] %d\n",(int) plb.Integrator,(int) plb.Integrator);
   printf("Dac        : [0x%08x] %d\n",(int) plb.Dac       ,(int) plb.Dac);
   printf("LastItLen  : [0x%08x] %d\n",(int) plb.LastItLen ,(int) plb.LastItLen);
   printf("KI         : [0x%08x] %d\n",(int) plb.KI        ,(int) plb.KI);
   printf("KP         : [0x%08x] %d\n",(int) plb.KP        ,(int) plb.KP);
   printf("NumAverage : [0x%08x] %d\n",(int) plb.NumAverage,(int) plb.NumAverage);
   printf("Phase      : [0x%08x] %d\n",(int) plb.Phase     ,(int) plb.Phase);
   printf("AsPrdNs    : %f (ns)\n",aspn);

   printf("Error*Period/Naverage  : %fns\n",er);
   printf("Phase*Period/Naverage  : %fns\n",ph);

   return arg;
}
/*****************************************************************/

int GetSetPll(int arg) { /* KI KP NAV PHASE */
ArgVal   *v;
AtomType  at;

CtrDrvrPll plb;
float aspn;
double ph, er;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {

      if (ioctl(ctr,CtrDrvrGET_PLL,&plb) < 0) {
	 IErr("GET_PLL",NULL);
	 return arg;
      }
      arg++;
      if (v->Number) plb.KI = v->Number;

      v = &(vals[arg]);
      at = v->Type;
      if (at == Numeric) {
	 arg++;
	 if (v->Number) plb.KP = v->Number;

	 v = &(vals[arg]);
	 at = v->Type;
	 if (at == Numeric) {
	    arg++;
	    if (v->Number) plb.NumAverage = v->Number;

	    v = &(vals[arg]);
	    at = v->Type;
	    if (at == Numeric) {
	       arg++;
	       if (v->Number) plb.Phase = v->Number;
	    }
	 }
      }
      printf("KI         : [0x%08x] %d\n",(int) plb.KI        ,(int) plb.KI);
      printf("KP         : [0x%08x] %d\n",(int) plb.KP        ,(int) plb.KP);
      printf("NumAverage : [0x%08x] %d\n",(int) plb.NumAverage,(int) plb.NumAverage);
      printf("Phase      : [0x%08x] %d\n",(int) plb.Phase     ,(int) plb.Phase);
      if (YesNo("Set PLL to above and ","read back") == 0) return arg;
      if (ioctl(ctr,CtrDrvrSET_PLL,&plb) < 0) {
	 IErr("SET_PLL",NULL);
	 return arg;
      }
   }

   sleep(1);

   if (ioctl(ctr,CtrDrvrGET_PLL,&plb) < 0) {
      IErr("GET_PLL",NULL);
      return arg;
   }
   if (ioctl(ctr,CtrDrvrGET_PLL_ASYNC_PERIOD,&aspn) < 0) {
      IErr("GET_PLL_ASYNC_PERIOD",NULL);
      return arg;
   }
   ph = aspn * ((double) (int) plb.Phase / (double) (int) plb.NumAverage);
   er = aspn * ((double) (int) plb.Error / (double) (int) plb.NumAverage);

   printf("Error      : [0x%08x] %d\n",(int) plb.Error     ,(int) plb.Error);
   printf("Integrator : [0x%08x] %d\n",(int) plb.Integrator,(int) plb.Integrator);
   printf("Dac        : [0x%08x] %d\n",(int) plb.Dac       ,(int) plb.Dac);
   printf("LastItLen  : [0x%08x] %d\n",(int) plb.LastItLen ,(int) plb.LastItLen);
   printf("KI         : [0x%08x] %d\n",(int) plb.KI        ,(int) plb.KI);
   printf("KP         : [0x%08x] %d\n",(int) plb.KP        ,(int) plb.KP);
   printf("NumAverage : [0x%08x] %d\n",(int) plb.NumAverage,(int) plb.NumAverage);
   printf("Phase      : [0x%08x] %d\n",(int) plb.Phase     ,(int) plb.Phase);
   printf("AsPrdNs    : %f (ns)\n",aspn);

   printf("Error*Period/Naverage  : %fns\n",er);
   printf("Phase*Period/Naverage  : %fns\n",ph);

   return arg;
}

/*****************************************************************/

int GetTelegram(int arg) {
ArgVal   *v;
AtomType  at;
CtrDrvrTgmBuf tgmb;
int i, m;

#ifdef PS_VER
int tm;
TgmGroupDescriptor desc;
TgmLineNameTable   ltab;
#endif

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      m = v->Number;
      if ((m > CtrDrvrMachineNONE) && (m <= CtrDrvrMachineMACHINES)) {
	 tgmb.Machine = m;
	 if (ioctl(ctr,CtrDrvrREAD_TELEGRAM,&tgmb) < 0) {
	    printf("Error: Cant read telegram for machine: %s Module: %d\n",MachineNames[m],(int) module);
	    IErr("READ_TELEGRAM",NULL);
	    return arg;
	 }
	 printf("Telegram for: %s Module: %d\n",MachineNames[m],(int) module);

#ifdef PS_VER
	 tm = TgvTgvToTgmMachine(m);
	 for (i=0; i<TgmLastGroupNumber(tm); i++) {
	    if (TgmGetGroupDescriptor(tm,i+1,&desc)) {
	       if (desc.Type == TgmEXCLUSIVE) {
		  if (TgmGetLineNameTable(tm,desc.Name,&ltab)) {
		     if ((tgmb.Telegram[i] > 0) && (tgmb.Telegram[i] <= ltab.Size)) {
			printf("%8s:%8s ",
			       desc.Name,
			       ltab.Table[tgmb.Telegram[i]-1].Name);
		     } else {
			printf("%8s: ??%05d ",
			       desc.Name,
			       tgmb.Telegram[i]);
		     }
		  } else {
		     printf("%8s:    %05d ",
			    desc.Name,
			    tgmb.Telegram[i]);
		  }
	       } else if (desc.Type == TgmBIT_PATTERN) {
		  printf("%8s:  0x%04X ",
			    desc.Name,
			    tgmb.Telegram[i]);
	       } else {
		  printf("%8s:   %05d ",
			    desc.Name,
			    tgmb.Telegram[i]);
	       }
	    } else {
	       printf("<Gn:%02d Gv:0x%04X %5d>",i+1,(int) tgmb.Telegram[i],(int) tgmb.Telegram[i]);
	    }
	    if ((i+1)%4 == 0) printf("\n");
	 }
#else
	 for (i=0; i<CtrDrvrTgmGROUP_VALUES; i++) {
	    printf("<Gn:%02d Gv:0x%04X %5d>",i+1,(int) tgmb.Telegram[i],(int) tgmb.Telegram[i]);
	    if ((i+1)%4 == 0) printf("\n");
	 }
#endif

	 printf("\n");
      } else printf("Error: Telegram ID: %d out of range\n",m);
   } else printf("Error: No Telegram ID supplied\n");
   return arg;
}

/*****************************************************************/

int GetEventHistory(int arg) {
ArgVal   *v;
AtomType  at;
unsigned long size;
int i, j, hi;
CtrDrvrEventHistory ehsb;

#ifdef PS_VER
int ctm;
char comment[128];
#endif

   arg++;

   size = 20;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      size = v->Number;
   }

   bzero((void *) &ehsb, sizeof(CtrDrvrEventHistory));
   if (ioctl(ctr,CtrDrvrREAD_EVENT_HISTORY,&ehsb) < 0) {
      printf("Error: Cant Get Event History for Module: %d\n",(int) module);
      IErr("READ_EVENT_HISTORY",NULL);
      return arg;
   }

   hi = ehsb.Index;
   if ((hi<0) || (hi>CtrDrvrHISTORY_TABLE_SIZE)) {
      printf("Error: CTR-Hardware: Bad history index: %d ==> 0\n",hi);
      hi = 0;
   }
   printf("\n");
   for (i=1; i<=size; i++) {
      j = hi - i;
      if (j<0) j += CtrDrvrHISTORY_TABLE_SIZE;

#ifdef PS_VER
      bzero((void *) comment,128);
      if ((ehsb.Entries[j].Frame.Long & 0x0f000000) == 0x03000000)
	 strcpy(comment,"Telegram frame");
      else
	 TgvFrameExplanation(ehsb.Entries[j].Frame.Long,comment);
      ctm = TgvGetMemberForFrame(ehsb.Entries[j].Frame.Long);

      printf("[%3d] Ctm:%04d/0x%08X C%04d %s ;%s\n",
	     j,
	     ctm,
	     (int) ehsb.Entries[j].Frame.Long,
	     (int) ehsb.Entries[j].CTime.CTrain,
	     TimeToStr(&(ehsb.Entries[j].CTime.Time),1),
	     comment);
#else
      printf("[%4d][%4d] 0x%08X C%04d %s\n",
	     i,
	     j,
	     (int) ehsb.Entries[j].Frame.Long,
	     (int) ehsb.Entries[j].CTime.CTrain,
	     TimeToStr(&(ehsb.Entries[j].CTime.Time),1));
#endif

   }
   return arg;
}

/*****************************************************************/

#define OB_NAME_LENGTH 17
#define OB_COMMENT_LENGTH 64
#define MACHINE_NAME_LENGTH 4

typedef struct {
   char           Name[OB_NAME_LENGTH];
   char           Comment[OB_COMMENT_LENGTH];
   unsigned long  EqpNum;
   unsigned long  Frame;
   CtrDrvrMachine Machine;
 } CtimMtgBinding;

/*=======================*/

int CtimRead(int arg) {

#ifdef PS_VER
int cnt, fal;
unsigned long equip, frame;
CtrDrvrCtimBinding ob;
TgvName eqname;
#endif

   arg++;

#ifdef PS_VER
   cnt = 0; fal = 0; equip = TgvFirstGMember();
   while (equip) {
      TgvGetNameForMember(equip,&eqname);
      frame = TgvGetFrameForMember(equip);
      ob.EqpNum = equip;
      ob.Frame.Long = frame;
      if (ioctl(ctr,CtrDrvrCREATE_CTIM_OBJECT,&(ob)) < 0) {
	 fal++;
	 if (arg) printf("Already exists: %s\n",eqname);
      } else {
	 cnt++;
	 printf("Created: CTIM: %s (%04d) 0x%08X OK\n",eqname,(int) equip, (int) frame);
      }
      equip = TgvNextGMember();
   }
   if (cnt) printf("Created: %d CTIM equipments OK\n",cnt);
   if (fal) printf("Existed: %d CTIM equipments\n"   ,fal);

#else
   printf("ctrtest has not been compiled with the PS_VER option\n");
#endif

   return arg;
}

/*****************************************************************/

int PtimReadFile(int arg) {
ArgVal   *v;
AtomType  at;
char txt[128], fname[128], *cp;
int n, earg;

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

      GetFile("Ctr.info");
      strcpy(fname,path);

   } else {

      bzero((void *) fname, 128);

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

   if (YesNo("PtimReadFile: Read from: ",fname) == 0)
      return earg;

#ifdef PS_VER
   if (_TgmSemIsBlocked(_TgmCfgGetTgmXXX ()->semName) ==0) {
      if (YesNo("Task get_tgm_tim is not running: ","Launch it") == 0) {
	 printf("Command aborted\n");
	 return earg;
      }
      sprintf(txt,"get_tgm_tim &");
      printf("Launching: %s\n",txt);
      system(txt);
   }
#endif

   if (ctr_device == CtrDrvrDeviceANY) sprintf(txt,"%s %s",GetFile("CtrReadInfo"),fname);
   else sprintf(txt,"%s %s %s",GetFile("CtrReadInfo"),fname,devnames[(int) ctr_device]);
   printf("Launching: %s\n\n",txt);
   system(txt);
   printf("\n");
   return earg;
}

/*****************************************************************/

int PtimWriteFile(int arg) {
ArgVal   *v;
AtomType  at;
char txt[128], fname[128], *cp;
int n, earg;

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

      GetFile("Ctr.info");
      strcpy(fname,path);

   } else {

      bzero((void *) fname, 128);

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

   if (YesNo("PtimWriteFile: Write to: ",fname) == 0)
      return earg;

#ifdef PS_VER
   if (_TgmSemIsBlocked(_TgmCfgGetTgmXXX ()->semName) ==0) {
      if (YesNo("Task get_tgm_tim is not running: ","Launch it") == 0) {
	 printf("Command aborted\n");
	 return earg;
      }
      sprintf(txt,"get_tgm_tim &");
      printf("Launching: %s\n",txt);
      system(txt);
   }
#endif

   if (ctr_device == CtrDrvrDeviceANY) sprintf(txt,"%s %s",GetFile("CtrWriteInfo"),fname);
   else sprintf(txt,"%s %s %s",GetFile("CtrWriteInfo"),fname,devnames[(int) ctr_device]);
   printf("Launching: %s\n\n",txt);
   system(txt);
   sprintf(txt,"%s %s",GetFile(editor),fname);
   system(txt);
   printf("\n");
   return earg;
}

/*****************************************************************/

int LaunchClock(int arg) {
char txt[128];

   arg++;

   GetFile("CtrClock");
   sprintf(txt,"%s %d",path,module);
   printf("Launching: %s\n\n",txt);
   Launch(txt);

   return arg;
}

/*****************************************************************/

int LaunchLookat(int arg) {  /* P/C <member> */
ArgVal   *v;
AtomType  at;

char txt[128], c;
int obj;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Alpha) {
      arg++;
      c = v->Text[0];
      v = &(vals[arg]);
      at = v->Type;
   } else c = 'P';

   obj = 0;
   if (at == Numeric) {
      arg++;
      obj = v->Number;
      v = &(vals[arg]);
      at = v->Type;
   } else {
      printf("LaunchLookat: No timing object ID specified\n");
      return arg;
   }
   GetFile("TimLookat");
   sprintf(txt,"%s %d %c %d",path,module,c,obj);
   printf("Launching: %s\n\n",txt);
   Launch(txt);

   return arg;
}

/*****************************************************************/

int LaunchVideo(int arg) {
ArgVal   *v;
AtomType  at;

char txt[128];
static int m;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Numeric) {
      arg++;
      m = v->Number;
      v = &(vals[arg]);
      at = v->Type;
   }
   if (at == Alpha) {
      arg++;
      for (m=1; m<CtrDrvrMachineMACHINES; m++) {
	 if (strcmp(MachineNames[m],v->Text) == 0) {
	    break;
	 }
      }
      v = &(vals[arg]);
      at = v->Type;
   }
   if ((m < 1) || (m >= CtrDrvrMachineMACHINES)) {
      printf("LaunchVideo: Bad machine name/number\n");
      return arg;
   }

#ifdef PS_VER
   if (_TgmSemIsBlocked(_TgmCfgGetTgmXXX ()->semName) ==0) {
      if (YesNo("Task get_tgm_tim is not running: ","Launch it") == 0) {
	 printf("Command aborted\n");
	 return arg;
      }
      sprintf(txt,"get_tgm_tim &");
      printf("Launching: %s\n",txt);
      system(txt);
   }
#endif

   GetFile("video");
   sprintf(txt,"%s %s",path,MachineNames[m]);
   printf("Launching: %s\n\n",txt);
   Launch(txt);

   return arg;
}

/*****************************************************************/

int ListActions(int arg) {
ArgVal   *v;
AtomType  at;

int i, start, count, eqp;
char *cp;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   start = 1;
   if (at == Numeric) {
      start = v->Number;
      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }
   count = 20;
   if (at == Numeric) {
      count = v->Number;
      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }

   eqp = 0;
   for (i=start; i<start+count; i++) {
      cp = ActionToStr(i);
      if (cp == NULL) continue;
      if ((eqp != 0) && (act.EqpNum != eqp)) printf("----------\n");
      eqp = act.EqpNum;
      printf("%s\n",cp);
   }
   return arg;
}

/*****************************************************************/

int Init(int arg) {
ArgVal   *v;
AtomType  at;

char txt[128], fname[64];
int i, cnt, enb;
unsigned long stat, option;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   option = 0;
   if (at == Numeric) {
      option = v->Number;
      arg++;
      v = &(vals[arg]);
      at = v->Type;
   }

   if (option == 0) {
      CtimRead(-1);

      GetFile("CtrReadInfo");
      strcpy(txt,path); strcat(txt," ");
      GetFile("Ctr.info");
      strcat(txt,path);
      if (ctr_device == CtrDrvrDeviceANY) {
	 strcat(txt," ");
	 strcat(txt,devnames[(int) ctr_device]);
      }
      printf("Launching: %s\n\n",txt);
      system(txt);
      printf("\n");
   }

   ioctl(ctr,CtrDrvrGET_MODULE_COUNT,&cnt);
   for (i=1; i<=cnt; i++) {
      ioctl(ctr,CtrDrvrSET_MODULE,&module);
      if (GetHptdcVersion() == HptdcVERSION) {

	 ioctl(ctr,CtrDrvrGET_STATUS,&stat);
	 if (stat & CtrDrvrStatusHPTDC_IN) continue; /* Already setup */

	 printf("HPTDC Found on Module:%d Setting up\n",i);
	 GetFile("Ctr.hptdc.setup.1");
	 strcpy(fname,path);
	 if (ReadSetup(fname)) {
	    HptdcSetupIo(0);
	    GetFile("Ctr.hptdc.control.1");
	    strcpy(fname,path);
	    if (ReadControl(fname)) {
	       HptdcControlIo(0);
	       GetFile("Ctr.hptdc.control.2");
	       strcpy(fname,path);
	       if (ReadControl(fname)) {
		  HptdcControlIo(0);
		  GetFile("Ctr.hptdc.control.3");
		  strcpy(fname,path);
		  if (ReadControl(fname)) {
		     HptdcControlIo(0);
		     GetFile("Ctr.hptdc.control.4");
		     strcpy(fname,path);
		     if (ReadControl(fname)) {
			HptdcControlIo(0);
			GetFile("Ctr.hptdc.control.5");
			strcpy(fname,path);
			if (ReadControl(fname)) {
			   HptdcControlIo(0);
			   enb = CtrDrvrCommandSET_HPTDC;
			   ioctl(ctr,CtrDrvrENABLE,&enb);
			   printf("HPTDC Setup on Module:%d OK\n",i);
			}
		     }
		  }
	       }
	    }
	 }
      } else {
	 enb = ~CtrDrvrCommandSET_HPTDC;
	 ioctl(ctr,CtrDrvrENABLE,&enb);
      }
   }
   return arg;
}

/* ======================================= */
/* Test a block of RAM memory with a value */

#define RAMAD 389
#define RAMSZ 12288

#define MAX_ERRS 10

static int TestBlock(unsigned int str,     /* Start index */
		     unsigned int end,     /* End index */
		     unsigned int skp,     /* Skip index increment */
		     unsigned int pat,     /* Pattern to write */
		     unsigned int flg) {   /* 0=Fixed, 1=Increment, 2=Read Only */

unsigned int i, ecnt, data, addr;
CtrDrvrRawIoBlock iob;

   ecnt = 0;

   if (flg != 2) {       /* Write to the RAM ? */
      data = pat;
      iob.UserArray = &data;
      for (i=str; i<=end; i+=skp) {
	 addr = i + RAMAD;
	 iob.Size = 1;
	 iob.Offset = addr;
	 if (flg == 1) data = pat + i;
	 if (ioctl(ctr,CtrDrvrRAW_WRITE,&iob) < 0) {
	    IErr("RAW_WRITE",NULL);
	    return -1;
	 }
      }
   }

   data = pat;
   iob.UserArray = &data;
   for (i=str; i<=end; i+=skp) {
      addr = i + RAMAD;
      iob.Size = 1;
      iob.Offset = addr;
      if (ioctl(ctr,CtrDrvrRAW_READ,&iob) < 0) {
	 IErr("RAW_READ",NULL);
	 return -1;
      }
      if (flg == 1) {
	 if (data != pat + i) {
	    ecnt++;
	    printf("\nErr: Add:0x%X Idx:%d Wrt:0x%X Read:0x%X",
		   (int) addr, i, (int) (pat + i), (int) data);
	 }
      } else {
	 if (data != pat) {
	    ecnt++;
	    printf("\nErr: Add:0x%X Idx:%d Wrt:0x%X Read:0x%X",
		   (int) addr, i, (int) pat, (int) data);
	 }
      }
      if (ecnt >= MAX_ERRS) {
	 printf("\nTerminating test after:%d errors",ecnt);
	 return ecnt;
      }
   }

   if (ecnt) printf("\n");
   return ecnt;
}

/*****************************************************************/
/** This is a Memory test routine. First it checks the data bus, */
/** Then it togles every address line. Finally it makes a 	 */
/** full memory test						 */
/** Written by Pablo Alvarez					 */
/*****************************************************************/   

int MemoryTest(int arg) {

unsigned long stat;

   arg++;

   if (ioctl(ctr,CtrDrvrGET_STATUS,&stat) < 0) {
      printf("Error: Cant read status from module: %d\n",(int) module);
      IErr("GET_STATUS",NULL);
      return arg;
   }
   if (stat & CtrDrvrStatusENABLED) {
      printf("Sorry: Module event reception is enabled\n");
      printf("This is a destructive memory test\n");
      printf("First disable the module, and try again\n");
      printf("Memory test has been aborted\n");
      return arg;
   }

   printf("Memory[i]=0xFFFFFFFF Test: ");
   if (TestBlock(0,RAMSZ,1,0xFFFFFFFF,0)) printf("Failed\n");
   else                                   printf("OK\n");

   printf("Memory[i]=0x55555555 Test: ");
   if (TestBlock(0,RAMSZ,1,0x55555555,0)) printf("Failed\n");
   else                                   printf("OK\n");

   printf("Memory[i]=0xAAAAAAAA Test: ");
   if (TestBlock(0,RAMSZ,1,0xAAAAAAAA,0)) printf("Failed\n");
   else                                   printf("OK\n");

   printf("Memory[i]=0x55555555 SKIP Test: ");
   if (TestBlock(0,RAMSZ,2,0x55555555,0)) printf("Failed\n");
   else                                   printf("OK\n");

   printf("Memory[i]=0xAAAAAAAA SKIP Test: ");
   if (TestBlock(1,RAMSZ,2,0xAAAAAAAA,0)) printf("Failed\n");
   else                                   printf("OK\n");

   printf("Memory[i]=0x55555555 SKIP ReadBack Test: ");
   if (TestBlock(0,RAMSZ,2,0x55555555,2)) printf("Failed\n");
   else                                   printf("OK\n");

   printf("Memory[i]=0xAAAAAAAA SKIP ReadBack Test: ");
   if (TestBlock(1,RAMSZ,2,0xAAAAAAAA,2)) printf("Failed\n");
   else                                   printf("OK\n");

   printf("Memory[i]=i Test: ");
   if (TestBlock(0,RAMSZ,1,0,1))          printf("Failed\n");
   else                                   printf("OK\n");

   printf("Memory[i]=0 Test: ");
   if (TestBlock(0,RAMSZ,1,0,0))          printf("Failed\n");
   else                                   printf("OK\n");

   return arg;
}

/*****************************************************************/

int PlotPll(int arg) { /* Plot PLL curves */
ArgVal   *v;
AtomType  at;

unsigned long nav, kp, ki, points, phase, qflag, qsize, qsave, to, tn;
CtrDrvrConnection con;
CtrDrvrReadBuf rbf, ibf;
CtrDrvrPll *pll, plb;
FILE *err, *sum, *dac;
int i, cc;

   arg++;

   points = 1000;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) points = v->Number;
   }

   bzero((void *) &plb, sizeof(CtrDrvrPll));
   if (ioctl(ctr,CtrDrvrGET_PLL,&plb) < 0) {
      IErr("GET_PLL",NULL);
      return arg;
   }

   ki = 0;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      ki = v->Number;
      if (ki) plb.KI = ki;
   }

   kp = 0;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      kp = v->Number;
      if (kp) plb.KP = kp;
   }

   nav = 0;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      nav = v->Number;
      if (nav) plb.NumAverage = nav;
   }

   phase = 0;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      phase = v->Number;
      if (phase) plb.Phase = phase;
   }

   if (points < 2) {
      printf("PLOT: Nav(%d) Kp(%d) Ki(%d) Points(%d) :Invalid\n",(int) nav,(int) kp,(int) ki,(int) points);
      return arg;
   } else {
      printf("Plot Points: %d\n",(int) points);
      printf("KI         : [0x%08x] %d\n",(int) plb.KI        ,(int) plb.KI);
      printf("KP         : [0x%08x] %d\n",(int) plb.KP        ,(int) plb.KP);
      printf("NumAverage : [0x%08x] %d\n",(int) plb.NumAverage,(int) plb.NumAverage);
      printf("Phase      : [0x%08x] %d\n",(int) plb.Phase     ,(int) plb.Phase);
      if (YesNo("Set PLL to above and ","Plot") == 0) return arg;
   }

   pll = (CtrDrvrPll *) malloc(sizeof(CtrDrvrPll) * points);
   if (pll == NULL) {
      printf("PLOT: Not enough memory to plot %d points\n",(int) points);
      return arg;
   }

   con.Module   = module;
   con.EqpClass = CtrDrvrConnectionClassHARD;
   con.EqpNum   = CtrDrvrInterruptMaskPLL_ITERATION;
   if (ioctl(ctr,CtrDrvrCONNECT,&con) < 0) {
      IErr("CONNECT",NULL);
      free(pll);
      return arg;
   }

   if (ioctl(ctr,CtrDrvrSET_PLL,&plb) < 0) {
      IErr("SET_PLL",NULL);
      free(pll);
      return arg;
   }

   qsave = 0;
   if (ioctl(ctr,CtrDrvrGET_QUEUE_FLAG,&qsave) < 0) {
      IErr("GET_QUEUE_FLAG",(int *) &qsave);
      free(pll);
      return arg;
   }

   qflag = 0;
   if (ioctl(ctr,CtrDrvrSET_QUEUE_FLAG,&qflag) < 0) {
      IErr("SET_QUEUE_FLAG",(int *) &qflag);
      free(pll);
      return arg;
   }

   while (connected) {
      cc = read(ctr,&ibf,sizeof(CtrDrvrReadBuf));
      if (cc <= 0) break;
      ioctl(ctr,CtrDrvrGET_QUEUE_SIZE,&qsize);
      if ((qsize == 0)
      &&  ((CtrDrvrInterruptMaskPLL_ITERATION & ibf.Connection.EqpNum) == 0)) break;
   }

   qflag = 1;
   if (ioctl(ctr,CtrDrvrSET_QUEUE_FLAG,&qflag) < 0) {
      IErr("SET_QUEUE_FLAG",(int *) &qflag);
      free(pll);
      return arg;
   }

   i = 0;
   while (i < points) {
      cc = read(ctr,&rbf,sizeof(CtrDrvrReadBuf));
      if (cc <= 0) {
	 printf("Time out or Interrupted call\n");
	 free(pll);
	 return arg;
      }

      if ((CtrDrvrInterruptMaskPLL_ITERATION & rbf.Connection.EqpNum)
      &&  (rbf.Connection.EqpClass == CtrDrvrConnectionClassHARD)) {
	 if (ioctl(ctr,CtrDrvrGET_PLL,&pll[i++]) < 0) {
	    IErr("GET_PLL",NULL);
	    free(pll);
	    return arg;
	 }
      }
   }

   if (connected) {
      printf("PLOT: Trigger was:\n");
      if (ibf.Connection.EqpClass == CtrDrvrConnectionClassHARD) {
	 printf("Mod[%d] Int[0x%03x %d] Time[%s]",
	  (int) ibf.Connection.Module,
	  (int) ibf.Connection.EqpNum,
	  (int) ibf.InterruptNumber,
		TimeToStr(&ibf.OnZeroTime.Time,1));

	 for (i=0; i<CtrDrvrInterruptSOURCES; i++) {
	    if ((1<<i) & ibf.Connection.EqpNum) {
	       printf(" %s",int_names[i]);
	    }
	 }

      } else if (ibf.Connection.EqpClass == CtrDrvrConnectionClassPTIM) {
	 printf("Mod[%d] PTIM[%d] Cntr[%d] Time[%s]",
	  (int) ibf.Connection.Module,
	  (int) ibf.Connection.EqpNum,
	  (int) ibf.InterruptNumber,
		TimeToStr(&ibf.OnZeroTime.Time,1));
      } else {
	 printf("Mod[%d] CTIM[%d] Cntr[%d] Time[%s]",
	  (int) ibf.Connection.Module,
	  (int) ibf.Connection.EqpNum,
	  (int) ibf.InterruptNumber,
		TimeToStr(&ibf.StartTime.Time,1));
      }
      printf("\n");
   } else printf("PLOT: No triger used, there are NO connected interrupts\n");

   con.Module   = module;
   con.EqpClass = CtrDrvrConnectionClassHARD;
   con.EqpNum   = CtrDrvrInterruptMaskPLL_ITERATION;
   if (ioctl(ctr,CtrDrvrDISCONNECT,&con) < 0) {
      IErr("DISCONNECT",NULL);
      free(pll);
      return arg;
   }

   if (ioctl(ctr,CtrDrvrSET_QUEUE_FLAG,&qsave) < 0) {
      IErr("SET_QUEUE_FLAG",(int *) &qsave);
      free(pll);
      return arg;
   }

   printf("PLOT: Measurment done: Writing: %d points to output files\n",(int) points);

   err = fopen("/dsc/local/data/err","w");
   sum = fopen("/dsc/local/data/sum","w");
   dac = fopen("/dsc/local/data/dac","w");
   if (err && sum && dac) {
      to = 0;
      for (i=0; i<points; i++) {
	 tn = pll[i].LastItLen + to;
	 to = tn;
	 fprintf(err,"%d %d\n",(int) to, (int) pll[i].Error);
	 fprintf(sum,"%d %d\n",(int) to, (int) pll[i].Integrator);
	 fprintf(dac,"%d %d\n",(int) to, (int) pll[i].Dac);
      }
   }
   fclose(err);
   fclose(sum);
   fclose(dac);
   free(pll);

   printf("PLOT: Files written: Calling: gnuplot /usr/local/drivers/ctr/pllplot\n");
   system("cd /dsc/local/data; set term = xterm; gnuplot /usr/local/drivers/ctr/pllplot");
   return arg;
}

/*****************************************************************/

int AqLog(int arg) { /* Dump ptim aqns to log file for all monitored events */

#if PS_VER
ArgVal   *v;
AtomType  at;

FILE *lgf;
CtrDrvrConnection con;
char fname[64], *cp;
int i, cc, cnt, msd;
unsigned long sctim, ectim, cbl, usr, qsz;
TgmMachine mch;
TgmLineNameTable ltab;
CtrDrvrCTime stim;
CtrDrvrReadBuf rbf;
TgvName eqname;
char host[49];
CtrDrvrAction act;
#endif

   arg++;

#if PS_VER
   sctim = 386;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      sctim = v->Number;
   }

   ectim = sctim;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      sctim = v->Number;
   }

   con.Module   = 0;
   con.EqpNum   = 0;
   if (ioctl(ctr,CtrDrvrDISCONNECT,&con) < 0) {
      IErr("DISCONNECT",NULL);
      return arg;
   }
   connected = 0;

   if (ioctl(ctr,CtrDrvrGET_CABLE_ID,&cbl) < 0) {
      IErr("GET_CABLE_ID",NULL);
      return arg;
   }

   mch = TgvTgvToTgmMachine(TgvFirstMachineForCableId(cbl));

   if (TgmGetLineNameTable(mch,"USER",&ltab) != TgmSUCCESS) {
      return arg;
   }
   TgvGetNameForMember(sctim,&eqname);
   gethostname(host,48);

   bzero((void *) &stim, sizeof(CtrDrvrCTime));

   con.Module   = 0;
   con.EqpClass = CtrDrvrConnectionClassCTIM;
   con.EqpNum   = sctim;
   if (ioctl(ctr,CtrDrvrCONNECT,&con) < 0) {
      IErr("CONNECT",NULL);
      fclose(lgf);
      return arg;
   }

   if (ectim != sctim) {
      con.EqpClass = CtrDrvrConnectionClassCTIM;
      con.EqpNum   = ectim;
      if (ioctl(ctr,CtrDrvrCONNECT,&con) < 0) {
	 IErr("CONNECT",NULL);
	 return arg;
      }
   }

   GetFile("AqLog");
   strcpy(fname,path);
   lgf = fopen(fname,"w");
   if (lgf) {
      if (ptm_names_size == 0) GetPtmName(0,1);
      for (i=0; i<ptm_names_size; i++) {
	 if (ptm_names[i].Flg) {
	    con.EqpClass = CtrDrvrConnectionClassPTIM;
	    con.EqpNum   = ptm_names[i].Eqp;
	    if (ioctl(ctr,CtrDrvrCONNECT,&con) < 0) {
	       IErr("CONNECT",NULL);
	       fclose(lgf);
	       return arg;
	    }
	 }
      }

      printf("AqLog: Waiting for start...\n");
      while (1) {
	 if (ioctl(ctr,CtrDrvrGET_QUEUE_SIZE,&qsz) < 0) {
	    IErr("CONNECT",NULL);
	    fclose(lgf);
	    return arg;
	 }
	 if (qsz) cc = read(ctr,&rbf,sizeof(CtrDrvrReadBuf));
	 else break;
      }

      while (1) {
	 cc = read(ctr,&rbf,sizeof(CtrDrvrReadBuf));
	 if ((rbf.Connection.EqpNum == sctim)
	 &&  (rbf.Connection.EqpClass == CtrDrvrConnectionClassCTIM)) {
	    bcopy((void *) &rbf.TriggerTime,
		  (void *) &stim,
			   sizeof(CtrDrvrCTime));
	    printf("AqLog: Start:%s (%d) detected at %s\7\n",
		   (char *) eqname,
		   (int)    sctim,
			    TimeToStr(&stim.Time,1));

	    stim.Time.TicksHPTDC = (unsigned long)
				   ((double) (2.56 *
				   (unsigned long)
				   (((HptdcToNano(stim.Time.TicksHPTDC)/1000000) + 1)*1000000)));
	    break;
	 }
      }

      cnt = 0;
      while (1) {
	 cc = read(ctr,&rbf,sizeof(CtrDrvrReadBuf));

	 if ((rbf.Connection.EqpNum == ectim)
	 &&  (rbf.Connection.EqpClass == CtrDrvrConnectionClassCTIM)) break;

	 if (rbf.Connection.EqpClass == CtrDrvrConnectionClassPTIM) {

	    ioctl(ctr,CtrDrvrSET_MODULE,&rbf.Connection.Module);
	    act.TriggerNumber = rbf.TriggerNumber;
	    if (ioctl(ctr,CtrDrvrGET_ACTION,&act) < 0) continue;
	    if ((act.Config.OnZero & CtrDrvrCounterOnZeroOUT) == 0) continue;

	    usr = rbf.Frame.Struct.Value;

	    if ((usr >= 1) && (usr <= ltab.Size)) cp = ltab.Table[usr -1].Name;
	    else                                  cp = "Legacy";

	    TgvGetNameForFrame(rbf.Frame.Long,&eqname);

	    if (stim.Time.Second == 0) {
	       bcopy((void *) &rbf.OnZeroTime,
		     (void *) &stim,
			      sizeof(CtrDrvrCTime));
	    }

	    fprintf(lgf,"%2.6f\t%04d %12s %05d   %9s %15s %05d   %2.6f ;%s",
				 TimeDiff(&rbf.OnZeroTime.Time,&stim.Time),
			(int)    rbf.Connection.EqpNum,
				 GetPtmName(rbf.Connection.EqpNum,1),
			(int)    rbf.OnZeroTime.CTrain,
				 cp,
			(char *) eqname,
			(int)    rbf.StartTime.CTrain,
				 TimeDiff(&rbf.OnZeroTime.Time,&rbf.StartTime.Time),
				 ptm_comment_txt);

	    msd = 0;
	    ioctl(ctr,CtrDrvrGET_QUEUE_OVERFLOW,&msd);
	    if (msd) fprintf(lgf," Msd %d\n",msd);
	    else     fprintf(lgf,"\n");

	    cnt++;
	 }
      }

      con.Module   = 0;
      con.EqpNum   = 0;
      ioctl(ctr,CtrDrvrDISCONNECT,&con);

      fclose(lgf);
      printf("AqLog: Written:%d Acquisitions to:%s\n",cnt,fname);
   }

#else
   printf("ctrtest has not been compiled with the PS_VER option\n");
#endif

   return arg;
}

/*****************************************************************/

int RecpErrs(int arg) {

CtrDrvrReceptionErrors rers;
CtrDrvrTime t;

   arg++;
   if (ioctl(ctr,CtrDrvrGET_RECEPTION_ERRORS,&rers) < 0) {
      IErr("GET_RECEPTION_ERRORS",NULL);
      return arg;
   }

   t.Second = rers.LastReset; t.TicksHPTDC = 0;
   printf("LastReset   : %d %s\n",(int) rers.LastReset,TimeToStr(&t,0));
   printf("PartityErrs : %d\n"   ,(int) rers.PartityErrs);
   printf("SyncErrs    : %d\n"   ,(int) rers.SyncErrs);
   printf("CodeViolErrs: %d\n"   ,(int) rers.CodeViolErrs);
   printf("QueueErrs   : %d\n"   ,(int) rers.QueueErrs);
   printf("TotalErrs   : %d\n"   ,(int) rers.TotalErrs);

   return arg;
}

/*****************************************************************/

int IoStatus(int arg) {

unsigned long iostat;

   arg++;
   if (ioctl(ctr,CtrDrvrGET_IO_STATUS,&iostat) < 0) {
      IErr("GET_IO_STATUS",NULL);
      return arg;
   }
   printf("IoStatus: 0x%08x\n",(int) iostat);
   return arg;
}

/*****************************************************************/

int GetBoardId(int arg) {

CtrDrvrBoardId bird;

   arg++;
   if (ioctl(ctr,CtrDrvrGET_IDENTITY,&bird) < 0) {
      IErr("GET_IDENTITY",NULL);
      return arg;
   }
   printf("BoardIdentity: 0x%08x%08X\n",(int) bird.IdMSL,(int) bird.IdLSL);
   return arg;
}
