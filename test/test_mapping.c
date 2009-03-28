/*
 * test_mapping.L865 -c -m 0x29 -w 16 -a 0x0 -s 0x10000
 *
 * test_mapping.L865 -c -d -n 60 -m 0x29 -w 16 -a 0x0 -s 0x10000
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
struct vme_mapping mapping;
int am = -1;
int dsize = -1;
int vmeaddr = -1;
int length = -1;
int force_create = -1;
int force_destroy = -1;
int snooze = 0;

#define VME_MWINDOW_DEV "/dev/vme_mwindow"

void usage()
{
	printf("Usage: %s [-c] [-d] [-n snooze] -m am -w dwidth "
	       "-a vmeaddr -s size\n",
	       prgname);
	printf(" -c:             force creation of window if needed\n");
	printf(" -d:             force deletion of window if needed\n");
	printf(" -n snooze:      sleep duration (default 0)\n");
	printf(" -m am:          standard VME address modifier (in hex)\n");
	printf(" -w dwidth:      Data width 16 or 32\n");
	printf(" -a vmeaddr:     VME start address (in hex)\n");
	printf(" -s size:        mapping size (in hex)\n");
	printf("\n");
}

void parse_args(int argc, char **argv)
{
	int err = 0;

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
		case 'c':
			force_create = 1;
			break;
		case 'd':
			force_destroy = 1;
			break;
		case 'n':
			if (sscanf(*argv, "%d", &snooze) != 1) {
				printf("Failed to parse snooze duration\n");
				usage();
				exit(1);
			}
			argv++;
			argc--;
			break;

		case 'm':
			if (sscanf(*argv, "%x", &am) != 1) {
				printf("Failed to parse address modifier\n");
				usage();
				exit(1);
			}
			argv++;
			argc--;
			break;

		case 'w':
			if (sscanf(*argv, "%d", &dsize) != 1) {
				printf("Failed to parse data size\n");
				usage();
				exit(1);
			}
			argv++;
			argc--;
			break;
			
		case 'a':
			if (sscanf(*argv, "%x", &vmeaddr) != 1) {
				printf("Failed to parse VME address\n");
				usage();
				exit(1);
			}
			argv++;
			argc--;
			break;
			
		case 's':
			if (sscanf(*argv, "%x", &length) != 1) {
				printf("Failed to parse length\n");
				usage();
				exit(1);
			}
			argv++;
			argc--;
			break;

		default:
			printf("Unknown option -%c\n\n", opt);
			usage();
			exit(1);
			break;
		}
	}

	if (am == -1) {
		err = 1;
		printf("No address modifier specified\n");
	}

	if (dsize == -1) {
		err = 1;
		printf("No data width specified\n");
	}

	if (vmeaddr == -1) {
		err = 1;
		printf("No VME start address specified\n");
	}

	if (length == -1) {
		err = 1;
		printf("No length specified\n");
	}

	if (err) {
		printf("\n");
		usage();
		exit(1);
	}

}

void dump_mapping(struct vme_mapping *m)
{
	printf("\n");
	printf("\tWindow number:         %d\n", m->window_num);
	printf("\tAddress modifier:      0x%0x\n", m->am);
	printf("\tData width:            %d\n", m->data_width);
	printf("\tRead prefetch enabled: %d\n", m->read_prefetch_enabled);
	printf("\tRead prefetch size:    %d\n", m->read_prefetch_size);
	printf("\t2eSST speed:           %d\n", m->v2esst_mode);
	printf("\t2eSST bcast select:    %d\n", m->bcast_select);
	printf("\tPCI address: 0x%08x:%08x\n", m->pci_addru, m->pci_addrl);
	printf("\tVME address: 0x%08x:%08x\n", m->vme_addru, m->vme_addrl);
	printf("\tSize:        0x%08x:%08x\n", m->sizeu, m->sizel);
	printf("\n");
}

int main(int argc, char **argv)
{
	int fd;
	int org_force_create;
	int org_force_destroy;

	parse_args(argc, argv);

	memset(&mapping, 0, sizeof(struct vme_mapping));

	mapping.window_num = 0;
	mapping.am = am;

	switch (dsize) {
	case 16:
		mapping.data_width = VME_D16;
		break;
	case 32:
		mapping.data_width = VME_D32;
		break;
	default:
		printf("Unsupported data width %d\n", dsize);
		exit(1);
	}

	mapping.vme_addru = 0;
	mapping.vme_addrl = vmeaddr;

	mapping.sizeu = 0;
	mapping.sizel = length;

        printf("Creating mapping\n\tVME addr: 0x%08x size: 0x%08x "
	       "AM: 0x%02x data width: %d\n\n",
	       vmeaddr, length, am, dsize);

	if ((fd = open(VME_MWINDOW_DEV, O_RDWR)) < 0) {
		printf("Failed to open %s: %s\n", VME_MWINDOW_DEV,
		       strerror(errno));
		exit(1);
	}

	if (ioctl(fd, VME_IOCTL_GET_CREATE_ON_FIND_FAIL,
		  &org_force_create) < 0) {
		printf("Failed to get force create flag: %s\n",
		       strerror(errno));
		close(fd);
		exit(1);
	}

	if (ioctl(fd, VME_IOCTL_GET_DESTROY_ON_REMOVE,
		  &org_force_destroy) < 0) {
		printf("Failed to get force destroy flag: %s\n",
		       strerror(errno));
		close(fd);
		exit(1);
	}

	if (force_create != -1) {
		if (ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL,
			  &force_create) < 0) {
			printf("Failed to set force create flag: %s\n",
			       strerror(errno));
			close(fd);
			exit(1);
		}
	}

	if (force_destroy != -1) {
		if (ioctl(fd, VME_IOCTL_SET_DESTROY_ON_REMOVE,
			  &force_destroy) < 0) {
			printf("Failed to set force destroy flag: %s\n",
			       strerror(errno));
			close(fd);
			exit(1);
		}
	}

	if (ioctl(fd, VME_IOCTL_FIND_MAPPING, &mapping) < 0) {
		printf("Failed to create mapping: %s\n", strerror(errno));
		close(fd);
		exit(1);
	}

	dump_mapping(&mapping);

	if (snooze) {
		printf("Going to sleep for %ds\n", snooze);
		sleep(snooze);
		printf("Back to business\n");
	}

	if (ioctl(fd, VME_IOCTL_RELEASE_MAPPING, &mapping) < 0) {
		printf("Failed to release mapping: %s\n", strerror(errno));
		close(fd);
		exit(1);
	}

	/* Restore original force flags */
	if (ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL,
		  &org_force_create) < 0) {
		printf("Failed to restore force_create flag: %s\n",
		       strerror(errno));
		close(fd);
		exit(1);
	}

	if (ioctl(fd, VME_IOCTL_SET_DESTROY_ON_REMOVE,
			&org_force_destroy) < 0) {
		printf("Failed to restore force_destroy flag: %s\n",
		       strerror(errno));
		close(fd);
		exit(1);
	}


	close(fd);

	return 0;
}
