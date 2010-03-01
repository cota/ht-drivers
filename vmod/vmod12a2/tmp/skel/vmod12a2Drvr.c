
/**
 * @file myDrvr.c
 *
 * @brief Skeleton driver for myModule
 *
 * <long description>
 *
 * Copyright (c) 2009 CERN
 * @author Name Surname <email@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <cdcm/cdcm.h>
#include <skeluser.h>
#include <skeldrvrP.h>
#include <skel.h>
#include "skeluser_ioctl.h"
#include "vmod12a2.h"

static char *SpecificIoctlNames[] = {
	[_IOC_NR(VMOD_12A2_SELECT)]	= "VMOD_12A2_SELECT",
	[_IOC_NR(VMOD_12A2_PUT)]	= "VMOD_12A2_PUT",
};


static int mod_pci_slot_offsets[MOD_PCI_SLOTS] = {
	MOD_PCI_MODULBUS_SLOT0_OFFSET,
	MOD_PCI_MODULBUS_SLOT1_OFFSET,
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
 * @brief extract slot number information from extra field
 *
 * As a provisional means to provide the board position of
 *  the mezzanine, the extra field of the MODULE xml tag is
 *  (ab)used. This routine extracts that number.
 *
 *
 * @param xtra - textual contents of extra field
 * @return slot number the card lies in as provided in hw desc
 * @return <0 if failure
 */

static int get_slot_number(char *xtra)
{
	int slot;
	static char *slots[] = { "1", "2", "3", "4", NULL, };

	/* quick and dirty hack */
	slot = 0;
	while (slots[slot] != NULL) {
		if (!strcmp(xtra, slots[slot]))
			return slot;
		slot++;
	}
	return -1;
}


/**
 * @brief Initialize a 12A2 module, called from install()
 *
 * @param mcon - module context
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserModuleInit(SkelDrvrModuleContext *mcon)
{
	InsLibAnyAddressSpace *pcias;
	struct vmod12a2_registers *regs;
	int slot;

	pcias = InsLibGetAddressSpace(mcon->Modld,
			MOD_PCI_MODULBUS_MEMSPACE_LITTLE_BAR);
	if (!pcias)
		return SkelUserReturnFAILED;

	/* gnoti seauton */
	if (!mcon->Modld->Extra)
		return SkelUserReturnFAILED;

	slot = get_slot_number(mcon->Modld->Extra);
	if (slot < 0 || slot > MOD_PCI_SLOTS)
		return SkelUserReturnFAILED;

	/* allocate module user data  and
	 * let module context know its own PCI base address and
	 * its address given by PCI address + slot offset
	 */
	regs  = (void *)sysbrk(sizeof(struct vmod12a2_registers));
	if (!regs)
		return SkelUserReturnFAILED;

	regs->slot = slot;
	regs->address = pcias->Mapped + mod_pci_slot_offsets[slot];
	regs->channel[0] = regs->address + VMOD_12A2_CHANNEL0;
	regs->channel[1] = regs->address + VMOD_12A2_CHANNEL1;

	mcon->UserData = regs;

	return SkelUserReturnOK;
}

/**
 * @brief prepare module and user's context for uninstallation of the module
 *
 * @param mcon - module context
 */
void SkelUserModuleRelease(SkelDrvrModuleContext *mcon)
{
	sysfree(mcon->UserData, sizeof(struct vmod12a2_registers));
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
	/* no channel number specified, force SELECT ioctl before use */
	ccon->ChannelNr = -1;
	return SkelUserReturnOK;
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
 * @brief Perform a VMOD_12A2_SELECT ioctl
 *
 * A struct vmod12a2_channel pointer is passed as user argument,
 * specifying a LUN and channel to stick to.
 *
 * @param ccon - the calling client context
 * @param arg  - a struct vmod12a2_channel pointer with
 * 			lun and channel fields
 * @return    - SkelUserReturn
 */

static SkelUserReturn do_select_ioctl(SkelDrvrClientContext *ccon,
	void *arg)
{
	struct vmod12a2_channel *chp = arg;
	unsigned int lun, channel;
	SkelDrvrModuleContext *mcon;

	lun	= chp->lun;
	channel	= chp->channel;

	/* sanity check address parameters */
	if (!WITHIN_RANGE(0, channel, VMOD_12A2_CHANNELS-1) ||
		!(mcon = get_mcon(lun)))
	{
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	/* assign client context to channel */
	ccon->ModuleNumber = lun;
	ccon->ChannelNr = channel;

	return SkelUserReturnOK;
}

/**
 * @brief Perform a VMOD_12A2_PUT ioctl
 *
 * The channel to write to and datum to be converted are specified in
 * the homonymous fields of a struct vmod12a2_write structure.
 *
 * @param ccon - client context
 * @param arg  - conversion data as per struct vmod12a2_write definition
 * @ return    - SkelUserReturn
 */
static SkelUserReturn do_put_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
	struct vmod12a2_registers *regs;
	struct vmod12a2_write *put = arg;
	uint16_t *channel_register;
	SkelDrvrModuleContext *mcon;

	/* get address map to write to */
	mcon = get_mcon(ccon->ModuleNumber);
	if (!mcon) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}
	regs = mcon->UserData;

	/* NEVER touch a module context that does not know about its
	  own slot */
	if (!regs || regs->slot < 0) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}
	if (!WITHIN_RANGE(0, put->channel, VMOD_12A2_CHANNELS-1)) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	/* go there */
	channel_register = (uint16_t*)(regs->channel[put->channel]);
	cdcm_iowrite16le(put->datum, channel_register);

	return SkelUserReturnOK;
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
	/* dispatch ioctl */
	switch (cm) {
	case VMOD_12A2_SELECT:
		return do_select_ioctl(ccon, arg);
	break;
	case VMOD_12A2_PUT:
		return do_put_ioctl(ccon, arg);
	break;
	default:
		return SkelUserReturnFAILED;
	break;
	}
	return SkelUserReturnNOT_IMPLEMENTED;
}

struct skel_conf SkelConf; /* user-configurable entry points */

