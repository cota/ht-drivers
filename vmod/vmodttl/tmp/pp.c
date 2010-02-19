#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "skeluser_ioctl.h"
#include "vmodttl.h"

struct vmodttl_register reg;
struct vmodttl_register  *regp = &reg;

const char *devname = "/dev/vmodttl.1";

int write_value_to_a0(int fd, int value)
{
	int err;

	regp->slot = 0;
	regp->number = VMOD_TTL_CHANNEL_A;
	regp->hw_setting = VMOD_TTL_OUTPUT;
	regp->data = value;

        err = ioctl(fd, VMOD_TTL_WRITE, regp);
	if (err != 0) {
		fprintf(stderr, "ioctl failed, error %d\n", err);
		return err;
	}
	return 0;
}

int read_value_from_a0(int fd)
{
	int err;

	regp->slot = 0;
	regp->number = VMOD_TTL_CHANNEL_A;
	regp->hw_setting = VMOD_TTL_OUTPUT;
	regp->data = -1;

        err = ioctl(fd, VMOD_TTL_READ, regp);
	if (err != 0) {
		fprintf(stderr, "ioctl read failed, error %d\n", err);
		return err;
	}
	return regp->data;
}
int main(int argc, char *argv[])
{
	int fd, bit;

        fd = open(devname, O_RDWR);
        if (fd < 0) {
                fprintf(stderr, "cannot open %s\n", devname);
                return 1;
        }
	bit = 1;
	while (1) {
		printf("using ioctl 0x%x for read, 0x%x for write\n",
			VMOD_TTL_READ, VMOD_TTL_WRITE);
		write_value_to_a0(fd, bit);
		printf("wrote %d to a0\n", bit);
		printf("read %d from a0\n", read_value_from_a0(fd));
		bit = 1-bit;
		usleep(500000);
	}
}
