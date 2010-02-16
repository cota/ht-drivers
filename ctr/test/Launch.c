/**************************************************************************/
/* Launch the correct ctrtest program according to hardware type.         */
/* Julian Lewis 7th July 2005                                             */
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

#include <ctrdrvr.h>

/**************************************************************************/
/* Code section from here on                                              */
/**************************************************************************/

static int  ctr;
#include "CtrOpen.c"

/**************************************************************************/
/* Prompt and do commands in a loop                                       */
/**************************************************************************/

int main(int argc,char *argv[]) {

CtrDrvrVersion version;

   ctr = CtrOpen();
   if (ctr == 0) {
      printf("\nctrtest: WARNING: Could not open CTR driver: Not installed\n\n");
      exit(1);
   }

   version.HardwareType = CtrDrvrHardwareTypeNONE;

   if (ioctl(ctr,CtrDrvrGET_VERSION,&version) >= 0) {

      if ((version.HardwareType == CtrDrvrHardwareTypeCTRP)
      ||  (version.HardwareType == CtrDrvrHardwareTypeCTRI)) {
	 close(ctr);
	 system("/usr/local/ctr/ctrptest");
	 exit(0);
      }

      if (version.HardwareType == CtrDrvrHardwareTypeCTRV) {
	 close(ctr);
	 system("/usr/local/ctr/ctrvtest");
	 exit(0);
      }
   }

   perror("ctrtest");
   fprintf(stderr,"ctrtest: FATAL: Illegal hardware type: %d\n", (int) version.HardwareType);
   exit(1);
}
