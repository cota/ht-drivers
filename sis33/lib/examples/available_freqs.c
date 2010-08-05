/*
 * available_freqs.c
 *
 * Show the available sampling frequencies of a given device
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libsis33.h>
#include "my_stringify.h"

/* default module number (LUN) */
#define MODULE_NR	0

#define PROGNAME	"available_freqs"

static int		module_nr = MODULE_NR;
extern char *optarg;

static const char usage_string[] =
	"Show the available sampling frequencies of an sis33 device\n"
	" " PROGNAME " [-h] [-m<LUN>]";

static const char commands_string[] =
	"options:\n"
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

	for (;;) {
		c = getopt(argc, argv, "hm:");
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'm':
			module_nr = strtol(optarg, NULL, 0);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	unsigned int n_freqs;
	unsigned int *freqs;
	sis33_t *dev;
	int ret;
	int i;

	parse_args(argc, argv);

	dev = sis33_open(module_nr);
	if (dev == NULL) {
		fprintf(stderr, "Couldn't open sis33.%d\n", module_nr);
		exit(EXIT_FAILURE);
	}

	n_freqs = sis33_get_nr_clock_frequencies(dev);
	freqs = calloc(sizeof(*freqs), n_freqs);

	ret = sis33_get_available_clock_frequencies(dev, freqs, n_freqs);
	if (ret < 0) {
		fprintf(stderr, "sis33_get_available_clock_frequencies failed\n");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < n_freqs; i++) {
		printf("%f MHz\n", (double)freqs[i] / 1000000);
	}

	free(freqs);
	if (sis33_close(dev)) {
		fprintf(stderr, "sis33_close failed\n");
		exit(EXIT_FAILURE);
	}

	return 0;
}
