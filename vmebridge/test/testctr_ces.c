/*
 * testctr_ces.c - simple test to access a CTR card using the CES emulation.
 *
 * CTR Main logic - A24/D32 0x39 @0xC00000 size 0x10000
 *
 *  CtrDrvrInterruptMask InterruptSource;     Cleared on read 
 *  CtrDrvrInterruptMask InterruptEnable;     Enable interrupts 
 *  unsigned long        HptdcJtag;           Jtag control reg for HpTdc 
 *  unsigned long       InputDelay;   Specified in 40MHz ticks 
 *  unsigned long       CableId;      ID of CTG piloting GMT 
 *  unsigned long       VhdlVersion;  UTC time of VHDL build 
 *  unsigned long       OutputByte;   Output byte number 1..8 on VME P2 +
 *                                    Enable front pannel
 *  CtrDrvrStatus       Status;       Current status
 *  CtrDrvrCommand      Command;      Write command to module
 *  CtrDrvrPll          Pll;          Pll parameters 
 *  CtrDrvrCTime        ReadTime;     Latched date time and CTrain
 *  unsigned long       SetTime;      Used to set UTC if no cable
 *  CtrDrvrCounterBlock Counters;     The counter configurations and status
 *  CtrDrvrTgmBlock     Telegrams;    Active telegrams
 *  CtrDrvrRamTriggerTable       Trigs;        2048 Trigger conditions
 *  CtrDrvrRamConfigurationTable Configs;      2048 Counter configurations
 *  CtrDrvrEventHistory          EventHistory; 1024 Events deep
 *  unsigned long Setup;         VME Level and Interrupt vector
 *  unsigned long LastReset;     UTC Second of last reset
 *  unsigned long PartityErrs;   Number of parity errors since last reset
 *  unsigned long SyncErrs;      Number of frame synchronization errors since
 *                               last reset
 *  unsigned long TotalErrs;     Total number of IO errors since last reset
 *  unsigned long CodeViolErrs;  Number of code violations since last reset
 *  unsigned long QueueErrs;     Number of input Queue overflows since last
 *                               reset
 *
 *  New registers used only in recent VHDL, old makes a bus error !
 *
 *  CtrDrvrIoStatus IoStat;      IO status
 *  unsigned long IdLSL;         ID Chip value Least Sig 32-bits
 *  unsigned long IdMSL;         ID Chip value Most  Sig 32-bits
 *  CtrDrvrModuleStats ModStats; Module statistics
 *
 *
 *
 * CTR JTAG - A16/D16 0x29 @
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <libvmebus.h>


#define MODULE_AM	VME_A24_USER_DATA_SCT
#define MODULE_DW	VME_D32
#define MODULE_VME_ADDR	0xc00000
#define MODULE_VME_SIZE 0x10000

int main(int argc, char **argv)
{
	struct pdparam_master param;
	struct vme_mapping *desc;
	unsigned int *vme_buf;
	int i;
	unsigned int val;
	unsigned int saved;

	memset(&param, 0, sizeof(struct pdparam_master));

	if ((vme_buf = (unsigned int *)find_controller(MODULE_VME_ADDR,
						       MODULE_VME_SIZE,
						       MODULE_AM, 0, 4,
						       &param)) == 0) {
		printf("Failed to find_controller: %s\n", strerror(errno));
		exit(1);
	}

	printf("find_controller mmap'ed at %p\n\n", vme_buf);

	desc = (struct vme_mapping *)param.sgmin;

	/* Now test read access */
	for (i = 1; i <= 5; i++) {
		val = swapbe32(vme_buf[i]);

		printf("%x:  0x%08x\n", MODULE_VME_ADDR + i*4, val);
	}

	/* Check for a bus error */
	if (vme_bus_error_check(desc))
		printf("Bus error occured\n");

	/* Test a write access */
	printf("\n");
	saved = swapbe32(vme_buf[1]);
	printf("Read 0x%08x at 0x%08x\n", saved, MODULE_VME_ADDR + 4);

	/* Check for a bus error */
	if (vme_bus_error_check(desc))
		printf("Bus error occured\n");

	printf("\n");

	printf("Writing 0x55aa55aa at address %x\n", MODULE_VME_ADDR + 4);
	vme_buf[1] = swapbe32(0x55aa55aa);
	fsync(desc->fd);

	/* Check for a bus error */
	if (vme_bus_error_check(desc))
		printf("Bus error occured\n");

	val = swapbe32(vme_buf[1]);

	/* Check for a bus error */
	if (vme_bus_error_check(desc))
		printf("Bus error occured\n");

	printf("Read back 0x%08x at address %x\n", val, MODULE_VME_ADDR + 4);
/*
	vme_buf[1] = swapbe32(saved);

	if (vme_bus_error_check(desc))
		printf("Bus error occured\n");
*/
	printf("\n");

	if (return_controller(desc))
		printf("Failed to return_controler: %s\n", strerror(errno));


	return 0;
}
