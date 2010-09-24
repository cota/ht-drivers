/**
 * @file vmod12e16drvr.h
 *
 * @brief VMOD12E16 driver interface
 *
 * The VMOD12E16 mezzanine is a 12-bit resolution, 16 channel ADC for
 * which this driver provides support if installed in a modulbus carrier
 * board. So this file provides definitions for easy access to hardware
 * register on both VMODIO/MOD-PCI carrier and VMOD12E16
 * 
 * Copyright (c) 2009,2010 CERN
 * @author Juan David Gonzalez Cobas <dcobas@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifndef _VMOD12E16DRVR_H_
#define _VMOD12E16DRVR_H_

#include <linux/types.h>
#include <linux/ioctl.h>

/**
 * @brief  user argument  for ioctl VMOD12E16_IOCCONVERT	
 * defining channel, amplification factor, and the data to get in return
 */
struct vmod12e16_conversion {
	int amplification;	/**< amplification factor (0..3 for 1, 10, 100 or 1000 amplification) */
	int channel;		/**< analog channel to convert from (0..15) */
	int data;               /**< digital value after conversion	*/
};                             

/* IOCTLS for this driver */
#define VMOD12E16_IOCMAGIC	'm'
#define	VMOD12E16_IOCSELECT	_IOW (VMOD12E16_IOCMAGIC, 1, struct vmod12e16_state)
#define	VMOD12E16_IOCCONVERT	_IOWR(VMOD12E16_IOCMAGIC, 2, struct vmod12e16_conversion)

#endif /* _VMOD12E16DRVR_H_ */
