/* ========================================================= */
/* Xmem daemon                                               */
/* Julian Lewis 11th March 2005                              */
/* ========================================================= */

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
#include <time.h>

#include <libxmem.h>

/* ========================================================= */
/* Callback routins to handle events                         */

void callback(XmemCallbackStruct *cbs) {

unsigned long msk;
int i;

time_t tod;

   tod = time(NULL);    /* Time of interrupt */

   for (i=0; i<XmemEventMASKS; i++) {
      msk = 1 << i;
      if (cbs->Mask & msk) {
	 switch ((XmemEventMask) msk) {

	    case XmemEventMaskTIMEOUT:
	    break;

	    case XmemEventMaskSEND_TABLE:
	    break;

	    case XmemEventMaskUSER:
	    break;

	    case XmemEventMaskTABLE_UPDATE:
	    break;

	    case XmemEventMaskINITIALIZED:
	    break;

	    case XmemEventMaskIO:
	    break;

	    case XmemIoERRORS:
	    break;

	    case XmemEventMaskKILL:
	    break;

	    case XmemEventMaskSYSTEM:
	    break;

	    case XmemEventMaskSOFTWARE:
	    break;

	    default:
	 }
      }
   }
}

/* ========================================================= */
/* Xmem daemon, register callback, and wait.                 */

int main(int argc,char *argv[]) {

XmemEventMask emsk;
XmemError err;

   err = XmemInitialize(XmemDeviceANY);
   if (err == XmemErrorSUCCESS) {

      emsk = XmemEventMaskMASK;
      err = XmemRegisterCallback(callback,emsk);
      if (err == XmemErrorSUCCESS) {
	 printf("XmemDaemon: Running OK\n");
	 while(1) XmemWait(1000);
      }
   }
   printf("XmemDaemon: Fatal Error:%s\n",XmemErrorToString(err));
   exit((int) err);
}
