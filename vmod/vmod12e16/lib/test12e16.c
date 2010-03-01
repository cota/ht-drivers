#include <stdio.h>
#include <stdlib.h>
#include "libvmod12e16.h"

int main(int argc, char *argv[])
{
	int lun, channel, ampli, value;
	int fd;
	float decimal;

	if (argc != 4) {
		fprintf(stderr, "usage: %s lun channel ampli\n", argv[0]);
		return 1;
	}

	lun     = atoi(argv[1]);
	channel = atoi(argv[2]);
	ampli   = atoi(argv[3]);

	fd = vmod12e16_get_handle(lun);

	if (fd < 0) {
		fprintf(stderr, "cannot open handle for lun %d\n", lun);
		return 1;
	}

	if (vmod12e16_convert(fd, channel, ampli, &value)) {
		fprintf(stderr, "conversion failed at fd = %d\n"
			"   lun = %d channel = %d ampli = %d\n",
				fd, lun, channel, ampli);
	}

	decimal = 20.0*((float)value/(1<<12)) - 10.0;
	printf("read: %d = %6.4f   from "
		" lun = %d channel = %d ampli = %d\n",
			value, decimal, lun, channel, ampli);
	return 0;
}

