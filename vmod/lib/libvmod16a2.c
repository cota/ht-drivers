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

int vmod16a2_get_handle(unsigned int lun)
{
	const int filename_sz = 256;
	char devname[filename_sz];
	const char *driver_name = "vmod16a2";
	int fd;

	/* open the device file */
	snprintf(devname, filename_sz, "/dev/%s.%d", driver_name, lun);
	fd = open(devname, O_RDONLY);
	return fd;
}

int vmod16a2_convert(int fd, int channel, int datum)
{
	struct vmod16a2_convert cvt, *cvtp = &cvt;

	cvtp->channel = channel;
	cvtp->value   = datum;
        return ioctl(fd, VMOD16A2_IOCPUT, cvtp);
}

int vmod16a2_close(int fd)
{
	return close(fd);
}
