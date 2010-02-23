/*
 * testvd80.c - simple VD80 test.
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
#include <libvd80.h>

#define CHAN_USED	16
#define MAX_MBLT_FRAMES	492
#define MAX_BLT_FRAMES	44
#define SAMPLE_TIME 15000000

unsigned int shot_samples;

unsigned int *read_buf[CHAN_USED];

struct timespec read_start_time[CHAN_USED];
struct timespec read_end_time[CHAN_USED];

long long timespec_subtract(struct timespec * a, struct timespec *b)
{
   	long long ns;

	ns = (b->tv_sec - a->tv_sec) * 1000000000LL;
   	ns += (b->tv_nsec - a->tv_nsec);
   	return ns;
}

/**
 * test_sampling() - Run a sampling test
 * @duration: Sampling duration in us
 */
int test_sampling(unsigned int duration)
{
	int rc;
	struct timespec ts;

	ts.tv_sec = duration / 1000000;
	ts.tv_nsec = duration * 1000 - (ts.tv_sec * 1000000000); 

	/* Start sampling */
	printf("Starting sampling for %d us\n", duration);

	if ((rc = vd80_cmd_start()))
		return 1;

	/* Wait for a while */
	clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);

	/* Trigger */
	if ((rc = vd80_cmd_trig()))
		return 1;

	printf("Sampling stopped\n");

	/* Get the number of samples */
	shot_samples = vd80_get_shot_length();

	printf("Recorded %d samples\n", shot_samples);

	return 0;
}

/**
 * dump_samples() - Dump the samples read from the channels
 * @num_frames: number of frames
 * @num_chans: number of channels
 */
void
dump_samples(unsigned int num_frames, unsigned int num_chans)
{
	int i, j;

	printf("samp ");

	for (i = 0; i < num_chans; i++)
		printf("Ch%-2d ", i + 1);

	printf("\n");

	for (i = 0; i < num_frames; i++) {
		printf("%4d", i*2);

		for (j = 0; j < num_chans; j++)
			printf(" %.4x",
			       swapbe16((read_buf[j][i] >> 16) & 0xffff));

		printf("\n");

		printf("%4d", i*2 + 1);

		for (j = 0; j < num_chans; j++)
			printf(" %.4x",
			       swapbe16(read_buf[j][i] & 0xffff));

		printf("\n");
	}

}

/**
 * read_samples_dma() - Read recorded samples
 * @first_frame: First frame to read
 * @num_frames: Number of frames to read
 *
 * @note One frame is 32 samples which makes it 2 samples for each channel.
 *
 */
int time_read_samples_dma(unsigned int first_frame, unsigned int num_frames,
			  unsigned int num_chans)
{
	int rc = 0;
	int i, j;
	struct vme_dma dma_desc;
	unsigned int rem_start;
	unsigned int rem_length;
	double total_time = 0.0;
	double total_throughput;

	if (num_frames <= 0)
		return -1;

	/* Set readstart and readlen */
	if ((rc = vd80_set_readstart(first_frame)))
		return rc;

	if ((rc = vd80_set_readlen(num_frames - 1)))
		return rc;

	/* Setup the common parts of the DMA descriptor */
	memset(&dma_desc, 0, sizeof(struct vme_dma));
	dma_desc.dir = VME_DMA_FROM_DEVICE;
	dma_desc.novmeinc = 1;

	dma_desc.src.data_width = VD80_DMA_DW;
	dma_desc.src.am = VD80_DMA_AM;
	dma_desc.src.addru = 0;
	dma_desc.dst.addru = 0;

	dma_desc.ctrl.pci_block_size = VME_DMA_BSIZE_4096;
	dma_desc.ctrl.pci_backoff_time = VME_DMA_BACKOFF_0;
	dma_desc.ctrl.vme_block_size = VME_DMA_BSIZE_4096;
	dma_desc.ctrl.vme_backoff_time = VME_DMA_BACKOFF_0;

	/* Allocate memory for the buffers */
	for (i = 0; i < num_chans; i++) {
		read_buf[i] = NULL;

		if (posix_memalign((void **)&read_buf[i], 4096,
				   num_frames * 4)) {
			printf("Failed to allocate buffer for channel %d: %s\n",
			       i+1, strerror(errno));
			rc = 1;
			goto out_free;
		}
	}

	/* Read the samples for each channel */
	printf("Starting samples DMA readout of %d frames\n", num_frames);

	for (i = 0; i < num_chans; i++) {

		/* Read the samples */
		clock_gettime(CLOCK_MONOTONIC,&read_start_time[i]);

		/* Select the channel to read from */
		if ((rc = vd80_cmd_read(i + 1)))
			goto out_free;

		/*
		 * DMA read MAX_MBLT_FRAMES frames (MAX_MBLT_FRAMES 32-bit
		 * words) at a time
		 * */
		for (j = 0; j < num_frames/MAX_MBLT_FRAMES; j++) {
			/* Setup the DMA descriptor */
			dma_desc.src.addrl = VD80_DMA_BASE;
			dma_desc.dst.addrl = (unsigned int)(read_buf[i] +
							    (j*MAX_MBLT_FRAMES));
			dma_desc.length = MAX_MBLT_FRAMES * 4;

/* 			printf("DMA read frame %d len %d -> 0x%08x\n", */
/* 			       j * MAX_MBLT_FRAMES, MAX_MBLT_FRAMES*4, */
/* 			       dma_desc.dst.addrl); */

			/* DMA read the samples */
			if ((rc = vme_dma_read(&dma_desc))) {
				printf("Failed to read samples for channel %d: "
				       "%s\n", i + 1, strerror(errno));
				rc = 1;
				goto out_free;
			}
		}

		/* Get the remaining samples */
		rem_start = (num_frames/MAX_MBLT_FRAMES) * MAX_MBLT_FRAMES;
		rem_length = num_frames - rem_start;

		if (rem_length) {

			/* Setup the DMA descriptor */
			dma_desc.src.addrl = VD80_DMA_BASE;
			dma_desc.dst.addrl = (unsigned int)(read_buf[i] +
							    rem_start);
			dma_desc.length = rem_length * 4;

/* 			printf("DMA read frame %d len %d -> 0x%08x\n", */
/* 			       rem_start, rem_length, dma_desc.dst.addrl); */

			/* DMA read the samples */
			if ((rc = vme_dma_read(&dma_desc))) {
				printf("Failed to read samples for channel %d: "
				       "%s\n", i + 1, strerror(errno));
				rc = 1;
				goto out_free;
			}
		}

		clock_gettime(CLOCK_MONOTONIC,&read_end_time[i]);
	}

	printf("Done reading %d samples on %d channels\n",
	       num_frames * 2, num_chans);

	printf("Throughput: \n");

	/* Display throughput */
	for (i = 0; i < num_chans; i++) {
		unsigned long long delta_ns;
		double delta;
		double throughput;

		delta_ns = timespec_subtract(&read_start_time[i],
					     &read_end_time[i]);
		delta = (double)delta_ns/(double)1000000000.0;

		throughput = (double)(num_frames * 2) / delta;

		printf("  Ch%-2d: %.6fs - %.6f MSPS\n",
		       i+1, delta, throughput/(1024.0*1024.0));
		total_time += delta;
	}

	total_throughput = (double)(num_frames * 2 * num_chans) / total_time;
	printf("\n");
	printf("Total: %.6f s %.6f MSPS\n",
	       total_time, total_throughput/(1024.0*1024.0));
	printf("\n");

out_free:
	for (i = 0; i < num_chans; i++) {
		if (read_buf[i])
			free(read_buf[i]);
	}

	return rc;
}

int main(int argc, char **argv)
{
	int rc = 0;

	if (vd80_map())
		exit(1);

	if (vd80_init())
		exit(1);

	vd80_cmd_setbig(0);

	/* Run a 15 s recording to be sure to get 1M samples */
	if ((rc = test_sampling(SAMPLE_TIME)))
		goto out;

	/* Check we've got at least 1M samples */
	if (shot_samples < (1024*1024)) {
		printf("Not enough samples recorded - exiting\n");
		rc = 1;
		goto out;
	}

	/* Make sure we won't be swapped */
	if (mlockall(MCL_CURRENT|MCL_FUTURE)) {
		printf("Failed to lock process address space: %s\n",
		       strerror(errno));
		printf("Results may not be accurate\n");
	}

	/* Read 1M samples (512K frames) from 16 channels */
	if ((rc = time_read_samples_dma(0, 512*1024, 1)))
		goto out;

out:
	rc = vd80_exit();

	munlockall();

	return rc;
}
