/**************************************************************************/
/* Open the Mtt driver                                                    */
/**************************************************************************/

#ifdef MTT_PCI
#include <drm.h>
#endif

#include <mttdrvr.h>

/* ======================== */

int MttOpen() {

char fnm[32];
int  i, fn;

   for (i = 1; i <= SkelDrvrCLIENT_CONTEXTS; i++) {
      sprintf(fnm,"/dev/%s.%1d","mtt",i);
      if ((fn = open(fnm,O_RDWR,0)) > 0) return(fn);
   };
   return(0);
}
