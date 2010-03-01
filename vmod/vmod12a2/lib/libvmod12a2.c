/**
   @file   libvmod12a2.c
   @brief  Library file for the vmod12a2 driver
   @author Juan David Gonzalez Cobas
   @date   October 17 2009
 */

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "vmod12a2.h"
#include "libvmod12a2.h"

static int select_channel(int fd, int lun, int channel)
{
	struct vmod12a2_select ch, *chp = &ch;

	chp->lun     = lun;
	chp->channel = channel;
	return ioctl(fd, VMOD12A2_IOCSELECT, chp);
}

/**
 * @brief Get a handle for a vmod12a2 device and channel
 *
 * A channel is selected by giving the LUN for the board
 * and the channel number. The handle returned allows
 * further reference to the same channel
 *
 * @param carrier_lun - carrier LUN for vmod card
 * @param channel     - selected channel for reading
 *
 * @return >0 - on success, device file descriptor number
 * @return <0 - if failure
 *
 */
int vmod12a2_get_handle(unsigned int carrier_lun)
{
	const int filename_sz = 256;
	char devname[filename_sz];
	const char *driver_name = "vmod12a2";

	int fd, err;

	/* open the device file */
	snprintf(devname, filename_sz, "/dev/%s", driver_name);
	fd = open(devname, O_RDONLY);
	if (fd < 0)
		return fd;

	err = select_channel(fd, carrier_lun, 0);  /* channel 0 default */
	if (err != 0) 
		return err;

	return fd;
}

/**
 * @brief perform a digital-to-analog conversion
 *
 * The output voltage depends on hardware configuration (default
 * -10V..10V, configurable by jumpers to 0-10V range).
 *
 * @param  fd    	Device handle identifying board
 * @param  channel    	Channel on the board to write to
 * @param  datum	Digital 12-bit value (0x000-0xfff)
 *
 * @return 0 on success, <0 on failure
 */
int vmod12a2_convert(int fd, int channel, int datum)
{
	int err;
	/* hack! to comply with dynamic channel modification */
	err = select_channel(fd, VMOD12A2_NO_LUN_CHANGE, channel);
	if (err != 0)
		return err;
        return ioctl(fd, VMOD12A2_IOCPUT, &datum);
}

/**
 * @brief close a channel handle
 *
 * @param fd - Handle to close
 */
int vmod12a2_close(int fd)
{
	return close(fd);
}
