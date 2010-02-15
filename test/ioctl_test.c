/**
 * @file ioctl_test.c
 *
 * @brief ioctl test program to check the time it takes to serve an IOCTL
 *
 * to build the program just do:
 * gcc -lrt -o ioctl_test ioctl_test.c
 * and then call it with:
 * ./ioctl_test [n] -- default value for n: 10
 * , where n is the number of desired iterative calls to the ioctl
 *
 * @author Copyright (C) 2009 CERN CO/HT Emilio G. Cota
 * 					<emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include "../driver/xmemDrvr.h"

int clock_gettime(clockid_t clock_id, struct timespec *tp);

int XmemOpen()
{
	char fnm[32];
	int  i, fn;

	for (i = 1; i <= XmemDrvrCLIENT_CONTEXTS; i++) {
		sprintf(fnm,"/dev/XMEM%1d",i);
		if ((fn = open(fnm, O_RDWR, 0)) > 0) {
			return(fn);
		}
	}
	return 0;
}

int XmemClose(int fildes)
{
	return close(fildes);
}

int main(int argc, char *argv[])
{
	struct timespec timestart;
	struct timespec timeend;
	XmemDrvrVersion version;
	int devhandle;
	int iterations;
	double delta;
	int i;

	if (argc > 1) {
		argc--;
		argv++;
		iterations = atoi(*argv);
	}
	else
		iterations = 10;

	devhandle = XmemOpen();
	if (!devhandle) {
		printf("Cannot open /dev node. errno: %d. Exiting...\n", errno);
		exit(-1);
	}

	if (clock_gettime(CLOCK_MONOTONIC, &timestart)) {
		perror("clock_gettime 1\n");
		exit(-1);
	}

	for (i = 0; i < iterations; i++) {
		if (ioctl(devhandle, XmemDrvrGET_VERSION, &version) < 0)
			printf("IOCTL failed. iteration: %d.\n", i);
	}

	if (clock_gettime(CLOCK_MONOTONIC, &timeend)) {
		perror("clock_gettime 2\n");
		exit(-1);
	}

	if (XmemClose(devhandle)) {
		printf("Error while closing file handle. Error: %d", errno);
		exit(-1);
	}

	delta = (double)(timeend.tv_sec - timestart.tv_sec) * 1000000 +
		(double)(timeend.tv_nsec - timestart.tv_nsec)/1000.;
	printf("delta: %g us\n", delta);

	return 1;
}
