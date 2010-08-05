/*
 * clock.c
 *
 * Operate on the clock configuration of an sis33 device.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <libsis33.h>
#include "my_stringify.h"

/* default module number (LUN) */
#define MODULE_NR	0

#define PROGNAME	"clock"

static int			module_nr = MODULE_NR;
static unsigned int		clkfreq;
static enum sis33_clksrc	clksrc = -1;
extern char *optarg;

static const char usage_string[] =
	"Operate on the clock configuration of an sis33 device\n"
	" " PROGNAME " [-f<MHz>] [-h] [-m<LUN>]";

static const char commands_string[] =
	"options:\n"
	" -f = Frequency (MHz)\n"
	" -h = show this help text\n"
	" -m = Module number (default: " my_stringify(MODULE_NR) ")\n"
	" -s = Source: \"internal\" or \"external\"";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	float freq;
	int c;

	for (;;) {
		c = getopt(argc, argv, "f:hm:s:");
		if (c < 0)
			break;
		switch (c) {
		case 'f':
			if (!sscanf(optarg, "%g", &freq)) {
				fprintf(stderr, "Invalid frequency input\n");
				exit(EXIT_FAILURE);
			}
			clkfreq = freq * 1000000;
			break;
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'm':
			module_nr = strtol(optarg, NULL, 0);
			break;
		case 's':
			if (strcmp(optarg, "internal") == 0)
				clksrc = SIS33_CLKSRC_INTERNAL;
			else if (strcmp(optarg, "external") == 0)
				clksrc = SIS33_CLKSRC_EXTERNAL;
			else {
				fprintf(stderr, "Unknown clock source '%s'\n", optarg);
				exit(EXIT_FAILURE);
			}
			break;
		}
	}
}

static void print_clksrc(const enum sis33_clksrc clksrc)
{
	printf("Source: ");
	if (clksrc == SIS33_CLKSRC_INTERNAL)
		printf("Internal\n");
	else if (clksrc == SIS33_CLKSRC_EXTERNAL)
		printf("External\n");
	else
		printf("Unknown\n");
}

int main(int argc, char *argv[])
{
	sis33_t *dev;
	int ret;

	parse_args(argc, argv);

	/* log error messages */
	sis33_loglevel(3);

	dev = sis33_open(module_nr);
	if (dev == NULL)
		exit(EXIT_FAILURE);

	if (clksrc != -1 && sis33_set_clock_source(dev, clksrc) < 0)
		exit(EXIT_FAILURE);

	ret = sis33_get_clock_source(dev, &clksrc);
	if (ret)
		exit(EXIT_FAILURE);
	print_clksrc(clksrc);

	if (clkfreq) {
		clkfreq = sis33_round_clock_frequency(dev, clkfreq, SIS33_ROUND_NEAREST);
		if (sis33_set_clock_frequency(dev, clkfreq) < 0)
			exit(EXIT_FAILURE);
	}

	if (clksrc == SIS33_CLKSRC_INTERNAL) {
		ret = sis33_get_clock_frequency(dev, &clkfreq);
		if (ret)
			exit(EXIT_FAILURE);
		printf("Frequency: %3.3g MHz\n", (double)clkfreq / 1000000);
	}

	if (sis33_close(dev))
		exit(EXIT_FAILURE);

	return 0;
}
