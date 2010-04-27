/*
 * freq.c
 *
 * Configure the sampling frequency of a CVORG.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libcvorg.h>
#include "my_stringify.h"

/* default module number (LUN) */
#define MODULE_NR	0

#define PROGNAME	"freq"

static int		module_nr = MODULE_NR;
static unsigned int	sampfreq;
extern char *optarg;

static const char usage_string[] =
	"Configure the sampling frequency of a CVORG module\n"
	" " PROGNAME " [-h] [-f<MHz>] [-m<LUN>]";

static const char commands_string[] =
	"options:\n"
	" -f = sampling frequency, in MHz (float values accepted)\n"
	" -h = show this help text\n"
	" -m = Module number (default: " my_stringify(MODULE_NR) ")";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;
	float input_freq;

	for (;;) {
		c = getopt(argc, argv, "f:hm:");
		if (c < 0)
			break;
		switch (c) {
		case 'f':
			if (!sscanf(optarg, "%g", &input_freq)) {
				fprintf(stderr, "Invalid frequency input\n");
				exit(EXIT_FAILURE);
			}
			sampfreq = input_freq * 1000000;
			break;
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'm':
			module_nr = strtoul(optarg, NULL, 0);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	cvorg_t *cvorg;
	unsigned int freq;

	parse_args(argc, argv);

	/*
	 * Obtain a CVORG handle.
	 * Since the sampling frequency is set for both channels,
	 * the choice of a particular channel here is irrelevant.
	 */
	cvorg = cvorg_open(module_nr, CVORG_CHANNEL_B);
	if (cvorg == NULL) {
		cvorg_perror("cvorg_open");
		exit(EXIT_FAILURE);
	}

	/* set the logging level to 3 (print errors) */
	cvorg_loglevel(3);

	/* set the sampling frequency if the user provided one */
	if (sampfreq && cvorg_set_sampfreq(cvorg, sampfreq) < 0)
		exit(EXIT_FAILURE);

	/* read back the current sampling frequency */
	if (cvorg_get_sampfreq(cvorg, &freq) < 0)
		exit(EXIT_FAILURE);
	printf("%g MHz\n", (double)freq / 1000000);

	/* close the handle */
	if (cvorg_close(cvorg))
		exit(EXIT_FAILURE);

	return 0;
}
