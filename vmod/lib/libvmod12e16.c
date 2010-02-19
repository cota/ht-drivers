/**
   @file   libvmod12e16.c
   @brief  Library file for the vmod12e16 driver
   @author Juan David Gonzalez Cobas
   @date   October 17 2009
 */

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "vmod12e16.h"
#include "libvmod12e16.h"

/*
#include <skeluser_ioctl.h>
 */

/**
 * @brief Get a handle for a vmod12e16 device
 *
 * @param carrier_lun - carrier LUN for vmod card
 *
 * @return >0 - on success, device file descriptor number identifying 
 *		a mezzanine card
 * @return <0 - if failure
 *
 */
int vmod12e16_get_handle(unsigned int carrier_lun)
{
	const int name_size = 256;
	char devname[name_size];
	int fd, err;
	struct vmod12e16_state ch, *chp = &ch;

	/* Try to open a client context 
	for (i = 1; i <= SkelDrvrCLIENT_CONTEXTS; i++) {
		sprintf(devname, "/dev/%s.%d", "vmod12e16", i);
		fd = open(devname, O_RDONLY);
		if (fd > 0)
			break;
	}
	*/
	snprintf(devname, name_size, "/dev/%s", "vmod12e16");
	fd = open(devname, O_RDWR);
	if (fd < 0)
		return fd;

	/* select channel */
	chp->lun     = carrier_lun;
	chp->channel = 0;	/* assign channel 0 by default */

	err = ioctl(fd, VMOD12E16_IOCSELECT, chp);
	if (err < 0)
		return err;

	return fd;
}

/**
 * @brief perform an analog-to-digital conversion 
 * 
 * Start ADC conversion using 12E16 module addressed by handle fd.
 * Analog data are read from the channel given as a parameter
 * The desired amplification
 * factor is applied to it. At the address specified by value we get the
 * result of conversion.  The possible values of 12E16 instrumentation
 * amplifier are given by the symbolic constants values of factor, as
 * per enum vmod16a2_amplification.

 * Given the limited precision of the card, use of amplification factors
 * greater than 1 is probably uninteresting
 * 
 * @param  fd    	Device handle
 * @param  channel	Channel to read from
 * @param  factor	Amplification factor
 * @param  value	Return value 
 * 
 * @return 0 on success, <0 on failure
 */
int vmod12e16_convert(int fd, int channel,
	enum vmod12e16_amplification factor, int *value)
{
	struct vmod12e16_conversion cv, *cvp = &cv;
	int err;

	cvp->channel       = channel;
	cvp->amplification = factor;
        err = ioctl(fd, VMOD12E16_IOCCONVERT, cvp);
	if (err != 0) 
		return err;
	*value = cvp->data;
	return 0;
}

/**
 *
 * @brief close a handle
 *
 * @param fd Handle to close
 */
int vmod12e16_close(int fd)
{
	return close(fd);
}
