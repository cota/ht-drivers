/**
   @file   libvmod12e162dioaio.c
   @brief  Library file for the vmod12e16 driver
   @author Samuel Iglesias Gonsalvez
   @date   July 6 2010
 */

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "libvmod12e162dioaio.h"
#include <gm/moduletypes.h>
#include <ioconfiglib.h>
#include <dioaiolib.h>

#define VMOD12E16_MAX_MODULES    64
#define PURGE_CPUPIPELINE asm("eieio")

struct vmod12e16_registers {
	unsigned short	control;	/**< control register (wo) */
	unsigned short	unused;
	unsigned short	ready;    	/**< ready status register (ro) 
				     		(bit 15 == 0 if ready, and then bits 11-0 
				     		in data register are valid) */
	unsigned short	interrupt;	/**< interrupt mode get/set (rw) 
					     (bit 15 == 0 iff interrupt 
					     mode is in force/desired) */ 
};

int vmod12e16_get_handle(unsigned int carrier_lun)
{
	if(carrier_lun < 0 || carrier_lun >= VMOD12E16_MAX_MODULES){
		fprintf(stderr, "libvmod12e162dioaio : Invalid lun %d\n", carrier_lun);
		return -1;
	}
	return carrier_lun;
}

int vmod12e16_convert(int fd, int channel, enum vmod12e16_amplification factor, int *datum)
{
	int ret;
	void *address;
	struct vmod12e16_registers *reg;

	ret = IocModulPointer(IocVMOD12E16, fd, IOC_DEFAULTP, &address);
	if (ret < 0){
		fprintf(stderr,"libvmod12e162dioaio: error getting the address: %d\n", ret);
		return -1;
	}
	reg = (struct vmod12e16_registers *)address;	

	reg->interrupt = 0x8000; /* No interrupts*/
	PURGE_CPUPIPELINE;

	reg->control = channel;
	PURGE_CPUPIPELINE;

	while ((reg->ready & 0x8000) != 0) {};

	*datum = reg->control & 0x0fff;
	PURGE_CPUPIPELINE;

	return 0;
}

int vmod12e16_close(int fd)
{
	return 0;
}
