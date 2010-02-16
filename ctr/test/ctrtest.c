/**************************************************************************/
/* Ctr test                                                              */
/* Julian Lewis 15th Nov 2002                                             */
/**************************************************************************/

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
#include <ctype.h>

#ifdef CTR_PCI
#include <drm.h>
#endif

#include <ctrdrvr.h>
#include <ctrdrvrP.h>

#if PS_VER
#include <tgm/tgm.h>
#endif

/**************************************************************************/
/* Code section from here on                                              */
/**************************************************************************/

/* Print news on startup if not zero */

#define NEWS 1

#define HISTORIES 24
#define CMD_BUF_SIZE 128

static char history[HISTORIES][CMD_BUF_SIZE];
static char *cmdbuf = &(history[0][0]);
static int  cmdindx = 0;
static char prompt[32];
static char *pname = NULL;
static int  ctr;

#ifndef PS_VER
#define TgmMACHINES 8
#define TgmNAME_SIZE 9
#endif

static char MachineNames[TgmMACHINES][TgmNAME_SIZE];

#include "Cmds.h"
#include "GetAtoms.c"
#include "PrintAtoms.c"
#include "DoCmd.c"
#include "Cmds.c"
#include "CtrOpen.c"
#include "CtrCmds.c"

/**************************************************************************/
/* Prompt and do commands in a loop                                       */
/**************************************************************************/

int main(int argc,char *argv[]) {

char *cp, *ep;
char host[49];
char tmpb[CMD_BUF_SIZE];
CtrDrvrVersion version;

#if PS_VER
TgmMachine *mch;
TgmNetworkId nid;
#endif

int i, j;

#if NEWS
   printf("ctrtest: See <news> command\n");
   printf("ctrtest: Type h for help\n");
#endif

   pname = argv[0];
   printf("%s: Compiled %s %s\n",pname,__DATE__,__TIME__);

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
      printf("\nWARNING: Could not open driver");
      printf("\n\n");
   } else {
      printf("Driver opened OK: Using CTR module: 1\n\n");
   }

   version.HardwareType = CtrDrvrHardwareTypeNONE;

   if (ioctl(ctr,CtrDrvrGET_VERSION,&version) >= 0) {
      if (version.HardwareType == CtrDrvrHardwareTypeCTRP) printf("Hardware Type: CTRP PMC (3 Channel)\n");
      if (version.HardwareType == CtrDrvrHardwareTypeCTRI) printf("Hardware Type: CTRI PCI (4 Channel)\n");
      if (version.HardwareType == CtrDrvrHardwareTypeCTRV) printf("Hardware Type: CTRV VME (8 Channel)\n");

#ifdef CTR_VME
      if ((version.HardwareType == CtrDrvrHardwareTypeCTRP)
      ||  (version.HardwareType == CtrDrvrHardwareTypeCTRI)) {
	 close(ctr);
	 printf("ctrvtest is for the VME hardware type: Use ctrtest\n");
	 exit(1);
      }
#endif

#ifndef CTR_VME
      if (version.HardwareType == CtrDrvrHardwareTypeCTRV) {
	 close(ctr);
	 printf("ctrptest is for PCI and PMC hardware types: Use ctrvtest\n");
	 exit(1);
      }
#endif
   }

#if PS_VER
   for (i=0; i<TgmMACHINES; i++) bzero(MachineNames[i],TgmNAME_SIZE);
   nid = TgmGetDefaultNetworkId();
   mch = TgmGetAllMachinesInNetwork(nid);
   if (mch != NULL) {
      printf("On this network:");
      for (i=0; i<TgmMACHINES; i++) {
	 if (mch[i] == TgmMACHINE_NONE) break;
	 strcpy(MachineNames[TgvTgmToTgvMachine(mch[i])],TgmGetMachineName(mch[i]));
	 printf("%s ",TgmGetMachineName(mch[i]));
      }
      printf("\n");
   }
#endif

   bzero((void *) host,49);
   gethostname(host,48);

   while (True) {

      cmdbuf = &(history[cmdindx][0]);
      if (strlen(cmdbuf)) printf("{%s} ",cmdbuf);
      fflush(stdout);

      if (ctr) sprintf(prompt,"%s:Ctr[%02d]",host,cmdindx+1);
      else     sprintf(prompt,"%s:NoDriver:Ctr[%02d]",host,cmdindx+1);
      printf("%s",prompt);

      bzero((void *) tmpb,CMD_BUF_SIZE);
      if (fgets(tmpb,CMD_BUF_SIZE,stdin)==NULL) exit(1);

      cp = &(tmpb[0]);

      if (*cp == '!') {
	 ep = cp;
	 cmdindx = strtoul(++cp,&ep,0) -1;
	 if (cmdindx >= HISTORIES) cmdindx = 0;
	 cmdbuf = &(history[cmdindx][0]);
	 continue;
      } else if (*cp == '.') {
	 printf("Execute:%s\n",cmdbuf); fflush(stdout);
      } else if ((*cp == '\n') || (*cp == '\0')) {
	 cmdindx++;
	 if (cmdindx >= HISTORIES) { printf("\n"); cmdindx = 0; }
	 cmdbuf = &(history[cmdindx][0]);
	 continue;
      } else if (*cp == '?') {
	 printf("History:\n");
	 printf("\t!<1..24> Goto command\n");
	 printf("\tCR       Goto next command\n");
	 printf("\t.        Execute current command\n");
	 printf("\this      Show command history\n");
	 continue;
      } else {
	 cmdindx++; if (cmdindx >= HISTORIES) { printf("\n"); cmdindx = 0; }
	 strcpy(cmdbuf,tmpb);
      }
      bzero((void *) val_bufs,sizeof(val_bufs));
      GetAtoms(cmdbuf);
      DoCmd(0);
   }
}
