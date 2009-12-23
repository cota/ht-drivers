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
 * time_read_samples() - Measure the time it takes to read the samples
 * @first_frame: First frame to read
 * @num_frames: Number of frames to read
 * @num_chans: Number of channels to read
 *
 * @note One frame is 32 samples which makes it 2 samples for each channel.
 *
 */
int time_read_samples(unsigned int first_frame, unsigned int num_frames,
		      unsigned int num_chans)
{
	int rc = 0;
	int i, j;
	double total_time = 0.0;
	double total_throughput;

	if (num_frames <= 0)
		return -1;

	/* Set readstart and readlen */
	if ((rc = vd80_set_readstart(first_frame)))
		return rc;

	if ((rc = vd80_set_readlen(num_frames - 1)))
		return rc;

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
	printf("Starting samples readout\n");

	for (i = 0; i < num_chans; i++) {

		/* Select the channel to read from */
		if ((rc = vd80_cmd_read(i + 1)))
			goto out_free;

		/* Read the samples */
		clock_gettime(CLOCK_MONOTONIC,&read_start_time[i]);

		for (j = 0; j < num_frames; j++)
			read_buf[i][j] = vd80_get_sample();

		clock_gettime(CLOCK_MONOTONIC,&read_end_time[i]);
	}

	printf("Done reading %d samples on %d channels\n\n",
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
	if ((rc = test_sampling(15000000)))
		goto out;

	/* Check we've got at least 1M samples */
	if (shot_samples < (1024*1024)) {
		printf("No samples recorded - exiting\n");
		rc = 1;
		goto out;
	}

	/* Make sure we won't be swapped */
	if (mlockall(MCL_CURRENT|MCL_FUTURE)) {
		printf("Failed to lock process address space: %s\n",
		       strerror(errno));
		printf("Results may not be accurate\n");
	}

	/* Read 1M samples (512K frames) from 1 channel */
	if ((rc = time_read_samples(0, 512*1024, 16)))
		goto out;

out:
	rc = vd80_exit();

	munlockall();

	return rc;
}
