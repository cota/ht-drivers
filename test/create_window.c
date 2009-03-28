/*
 * create_window.L865 0 0x29 16 0x0 0x10000
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

struct vme_mapping window;
struct vme_mapping testw;
int window_num;
int am, dsize, vmeaddr, length;

#define VME_MWINDOW_DEV "/dev/vme_mwindow"

void usage(char * prgname)
{
	printf("Usage: %s winnum am dwidth vmeaddr size\n", prgname);
	printf(" window:         0..7\n");
	printf(" am:             standard VME address modifier (in hex)\n");
	printf(" data width:     16 or 32\n");
	printf(" VME address:    VME start address\n");
	printf(" size:           window size\n");
}

void parse_args(int argc, char **argv)
{
	if (argc != 6) {
		usage(argv[0]);
		exit(1);
	}

	if (sscanf(argv[1], "%d", &window_num) != 1) {
		printf("Failed to parse window number\n");
		usage(argv[0]);
		exit(1);
	}

	if (sscanf(argv[2], "%x", &am) != 1) {
		printf("Failed to parse address modifier\n");
		usage(argv[0]);
		exit(1);
	}

	if (sscanf(argv[3], "%d", &dsize) != 1) {
		printf("Failed to parse data size\n");
		usage(argv[0]);
		exit(1);
	}

	if (sscanf(argv[4], "%x", &vmeaddr) != 1) {
		printf("Failed to parse VME address\n");
		usage(argv[0]);
		exit(1);
	}

	if (sscanf(argv[5], "%x", &length) != 1) {
		printf("Failed to parse length\n");
		usage(argv[0]);
		exit(1);
	}
}

int compare_windows(struct vme_mapping *m1, struct vme_mapping *m2)
{
	int err = 0;

	printf("Checking windows:\n");

	if (m1->window_num != m2->window_num) {
		err = 1;
		printf("\tWindow number mismatch %d -> %d\n",
		       m1->window_num, m2->window_num);
	}

	if (m1->am != m2->am) {
		err = 1;
		printf("Address modifier mismatch 0x%0x -> 0x%0x\n",
		       m1->am, m2->am);
	}

	if (m1->data_width != m2->data_width) {
		err = 1;
		printf("Data width mismatch %d -> %d\n",
		       m1->data_width, m2->data_width);
	}

	if (m1->read_prefetch_enabled != m2->read_prefetch_enabled) {
		err = 1;
		printf("Read prefetch enabled mismatch %d -> %d\n",
		       m1->read_prefetch_enabled, m2->read_prefetch_enabled);
	}

	if (m1->read_prefetch_size != m2->read_prefetch_size) {
		err = 1;
		printf("Read prefetch size mismatch %d -> %d\n",
		       m1->read_prefetch_size, m2->read_prefetch_size);
	}

	if (m1->v2esst_mode != m2->v2esst_mode) {
		err = 1;
		printf("2eSST speed mismatch %d -> %d\n",
		       m1->v2esst_mode, m2->v2esst_mode);
	}

	if (m1->bcast_select != m2->bcast_select) {
		err = 1;
		printf("2eSST broadcast select mismatch %d -> %d\n",
		       m1->bcast_select, m2->bcast_select);
	}

	if ((m1->pci_addrl != m2->pci_addrl) &&
	    (m1->pci_addru != m2->pci_addru)) {
		err = 1;
		printf("PCI address mismatch 0x%08x:%08x -> 0x%08x:%08x\n",
		       m1->pci_addru, m1->pci_addrl,
		       m2->pci_addru, m2->pci_addrl);
	}

	if ((m1->vme_addrl != m2->vme_addrl) &&
	    (m1->vme_addru != m2->vme_addru)) {
		err = 1;
		printf("VME address mismatch 0x%08x:%08x -> 0x%08x:%08x\n",
		       m1->vme_addru, m1->vme_addrl,
		       m2->vme_addru, m2->vme_addrl);
	}

	if ((m1->sizel != m2->sizel) &&
	    (m1->sizeu != m2->sizeu)) {
		err = 1;
		printf("Size mismatch 0x%08x:%08x -> 0x%08x:%08x\n",
		       m1->sizeu, m1->sizel,
		       m2->sizeu, m2->sizel);
	}

	if (!err)
		printf("\tOK\n\n");

	return err;
}

void dump_window(struct vme_mapping *m)
{
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
}

int main(int argc, char **argv)
{
	int fd;

	parse_args(argc, argv);

	memset(&window, 0, sizeof(struct vme_mapping));

	if ((window_num < 0) || (window_num > 7)) {
		printf("Wrong window number %d_n", window_num);
		exit(1);
	}

	window.window_num = window_num;
	window.am = am;

	switch (dsize) {
	case 16:
		window.data_width = VME_D16;
		break;
	case 32:
		window.data_width = VME_D32;
		break;
	default:
		printf("Unsupported data width %d\n", dsize);
		exit(1);
	}

	window.vme_addru = 0;
	window.vme_addrl = vmeaddr;

	window.sizeu = 0;
	window.sizel = length;

        printf("Creating window %d\n\tVME addr: 0x%08x size: 0x%08x "
	       "AM: 0x%02x data width: %d\n\n",
	       window_num, vmeaddr, length, am, dsize);

	if ((fd = open(VME_MWINDOW_DEV, O_RDWR)) < 0) {
		printf("Failed to open %s: %s\n", VME_MWINDOW_DEV,
		       strerror(errno));
		exit(1);
	}

	if (ioctl(fd, VME_IOCTL_CREATE_WINDOW, &window) < 0) {
		printf("Failed to create window: %s\n", strerror(errno));
		close(fd);
		exit(1);
	}

	memset(&testw, 0, sizeof(struct vme_mapping));

	testw.window_num = window_num;

	if (ioctl(fd, VME_IOCTL_GET_WINDOW_ATTR, &testw) < 0) {
		printf("Failed to get window attributes: %s\n",
		       strerror(errno));
		close(fd);
		exit(1);
	}

	compare_windows(&window, &testw);

	dump_window(&window);

	close(fd);

	return 0;
}
