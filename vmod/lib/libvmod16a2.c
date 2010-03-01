/**
   @file   libvmod16a2.c
   @brief  Library file for the vmod16a2 driver
   @author Juan David Gonzalez Cobas
   @date   October 17 2009
 */

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "vmod16a2.h"
#include "libvmod16a2.h"

static int select_channel(int fd, int lun, int channel)
{
	struct vmod16a2_select ch, *chp = &ch;

	chp->lun     = lun;
	chp->channel = channel;
	return ioctl(fd, VMOD16A2_IOCSELECT, chp);
}

/**
 * @brief Get a handle for a vmod16a2 device
 *
 * A channel is selected by giving the LUN for the board
 *
 * @param lun - LUN for vmod card
 *
 * @return >0 - on success, device file descriptor number
 * @return <0 - if failure
 *
 */
int vmod16a2_get_handle(unsigned int lun)
{
	const int filename_sz = 256;
	char devname[filename_sz];
	const char *driver_name = "vmod16a2";

	int fd, err;

	/* open the device file */
	snprintf(devname, filename_sz, "/dev/%s", driver_name);
	fd = open(devname, O_RDONLY);
	if (fd < 0)
		return fd;

	err = select_channel(fd, lun, 0);  /* channel 0 default */
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
 * @param  datum	Digital 16-bit value (0x0000-0xffff)
 *
 * @return 0 on success, <0 on failure
 */
int vmod16a2_convert(int fd, int channel, int datum)
{
	int err;
	/* hack! to comply with dynamic channel modification */
	err = select_channel(fd, VMOD16A2_NO_LUN_CHANGE, channel);
	if (err != 0)
		return err;
        return ioctl(fd, VMOD16A2_IOCPUT, &datum);
}

/**
 * @brief close a channel handle
 *
 * @param fd - Handle to close
 */
int vmod16a2_close(int fd)
{
	return close(fd);
}
