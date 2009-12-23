/*
 * Set or clear the force create and force destroy flags
 *
 *
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

char *prgname;
int force_create = -1;
int force_destroy = -1;
int show_flags = 0;

#define VME_MWINDOW_DEV "/dev/vme_mwindow"

void usage()
{
	printf("Usage: %s -c {0|1} -d {0|1} [-s]\n",
	       prgname);
	printf(" -c:    Set or clear force create flag\n");
	printf(" -d:    Set or clear force destroy flag\n");
	printf(" -s:    Show the current flags status\n");
	printf("\n");
}

void parse_args(int argc, char **argv)
{
	prgname = argv[0];

	argv++;
	argc--;

	while (argc) {
		char opt;

		if (**argv != '-') {
			printf("Unknown option prefix %s, skipping\n",
			       *argv);
			argv++;
			argc--;

			if (argc == 0) {
				printf("No more arguments\n\n");
				usage();
				exit(1);
			}
		}

		opt = *(*argv + 1);
		argv++;
		argc--;

		switch (opt) {
		case 's':
			show_flags = 1;
			break;
		case 'c':
			if (sscanf(*argv, "%d", &force_create) != 1) {
				printf("Failed to parse force create flag\n");
				usage();
				exit(1);
			}
			argv++;
			argc--;
			break;
		case 'd':
			if (sscanf(*argv, "%d", &force_destroy) != 1) {
				printf("Failed to parse force destroy flag\n");
				usage();
				exit(1);
			}
			argv++;
			argc--;
			break;		default:
			printf("Unknown option -%c\n\n", opt);
			usage();
			exit(1);
			break;
		}
	}

	if ((force_create == -1) && (force_destroy == -1) &&
	    (show_flags == 0)) {
		printf("\n\nHuh, what do you want to do?\n\n");
		usage();
		exit(1);
	}
}

int main(int argc, char **argv)
{
	int fd;
	int rc;

	parse_args(argc, argv);

	if ((fd = open(VME_MWINDOW_DEV, O_RDWR)) < 0) {
		printf("Failed to open %s: %s\n", VME_MWINDOW_DEV,
		       strerror(errno));
		exit(1);
	}

	if (show_flags) {
		int orig_force_create;
		int orig_force_destroy;

		if ((rc = ioctl(fd, VME_IOCTL_GET_CREATE_ON_FIND_FAIL,
				&orig_force_create)) < 0) {
			printf("Failed to get force create flag: %s\n",
			       strerror(errno));
			close(fd);
			exit(1);
		}

		if ((rc = ioctl(fd, VME_IOCTL_GET_DESTROY_ON_REMOVE,
				&orig_force_destroy)) < 0) {
			printf("Failed to get force destroy flag: %s\n",
			       strerror(errno));
			close(fd);
			exit(1);
		}

		printf("\nForce flags state: create %d - destroy %d\n\n",
		       orig_force_create, orig_force_destroy);
	}

	if (force_create != -1) {
		printf("%s force create flag\n",
		       (force_create == 0)?"Clearing":"Setting");

		if ((rc = ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL,
				&force_create)) < 0) {
			printf("Failed to set force create flag: %s\n",
			       strerror(errno));
			close(fd);
			exit(1);
		}
	}

	if (force_destroy != -1) {
		printf("%s force destroy flag\n",
		       (force_destroy == 0)?"Clearing":"Setting");

		if ((rc = ioctl(fd, VME_IOCTL_SET_DESTROY_ON_REMOVE,
				&force_create)) < 0) {
			printf("Failed to set force destroy flag: %s\n",
			       strerror(errno));
			close(fd);
			exit(1);
		}
	}

	close(fd);

	return 0;
}
