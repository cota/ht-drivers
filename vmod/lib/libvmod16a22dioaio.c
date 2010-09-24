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
#include "libvmod16a22dioaio.h"
#include <gm/moduletypes.h>
#include <dioaiolib.h>

#define VMOD16A2_MAX_MODULES    64

/**
 * @brief Get a handle for a vmod16a2 device and channel
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
int vmod16a2_get_handle(unsigned int carrier_lun)
{
	if(carrier_lun < 0 || carrier_lun >= VMOD16A2_MAX_MODULES){
		fprintf(stderr, "libvmod16a22dioaio : Invalid lun %d\n", carrier_lun);
		return -1;
	}
	return carrier_lun;
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
int vmod16a2_convert(int fd, int channel, int datum)
{
        int ret;
	char *error;

	ret = DioChannelWrite(IocVMOD16A2, fd, channel, 2, datum);
	if (ret < 0){
		DioGetErrorMessage(ret, &error);
		fprintf(stderr, "libvmod16a22dioaio : %s\n", error);
	}
	return ret;
}

/**
 * @brief close a channel handle
 *
 * @param fd - Handle to close
 */
int vmod16a2_close(int fd)
{
	return 0;
}
