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

#include <libvmebus.h>
#include <libvd80.h>

#define CHAN_USED	1

unsigned int shot_samples;
unsigned int pre_samples;
unsigned int post_samples;
unsigned int illegal_samples;

unsigned int trigger_position;
unsigned int sample_start;
unsigned int sample_length;
unsigned int read_start;
unsigned int read_length;

unsigned int *read_buf[CHAN_USED];


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

	printf("delay=%d us - tv_sec=%ld tv_nsec=%ld\n", duration,
	       ts.tv_sec, ts.tv_nsec);

	/* Start sampling */
	printf("Starting sampling\n");

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
	post_samples = vd80_get_post_length();

	if ((shot_samples < 0) || (post_samples < 0))
		return 1;

	pre_samples = shot_samples - post_samples;

	sample_start = pre_samples;
	read_start = (pre_samples / 32) * 32;

	trigger_position = pre_samples - read_start;

	sample_length = post_samples + trigger_position;
	read_length = (sample_length % 32) ? ((sample_length / 32) + 1) * 32 :
		(sample_length / 32) * 32;

	/*
	 * Illegal samples are those samples at the end of the buffer which
	 * were not recorded but will be read because the unit for reading is
	 * 32 samples.
	 */
	illegal_samples = read_length - sample_length;

	printf("Requested post-trigger samples: %d\n", vd80_get_postsamples());
	printf("Recorded shot samples: %d\n", shot_samples);
	printf("Recorded pre-trigger samples: %d\n", pre_samples);
	printf("Recorded post-trigger samples: %d\n", post_samples);
	printf("sample_start: %d\n", sample_start);
	printf("sample_length: %d\n", sample_length);
	printf("read_start: %d\n", read_start);
	printf("read_length: %d\n", read_length);
	printf("illegal_samples: %d\n", illegal_samples);
	printf("trigger_position: %d\n", trigger_position);

	return 0;
}

/**
 * dump_samples() - Dump the samples read from the CHAN_USED channels
 * @num_frames: number of frames
 *
 */
void
dump_samples(unsigned int num_frames)
{
	int i, j;

	printf("samp ");

	for (i = 0; i < CHAN_USED; i++)
		printf("Ch%-2d ", i + 1);

	printf("\n");

	for (i = 0; i < num_frames; i++) {
		printf("%4d", i*2);

		for (j = 0; j < CHAN_USED; j++)
			printf(" %.4x",
			       swapbe16((read_buf[j][i] >> 16) & 0xffff));

		printf("\n");

		printf("%4d", i*2 + 1);

		for (j = 0; j < CHAN_USED; j++)
			printf(" %.4x",
			       swapbe16(read_buf[j][i] & 0xffff));

		printf("\n");
	}

}

/**
 * read_samples() - Read recorded samples
 * @first_frame: First frame to read
 * @num_frames: Number of frames to read
 *
 * @note One frame is 32 samples which makes it 2 samples for each channel.
 *
 */
int read_samples(unsigned int first_frame, unsigned int num_frames)
{
	int rc = 0;
	int i, j;

	if (num_frames <= 0)
		return -1;

	/* Set readstart and readlen */
	if ((rc = vd80_set_readstart(first_frame)))
		return rc;

	if ((rc = vd80_set_readlen(num_frames - 1)))
		return rc;

	/* Allocate memory for the buffers */
	for (i = 0; i < CHAN_USED; i++) {
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
	for (i = 0; i < CHAN_USED; i++) {

		/* Select the channel to read from */
		if ((rc = vd80_cmd_read(i + 1)))
			goto out_free;

		/* Read the samples */
		for (j = 0; j < num_frames; j++)
			read_buf[i][j] = vd80_get_sample();

	}

	printf("Done reading %d samples on %d channels\n",
	       num_frames * 2, CHAN_USED);

	dump_samples(num_frames);

out_free:
	for (i = 0; i < CHAN_USED; i++) {
		if (read_buf[i])
			free(read_buf[i]);
	}

	return rc;
}

/**
 * read_samples_dma() - Read recorded samples
 * @first_frame: First frame to read
 * @num_frames: Number of frames to read
 *
 * @note One frame is 32 samples which makes it 2 samples for each channel.
 *
 */
int read_samples_dma(unsigned int first_frame, unsigned int num_frames)
{
	int rc = 0;
	int i;
	struct vme_dma dma_desc;

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
	dma_desc.length = num_frames * 4;
	dma_desc.novmeinc = 1;

	dma_desc.src.data_width = VD80_DMA_DW;
	dma_desc.src.am = VD80_DMA_AM;
	dma_desc.src.addru = 0;
	dma_desc.src.addrl = VD80_DMA_BASE;

	dma_desc.dst.addru = 0;

	dma_desc.ctrl.pci_block_size = VME_DMA_BSIZE_4096;
	dma_desc.ctrl.pci_backoff_time = VME_DMA_BACKOFF_0;
	dma_desc.ctrl.vme_block_size = VME_DMA_BSIZE_4096;
	dma_desc.ctrl.vme_backoff_time = VME_DMA_BACKOFF_0;

	/* Allocate memory for the buffers */
	for (i = 0; i < CHAN_USED; i++) {
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
	for (i = 0; i < CHAN_USED; i++) {
		/* Setup the DMA descriptor */
		dma_desc.dst.addrl = (unsigned int)read_buf[i];

		/* Select the channel to read from */
		if ((rc = vd80_cmd_read(i + 1)))
			goto out_free;

		/* DMA read the samples */
		if ((rc = vme_dma_read(&dma_desc))) {
			printf("Failed to read samples for channel %d: %s\n",
			       i + 1, strerror(errno));
			rc = 1;
			goto out_free;
		}

	}

	printf("Done reading %d samples on %d channels\n",
	       num_frames * 2, CHAN_USED);

	dump_samples(num_frames);

out_free:
	for (i = 0; i < CHAN_USED; i++) {
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

	/* Run a 10 s recording */
	if ((rc = test_sampling(1000000)))
		goto out;

	if (shot_samples == 0) {
		printf("No samples recorded - exiting\n");
		rc = 1;
		goto out;
	}

	/* limit for BLT is 44 frames -> 88 samples -> 176 bytes */
//	if ((rc = read_samples_dma(0, 44)))

	/* limit for MBLT is 492 frames -> 984 samples -> 1968 samples */
	/* number of frames must be a multiple of 2 (multiple of 8 bytes) */
	if ((rc = read_samples_dma(0, 492)))
		goto out;

out:
	rc = vd80_exit();

	return rc;
}
