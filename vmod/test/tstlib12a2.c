#include <sys/ioctl.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#ifdef ppc4
	#include "libvmod12a22dioaio.h"
#else
	#include "libvmod12a2.h"
#endif


/* VMOD 12A2 precision */
#define VMOD_DAC_INPUT_RANGE (1<<12)

int volts2digital(float volts)
{
        unsigned int digital = VMOD_DAC_INPUT_RANGE*(volts+10.0)/20.0;

        if (digital >= VMOD_DAC_INPUT_RANGE)
                return -ERANGE;
        else
                return digital;
}

void usage(char *progname)
{
	fprintf(stderr, "usage: %s lun channel volts\n", progname);
}

int main(int argc, char *argv[])
{
	int lun, channel, digital;
	float volts;
	int fd, err;

	if (argc != 4) {
		usage(argv[0]);
		return 1;
	}
	lun     = atoi(argv[1]);
	channel = atoi(argv[2]);
        volts   = atof(argv[3]);
	digital = volts2digital(volts);
	if (digital < 0) {
		usage(argv[0]);
		return 1;
	}

	if ((fd = vmod12a2_get_handle(lun)) < 0) {
		printf("could not open lun %d\n", lun);
		return 1;
	}

	printf("writing %6.3f volts (digital 0x%04x = %d)"
		"to channel %d \n",
		volts, digital, digital, channel);
	if ((err = vmod12a2_convert(fd, channel, digital)) != 0) {
		fprintf(stderr, "vmod12a2_convert exit status %d\n", err);
		return 1;
	}
	return 0;
}

