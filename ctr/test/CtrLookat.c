/**************************************************************************/
/* Ctr look at a selected member                                          */
/**************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>        /* Error numbers */
#include <sys/file.h>
#include <a.out.h>

#ifdef Status
#undef Status
#endif

#ifdef CTR_PCI
#include <drm.h>
#endif

#include <err/err.h>      /* Error handling */
#include <tgm/tgm.h>      /* Telegram definitions for application programs */
#include <ctrdrvr.h>
#include <tgv/tgv.h>

#include <errno.h>
extern int errno;

int GetIdByName(char *cp);
char *GetNameById(int id);

static int machine = 3;
static int ctr=0;
static int eprint=1;

static char *TriggerConditionNames[CtrDrvrTriggerCONDITIONS] = {"*", "=", "&" };
static char *MachineNames         [CtrDrvrMachineMACHINES]   = {"000", "LHC", "SPS", "CPS", "PSB", "LEI", "ADE" };
static char *CounterStart         [CtrDrvrCounterSTARTS]     = {"Nor", "Ext1", "Ext2", "Chnd", "Self", "Remt" };
static char *CounterMode          [CtrDrvrCounterMODES]      = {"Once", "Mult", "Brst" };
static char *CounterClock         [CtrDrvrCounterCLOCKS]     = {"1KHz", "10MHz", "40MHz", "Ext1", "Ext2", "Chnd" };
static char *CounterOnZero        [4]                        = {"NoOut", "Bus", "Out", "BusOut" };

/**************************************************************************/
/* Code section from here on                                              */
/**************************************************************************/

#include "CtrOpen.c"
#include "DisplayLine.c"

/*****************************************************************/
/* Error handler, throw them away                                */
/*****************************************************************/

static Boolean error_handler(class,text)
ErrClass class;
char *text; {  return(False); }

/*****************************************************************/
/* Turn on error printing. If the value onoff is non zero then   */
/* error printing from the library is turned On. A value of zero */
/* turns error printing off. Note errno is always set up to have */
/* an explanation that can pe printed from standard Unix perror. */

int ErrorPrinting(unsigned int onoff) {

   if (onoff) eprint = 1;
   else       eprint = 0;
   return 1;
}

/*****************************************************************/
/* IOCTL Error printer                                           */

static void IErr(char *name, int *value) {

   if (eprint) {
      if (value != NULL)
	 fprintf(stderr,"CtrLib: [%02d] ioctl=%s arg=%d :Error\n",
		(int) ctr,name,*value);
      else
	 fprintf(stderr,"CtrLib: [%02d] ioctl=%s :Error\n",
		(int) ctr,name);
      perror("CtrLookat");
   }
}

/**************************************************************************/
/* Convert a CtrDrvr time in milliseconds to a string routine.            */
/* Result is a pointer to a static string representing the time.          */
/*    the format is: Thu-18/Jan/2001 08:25:14.967                         */
/*                   day-dd/mon/yyyy hh:mm:ss.ddd                         */

static char *TimeToStr (CtrDrvrTime *t, int hpflag) {

static char buf[32];

char tmp[32];
char *yr, *ti, *md, *mn, *dy;
int ms;
double fms;

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
	   fms  = ((double) t->TicksHPTDC) / 2560000.0;   /* I multiply by 10^3 for 3 sig figs */
	   ms = fms;
	   sprintf (buf, "%s-%s/%s/%s %s.%03d", dy, md, mn, yr, ti, ms); /* 3 figs */
	} else
	   sprintf (buf, "%s-%s/%s/%s %s"  , dy, md, mn, yr, ti);

    }
    else {
        sprintf (buf, "--- Zero ---");
    }
    return (buf);
}

/*****************************************************************/
/* Get the cable ID of the receiver card                         */

int GetCableId() {

int cbid = -1;

   if (ioctl(ctr,CtrDrvrGET_CABLE_ID,&cbid) < 0) {
      IErr("GET_CABLE_ID",NULL);
      return 0;
   }
   return cbid;
}

/*****************************************************************/
/* Connect to an event by object ID, the current module is used. */

int Connect(CtrDrvrConnection *con) {

   if (ioctl(ctr,CtrDrvrCONNECT,con) < 0) {
      IErr("CONNECT",(int *) &(con->EqpNum));
      return 0;
   }
   return 1;
}

/*****************************************************************/

int Wait(CtrDrvrReadBuf *rbf) {

int cc;

   cc = read(ctr,rbf,sizeof(CtrDrvrReadBuf));
   if (cc <= 0) {
      if (eprint) {
	 fprintf(stderr,"CtrLookat: In Wait: Read: Error\n");
	 perror("CtrLookat");
      }
      return 0;
   }
   return 1;
}

/*****************************************************************/
/* Get the current User                                          */

char *GetUser(int *user) {

CtrDrvrTgmBuf tgm;
int u;

   tgm.Machine = machine;
   if (ioctl(ctr,CtrDrvrREAD_TELEGRAM,&tgm) < 0)
      IErr("READ_TELEGRAM",&machine);
   u = tgm.Telegram[0]; *user = u;
   return (char *) TgmGetLineName(TgvTgvToTgmMachine(machine),"USER",u);
}

/*****************************************************************/

#define ACT_STR_LEN 256

static CtrDrvrAction act;
static char act_str[ACT_STR_LEN];

char *ActionToStr(int anum) {

CtrDrvrTrigger              *trg;
CtrDrvrCounterConfiguration *cnf;
CtrDrvrTgmGroup             *grp;
char *cp = act_str;

   bzero((void *) &act, sizeof(CtrDrvrAction));
   bzero((void *) &act_str,ACT_STR_LEN);

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
      sprintf(cp,"[%s %2d%s0x%X]",
	     MachineNames[trg->Machine],
	     (int) grp->GroupNumber,
	     TriggerConditionNames[trg->TriggerCondition],
	     (int) grp->GroupValue);
      cp = &act_str[strlen(act_str)];
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

/**************************************************************************/
/* Prompt and do commands in a loop                                       */
/**************************************************************************/

int main(int argc,char *argv[]) {

char *cp, *ep, buf[256], comment[256], title[256];
int i, j, module, id, u, li, size, start, first_time;

CtrDrvrPtimBinding ob;
CtrDrvrConnectionClass ccls;
CtrDrvrConnection con;
CtrDrvrReadBuf rbf;

   printf("CtrLookat: Compiled %s %s\n",__DATE__,__TIME__);

   if (argc < 4) {
      printf("CtrLookat: Args: CtrLookat <module> P/C <member>\n");
      exit(0);
   }

   cp = argv[1];
   module = strtol(cp,&ep,0);
   if ((module == 0) || (cp == ep)) {
      printf("CtrLookat: No module number\n");
      exit(0);
   }

   if      ((argv[2][0] == 'P') || (argv[2][0] == 'p')) ccls = CtrDrvrConnectionClassPTIM;
   else if ((argv[2][0] == 'C') || (argv[2][0] == 'c')) ccls = CtrDrvrConnectionClassCTIM;
   else {
      printf("CtrLookat: Illegal timing class: %c Use only P/C\n",argv[2][0]);
      exit(0);
   }

   cp = argv[3];
   id = strtol(cp,&ep,0);
   if ((id == 0) || (cp == ep)) {
      printf("CtrLookat: No member number\n");
      exit(0);
   }

   if (ccls == CtrDrvrConnectionClassPTIM) sprintf(title,"CtrLookat: Mod(%d) PTIM(%d)",module,id);
   else                                    sprintf(title,"CtrLookat: Mod(%d) CTIM(%d)",module,id);
   printf("%s\n",title);

   ErrSetHandler((ErrHandler) error_handler);

   for (j=1; j<argc; j++) {
      for (i=0; i<CtrDrvrDEVICES; i++) {
	 if (strcmp(argv[j],devnames[i]) == 0) {
	    ctr_device = (CtrDrvrDevice) i;
	    printf("Forcing CTR device type:%s\n",devnames[i]);
	    break;
	 }
      }
   }

   ctr = CtrOpen();
   if (ctr == 0) {
      printf("CtrLookat: Could not open ctr driver.\n");
      exit(1);
   }

   ErrorPrinting(1);
   machine = GetCableId();
   if ((machine <= 0) || (machine >= CtrDrvrMachineMACHINES)) machine = 3;

   con.Module = module;
   con.EqpClass = ccls;
   con.EqpNum = id;

   if (!Connect(&con)) {
      printf("%s: Can't connect\n",title);
      exit(1);
   }

   if (ccls == CtrDrvrConnectionClassPTIM) {
      ob.EqpNum = id;
      ob.ModuleIndex = module-1;
      ob.StartIndex = 0;
      ob.Counter = 0;
      ob.Size = 0;
      if (ioctl(ctr,CtrDrvrGET_PTIM_BINDING,&ob) < 0) {
	 printf("Error: Cant get Binding for PTIM object: %d\n",(int) ob.EqpNum);
	 IErr("GET_PTIM_BINDING",NULL);
	 exit(1);
      }
      size  = ob.Size;
      start = ob.StartIndex;
   } else {
      size = 8;
   }

   first_time = 1;

   for (i=0; i<1000; i++) {
      if (!Wait(&rbf)) continue;

      cp = GetUser(&u);

      bzero((void *) buf, 256);

      if (ccls == CtrDrvrConnectionClassPTIM) {
	 li = rbf.TriggerNumber - start -1;
	 sprintf(buf,"%s %s %s C%04d",
		 cp,
		 ActionToStr(rbf.TriggerNumber),
		 TimeToStr(&(rbf.OnZeroTime.Time),1),
		 (int) rbf.OnZeroTime.CTrain);
      } else {
	 li++;
	 bzero((void *) comment,256);
	 TgvFrameExplanation(rbf.Frame.Long,comment);
	 sprintf(buf,"Ctm:%u Frm:%02x:%02x:%04x %s %s %s C%04d",
		 (unsigned int) rbf.Ctim,
		 (unsigned int) rbf.Frame.Struct.Header,
		 (unsigned int) rbf.Frame.Struct.Code,
		 (unsigned int) rbf.Frame.Struct.Value,
		 cp,
		 comment,
		 TimeToStr(&(rbf.OnZeroTime.Time),1),
		 (int) rbf.OnZeroTime.CTrain);
      }
      free(cp);

      if (first_time) {
	 first_time = 0;
	 DisplayInit(title,size,strlen(buf)+14);
      }

      DisplayLine(buf,(li % size));
   }
   exit(0);
}
