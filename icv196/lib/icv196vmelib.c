/**
 * @file icv196vmelib.c
 *
 * @brief icv196 library interface
 *
 * <long description>
 *
 * @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 18/12/2009
 *
 * @section license_sec License
 *          Released under the GPL
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <general_both.h>
#include <icv196vmelib.h>
#include <skeluser_ioctl.h>

#define UCA (MAX_UCA-1)
static int hdl[UCA] = { [0 ... UCA-1] = -1 }; /* MAX allowed user contexts
						 -1 -- means it is free */
static char path[] =  "/dev/icv196vme.";

/**
 * @brief Gets new handle
 *
 * Opens free driver node and assign new user context.
 * It can be used in file operations (read/write/ioctl etc...)
 *
 * @return  -1 -- failed
 * @return > 0 -- OK
 */
int icv196_get_handle(void)
{
	int i;
	int newhdl = -1;
	char pathname[sizeof(path)+3] = { 0 };

	for (i = 0; i < UCA; i++) {
		if (hdl[i] >= 0)
			continue; /* busy */
		sprintf(pathname, "%s%d", path, i+1);
		newhdl = open(pathname, O_RDWR);
		if (newhdl == -1) {
			perror("icv196_get_handle() Can't open driver node");
			hdl[i] = _MAGIC_; /* mark as busy */
			/* maybe some other application is using it
			   Continue to scan through all possible driver nodes */
			continue;
		}

		hdl[i] = newhdl;
		printf("%s driver handle opened\n", pathname);
		break;
	}

	return newhdl;
}

/**
 * @brief Free library handle
 *
 * @param h -- library handle, returned by icv196_get_handle()
 *
 * @return  -1 -- failed
 * @return   0 -- OK
 */
int icv196_put_handle(int h)
{
	int i, rc;
	for (i = 0; i < UCA; i++) {
		if (hdl[i] != h)
			continue;
		rc = close(h);
		if (rc < 0) {
			perror("Could not free library handle\n");
			break;
		}
		hdl[i] = -1; /* mark as free */
	}

	return rc;
}

/**
 * @brief
 *
 * @param h      -- open driver node file descriptor
 * @param module -- module index [0 - 7]
 * @param line   -- line index [0 - 15]
 * @param mode   -- cumulative[1]/non-cumulative[0] mode
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
int icv196_connect(int h, short module, short line, short mode)
{
        /* TODO */
#if 0
        struct icv196T_connect conn;

        conn.source.group = module;
        conn.source.index = line;
        conn.mode = mode;
        return ioctl(h, ICVVME_connect, &conn);
#endif
        return -1;
}

/**
 * @brief
 *
 * @param fd     -- open driver node file descriptor
 * @param module -- module index [0 - 7]
 * @param line   -- line index [0 - 15]
 * @param mode   -- cumulative[1]/non-cumulative[0] mode
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
int icv196_disconnect(int h, short module, short line)
{
        /* TODO */
#if 0
        struct icv196T_connect conn;

        conn.source.group = group;
        conn.source.index = index;
        return ioctl(h, ICVVME_disconnect, &conn);
#endif
        return -1;
}


int icv196_read_channel(int h, int module, int grp, char *data)
{
	struct icv196T_Service arg = { 0 };

	arg.module  = module;
	arg.line    = grp;

	if (ioctl(h, ICV196_GR_READ, &arg) < 0) {
		perror("ICV196_GR_READ ioctl failed");
		return -1;
	}
	return 0;
}

int icv196_write_channel(int h, int module, int grp, char *data)
{
	struct icv196T_Service arg = { 0 };

	arg.module  = module;
	arg.line    = grp;
	arg.data[0] = *data;

	printf("Data to write is %hd\n", *data);
	if (ioctl(h, ICV196_GR_WRITE, &arg) < 0) {
		perror("ICV196_GR_WRITE ioctl failed");
		return -1;
	}
	return 0;
}

/**
 * @brief Set direction of input/output ports
 *
 * @param h      -- library handle, returned by icv196_get_handle()
 * @param module -- module index [0 -- 7]
 * @param grp    -- group index [0 -- 11]
 *                  See description for more details.
 * @param size   -- group size in bytes [1, 2]
 *                  See description for more details.
 * @param dir    -- direction [1 -- output, 0 -- input]
 *
 * Groups can be one or two bytes long.
 * Therefore each module can have up to 6 2-byte-long groups,
 * and up to 12 1-byte-long groups.
 *
 * If group size is 1 byte -- group index (@b grp param) can be any number
 * within [0 -- 11] range.
 *
 * If group size is 2 bytes -- group index (@b grp param) can be @b only
 * @e even indexes
 *
 * @return -1 -- failed
 * @return  0 -- OK
 */
int icv196_init_channel(int h, int module, int grp, int size, int dir)
{
	struct icv196T_Service arg = { 0 };

	if (!WITHIN_RANGE(0, module, 7) ||
	    !WITHIN_RANGE(0, grp, 11) ||
	    !WITHIN_RANGE(0, dir, 1))
		return -1;

	arg.data[0] = grp;
	arg.data[1] = dir;

	if (ioctl(h, ICVVME_setio, &arg) < 0) {
		perror("ICVVME_setio ioctl failed");
		return -1;
	}

	return 0;
}

/**
 * @brief Get information about the lines used by the icv196test test program
 *
 * @param h       -- library handle, returned by icv196_get_handle()
 * @param m       --
 * @param buff_sz --
 * @param buff    --
 *
 * <long-description>
 *
 * @return   0 -- all OK
 * @return < 0 -- failed
 */
int icv196_get_info(int h, int m, int buff_sz, struct icv196T_ModuleInfo *buff)
{
	int cc;

	/* call the driver */
	cc = ioctl(h, ICVVME_getmoduleinfo, (char *)buff);
	if (cc < 0) {
		perror("icv196_get_info ioctl failed");
		return cc;
	}
	return 0;
}

/**
 * @brief
 *
 * @param h   -- library handle, returned by icv196_get_handle()
 * @param val -- at call: new TO, at return the old one
 *
 * Set time out for waiting Event on read of synchronization channel
 * the synchronization is requested by mean of the general purpose
 * function gpevtconnect see in real time facilities: rtfclty/gpsynchrolib.c
 *
 * @return   0 -- all OK
 * @return < 0 -- failed
 */
int icv196_set_to(int h, int *val)
{
	int newval;
	int cc;

	/* set up an argument for ioctl call */
	newval = *val;

	/* Call the driver */
	cc = ioctl(h, ICVVME_setupTO, (char *)&newval);
	if (cc < 0) {
		perror("icv196_set_to ioctl failed");
		return cc;
	}

	/* Give back data in user array */
	*val = newval;
	return 0;
}


















