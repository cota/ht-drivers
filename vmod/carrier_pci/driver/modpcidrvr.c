/**
 * @file mod-pci.c
 *
 * @brief Skeleton driver for MOD-PCI carrier
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
#include "carrier.h"
#include "mod-pci.h"

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

/* make true when first calling ModuleInit. hack! */
static int driver_registered = 0;

static int mod_pci_get_address_space(struct carrier_as *as,
				int board_number,
				int board_position,
				int address_space_number);
/**
 * @brief Initialize a mod-pci module, called from install()

 * We store in UserData the addresses of MODULBUS address spaces
 * corresponding to slot0 and slot1
 *
 * @param mcon - module context
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserModuleInit(SkelDrvrModuleContext *mcon)
{
	InsLibAnyAddressSpace *pcias;
	struct mod_pci_memory_space *regs;
	int slot;

	SK_INFO("%s: Initializing mod-pci module %d\n",
	THIS_MODULE->name, mcon->ModuleNumber);

	/* get room for slot address data */
	regs = (void*)sysbrk(sizeof(struct mod_pci_memory_space[MOD_PCI_SLOTS]));
	if (!regs)
		return SkelUserReturnFAILED;

	/* get base PCI address for LE access */
	pcias = InsLibGetAddressSpace(mcon->Modld,
			MOD_PCI_MODULBUS_MEMSPACE_LITTLE_BAR);
	if (!pcias)
		return SkelUserReturnFAILED;

	/* store addresses of slots as UserData */
	for (slot = 0; slot < MOD_PCI_SLOTS; slot++) {
		regs[slot].slot = slot;
		regs[slot].address = pcias->Mapped + mod_pci_slot_offsets[slot];
		regs[slot].width = MOD_PCI_MODULBUS_WIDTH;
	}

	/* store at module context UserData */
	mcon->UserData = regs;

	/* register carrier-mezzanine entry points */
	if (!driver_registered) {
		carrier_register("mod-pci", mod_pci_get_address_space, NULL);
		driver_registered = 1;
	}

	return SkelUserReturnOK;
}

/**
 * @brief prepare module and user's context for uninstallation of the module
 *
 * @param mcon - module context
 */
void SkelUserModuleRelease(SkelDrvrModuleContext *mcon)
{
	sysfree(mcon->UserData, sizeof(struct mod_pci_memory_space));
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
    [_IOC_NR(MOD_PCI_GETSLOTADDR)] = "MOD_PCI_GETSLOTADDR",
};

/**
 * @brief Perform a MOD_PCI_GETSLOTADDR ioctl
 *
 * This is the interface for a mezzanine module to know
 * what memory address it has its registers mapped to.
 *
 * @param ccon - the calling client context
 * @param arg  - a struct mod_pci_memory_space pointer to ask
 *		  for a specific slot's address space
 * @ return    - SkelUserReturn
 */
static SkelUserReturn do_getslotaddr_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
    struct mod_pci_memory_space *msp = arg, *aux;
    unsigned int slot;
    SkelDrvrModuleContext *mcon;


    /* sanity check address parameters */
    slot    = msp->slot;
    if (!WITHIN_RANGE(0, slot, MOD_PCI_SLOTS-1)) {
    	pseterr(EINVAL);
    	return SkelUserReturnFAILED;
    }
    if (!(mcon = get_mcon(ccon->ModuleNumber))) {
    	pseterr(EINVAL);
	return SkelUserReturnFAILED;
    }

    /* ok, go for it */
    aux = mcon->UserData ;
    msp->slot    = aux[slot].slot;
    msp->address = aux[slot].address;
    msp->width   = aux[slot].width;

    return SkelUserReturnOK;
}

static int mod_pci_get_address_space(struct carrier_as *as,
				int board_number,
				int board_position,
				int address_space_number)
{
	struct mod_pci_memory_space *msp;
	SkelDrvrModuleContext *mcon;

	/* MOD-PCI has a single space */
	if (address_space_number != 1) {
		SK_ERROR("MOD-PCI invalid address space request\n");
		return -1;
	}
	mcon = get_mcon(board_number);
	if (mcon == NULL) {
		SK_ERROR("Invalid MOD-PCI board number %d\n", board_number);
		return -1;
	}
	if ((board_position <= 0) || (board_position > MOD_PCI_SLOTS)) {
		SK_ERROR("Invalid MOD-PCI board position %d\n", board_position);
		return -1;
	}

	/* success, go ahead */
	msp = mcon->UserData;
	as->address 	  = (unsigned long)msp[board_position-1].address;
	as->width   	  = MOD_PCI_MODULBUS_WIDTH;
	as->size    	  = MOD_PCI_MODULBUS_WINDOW_SIZE;
	as->is_big_endian = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(mod_pci_get_address_space);

/**
 * @brief Dispatch Skel User Ioctls
 *
 *  Sanity checks are performed on parameters, and we dispatch the
 *  (single) ioctl command needed.
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
	case MOD_PCI_GETSLOTADDR:
		return do_getslotaddr_ioctl(ccon, arg);
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

