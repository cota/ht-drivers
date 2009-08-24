/* ========================================================================== */
/* A simple telegram display X-Window application that needs no configuration */
/* Julian Lewis 22nd August 2007                                              */
/* ========================================================================== */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <err/err.h>
#include <tgm/tgm.h>

#ifdef Status
#undef Status
#endif

#include <mtthard.h>
#include <mttdrvr.h>
#include <libmtt.h>

extern int errno;

static TgmMachine         tgmMch             = TgmMACHINE_NONE;
static int                tgmGroupNamesValid = 0;
static int                tgmLineNamesValid  = 0;
static char               tgmGroupNames[TgmGROUPS_IN_TELEGRAM][TgmNAME_SIZE];
static TgmGroupDescriptor tgmGroupDescs[TgmGROUPS_IN_TELEGRAM];
static TgmLineNameTable   tgmLineNames[TgmGROUPS_IN_TELEGRAM];

#include "DisplayLine.c"

/* ======================================== */
/* Error handler to suppress error messages */
/* ======================================== */

static int error_handler (ErrClass class, char *text) {

   if (class == ErrFATAL) return 1;
   return 0;
}

/* =========================== */
/* Set up telegram group names */

int SetupGroupNames(char *name) {

int i, n;
TgmMachine  *mch;
TgmNetworkId nid;

   if (tgmGroupNamesValid) return tgmGroupNamesValid;

   nid = TgmGetDefaultNetworkId();
   mch = TgmGetAllMachinesInNetwork(nid);

   if (mch != NULL) {
      printf("On this network:");
      n = 0;
      for (i=0; i<TgmMACHINES; i++) {
	 if (mch[i] == TgmMACHINE_NONE) break;
	 n++;
	 printf("%d/%s ",mch[i],TgmGetMachineName(mch[i]));
      }
      if (n == 1) tgmMch = mch[0];
      else        tgmMch = TgmGetMachineId(name);

      if (tgmMch == TgmMACHINE_NONE) {
	 printf("Can't find Tgm machine:%s\n",name);
	 exit(1);
      }

      printf("[Using:%s]\n",TgmGetMachineName(tgmMch));

      if (TgmAttach(tgmMch,TgmTELEGRAM | TgmLINE_NAMES) != TgmSUCCESS) {
	 printf("Can't attach to telegram:%s\n",TgmGetMachineName(tgmMch));
	 tgmGroupNamesValid = 0;
	 return 0;
      }

      for (i=0; i<TgmLastGroupNumber(tgmMch); i++) {
	 if (TgmGetGroupDescriptor(tgmMch,i+1,&tgmGroupDescs[i]) == TgmSUCCESS) {
	    strcpy(tgmGroupNames[i],tgmGroupDescs[i].Name);
	    tgmGroupNamesValid++;
	    continue;
	 }
	 printf("Can't get descriptior for group:%d\n",i+1);
	 tgmGroupNamesValid = 0;
	 return 0;
      }

      for (i=0; i<TgmLastGroupNumber(tgmMch); i++) {
	 if (tgmGroupNames[i][0] == 'N') {
	    tgmGroupNames[i][0] = 0;
	    tgmGroupNamesValid--;
	    continue;
	 }
	 if (tgmGroupDescs[i].Type == TgmNUMERIC_VALUE) continue;

	 if (TgmGetLineNameTable(tgmMch,tgmGroupNames[i],&(tgmLineNames[i])) == TgmSUCCESS) {
	    tgmLineNamesValid++;
	    continue;
	 }
	 printf("Can't get line names for group:%d %s\n",i+1,tgmGroupNames[i]);
      }
   }
   return tgmGroupNamesValid;
}

/* ============================= */
/* Pack display line with spaces */

#define LN 80
char displayLine[LN];

void PackDisplayLine() {

int i, n;

   n = strlen(displayLine);
   for (i=n; i<LN; i++) {
      strcat(displayLine," ");
   }
   return;
}

/* ================================ */
/* Build a display line for a group */

char *BuildDisplayLine(int ig, unsigned long gv) {

int i;
unsigned int msk;

   bzero((void *) displayLine, LN);

   if ((gv < tgmGroupDescs[ig].Minimum)
   ||  (gv > tgmGroupDescs[ig].Maximum))
      gv = tgmGroupDescs[ig].Default;

   if (tgmGroupDescs[ig].Type == TgmNUMERIC_VALUE) {
      sprintf(displayLine,
	      "%8s 0x%04X [%d]",
	      tgmGroupNames[ig],
	      (int) gv,
	      (int) gv);
      PackDisplayLine();
      return displayLine;
   }

   if (tgmGroupDescs[ig].Type == TgmEXCLUSIVE) {
      sprintf(displayLine,
	      "%8s 0x%04X %s",
	      tgmGroupNames[ig],
	      (int) gv,
	      tgmLineNames[ig].Table[gv-1].Name);
      PackDisplayLine();
      return displayLine;
   }

   if (tgmGroupDescs[ig].Type == TgmBIT_PATTERN) {
      sprintf(displayLine,
	      "%8s 0x%04X ",
	      tgmGroupNames[ig],
	      (int) gv);
      if (gv) {
	 for (i=0; i<tgmLineNames[ig].Size; i++) {
	    msk = 1 << i;
	    if (msk & gv) {
	       strcat(displayLine,tgmLineNames[ig].Table[i].Name);
	       strcat(displayLine," ");
	    }
	 }
      } else strcat(displayLine,"---");
   }

   PackDisplayLine();
   return displayLine;
}

/* =============================== */
/* Read telegrams and display them */
/* =============================== */

int main (int argc, char *argv[]) {

int i, row;
unsigned int gn, gv;
char titl[LN];

   ErrSetHandler(error_handler);

   if (SetupGroupNames(argv[1])) {

      sprintf(titl,"Generic video for:%s",TgmGetMachineName(tgmMch));
      DisplayInit(titl,tgmGroupNamesValid,LN);

      while (1) {
	 TgmWaitForNextTelegram(tgmMch);
	 row = 0;
	 for (i=0; i<TgmLastGroupNumber(tgmMch); i++) {
	    if (strlen(tgmGroupNames[i])) {
	       row++; gn = i+1;
	       TgmGetGroupValue(tgmMch,TgmCURRENT,0,gn,&gv);
	       BuildDisplayLine(i,gv);
	       DisplayLine(displayLine,row,0);
	    }
	 }
      }
   }
   exit(1);
}
