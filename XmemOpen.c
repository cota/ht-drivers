/**************************************************************************/
/* Open the Ctr driver                                                    */
/**************************************************************************/

#include "../driver/xmemDrvr.h"

int XmemOpen() {

char fnm[32];
int  i, fn;

   for (i = 1; i <= XmemDrvrCLIENT_CONTEXTS; i++) {
      sprintf(fnm,"/dev/xmem.%1d",i);
      if ((fn = open(fnm,O_RDWR,0)) > 0) {
	 return(fn);
      }
   }
   return 0;
}
