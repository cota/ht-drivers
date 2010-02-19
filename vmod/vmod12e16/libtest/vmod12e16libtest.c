#include <stdio.h>
#include <libvmod12e16.h>

int cmd_get_handle(lun, slot, channel)
{
        int lun, slot, channel, amplification, value;
        int fd;
        int err = 0, result;

	if (argc != 5) {
		fprintf(stderr, 
			"usage: pp lun slot channel amplification\n");
		return 1;
	}

	lun     = atoi(argv[1]);
        slot    = atoi(argv[2]);
        channel = atoi(argv[3]);
	amplification = atoi(argv[4]);

	fd = vmod12e16_get_handle(lun, slot, channel);
	if (fd < 0) {
		fprintf(stderr, "resource could not be opened\n");
		perror("error ");
		return 1;
	}
	err = vmod12e16_convert(fd, channel, amplification, &result);
	if (err < 0) {
		fprintf(stderr, "conversion failed\n");
		perror("error ");
		return 1;
	}
	printf("conversion returned value 0x%x = %0.5f volts\n",
		result, ((float)result)/(1<<12) * 10.0);
}

