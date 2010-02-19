/**
 * @file vmod12e16Drvr.c
 *
 * @brief Skeleton driver for VMOD12E16 ADC mezzanine on MOD-PCI carrier
 *
 * Structure of this file: first the skel calls are defined, then the
 * static specific IOCTLs are defined, and last comes the IOCTL handler.
 *
 * Copyright (c) 2009 CERN
 * @author Juan David Gonzalez Cobas <dcobas@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#include <cdcm/cdcm.h>
#include <skeldrvrP.h>
#include <skel.h>
#include "skeluser.h"
#include "skeluser_ioctl.h"
#include "vmod12e16.h"

/**
 *  @brief Obtain module version
 *  @param mcon Module context
 *  @param mver Module version
 *  @return Error code
 */
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

/* MOD-PCI slot offsets */
static int mod_pci_slot_offsets[MOD_PCI_SLOTS] = {
	MOD_PCI_MODULBUS_SLOT0_OFFSET,
	MOD_PCI_MODULBUS_SLOT1_OFFSET,
};

/**
 * @brief extract slot number information from extra field
 *
 * As a provisional means to provide the board position of
 *  the mezzanine, the extra field of the MODULE xml tag is
 *  (ab)used. This routine extracts that number.
 *
 * @param xtra - textual contents of extra field
 * @return slot number the card lies in as provided in hw desc
 * @return <0 if failure
 */

static int get_slot_number(char *xtra)
{
	char *end;
	int slot = simple_strtol(xtra, &end, 10);

	return slot - 1;
}

/**
 * @brief Initialize a vmod12e16 module, called from install()
 *
 * We store in UserData the slot number as extracted from the Extra
 * field of module configuration. This will be replaced by a proper call
 * to the MOD-PCI interface as soon as a proper MOD-PCI (or, for that
 * matter, general abstract bus-agnostic) carrier driver is available
 *
 * @param mcon - module context
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserModuleInit(SkelDrvrModuleContext *mcon)
{
	InsLibAnyAddressSpace *pcias;
	struct vmod12e16_registers *regs;
	int slot;

	/* get base module address space */
	pcias = InsLibGetAddressSpace(mcon->Modld,
			MOD_PCI_MODULBUS_MEMSPACE_LITTLE_BAR);
	if (!pcias)
		return SkelUserReturnFAILED;

	/* get slot number from Extra field of module description
	 * hack! */
	slot = get_slot_number(mcon->Modld->Extra);
	if (slot < 0 || slot >= MOD_PCI_SLOTS)
		return SkelUserReturnFAILED;

	/* allocate room for module UserData */
	regs = (void*)sysbrk(sizeof(struct vmod12e16_registers[2]));
	if (!regs)
		return SkelUserReturnFAILED;

	/* let mcon know its own PCI base address */
	regs->slot = slot;

	/* while we're at it, set register addresses */
	regs->address   = pcias->Mapped + mod_pci_slot_offsets[slot];
	regs->control  	= regs->address + VMOD_12E16_CONTROL;
	regs->data     	= regs->address + VMOD_12E16_ADC_DATA;
	regs->ready    	= regs->address + VMOD_12E16_ADC_RDY;
	regs->interrupt	= regs->address + VMOD_12E16_INTERRUPT;

	/* store at module context UserData */
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
	sysfree(mcon->UserData, sizeof(struct vmod12e16_registers));
}

SkelUserReturn SkelUserClientInit(SkelDrvrClientContext *ccon)
{
	/* no channel number specified, force SELECT ioctl before use */
	ccon->ChannelNr = -1;
	return SkelUserReturnOK;
}

void SkelUserClientRelease(SkelDrvrClientContext *ccon)
{
}

/* =========================================================== */
/* Then decide on howmany IOCTL calls you want, and fill their */
/* debug name strings.                                         */

static char *SpecificIoctlNames[] = {
    [_IOC_NR(VMOD_12E16_SELECT)] = "VMOD_12E16_SELECT",
    [_IOC_NR(VMOD_12E16_CONVERT)] = "VMOD_12E16_CONVERT",
};

/**
 * @brief Start a conversion cycle by writing to ctrl register
 *
 * @param regs - register addresses from PCI map
 * @param amplification - amplification factor (0-3 for 1,10,100,1000x)
 * @param channel - analog channel to convert from (0-15).
 *
 */
static void start_conversion(struct vmod12e16_registers *regs, int amplification, int channel)
{
	int channel_mask = 0xf;
	int ampli_mask   = 0x3;
	uint16_t data;

	data = ((amplification & ampli_mask) << 4) |
		(channel & channel_mask);
	cdcm_iowrite16le(data, regs->control);
}

/**
 * @brief Check if ADC conversion is over
 *
 * @param regs - register addresses from PCI map
 */
static int is_ready(struct vmod12e16_registers *regs)
{
	return (VMOD_12E16_RDY_BIT &
		cdcm_ioread16le(regs->ready)) == 0;
}

/**
 * @brief Get data from ADC data reg.
 *
 * @param regs - register addresses from PCI map
 * @return data register contents (12-bit value)
 */
static uint16_t read_data(struct vmod12e16_registers *regs)
{
	return VMOD_12E16_ADC_DATA_MASK &
		cdcm_ioread16le(regs->data);
}

#ifdef VMOD12E16_WITH_INTERRUPTS
/**
 * @brief Get interrupt status
 *
 * @param regs - register addresses from PCI map
 * @return 1 if interrupt mode enabled
 * @return 0 if interrupt mode disabled
 */
static int get_interrupt_status(struct vmod12e16_registers *regs)
{
	return (VMOD_12E16_ADC_INTERRUPT_MASK &
		cdcm_ioread16le(regs->interrupt)) == 0;
}

/**
 * @brief Enable interrupt mode
 *
 * @param regs - register addresses from PCI map
 */
static void enable_interrupt_mode(struct vmod12e16_registers *regs)
{
	int data = ~VMOD_12E16_ADC_INTERRUPT_MASK;
	cdcm_iowrite16le(data, regs->interrupt);
}
#endif /* VMOD12E16_WITH_INTERRUPTS */

/**
 * @brief Disable interrupt mode
 *
 * @param regs - register addresses from PCI map
 */
static void disable_interrupt_mode(struct vmod12e16_registers *regs)
{
	int data = VMOD_12E16_ADC_INTERRUPT_MASK;
	cdcm_iowrite16le(data, regs->interrupt);
}

/**
 * @brief Perform a VMOD_12E16_SELECT ioctl
 *
 * A struct vmod12e16_channel pointer is passed as user argument,
 * specifying a LUN and channel to stick to.
 *
 * The slot field of the arg struct is not needed anymore, as long
 * as the driver gets the board position from the installation data
 * tree (Extra field at module section of XML config).
 *
 * @param ccon - the calling client context
 * @param arg  - a struct vmod12e16_channel pointer with
 * 			lun, slot (UNUSED), channel fields
 * @return    - SkelUserReturn
 */
static SkelUserReturn do_select_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
    struct vmod12e16_channel *chp = arg;
    SkelDrvrModuleContext *mcon;

    /* sanity check address parameters */
    if (!chp) {
    	pseterr(EINVAL);
    	return SkelUserReturnFAILED;
    }
    if (!WITHIN_RANGE(0, chp->channel, VMOD_12E16_CHANNELS-1)) {
    	pseterr(EINVAL);
    	return SkelUserReturnFAILED;
    }

    /* check existence of module to work with */
    mcon = get_mcon(chp->lun);
    if (mcon == NULL) {
    	      pseterr(EINVAL);
	      return SkelUserReturnFAILED;
    }

    /* allright, set module and channel in client context */
    ccon->ModuleNumber = chp->lun;
    ccon->ChannelNr = chp->channel;

    return SkelUserReturnOK;
}

/**
 * @brief Perform a VMOD_12E16_CONVERT ioctl
 *
 * A conversion is performed in polling mode, i.e., VMOD12E16 interrupt
 * mode of operation is not used. After initiating conversion cycle, we
 * sleep for a delay long enough to get a RDY state and a correct value
 * read from DATA register
 *
 * @param ccon - client context
 * @param arg  - conversion data as per struct vmod12e16_conversion definition
 * @ return    - SkelUserReturn
 */
static SkelUserReturn do_cnvrt_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
	struct vmod12e16_registers *regs;
	struct vmod12e16_conversion *conv = arg;
	SkelDrvrModuleContext *mcon;
	int us_elapsed;
	int channel;

	/* get address map to write to */
	mcon = get_mcon(ccon->ModuleNumber);
	if (!mcon) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}
	regs = mcon->UserData;

	/* NEVER touch a module context that does not know about its
	 * own slot */
	if (regs->slot < 0)
		return SkelUserReturnFAILED;

	/* Channel has to be selected properly through SELECT ioctl
	 * call, fail otherwise */
	channel = conv->channel;
	if (channel < 0 || channel > VMOD_12E16_CHANNELS) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}
	ccon->ChannelNr = channel;

	/* allright, go there */
	disable_interrupt_mode(regs);		/* we work in simple polling mode */
	start_conversion(regs, conv->amplification, ccon->ChannelNr);

	/* wait at most the manufacturer-supplied max time */
	us_elapsed = 0;
	while (us_elapsed < VMOD_12E16_MAX_CONVERSION_TIME) {
		usec_sleep(VMOD_12E16_CONVERSION_TIME<1);
		if (is_ready(regs)) {
			conv->data = read_data(regs);
			return SkelUserReturnOK;
		}
	}

	/* timeout */
	pseterr(ETIME);
	return SkelUserReturnFAILED;
}

/**
 * @brief Dispatch Skel User Ioctls
 *
 *  Until a full-fledged carrier driver is released, slot addresses
 *  inside MOD-PCI are handled in an ad-hoc manner, by getting the slot
 *  number from the user and storing the resulting register addresses in
 *  module context, as initialized in SkelUserModuleInit.
 *
 *  Sanity checks are performed on parameters, and we dispatch the
 *  (single) ioctl command needed.
 *
 *  A future release will incorporate a similar interface for
 *  interrupt-driven, non-polling conversion.
 *
 * @param mcon - Module context
 * @param ccon - Client context
 * @param cm   - ioctl command
 * @param arg  - ioctl user argument
 */
SkelUserReturn SkelUserIoctls(SkelDrvrClientContext *ccon,  /* Calling clients context */
			      SkelDrvrModuleContext *mcon,  /* Calling clients selected module */
			      int                    cm,    /* Ioctl number */
			      char                    *arg) { /* Pointer to argument space */

	/* dispatch ioctl */
   	switch (cm) {
	case VMOD_12E16_SELECT:
		return do_select_ioctl(ccon, arg);
	break;
	case VMOD_12E16_CONVERT:
		return do_cnvrt_ioctl(ccon, arg);
	break;

	default:
		return SkelUserReturnFAILED;
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

