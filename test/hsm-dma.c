/*
 * hsm-dma.c - DMA throughput measurement of an HSM 8170 from CES.
 *
 * This module has 1MB of memory which can be accessed in BLT and SCT modes.
 *
 * NOTE: In our tests we have found that this module doesn't work reliably
 * in BLT mode: DMAing to the board and then reading back again through
 * DMA yields non-consistent results; sometimes the read values are off
 * with respect to the block that was written by one 'int'.
 *
 * Anyway, since this test is only concerned about DMA throughput, data
 * coherency is not checked at all. We just transfer zeroes back and forth.
 */

#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <libvmebus.h>

#define HSM_VME_ADDR	0x11000000
#define HSM_SIZE	0x100000
#define HSM_AM		VME_A32_USER_BLT
#define HSM_DW		VME_D32

struct stats {
	int	cycles;
	double	delta;
};

#define STATS_MAXLEN	100

static unsigned int	duration;
static struct stats	rd_stats;
static struct stats	wr_stats;
static time_t		init_time;

long long ts_subtract(struct timespec *a, struct timespec *b)
{
	long long ns;

	ns = (b->tv_sec - a->tv_sec) * 1000000000LL;
	ns += (b->tv_nsec - a->tv_nsec);
	return ns;
}

static void
dma_desc_init(int32_t *buf, struct vme_dma *desc, enum vme_dma_dir dir)
{
	struct vme_dma_attr *pci;
	struct vme_dma_attr *vme;

	memset(desc, 0, sizeof(struct vme_dma));
	desc->dir = dir;
	desc->length = HSM_SIZE;

	desc->ctrl.pci_block_size	= VME_DMA_BSIZE_4096;
	desc->ctrl.pci_backoff_time	= VME_DMA_BACKOFF_0;
	desc->ctrl.vme_block_size	= VME_DMA_BSIZE_4096;
	desc->ctrl.vme_backoff_time	= VME_DMA_BACKOFF_0;

	if (dir == VME_DMA_TO_DEVICE) {
		pci = &desc->src;
		vme = &desc->dst;
	} else {
		pci = &desc->dst;
		vme = &desc->src;
	}

	pci->addru	= 0;
	pci->addrl	= (unsigned int)buf;

	vme->addru	= 0;
	vme->addrl	= HSM_VME_ADDR;
	vme->am		= HSM_AM;
	vme->data_width	= HSM_DW;
}

static inline void
wr_desc_init(int32_t *buf, struct vme_dma *desc)
{
	return dma_desc_init(buf, desc, VME_DMA_TO_DEVICE);
}

static inline void
rd_desc_init(int32_t *buf, struct vme_dma *desc)
{
	return dma_desc_init(buf, desc, VME_DMA_FROM_DEVICE);
}

static inline void update_stats(struct stats *stats, unsigned long long ns)
{
	stats->cycles++;
	stats->delta += (double)ns/(double)1000000000.0;
}

static inline double calc_tput(struct stats *stats)
{
	return (double)(HSM_SIZE) * stats->cycles / stats->delta / 1024.0 /
		1024.0;
}

static void fill_entry(char *str, struct stats *stats, const char *title)
{
	snprintf(str, STATS_MAXLEN, "%s\t\t%16.6f\t%11llu\t%11.6f", title,
		calc_tput(stats), (unsigned long long)stats->cycles * HSM_SIZE,
		stats->delta);
	str[STATS_MAXLEN - 1] = '\0';
}

/*
 * Several instances of this program are likely to be called at the same
 * time. To avoid mixing up statistics coming from each of the instances,
 * we show the stats in a single printf statement.
 */
static void print_stats(void)
{
	char rd_str[STATS_MAXLEN];
	char wr_str[STATS_MAXLEN];

	fill_entry(rd_str, &rd_stats, "Read ");
	fill_entry(wr_str, &wr_stats, "Write");

	printf("\nDMA Stats\tThroughput(MB/s)\tSize(bytes)\tDuration(s)\n"
		"======================================================="
		"============\n"
		"%s\n"
		"%s\n", rd_str, wr_str);
}

static int __dma(struct vme_dma *desc, int write)
{
	unsigned long long delta_ns;
	struct timespec ts_start;
	struct timespec	ts_end;
	struct stats *stats = write ? &wr_stats : &rd_stats;
	int rc;

	clock_gettime(CLOCK_MONOTONIC, &ts_start);
	if (write)
		rc = vme_dma_write(desc);
	else
		rc = vme_dma_read(desc);

	if (rc) {
		printf("DMA %s failed: %s\n", write ? "write" : "read",
			strerror(errno));
		return -1;
	}

	clock_gettime(CLOCK_MONOTONIC, &ts_end);
	delta_ns = ts_subtract(&ts_start, &ts_end);
	update_stats(stats, delta_ns);

	return 0;
}

static inline int dma_write(struct vme_dma *desc)
{
	return __dma(desc, 1);
}

static inline int dma_read(struct vme_dma *desc)
{
	return __dma(desc, 0);
}

static void bail_out(void)
{
	print_stats();
	exit(EXIT_SUCCESS);
}

static void sighandler(int sig)
{
	return bail_out();
}

static void sighandler_init(void)
{
	struct sigaction act;

	act.sa_handler = sighandler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, 0);
	sigaction(SIGINT, &act, 0);
}

static void __check_completion(void)
{
	time_t now = time(NULL);

	if (now >= init_time + duration)
		return bail_out();
}

static inline void check_completion(void)
{
	if (!duration)
		return;
	return __check_completion();
}

int main(int argc, char *argv[])
{
	struct vme_dma	rd_desc;
	struct vme_dma	wr_desc;
	int32_t		*wr_buf = NULL;
	int32_t		*rd_buf = NULL;
	int		rc = 0;

	if (argc != 2) {
		printf("%s: measure DMA throughput the HSM 8170\n", argv[0]);
		printf("Usage: %s <duration>\n", argv[0]);
		printf("  d: duration in seconds of the test.\n"
			"     default: 0 (loop forever.)\n"
			"This program can be interrupted anytime with the "
			"TERM or INT signals\n");
		exit(EXIT_FAILURE);
	}
	sscanf(argv[1], "%d", &duration);
	sighandler_init();

	if (posix_memalign((void **)&rd_buf, 4096, HSM_SIZE)) {
		printf("Failed to allocate rd buffer: %s\n", strerror(errno));
		rc = 1;
		goto out;
	}

	if (posix_memalign((void **)&wr_buf, 4096, HSM_SIZE)) {
		printf("Failed to allocate wr buffer: %s\n", strerror(errno));
		rc = 1;
		goto out;
	}

	rd_desc_init(rd_buf, &rd_desc);
	wr_desc_init(wr_buf, &wr_desc);

	init_time = time(NULL);

	while (1) {
		rc = dma_write(&wr_desc);
		if (rc)
			goto out;
		rc = dma_read(&rd_desc);
		if (rc)
			goto out;
		check_completion();
	}
out:
	if (wr_buf)
		free(wr_buf);
	if (rd_buf)
		free(rd_buf);
	return rc;
}
