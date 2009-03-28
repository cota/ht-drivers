/*
 * mm6390-dma.c - Test DMA using an MM6390 memory board
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>

#include <libvmebus.h>

/* Registers address space */
#define REGS_BASE	0
#define REGS_OFFSET	0xbe00
#define REGS_SIZE	0x10000
#define REGS_AM		VME_A16_USER
#define REGS_DBW	VME_D16

/* Memory array address space */
#define MEM_BASE	0
#define MEM_SIZE	0x2000000
#define MEM_AM		VME_A32_USER_MBLT
#define MEM_DBW		VME_D32

struct vme_mapping regs_desc;
unsigned char *regs;

struct vme_dma dma_desc;
unsigned long long *wr_buf;
unsigned long long *rd_buf;

long long timespec_subtract(struct timespec * a, struct timespec *b)
{
   	long long ns;

	ns = (b->tv_sec - a->tv_sec) * 1000000000LL;
   	ns += (b->tv_nsec - a->tv_nsec);
   	return ns;
}

int main(int argc, char **argv)
{
	int rc = 0;
	int i;
	int val;
	struct timespec write_start;
	struct timespec write_end;
	struct timespec read_start;
	struct timespec read_end;
	unsigned long long delta_ns;
	double delta;
	double throughput;

	/* Open a mapping for the CSR register */
	memset(&regs_desc, 0, sizeof(struct vme_mapping));

	regs_desc.am = REGS_AM;
	regs_desc.read_prefetch_enabled = 0;
	regs_desc.data_width = REGS_DBW;
	regs_desc.sizeu = 0;
	regs_desc.sizel = REGS_SIZE;
	regs_desc.vme_addru = 0;
	regs_desc.vme_addrl = REGS_BASE;

	if ((regs = vme_map(&regs_desc, 1)) == NULL) {
		printf("Failed to map CSR register: %s\n",
		       strerror(errno));
		printf("  addr: %x size: %x am: %x dw: %d\n", REGS_BASE,
		       REGS_SIZE, REGS_AM, REGS_DBW);
		return 1;
	}

	val = regs[REGS_OFFSET+1];

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error reading CSR\n");
		rc = 1;
		goto out;
	}

	printf("CSR reg: %x\n", val);

	regs[REGS_OFFSET+1] = 0;

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error writing CSR\n");
		rc = 1;
		goto out;
	}

	/* Allocate memory for the buffers */
	if (posix_memalign((void **)&wr_buf, 4096, MEM_SIZE)) {
		printf("Failed to allocate buffer: %s\n", strerror(errno));
		rc = 1;
		goto out;
	}

	/* Allocate memory for the buffers */
	if (posix_memalign((void **)&rd_buf, 4096, MEM_SIZE)) {
		printf("Failed to allocate buffer: %s\n", strerror(errno));
		rc = 1;
		goto out;
	}

	/* Initialize the buffer */
	printf("Initializing buffer\n");
	for (i = 0; i < MEM_SIZE/8; i++)
		wr_buf[i] = i;

	/* DMA write the buffer */
	printf("DMA Writing %d Bytes\n", MEM_SIZE);
	memset(&dma_desc, 0, sizeof(struct vme_dma));

	dma_desc.dir = VME_DMA_TO_DEVICE;

	dma_desc.src.addru = 0;
	dma_desc.src.addrl = (unsigned int)wr_buf;

	dma_desc.dst.data_width = MEM_DBW;
	dma_desc.dst.am = MEM_AM;
	dma_desc.dst.addru = 0;
	dma_desc.dst.addrl = MEM_BASE;

	dma_desc.length = MEM_SIZE;

	dma_desc.ctrl.pci_block_size = VME_DMA_BSIZE_4096;
	dma_desc.ctrl.pci_backoff_time = VME_DMA_BACKOFF_0;
	dma_desc.ctrl.vme_block_size = VME_DMA_BSIZE_4096;
	dma_desc.ctrl.vme_backoff_time = VME_DMA_BACKOFF_0;

	clock_gettime(CLOCK_MONOTONIC, &write_start);

	if ((rc = vme_dma_write(&dma_desc))) {
		printf("Failed to DMA write memory: %s\n", strerror(errno));
		goto out;
	}

	clock_gettime(CLOCK_MONOTONIC, &write_end);

	/* DMA read the buffer back */
	printf("DMA Reading %d Bytes\n", MEM_SIZE);
	memset(&dma_desc, 0, sizeof(struct vme_dma));

	dma_desc.dir = VME_DMA_FROM_DEVICE;

	dma_desc.src.data_width = MEM_DBW;
	dma_desc.src.am = MEM_AM;
	dma_desc.src.addru = 0;
	dma_desc.src.addrl = MEM_BASE;

	dma_desc.dst.addru = 0;
	dma_desc.dst.addrl = (unsigned int)rd_buf;

	dma_desc.length = MEM_SIZE;

	dma_desc.ctrl.pci_block_size = VME_DMA_BSIZE_4096;
	dma_desc.ctrl.pci_backoff_time = VME_DMA_BACKOFF_0;
	dma_desc.ctrl.vme_block_size = VME_DMA_BSIZE_4096;
	dma_desc.ctrl.vme_backoff_time = VME_DMA_BACKOFF_0;

	clock_gettime(CLOCK_MONOTONIC, &read_start);

	if ((rc = vme_dma_read(&dma_desc))) {
		printf("Failed to DMA read memory: %s\n", strerror(errno));
		goto out;
	}

	clock_gettime(CLOCK_MONOTONIC, &read_end);

	/* Compare buffers */
	for (i = 0; i < MEM_SIZE/8; i++) {
		if (rd_buf[i] != wr_buf[i]) {
			rc = 1;
			break;
		}
	}

	if (rc != 0) {
		printf("Buffer mismatch at offset 0x%08x\n", i*8);

		for (i = 0; i < 10; i++)
			printf("%04x  %016llx   %016llx\n",
			       i*8, wr_buf[i], rd_buf[i]); 
	} else
		printf("Success\n\n");

	delta_ns = timespec_subtract(&write_start, &write_end);
	delta = (double)delta_ns/(double)1000000000.0;
	throughput = (double)(MEM_SIZE) / delta;

	printf("Write time: %.6fs - throughput: %.6f MB/s\n",
	       delta, throughput/(1024.0*1024.0));

	delta_ns = timespec_subtract(&read_start, &read_end);
	delta = (double)delta_ns/(double)1000000000.0;
	throughput = (double)(MEM_SIZE) / delta;

	printf("Read time: %.6fs - throughput: %.6f MB/s\n",
	       delta, throughput/(1024.0*1024.0));

out:
	if (vme_unmap(&regs_desc, 1))
		printf("Failed to unmap CSR register: %s\n",
		       strerror(errno));

	return rc;
}
	    
