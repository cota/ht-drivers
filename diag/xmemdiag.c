/**************************************************************************/
/* Xmem Library test and diagnostics program.                             */
/* Julian Lewis 4th March 2005                                            */
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

#include <libxmem.h>

extern char *defaultconfigpath;
extern char *configpath;

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

#include "Cmds.h"
#include "GetAtoms.c"
#include "PrintAtoms.c"
#include "DoCmd.c"
#include "Cmds.c"
#include "XmemCmds.c"

/**************************************************************************/
/* Prompt and do commands in a loop                                       */
/**************************************************************************/

int main(int argc,char *argv[]) {

int rp, pr;
char *cp;
char host[49], *nnm;
char tmpb[CMD_BUF_SIZE];
XmemError err;
XmemEventMask emsk;
XmemNodeId nid;

   pname = argv[0];
   printf("%s: Compiled %s %s Conf:%s\n",pname,__DATE__,__TIME__,defaultconfigpath);

   err = XmemInitialize(XmemDeviceANY);
   if (err != XmemErrorSUCCESS) {
      printf("\nFatal Error: XmemLib: Initialize: %s",XmemErrorToString(err));
      printf("\n\n");
      exit((int) err);
   } else {
      printf("XmemLib: Initialized: OK\n\n");
   }
   emsk = XmemEventMaskMASK;
   err = XmemRegisterCallback(callback,emsk);

   bzero((void *) host,49);
   gethostname(host,48);

   nid = XmemWhoAmI();
   nnm = XmemGetNodeName(nid);

   while (True) {

      if (strcmp(nnm,host) !=0)
	 sprintf(prompt,"Nd:0x%02X:%s %s:XmemDiag[%02d]",(int) nid, nnm, host, cmdindx+1);
      else
	 sprintf(prompt,"Nd:0x%02X:%s:XmemDiag[%02d]",(int) nid, nnm, cmdindx+1);
      printf("%s",prompt);

      cmdbuf = &(history[cmdindx][0]);

      bzero((void *) tmpb,CMD_BUF_SIZE);
      if (gets(tmpb)==NULL) exit(1);

      cp = &(tmpb[0]);
      pr = 0;           /* Dont print a history */
      rp = 0;           /* Dont repeat a command */

      while ((*cp == '-')
      ||     (*cp == '+')
      ||     (*cp == '.')
      ||     (*cp == '!')) {

	 pr = 1;        /* Print command on */

	 if (*cp == '!') {
	    cmdindx = strtoul(++cp,&cp,0) -1;
	    if (cmdindx >= HISTORIES) cmdindx = 0;
	    if (cmdindx < 0) cmdindx = HISTORIES -1;
	    rp = 1;
	    break;
	 }

	 if (*cp == '-') {
	    if (--cmdindx < 0) cmdindx = HISTORIES -1;
	    cmdbuf = &(history[cmdindx][0]);
	 }

	 if (*cp == '+') {
	    if (++cmdindx >= HISTORIES) cmdindx = 0;
	    cmdbuf = &(history[cmdindx][0]);
	 }

	 if (*cp == '.') {
	    rp = 1;
	    break;
	 };
	 cp++;
      }
      if (pr) {
	 printf("{%s}\t ",cmdbuf); fflush(stdout);
	 if (!rp) continue;
      }
      if (!rp) strcpy(cmdbuf,tmpb);

      bzero((void *) val_bufs,sizeof(val_bufs));
      GetAtoms(cmdbuf);
      DoCmd(0);

      if ((!rp) && (++cmdindx >= HISTORIES)) cmdindx = 0;
   }
}
