/*
 * testctr.c - simple mmap test to access a CTR card.
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
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <vmebus.h>

#define VME_MWINDOW_DEV "/dev/vme_mwindow"

#define MODULE_AM	VME_A24_USER_DATA_SCT
#define MODULE_DW	VME_D32
#define MODULE_VME_ADDR	0xc00000
#define MODULE_VME_SIZE 0x10000


static unsigned int swapbe(unsigned int val)
{
	return (((val & 0xff000000) >> 24) | ((val & 0xff0000) >> 8) |
		((val & 0xff00) << 8) | ((val & 0xff) << 24));
}

int main(int argc, char **argv)
{
	int fd;
	struct vme_mapping desc;
	unsigned int *vme_buf;
	int org_force_create;
	int org_force_destroy;
	int force_create;
	int force_destroy;
	int i;
	unsigned int test;
	int bus_err;

	/* Setup the mapping descriptor */
	memset(&desc, 0, sizeof(struct vme_mapping));

	desc.window_num = 0;

	desc.am		= MODULE_AM;
	desc.data_width	= VME_D32;
	desc.vme_addru	= 0;
	desc.vme_addrl	= MODULE_VME_ADDR;
	desc.sizeu	= 0;
	desc.sizel	= MODULE_VME_SIZE;

	printf("Creating mapping\n\tVME addr: 0x%08x size: 0x%08x "
	       "AM: 0x%02x data width: %d\n\n",
	       desc.vme_addrl, desc.sizel, desc.am, desc.data_width);


	if ((fd = open(VME_MWINDOW_DEV, O_RDWR)) < 0) {
		printf("Failed to open %s: %s\n", VME_MWINDOW_DEV,
		       strerror(errno));
		exit(1);
	}

	/* Save the force flags */
	if (ioctl(fd, VME_IOCTL_GET_CREATE_ON_FIND_FAIL,
		  &org_force_create) < 0) {
		printf("Failed to get force create flag: %s\n",
		       strerror(errno));
		goto out_err;
	}

	if (ioctl(fd, VME_IOCTL_GET_DESTROY_ON_REMOVE,
		  &org_force_destroy) < 0) {
		printf("Failed to get force destroy flag: %s\n",
		       strerror(errno));
		goto out_err;
	}

	/* Set the force flags */
	force_create = 1;

	if (ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL, &force_create) < 0) {
		printf("Failed to set force create flag: %s\n",
		       strerror(errno));
		goto out_err;
	}

	force_destroy = 1;

	if (ioctl(fd, VME_IOCTL_SET_DESTROY_ON_REMOVE, &force_destroy) < 0) {
		printf("Failed to set force create flag: %s\n",
		       strerror(errno));
		goto out_err;
	}

	/* Create a new mapping for the area */
	if (ioctl(fd, VME_IOCTL_FIND_MAPPING, &desc) < 0) {
		printf("Failed to create mapping: %s\n", strerror(errno));
		goto out_err;
	}

	/* Now mmap the area */
	if ((vme_buf = mmap(0, MODULE_VME_SIZE, PROT_READ, MAP_PRIVATE, fd,
			    desc.pci_addrl)) == MAP_FAILED) {
		printf("Failed to mmap: %s\n", strerror(errno));
		goto out_mmap_err;
	}

	/* Now test read access */
	for (i = 1; i <= 5; i++) {
		test = swapbe(vme_buf[i]);

		printf("Read 0x%08x at address 0x%08x\n",
		       test, MODULE_VME_ADDR + i*4);
	}

	/* Check for a bus error */
	if (ioctl(fd, VME_IOCTL_GET_BUS_ERROR, &bus_err) < 0) {
		printf("Failed to get bus error status: %s\n", strerror(errno));
		goto out_err;
	}

	if (bus_err)
		printf("Bus error occured\n");


	sleep(10);



	if (munmap(vme_buf, MODULE_VME_SIZE))
		printf("Failed to unmap: %s\n", strerror(errno));


out_mmap_err:
	if (ioctl(fd, VME_IOCTL_RELEASE_MAPPING, &desc) < 0)
		printf("Failed to release mapping: %s\n", strerror(errno));

out_err:
	/* Restore flags */
	if (ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL, &org_force_create) < 0)
		printf("Failed to restore force create flag: %s\n",
		       strerror(errno));


	if (ioctl(fd, VME_IOCTL_SET_DESTROY_ON_REMOVE, &org_force_destroy) < 0)
		printf("Failed to restore force destroy flag: %s\n",
		       strerror(errno));

	close(fd);

	return 0;
}
