/*
 * fourier1.c
 *
 * Calculate the first pair of Fourier Coefficients of an acquisition data set
 * and print the difference with the data set's signal power.
 *
 * The core function of this file comes from "A fast way to check whether a
 * sine wave is actually sinusoidal" by Juan David Gonzalez Cobas.
 * A copy of this document is in the doc/ directory.
 *
 * Copyright (c) 2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#define PROGNAME	"fourier1"

static double origfreq;
static double sampfreq;
static char filename[PATH_MAX];
static float *buf;

static const char usage_string[] =
	"Calculate the first pair of Fourier Coefficients of an acquisition data set\n"
	" " PROGNAME " [-f<file>] [-h] [-o<ORIGFREQ>] [-s<SAMPFREQ>]";

static const char commands_string[] =
	"options:\n"
	" -f = path to acquisition data file (default: stdin)\n"
	" -h = show this help text\n"
	" -o = Original waveform's frequency (MHz)\n"
	" -s = Sampling frequency (MHz)";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;

	for (;;) {
		c = getopt(argc, argv, "f:ho:s:");
		if (c < 0)
			break;
		switch (c) {
		case 'f':
			strncpy(filename, optarg, PATH_MAX -1);
			filename[PATH_MAX - 1] = '\0';
			break;
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'o':
			origfreq = strtod(optarg, NULL);
			origfreq *= 1000000; /* convert to Hz */
			break;
		case 's':
			sampfreq = strtod(optarg, NULL);
			sampfreq *= 1000000; /* convert to Hz */
			break;
		}
	}
	if (origfreq == 0) {
		fprintf(stderr, "The original frequency must be provided\n");
		exit(EXIT_FAILURE);
	}
	if (sampfreq == 0) {
		fprintf(stderr, "The sampling frequency must be provided\n");
		exit(EXIT_FAILURE);
	}
}

static double f(int k)
{
	return buf[k];
}

static double cos_factor(int samples, double n)
{
	double sum = 0;
	double i;

	for (i = 0; i < samples; i++)
		sum += f(i) * cos(2 * M_PI * i / n);
	return sum / samples;
}

static double sin_factor(int samples, double n)
{
	double sum = 0;
	double i;

	for (i = 0; i < samples; i++)
		sum += f(i) * sin(2 * M_PI * i / n);
	return sum / samples;
}

static unsigned int fill_buf(void)
{
	FILE *finput;
	FILE *fmem;
	char *str = NULL;
	size_t strsize = 0;
	int elems = 0;
	int val;
	int i;

	/* read from standard input and store it in a memstream */
	fmem = open_memstream(&str, &strsize);
	if (fmem == NULL) {
		fprintf(stderr, "Failed to open a memstream\n");
		exit(EXIT_FAILURE);
	}
	if (filename[0]) {
		finput = fopen(filename, "r");
		if (finput == NULL) {
			fprintf(stderr, "Cannot open '%s'\n", filename);
			perror("fopen");
			exit(EXIT_FAILURE);
		}
	} else {
		finput = stdin;
	}
	while (fscanf(finput, "0x%x\n", &val) != EOF) {
		fprintf(fmem, "0x%x\n", val);
		elems++;
	}
	if (filename[0])
		fclose(finput);
	if (elems == 0) {
		fprintf(stderr, "No samples read\n");
		exit(EXIT_FAILURE);
	}
	buf = malloc(elems * sizeof(*buf));
	if (buf == NULL) {
		fprintf(stderr, "Couldn't allocate memory\n");
		exit(EXIT_FAILURE);
	}
	fflush(fmem);
	i = 0;
	while (fscanf(fmem, "0x%x\n", &val) != EOF)
		buf[i++] = val;
	fclose(fmem);
	free(str);
	return elems;
}

static void calculate(unsigned int nr_samples, double npp, double rms)
{
	double cosf, cosf2, sinf, sinf2;
	double diff, reldiff;

	cosf = sqrt(2.0) * cos_factor(nr_samples, npp);
	sinf = sqrt(2.0) * sin_factor(nr_samples, npp);
	cosf2 = cosf * cosf;
	sinf2 = sinf * sinf;
	diff = fabs(rms - cosf2 - sinf2);
	reldiff = diff / rms;
	printf("C^2:\t%15f\n", cosf2);
	printf("S^2:\t%15f\n", sinf2);
	printf("rms:\t%15f\n", rms);
	printf("diff:\t%15f\n", diff);
	printf("reldiff:%15f\n", reldiff);
}

unsigned long gcd(unsigned long a, unsigned long b)
{
	unsigned long r;

	if (a < b) {
		unsigned long tmp = a;

		a = b;
		b = tmp;
	}
	while ((r = a % b) != 0) {
		a = b;
		b = r;
	}
	return b;
}

int main(int argc, char *argv[])
{
	unsigned int nr_samples;
	unsigned int n_base;
	double factor;
	double npp;
	float rms = 0;
	float avg = 0;
	unsigned int elems;
	int i;

	parse_args(argc, argv);

	/* number of samples per period */
	npp = sampfreq / origfreq;
	if (npp == 0) {
		fprintf(stderr, "Error: number of samples per period is 0\n");
		exit(EXIT_FAILURE);
	}
	/*
	 * n_base: number of sampfreq samples to obtain an exact number of
	 * periods of the original waveform
	 */
	factor = origfreq / gcd(sampfreq, origfreq);
	n_base = factor;
	printf("origfreq %fHz sampfreq %fHz n_samp_per_period %f n_base %u\n", origfreq, sampfreq, npp, n_base);
	/*
	 * Allocate and fill the buffer to keep the samples in memory.
	 * Since the samples we use must contain an exact number of
	 * periods of the original wave, we may discard a few samples
	 * from the end of the set.
	 */
	elems = fill_buf();
	printf("elems %u\n", elems);
	/* NOTE: integer division so that we discard some samples */
	nr_samples = (elems / n_base) * n_base;
	printf("nr_samples %u\n", nr_samples);
	if (nr_samples == 0) {
		fprintf(stderr, "Not enough samples %u (min: %u)\n", nr_samples, n_base);
		exit(EXIT_FAILURE);
	}
	if (elems < nr_samples) {
		fprintf(stderr, "elems %u < pn %u", elems, nr_samples);
		exit(EXIT_FAILURE);
	}
	/*
	 * calculate the average, the root mean square, and center the values
	 * in buf around their average.
	 */
	for (i = 0; i < nr_samples; i++)
		avg += buf[i];
	avg /= nr_samples;
	for (i = 0; i < nr_samples; i++) {
		buf[i] = buf[i] - avg;
		rms += buf[i] * buf[i];
	}
	rms /= nr_samples;

	calculate(nr_samples, npp, rms);

	return 0;
}
