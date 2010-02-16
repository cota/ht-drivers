/**************************************************************************/
/* Open the Ctr driver                                                    */
/**************************************************************************/

#ifdef CTR_PCI
#include <drm.h>
#endif

#include <ctrdrvr.h>

static CtrDrvrDevice ctr_device = CtrDrvrDeviceANY;
static char *devnames[CtrDrvrDEVICES] = {"ctr", "ctri", "ctrp", "ctrv", "ctre" };

/* ======================== */

int CtrOpen() {

char fnm[32];
int  i, fn;

   for (i = 1; i <= CtrDrvrCLIENT_CONTEXTS; i++) {
      sprintf(fnm,"/dev/%s.%1d",devnames[ctr_device],i);
      if ((fn = open(fnm,O_RDWR,0)) > 0) return(fn);
   };
   return(0);
}
