/**
 * @file vmeio.c
 *
 * @brief VME bus DMA support through TSI148 chip
 *
 * @author Copyright (C) 2009-2010 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 02/12/2009
 *
 * @section license_sec License
 *          Released under the GPL
 */
#define _GNU_SOURCE /* asprintf rocks */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vmebus.h>

/* error handling */
#define TSI148_LCSR_DSTA_VBE           (1<<28)  /* Error */
#define TSI148_LCSR_DSTA_ABT           (1<<27)  /* Abort */
#define TSI148_LCSR_DSTA_PAU           (1<<26)  /* Pause */
#define TSI148_LCSR_DSTA_DON           (1<<25)  /* Done */
#define TSI148_LCSR_DSTA_BSY           (1<<24)  /* Busy */
#define TSI148_LCSR_DSTA_ERRS          (1<<20)  /* Error Source */
#define TSI148_LCSR_DSTA_ERT           (3<<16)  /* Error Type */
#define TSI148_LCSR_DSTA_MASK          0x1f130000

unsigned int signum;  /* current signal number */

void sighandler(unsigned int sig)
{
	signum = sig;

	if (signum != 0) {
		printf("\nBus error\n");
		exit(0);
	}
}

/* swap bytes */
static __inline__ void __endian(const void *src, void *dest, unsigned int size)
{
	unsigned int i;
	for (i = 0; i < size; i++)
		((unsigned char*)dest)[i] = ((unsigned char*)src)[size - i - 1];
}

/* BE <-> LE convertion */
#define _ENDIAN(x)					\
({							\
	typeof(x) __x;					\
	typeof(x) __val = (x);				\
	__endian(&(__val), &(__x), sizeof(__x));	\
	__x;						\
 })

/**
 * @brief Setup DMA description
 *
 * @param t   -- operation type (r -- read, w -- write)
 * @param ka  -- VME address
 * @param am  -- VME address modifier
 * @param dps -- Data Port Size (8, 16, 32, 64)
 * @param buf -- user-space buffer to get/put data from/to
 * @param eln -- number of elements to r/w
 * @param d   -- description table to init
 */
static void setup_vme_desc(uint8_t t, void *ka, short am, uint8_t dps,
			   void *buf, int eln, struct vme_dma *d)
{
	ulong vmeaddr = (ulong) ka;
	memset(d, 0, sizeof(*d));

        if (t == 'r') { /* read */
		d->dir = VME_DMA_FROM_DEVICE;
                d->src.data_width = dps;
		d->src.am = am;
                d->src.addrl = (unsigned int) vmeaddr;
                d->dst.addrl = (unsigned int) buf;
        } else { /* write */
		d->dir = VME_DMA_TO_DEVICE;
                d->dst.data_width = dps;
                d->dst.am = am;
                d->src.addrl = (unsigned int) buf;
		d->dst.addrl = (unsigned int) vmeaddr;
        }
        d->length = eln * dps/8; /* number of bytes to r/w */
        d->novmeinc = 0; /* FIFO r/w (1 -- yes, 0 -- no) */

        d->ctrl.pci_block_size   = VME_DMA_BSIZE_4096;
        d->ctrl.pci_backoff_time = VME_DMA_BACKOFF_0;
        d->ctrl.vme_block_size   = VME_DMA_BSIZE_4096;
        d->ctrl.vme_backoff_time = VME_DMA_BACKOFF_0;
}

/**
 * @brief BE <-> LE converter
 *
 * @param rd  -- register description
 * @param eln -- number of elements to swap
 * @param buf -- data to be swapped
 */
static void swap_bytes(uint8_t dps, void *buf, int eln)
{
	uint16_t *w;
	uint32_t *dw;
	uint64_t *llw;

	switch (dps) {
	case 8:
		break;
	case 16:
		w = (uint16_t *) buf;
		while (eln--) { *w = _ENDIAN(*w); w++; }
		break;
	case 32:
		dw = (uint32_t *) buf;
		while (eln--) { *dw = _ENDIAN(*dw); dw++; }
		break;
	case 64:
		llw = (uint64_t *) buf;
		while (eln--) { *llw = _ENDIAN(*llw); llw++; }
		break;
	}
}

/**
 * @brief Decode TSI148 DMA errors. Should go into vmebus lib.
 *
 * @param desc -- VME DMA description table
 * @param ptr  -- if not NULL, error string will go here
 *
 * @note ptr (if not NULL) should be freed afterwards by the caller
 *
 * @return  0 -- no DMA error
 * @return -1 -- DMA error occurs
 */
static int vme_dma_error_decode(struct vme_dma *desc, char **ptr)
{
	char str[256];
	char *p = str;
	int rc = 0, i;
	/* Do not care about masked stuff */
	int stat = desc->status & TSI148_LCSR_DSTA_MASK;

	if (stat & TSI148_LCSR_DSTA_VBE) {
		i = sprintf(p, "%s", "VME DMA error\n");
		p += i;
		rc = -1;
		if ((stat & TSI148_LCSR_DSTA_ERRS) == 0) {
			switch (stat & TSI148_LCSR_DSTA_ERT) {
			case (0 << 16):
				i = sprintf(p, "%s", "Bus error: SCT, BLT,"
					    " MBLT, 2eVME even data, 2eSST\n");
				p += i;
				break;
			case (1 << 16):
				i = sprintf(p, "%s",
					    "Bus error: 2eVME odd data\n");
				p += i;
				break;
			case (2 << 16):
				i = sprintf(p, "%s", "Slave termination:"
					    " 2eVME even data, 2eSST read\n");
				p += i;
				break;
			case (3 << 16):
				i = sprintf(p, "%s", "Slave termination:"
					        " 2eVME odd data, 2eSST read"
					    " last word invalid\n");
				p += i;
				break;
			default:
				break;
			}
		} else {
			i = sprintf(p, "%s", "PCI/X Bus Error\n");
			p += i;
		}
	}

	if (stat & TSI148_LCSR_DSTA_ABT) {
		i = sprintf(p, "%s", "VME DMA aborted\n");
		p += i;
		rc = -1;
	}

	if (stat & TSI148_LCSR_DSTA_PAU) {
		i = sprintf(p, "%s", "DMA paused\n");
		p += i;
		rc = -1;
	}

	if (stat & TSI148_LCSR_DSTA_DON) {
		i = sprintf(p, "%s", "DMA done\n");
		p += i;
		rc = 0;
	}

	if (stat & TSI148_LCSR_DSTA_BSY) {
		i = sprintf(p, "%s", "DMA busy\n");
		p += i;
		rc = -1;
	}

	*p = '\0';

	if (ptr)
		asprintf(ptr, "%s", str);

	return rc;
}

/**
 * @brief Perform DMA access
 *
 * @param t       -- operation type (r -- read, w -- write)
 * @param vmeaddr -- vme address to r/w
 * @param am      -- VME address modifier
 * @param dps     -- Data Port Size
 * @param buf     -- buffer to get/put data from/to
 * @param eln     -- number of elements to r/w
 *
 * @return -1 -- if FAILED
 * @return  0 -- if OK
 */
int do_dma(uint8_t t, void *vmeaddr, short am, uint8_t dps, void *buf, int eln)
{
	struct vme_dma d;
	char *errstr;
	int f = open("/dev/vme_dma", O_RDWR);

	if (!f)
		return -1;

	if (t == 'w')
		/* should swap in case of write operation */
		swap_bytes(dps, buf, eln);

	setup_vme_desc(t, vmeaddr, am, dps, buf, eln, &d);

	if (ioctl(f, VME_IOCTL_START_DMA, &d) < 0) {
                perror("VME DMA access");
		close(f);
                return -1;
        }

	if (vme_dma_error_decode(&d, &errstr)) {
		fprintf(stderr, "%s\n", errstr);
		free(errstr);
		close(f);
		return -1;
	}

	free(errstr);

	/* always swap data.
	   In case of read  -- BE will come from VME
	   In case of write -- it was swapped by us at the beginning,
	   so swap it back */
	swap_bytes(dps, buf, eln);

	close(f);
	return 0;
}

void get_obligitary_params(char *argv[], void **vmeaddr, int *am, int *dps)
{
	sscanf(argv[2], "%p", vmeaddr);
        sscanf(argv[3], "%x", am);
        sscanf(argv[4], "%d", dps);
}

void do_read(int argc, char *argv[], char *envp[])
{
	void *vmeaddr;
	int am;
	int dps;
	int eln;
	void *buf;
	uint8_t *cp;
	uint16_t *sp;
	uint32_t *ip;
	uint64_t *lp;
	int i;

	if (argc != 6) {
		printf("Usage: r <VME Address> <Address Modifier>"
		       " <Data Port Size> <number of elements>\n");
		exit(EXIT_FAILURE);
	}

	get_obligitary_params(argv, &vmeaddr, &am, &dps);
	sscanf(argv[5], "%d", &eln);

	if (!(buf = calloc(eln, dps/8))) {
		perror("calloc");
		exit(1);
	}

	do_dma('r', vmeaddr, am, dps, buf, eln);

	printf("Read %d elements ", eln);
	switch (dps) {
	case 8:
		printf("one byte long:\n");
		cp = buf;
		for (i = 0; i < eln; i++, cp++)
			printf("[%d] --> %#x\n", i, *cp);
		break;
	case 16:
		printf("2 bytes long:\n");
		sp = buf;
		for (i = 0; i < eln; i++, sp++)
			printf("[%d] --> %#x\n", i, *sp);
		break;
	case 32:
		printf("4 bytes long:\n");
		ip = buf;
		for (i = 0; i < eln; i++, ip++)
			printf("[%d] --> %#x\n", i, *ip);

		break;
	case 64:
		printf("8 bytes long:\n");
		lp = buf;
		for (i = 0; i < eln; i++, lp++)
			printf("[%d] --> %#llx\n", i, *lp);
		break;
	default:
		break;
	}

	free(buf);

}

void *get_data_from_file(char *fn)
{
	printf("Should write data from '%s' file\n", fn);
	printf("Not supported yet!\n");
	return (void*)0;
}

void *get_data_from_cmd_line(char *argv[], int i, int dps, int eln)
{
	uint8_t *cp;
	uint16_t *sp;
	uint32_t *ip;
	uint64_t *lp;
	void *buf = calloc(eln, dps/8);

	switch (dps) {
	case 8:
		cp = buf;
		while (eln--)
			sscanf(argv[i++], "%x", (int*)cp++);
		break;
	case 16:
		sp = buf;
		while (eln--)
			sscanf(argv[i++], "%hx", sp++);
		break;
	case 32:
		ip = buf;
		while (eln--)
			sscanf(argv[i++], "%x", ip++);
		break;
	case 64:
		lp = buf;
		while (eln--)
			sscanf(argv[i++], "%llx", lp++);
		break;
	}

	return buf;
}

void do_write(int argc, char *argv[], char *envp[])
{
	void *vmeaddr;
	int am;
	int dps;
	int eln;
	void *buf = NULL;
	uint8_t *cp;
	uint16_t *sp;
	uint32_t *ip;
	uint64_t *lp;
	int i = 0;

	if (argc < 6) {
	usage:
		printf("Usage: w <VME Address> <Address Modifier>"
		       " <Data Port Size> [-- <elements to write>] [<filename>]\n");
		exit(EXIT_FAILURE);
	}

	while (strcmp(argv[i], "--")) {
		if (i == 5) {
			buf = get_data_from_file(argv[i]);
			break;
		}
		++i;
	}
	if (!(argc - (++i)) && !buf)
		goto usage;

	get_obligitary_params(argv, &vmeaddr, &am, &dps);

	if (!buf) {
		eln = argc - i;
		buf = get_data_from_cmd_line(argv, i, dps, eln);
	}

	printf("Will write %d element(s)@%p ", eln, vmeaddr);
	i = 0;
	switch (dps) {
	case 8:
		printf("one byte long:\n");
		cp = buf;
		while (i < eln)
			printf("[%d] --> %#x\n", i++, *cp++);
		break;
	case 16:
		printf("2 bytes long:\n");
		sp = buf;
		while (i < eln)
			printf("[%d] --> %#x\n", i++, *sp++);
		break;
	case 32:
		printf("4 bytes long:\n");
		ip = buf;
		while (i < eln)
			printf("[%d] --> %#x\n", i++, *ip++);
		break;
	case 64:
		printf("8 bytes long:\n");
		lp = buf;
		while (i < eln)
			printf("[%d] --> %#llx\n", i++, *lp++);
		break;
	}

	do_dma('w', vmeaddr, am, dps, buf, eln);
}

int main(int argc, char *argv[], char *envp[])
{
	char op;
	struct sigaction sigact;

	sigact.sa_handler = (void(*)(int))sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	if (sigaction(SIGBUS, &sigact, (struct sigaction *)NULL)) {
		perror("sigaction(1)");
		exit(1);
	}

	if (sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL)) {
		perror("sigaction(1)");
		exit(1);
	}


	sscanf(argv[1], "%c", &op);

	switch (op) {
	case 'r':
		do_read(argc, argv, envp);
		break;
	case 'w':
		do_write(argc, argv, envp);
		break;
	default:
		printf("Wrong optype (%c)\n", op);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
