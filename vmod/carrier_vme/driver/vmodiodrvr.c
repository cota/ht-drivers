/**
 * @file vmodio.c
 *
 * @brief VMOD/IO skel driver
 *
 * Janz's VMOD/IO is a VME carrier board with 4 slots
 * for MODULBUS mezzanine carriers.
 *
 * This driver provides basic functionality to allow
 * the mezzanine drivers to configure their address
 * spaces and be called back from the ISR that handles
 * their interrupts.
 *
 * More information about the carrier driver architecture
 * can be found in the COHT drivers team proposal for carrier
 * drivers.
 *
 * Copyright (c) 2009 CERN
 * @author Juan David Gonzalez Cobas <dcobas@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#include <cdcm/cdcm.h>
#include <skeluser.h>
#include <skeldrvrP.h>
#include <skeluser_ioctl.h>

#include "carrier.h"
#include "vmodio.h"

/* flag for calling carrier_register just once (hack!) */
static int registered = 0;

/* forward */
static int get_address_space(
	struct carrier_as *asp,
	int board_number, int board_position, int address_space_number);

static int vmodio_offsets[VMODIO_SLOTS] = {
	VMODIO_SLOT0,
	VMODIO_SLOT1,
	VMODIO_SLOT2,
	VMODIO_SLOT3,
};

static char *SpecificIoctlNames[] = {
	[_IOC_NR(VMODIO_GETSLTADDR)]	= "VMODIO_GETSLTADDR",
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
 * @brief Initialize a Vd80 module, called from install()
 *
 * @param mcon - module context
 *
 * @return SkelUserReturn
 */
SkelUserReturn SkelUserModuleInit(SkelDrvrModuleContext *mcon)
{
	if (!registered)
		if (carrier_register("vmodio", get_address_space, NULL) != 0)
			return SkelUserReturnFAILED;
	registered = 1;
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
 * @brief Get virtual address of a mezzanine board AS
 *
 * VMOD/IO assigns a memory-mapped IO area to each slot of slot0..3
 * by the rule slot_address = base_address + slot#*0x200.
 *
 * @param board_number  - logical module number of VMOD/IO card
 * @param board_positio - slot the requesting mz is plugged in
 * @param address_space_number
 *                      - must be 1 (only one address space available)
 * @return 0 on success
 * @retrun != 0 on failure
 */
static int get_address_space(
	struct carrier_as *asp,
	int board_number, int board_position, int address_space_number)
{
	InsLibAnyAddressSpace *vmeas16, *vmeas24;
	SkelDrvrModuleContext *mcon;
	void *addr;

	/* VMOD/IO has a single space */
	if (address_space_number != 1) {
		SK_ERROR("VMOD/IO: invalid address space request\n");
		return -1;
	}
	mcon = get_mcon(board_number);
	if (mcon == NULL) {
		SK_ERROR("VMOD/IO: invalid module number %d\n", board_number);
		return -1;
	}
	if ((board_position <= 0) || (board_position > VMODIO_SLOTS)) {
		SK_ERROR("Invalid VMOD/IO board position %d\n", board_position);
		return -1;
	}

	vmeas16 = InsLibGetAddressSpace(mcon->Modld, VMODIO_SHORT);
	vmeas24 = InsLibGetAddressSpace(mcon->Modld, VMODIO_STANDARD);
	if (vmeas16 != NULL) {
		addr = vmeas16->Mapped;
		SK_INFO("Short (A16) address space at 0x%p\n", addr);
		asp->address = (unsigned long)addr + vmodio_offsets[board_position-1];
		asp->size = 4096;
		asp->width = 16;
		asp->is_big_endian = 1;
		return  0;
	}
	if (vmeas24 != NULL) {
		addr = vmeas24->Mapped;
		SK_INFO("Standard (A24) address space at 0x%p\n", addr);
		asp->address = (unsigned long)addr + vmodio_offsets[board_position-1];
		asp->size = 4096;
		asp->width = 16;
		asp->is_big_endian = 1;
		return  0;
	}
	SK_ERROR("VMOD/IO: no address space %d found\n", address_space_number);
	return -1;
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
#if 0
	struct carrier_as *casp = (void *)arg, as;

	switch (cm) {
	case VMODIO_GETSLTADDR:
		if (get_address_space(&as, casp->board_number, casp->board_position, 1)) {
			pseterr(EINVAL);
			return SkelUserReturnFAILED;
		}
		return SkelUserReturnOK;
	break;

	default:
	break;
	}
#endif

	return SkelUserReturnNOT_IMPLEMENTED;
}

/**
 *  @brief Wrapper around get_address_space
 *
 *  Same parameters and semantics, only intended for export (via
 *  linux EXPORT_SYMBOL or some LynxOS kludge)
 */
int  vmodio_get_address_space(
	struct carrier_as *as,
	int board_number,
	int board_position,
	int address_space_number)
{
	return	get_address_space(as, board_number, board_position, address_space_number);
}
EXPORT_SYMBOL_GPL(vmodio_get_address_space);

struct skel_conf SkelConf; /* user-configurable entry points */
