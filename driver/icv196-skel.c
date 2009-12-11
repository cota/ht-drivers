/**
 * @file icv196-skel.c
 *
 * @brief Skeleton driver for ICV196
 *
 * Structure of this file: first the skel calls are defined, then the
 * static specific IOCTLs are defined, and last comes the IOCTL handler.
 *
 * Copyright (c) 2009 CERN
 * @author Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <cdcm/cdcm.h>
#include <skeluser.h>
#include <skeldrvrP.h>
#include <skeluser_ioctl.h>
#include <icv196vme.h>

extern char* icv196_install(InsLibModlDesc *);
extern int   icv196_read(void *, struct cdcm_file *, char *, int);
extern int   icv196_write(void *, struct cdcm_file *, char *, int);
extern int   icv196_vmeisr(void *);


static char *SpecificIoctlNames[] = {
	[_IOC_NR(MY_IOCTL_NAME_1)]	= "MyIoctl-1",
	[_IOC_NR(MY_IOCTL_NAME_2)] 	= "MyIoctl-2"
};

/**
 * @brief Get the name of an IOCTL
 *
 * @param cmd - IOCTL number
 *
 * @return IOCTL name
 */
char *SkelUserGetIoctlName(int cmd)
{
	return SpecificIoctlNames[_IOC_NR(cmd)];
}

/**
 * @brief Get module version
 *
 * @param mcon - module context
 * @param mver - module version to be filled
 *
 * The module version can be the compilation date for the FPGA code
 * or some other identity readable from the hardware
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserGetModuleVersion(SkelDrvrModuleContext *mcon,
					char *mver)
{
	strncpy(mver, 0, 0);
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief Get hardware status
 *
 * @param mcon - module context
 * @param hsts - hardware status to be filled
 *
 * Note that the standard status will be put by skel at mcon->StandardStatus
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserGetHardwareStatus(SkelDrvrModuleContext *mcon,
					 uint32_t *hsts)
{
	*hsts = 0;
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief Get UTC time
 *
 * @param mcon - module context
 * @param utc - utc time to be filled
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserGetUtc(SkelDrvrModuleContext *mcon,
			      SkelDrvrTime *utc)
{
	utc->Second = 0;
	utc->NanoSecond = 0;
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief Hardware reset
 *
 * @param mcon - module context
 *
 * This is the hardware reset routine. Don't forget to copy
 * the registers block in the module context to the hardware
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserHardwareReset(SkelDrvrModuleContext *mcon)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief Get and Clear the interrupt source register from the module
 *
 * @param mcon - module context
 * @param itim - utc time to be filled
 * @param isrc - interrupt source mask
 *
 * Ideally, this operation should be done atomically (clear-on-read).
 * Returning the utc time of the interrupt is advised but not mandatory
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserGetInterruptSource(SkelDrvrModuleContext *mcon,
					  SkelDrvrTime *itim, uint32_t *isrc)
{
	bzero((void *)itim, sizeof(SkelDrvrTime));
	*isrc = 0;
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief enable module interrupts
 *
 * @param mcon - module context
 * @param imsk - mask with the interrupts to be enabled
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserEnableInterrupts(SkelDrvrModuleContext *mcon,
					uint32_t imsk)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief enable/disable a module
 *
 * @param mcon - module context
 * @param flag - 1 --> enable, 0 --> disable
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserHardwareEnable(SkelDrvrModuleContext *mcon,
				      uint32_t flag)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief FPGA JTAG I/O, read a byte
 *
 * @param mcon - module context
 * @param byte - byte to be filled in
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserJtagReadByte(SkelDrvrModuleContext *mcon,
				    uint32_t *byte)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief FPGA JTAG I/O, write a byte
 *
 * @param mcon - module context
 * @param byte - byte to be written
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserJtagWriteByte(SkelDrvrModuleContext *mcon,
				     uint32_t byte)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief Initialize a module, called from install()
 *
 * @param mcon - module context
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserModuleInit(SkelDrvrModuleContext *mcon)
{
	mcon->UserData = icv196_install(mcon->Modld);
	return SkelUserReturnOK;
}

/**
 * @brief prepare module and user's context for uninstallation of the module
 *
 * @param mcon - module context
 */
void SkelUserModuleRelease(SkelDrvrModuleContext *mcon)
{
}

/**
 * @brief initialise a client's context -- called when doing open()
 *
 * @param ccon - client context
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserClientInit(SkelDrvrClientContext *ccon)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 * @brief release a client's context -- called during close()
 *
 * @param ccon - client context
 */
void SkelUserClientRelease(SkelDrvrClientContext *ccon)
{
}

/**
 * @brief Module-specific IOCTL handler
 *
 * @param ccon - client context
 * @param mcon - module context
 * @param cm - IOCTL number
 * @param arg - pointer to the user's argument
 *
 * @todo rcnt and wcnt should be removed -- they're already checked in
 * skeldrvr.c
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserIoctls(SkelDrvrClientContext *ccon,
			      SkelDrvrModuleContext *mcon, int cm, char *arg)
{
	switch (cm) {

	case MY_IOCTL_NAME_1:
		break;

	case MY_IOCTL_NAME_2:
		break;

	default:
		break;
	}

	return SkelUserReturnNOT_IMPLEMENTED;
}

struct skel_conf SkelConf = {
        .read        = icv196_read,
        .write       = icv196_write,
        .intrhandler = icv196_vmeisr
};
