/* =========================================================== */
/* SkelDriver user code frame. Fill in this frame to implement */
/* a specific driver for PCI/VME modules.                      */
/* Julian Lewis Tue 16th Dec 2008 AB/CO/HT                     */
/* =========================================================== */
#include <cdcm/cdcm.h>
#include <skeldrvrP.h>
#include <skel.h>
#include <skeluser.h>
#include <skeluser_ioctl.h>

/* =========================================================== */
/* The module version can be the compilation date for the FPGA */
/* code or some other identity readable from the hardware.     */

SkelUserReturn SkelUserGetModuleVersion(SkelDrvrModuleContext *mcon,
					char                  *mver) {
   ksprintf(mver, "High Five!");
   return SkelUserReturnNOT_IMPLEMENTED;
}

/* =========================================================== */
/* The specific hardware status is read here. From this it is  */
/* possible to build the standard SkelDrvrStatus inthe module  */
/* context at location mcon->StandardStatus                    */

SkelUserReturn SkelUserGetHardwareStatus(SkelDrvrModuleContext *mcon,
					 U32                   *hsts) {
   *hsts = 0;
   return SkelUserReturnNOT_IMPLEMENTED;
}

/* =========================================================== */
/* Get the UTC time, called from the ISR                       */

SkelUserReturn SkelUserGetUtc(SkelDrvrModuleContext *mcon,
			      SkelDrvrTime          *utc) {
   utc->Second = 0;
   utc->NanoSecond = 0;
   return SkelUserReturnNOT_IMPLEMENTED;
}

/* =========================================================== */
/* This is the hardware reset routine. Don't forget to copy    */
/* the registers block in the module context to the hardware.  */

SkelUserReturn SkelUserHardwareReset(SkelDrvrModuleContext *mcon) {
   return SkelUserReturnNOT_IMPLEMENTED;
}

/* =========================================================== */
/* This routine gets and clears the modules interrupt source   */
/* register. Try to do this with a Read/Modify/Write cycle.    */
/* Its a good idea to return the time of interrupt.            */

SkelUserReturn SkelUserGetInterruptSource(SkelDrvrModuleContext *mcon,
					  SkelDrvrTime          *itim,
					  U32                   *isrc) {
   bzero((void *) itim, sizeof(SkelDrvrTime));
   *isrc = 0;
   return SkelUserReturnNOT_IMPLEMENTED;
}

/* =========================================================== */
/* Define howmany interrupt sources you want and provide this  */
/* routine to enable them.                                     */

SkelUserReturn SkelUserEnableInterrupts(SkelDrvrModuleContext *mcon,
					U32                    imsk) {
   return SkelUserReturnNOT_IMPLEMENTED;
}

/* =========================================================== */
/* Provide a way to enable or disable the hardware module.     */

SkelUserReturn SkelUserHardwareEnable(SkelDrvrModuleContext *mcon,
				      U32                    flag) {
   return SkelUserReturnNOT_IMPLEMENTED;
}

/* =========================================================== */
/* Standard CO FPGA JTAG IO, Read a byte                       */

SkelUserReturn SkelUserJtagReadByte(SkelDrvrModuleContext *mcon,
				    U32                   *byte) {
   return SkelUserReturnNOT_IMPLEMENTED;
}

/* =========================================================== */
/* Standard CO FPGA JTAG IO, Write a byte                      */

SkelUserReturn SkelUserJtagWriteByte(SkelDrvrModuleContext *mcon,
				     U32                    byte) {
   return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief Initialize a Vd80 module, called from install()
 *
 * @param mcon - module context
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserModuleInit(SkelDrvrModuleContext *mcon)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief prepare module and user's context for uninstallation of the module
 *
 * @param mcon - module context
 */
void SkelUserModuleRelease(SkelDrvrModuleContext *mcon)
{
}

SkelUserReturn SkelUserClientInit(SkelDrvrClientContext *ccon)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

void SkelUserClientRelease(SkelDrvrClientContext *ccon)
{
}

/* =========================================================== */
/* Then decide on howmany IOCTL calls you want, and fill their */
/* debug name strings.                                         */

static char *SpecificIoctlNames[] = {
   [_IOC_NR(NullDrvrIoctlMT_IOCTL_1)]	= "MyIoctl-1",
   [_IOC_NR(NullDrvrIoctlMT_IOCTL_2)]	= "MyIoctl-2"
};

SkelUserReturn SkelUserIoctls(SkelDrvrClientContext *ccon,  /* Calling clients context */
			      SkelDrvrModuleContext *mcon,  /* Calling clients selected module */
			      int                    cm,    /* Ioctl number */
			      char                    *arg) { /* Pointer to argument space */

   switch (cm) {

      case NullDrvrIoctlMT_IOCTL_1:
      break;

      case NullDrvrIoctlMT_IOCTL_2:
      break;

      default:
      break;
   }

   return SkelUserReturnNOT_IMPLEMENTED;
}

/* =========================================================== */
/* Get the name of an IOCTL                                    */

char *SkelUserGetIoctlName(int cmd) {
   return SpecificIoctlNames[_IOC_NR(cmd)];
}


SkelUserReturn SkelUserConfig(SkelDrvrWorkingArea *wa, SkelDrvrModuleContext *mcon)
{
   return SkelUserReturnNOT_IMPLEMENTED;
}

struct skel_conf SkelConf; /* user-configurable entry points */
