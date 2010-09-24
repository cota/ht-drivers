/**
 * @file vmod12e16.h
 *
 * @brief VMOD12E16 ADC definitions
 *
 * The VMOD12E16 mezzanine is a 12-bit resolution, 16 channel ADC for
 * which this driver provides support if installed in a modulbus carrier
 * board. So this file provides definitions for easy access to hardware
 * register on both VMODIO/MOD-PCI carrier and VMOD12E16
 * 
 * Copyright (c) 2009 CERN
 * @author Juan David Gonzalez Cobas <dcobas@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifndef _VMOD12E16_H_
#define _VMOD12E16_H_

#include <linux/types.h>
#include <linux/ioctl.h>
#include "vmod12e16drvr.h"

/* VMOD 12E16 registers */
#define VMOD_12E16_CONTROL	0x00	/* write: set amp and channel, start conversion */
#define VMOD_12E16_ADC_DATA	0x00	/* read: converted value, lower 12 bits */
#define VMOD_12E16_ADC_RDY	0x04	/* ro: bit 15 = 0 when ready */
#define VMOD_12E16_INTERRUPT	0x06	/* rw: bit 15 = 1 disable end-of-conversion interrupt */

#define VMOD_12E16_ADC_DATA_MASK	0x0fff
#define VMOD_12E16_ADC_INTERRUPT_MASK	(1<<15)
#define VMOD_12E16_RDY_BIT 		(1<<15)
#define VMOD_12E16_CHANNELS	16		/* channels of VMOD12E16 */

#define VMOD_12E16_CONVERSION_TIME	2	/* two microseconds, up to 30 */
#define VMOD_12E16_MAX_CONVERSION_TIME	30	


/**
 * @brief map of vmod12e16 register layout
 */
struct vmod12e16_registers {
	union {
	unsigned short	control;	/**< control register (wo) */
	unsigned short	data;     	/**< data register (ro)  */
	};
	unsigned short	unused;
	unsigned short	ready;    	/**< ready status register (ro) 
				     		(bit 15 == 0 if ready, and then bits 11-0 
				     		in data register are valid) */
	unsigned short	interrupt;	/**< interrupt mode get/set (rw) 
					     (bit 15 == 0 iff interrupt 
					     mode is in force/desired) */ 
};

/**
 * @brief vmod12e16 device information consists of ordinary vmod
 * geographical info plus a semaphore for mutexing during conversion
 * cycle
 */
struct vmod12e16_dev {
	struct vmod_dev		*config;	/**< vmod lunargs */
	struct semaphore	sem;		/**< locks conversion */
};

#endif /* _VMOD12E16_H_ */
