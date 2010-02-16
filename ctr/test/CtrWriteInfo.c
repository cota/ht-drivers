/* ************************************************************************** */
/*                                                                            */
/* Save the driver context in an ASCII file Ctr.info in a format that can be  */
/* parsed by the CtrReadInfo program. This may be used to initialize the CTR  */
/* driver at start up time.                                                   */
/*                                                                            */
/* Julian Lewis 1st/March/2004                                                */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <a.out.h>
#include <ctype.h>

#ifdef PS_VER
#include <tgv/tgv.h>
#include <tgm/tgm.h>
#endif

#ifdef CTR_PCI
#include <drm.h>
#endif

#include <ctrhard.h>
#include <ctrdrvr.h>
#include <ctrdrvrP.h>

#ifdef PS_VER
#define INFO "/dsc/local/data/Ctr.info"
#else
#define INFO "/usr/local/ctr/Ctr.info"
#endif

#include "CtrOpen.c"
static int ctr;

static CtrDrvrPtimObjects             ptimo;
static CtrDrvrPtimBinding            *ptim;
static CtrDrvrAction                  actn;
static CtrDrvrTrigger                *trig;
static CtrDrvrCounterConfiguration   *conf;
static CtrDrvrConnectionClass         class;
static CtrDrvrCounterConfiguration   *cnf;
static CtrDrvrCounterConfigurationBuf cnfb;
static CtrdrvrRemoteCommandBuf        crmb;
static CtrDrvrCounterMaskBuf          cmsb;

static char *pname                                     =  "CtrWriteInfo";
static char *condition_names[CtrDrvrTriggerCONDITIONS] = {"NO_CHECK","EQUALS","AND"};
static char *enable_names[2]                           = {"DISABLE","ENABLE"};
static char *start_names[CtrDrvrCounterSTARTS]         = {"NORMAL","EXT1","EXT2","CHAINED","SELF","REMOTE","PPS","CHSTOP"};
static char *mode_names[CtrDrvrCounterMODES]           = {"NORMAL","MULTIPLE","BURST"};
static char *clock_names[CtrDrvrCounterCLOCKS]         = {"KHZ1","MHZ10","MHZ40","EXT1","EXT2","CHAINED"};
static char *onzero_names[4]                           = {"NO_OUT", "BUS", "OUT", "BUS_OUT" };
static char *polarity_names[2]                         = {"TTL_POS", "TTL_BAR" };

#ifdef PS_VER
static char machine_names[TgmMACHINES][TgmNAME_SIZE];
#endif

int CheckRange(int low, int high, int x, char *name) {

   if ((x >= low) && (x < high)) return 1;
   fprintf(stderr,"%s: %d<=%s[%d]<%d Assert violation: Out of range\n",
	   pname, low, name, x, high);
   return 0;
}

/**************************************************************************/
/* Read Ptim equip list, get corresponding actions, write them to file.   */
/**************************************************************************/

int main(int argc,char *argv[]) {
#define FN_SIZE 80
char ass[FN_SIZE];
FILE *assFile = NULL;

#ifdef PS_VER
TgmLineNameTable ltab;
TgmGroupDescriptor desc;
TgmMachine *mch;
TgmNetworkId nid;
#endif

int i, j, tm, rows, en, modnum, rcnt, cnt, ch;
char *p = "%";

#ifdef CTR_VME
unsigned int oby = 0;
#endif

#ifdef PS_VER
   printf("%s: Compiled %s %s\n",pname,__DATE__,__TIME__);
   nid = TgmGetDefaultNetworkId();
   mch = TgmGetAllMachinesInNetwork(nid);
   if (mch != NULL) {
      for (i=0; i<TgmMACHINES; i++) {
	 if (mch[i] == TgmMACHINE_NONE) break;
	 strcpy(machine_names[TgvTgmToTgvMachine(mch[i])],TgmGetMachineName(mch[i]));
      }
   }
#endif

   if (argc < 2) strcpy(ass,INFO);
   else          strcpy(ass,argv[1]);

   assFile = fopen(ass,"w");
   if (assFile == NULL) {
      fprintf(stderr,"%s: Can't open: %s for write\n",pname,ass);
      perror(pname);
      exit(1);
   }

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
      fprintf(stderr,"\nFATAL: Could not open CTR driver");
      exit(1);
   }
   if (ioctl(ctr,CtrDrvrLIST_PTIM_OBJECTS,&ptimo) < 0) {
      fprintf(stderr,"%s: Cant get PTIM definitions from driver\n",pname);
      perror(pname);
      exit(1);
   }

   rows = 0;
   for (i=0; i<ptimo.Size; i++) {
      ptim = &(ptimo.Objects[i]);
      fprintf(assFile,"{\n   {PTIM %u,%u,%u}\n",
	      (int) ptim->EqpNum,
	      (int) ptim->ModuleIndex+1,
	      (int) ptim->Counter);
      modnum = ptim->ModuleIndex+1;
      ioctl(ctr,CtrDrvrSET_MODULE,&modnum);
      for (j=ptim->StartIndex; j<ptim->StartIndex + ptim->Size; j++) {
	 bzero((void *) &actn, sizeof(CtrDrvrAction));
	 actn.TriggerNumber = j+1;
	 if (ioctl(ctr,CtrDrvrGET_ACTION,&actn) < 0) {
	    fprintf(stderr,"%s: Cant read action from driver\n",pname);
	    perror(pname);
	    exit(1);
	 }
	 rows ++;
	 trig = &(actn.Trigger);
	 conf = &(actn.Config);
	 class = actn.EqpClass;
	 if ((actn.EqpClass != CtrDrvrConnectionClassPTIM)
	 ||  (actn.EqpNum   != ptim->EqpNum)) {
	    fprintf(stderr,"%s: Internal assert violataion: Bug: FATAL\n",pname);
	    exit(2);
	 }
	 fprintf(assFile,"   {CTIM %u 0x%04X}",
		 (int) trig->Ctim,
		 (int) trig->Frame.Struct.Value);

#ifdef PS_VER
	 if (trig->TriggerCondition != CtrDrvrTriggerConditionNO_CHECK) {
	    tm = TgvTgvToTgmMachine(trig->Machine);
	    if ((TgmGetGroupDescriptor(tm,trig->Group.GroupNumber,&desc))
	    &&  (TgmGetLineNameTable(tm,desc.Name,&ltab))
	    &&  (CheckRange(0,CtrDrvrMachineMACHINES  ,trig->Machine,"MACHINE"))
	    &&  (CheckRange(0,CtrDrvrTriggerCONDITIONS,trig->TriggerCondition,"CONDITION"))) {

	       if (desc.Type == TgmEXCLUSIVE) {
		  fprintf(assFile,"   {TRIG %s,%s,%s,%s}",
			  machine_names[trig->Machine],
			  condition_names[trig->TriggerCondition],
			  desc.Name,
			  ltab.Table[trig->Group.GroupValue-1].Name);
	       } else if (desc.Type == TgmBIT_PATTERN) {
		  fprintf(assFile,"   {TRIG %s,%s,%s,0x%04X}",
			  machine_names[trig->Machine],
			  condition_names[trig->TriggerCondition],
			  desc.Name,
			  trig->Group.GroupValue);
	       } else {
		  fprintf(assFile,"   {TRIG %s,%s,%s,%u}",
			  machine_names[trig->Machine],
			  condition_names[trig->TriggerCondition],
			  desc.Name,
			  trig->Group.GroupValue);
	       }
	    }
	 }
#endif

	 if ((CheckRange(0,CtrDrvrCounterSTARTS,conf->Start,"START"))
	 &&  (CheckRange(0,CtrDrvrCounterMODES ,conf->Mode ,"MODE"))
	 &&  (CheckRange(0,CtrDrvrCounterCLOCKS,conf->Clock,"CLOCK"))) {

	    en = 0;
	    if (conf->OnZero & CtrDrvrCounterOnZeroOUT) en = 1;

	    fprintf(assFile,"   {CONF %s,%s,%s,%s,%u,%u}\n",
		   enable_names[en],
		   start_names[conf->Start],
		   mode_names[conf->Mode],
		   clock_names[conf->Clock],
		   (int) conf->Delay,
		   (int) conf->PulsWidth);
	 } else fprintf(assFile,"\n%s Error bad action data\n",p);
      }
      fprintf(assFile,"}\n\n");
   }

   rcnt = 0;
   cnf = &cnfb.Config;
   if (ioctl(ctr,CtrDrvrGET_MODULE_COUNT,&cnt) >=0 ) {
      for (modnum=1; modnum<=cnt; modnum++) {
	 ioctl(ctr,CtrDrvrSET_MODULE,&modnum);
	 fprintf(assFile,"{\n");
	 for (ch=CtrDrvrCounter1; ch<=CtrDrvrCounter8; ch++) {
	    cmsb.Counter = ch;
	    if (ioctl(ctr,CtrDrvrGET_OUT_MASK,&cmsb) >=0) {
	       fprintf(assFile,"   {OUTMSK %d %d,0x%08X,%s}\n",
		       modnum,
		       ch,
		       (int) cmsb.Mask,
		       polarity_names[cmsb.Polarity]);
	    }
	 }
	 fprintf(assFile,"}\n\n");

#ifdef CTR_VME
	 ioctl(ctr,CtrDrvrGET_OUTPUT_BYTE,&oby);
	 if (oby) {
	    fprintf(assFile,"{\n");
	    fprintf(assFile,"{OUTBYTE %d %d}\n",modnum,(int) oby);
	    fprintf(assFile,"}\n\n");
	 }
#endif

	 fprintf(assFile,"{\n");
	 for (ch=CtrDrvrCounter1; ch<=CtrDrvrCounter8; ch++) {
	    crmb.Counter = ch;
	    crmb.Remote  = 0;
	    if (ioctl(ctr,CtrDrvrGET_REMOTE,&crmb) >= 0 ) {
	       if (crmb.Remote) {
		  cnfb.Counter = ch;
		  if (ioctl(ctr,CtrDrvrGET_CONFIG,&cnfb) >= 0) {
		     fprintf(assFile,"   {REMOTE %d %d,%s,%s,%s,%d,%d,%s}\n",
			     modnum,
			     ch,
			     start_names[cnf->Start],
			     mode_names[cnf->Mode],
			     clock_names[cnf->Clock],
			     (int) cnf->Delay,
			     (int) cnf->PulsWidth,
			     onzero_names[cnf->OnZero]);
		     rcnt++;
		  }
	       }
	    }
	 }
	 fprintf(assFile,"}\n\n");
      }
   }

   fprintf(assFile,"\n%s End Of File\n",p);
   printf("%s: %u PTIM objects defined using %u actions\n",pname,i,rows);
   if (rcnt) printf("%s: %d Remote counter configurations\n",pname,rcnt);
   exit(0);
}
