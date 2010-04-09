/**
 * @file lib-usage-example.c
 *
 * @brief
 *
 * Code snip shows how to use ICV196 library.
 *
 * @author Copyright (C) 2010 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 09/04/2010
 *
 * @section license_sec License
 *          Released under the GPL
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <icv196vmelib.h>

#define LUN 0

enum channel_dir {
	INPUT  = 0,
	OUTPUT = 1
};

int main(int argc, char *argv[], char *envp[])
{
	int fd;
	int ret;
	short wbuff, rbuff;

	fd = icv196_get_handle();
	if (fd < 0) {
		printf("Can't get library handle...\n");
		exit(EXIT_FAILURE);
	}
	if (icv196_init_channel(fd, LUN, 3, 2, OUTPUT)) {
		printf("icv196_init_channel() failed\n");
		return -1;
	}
	if (icv196_init_channel(fd, LUN, 11, 2, INPUT)) {
		printf("icv196_init_channel() failed\n");
		return -1;
	}

	wbuff = 56;
	printf("Will write %hd\n", wbuff);
	ret = icv196_write_channel(fd, LUN, 3, 2, (char*)&wbuff);
	if (ret) {
		printf("icv196_write_channel() failed\n");
		return -1;
	}

	usleep(500); /* delay for HW to do the job */
	ret = icv196_read_channel(fd, LUN, 11, 2, (char*)&rbuff);
	if (ret < 0) {
		printf("icv196_read_channel() failed\n");
		return -1;
	}

	printf("Write %hd, read back %hd\n", wbuff, rbuff);
	exit(EXIT_SUCCESS);
}
