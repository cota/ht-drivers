/**
   @file   libvmod12e16.c
   @brief  Library file for the vmod12e16 driver
   @author Juan David Gonzalez Cobas
   @date   October 17 2009
 */

#include <sys/ioctl.h>
#include <linux/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "vmod12e16drvr.h"
#include "libvmod12e16.h"

int vmod12e16_get_handle(unsigned int lun)
{
	const int name_size = 256;
	char devname[name_size];
	const char *driver_name = "vmod12e16";

	snprintf(devname, name_size, "/dev/%s.%d", driver_name, lun);
	return open(devname, O_RDWR);
}

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

int vmod12e16_close(int fd)
{
	return close(fd);
}
