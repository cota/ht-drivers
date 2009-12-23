/*
 * destroy_window.L865 0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include <vmebus.h>

int window_num;

#define VME_MWINDOW_DEV "/dev/vme_mwindow"

void usage(char * prgname)
{
	printf("Usage: %s winnum\n", prgname);
	printf(" window:         0..7\n");
}

void parse_args(int argc, char **argv)
{
	if (argc != 2) {
		usage(argv[0]);
		exit(1);
	}

	if (sscanf(argv[1], "%d", &window_num) != 1) {
		printf("Failed to parse window number\n");
		usage(argv[0]);
		exit(1);
	}
}

int main(int argc, char **argv)
{
	int fd;

	parse_args(argc, argv);

	if ((window_num < 0) || (window_num > 7)) {
		printf("Wrong window number %d_n", window_num);
		exit(1);
	}

	if ((fd = open(VME_MWINDOW_DEV, O_RDWR)) < 0) {
		printf("Failed to open %s: %s\n", VME_MWINDOW_DEV,
		       strerror(errno));
		exit(1);
	}

        printf("Destroying window %d\n\n", window_num);

	if (ioctl(fd, VME_IOCTL_DESTROY_WINDOW, &window_num) < 0) {
		printf("Failed to destroy window#%d: %s\n",
			window_num, strerror(errno));
		close(fd);
		exit(1);
	}

	close(fd);

	return 0;
}
