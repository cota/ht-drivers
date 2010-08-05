/*
 * segments.c
 *
 * configure the number of segments of an sis33xx.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <libsis33.h>
#include "my_stringify.h"

#define PROGNAME	"segments"

/* default module number (LUN) */
#define MODULE_NR	0

static int			module_nr = MODULE_NR;
static unsigned int		segments;
extern char *optarg;

static const char usage_string[] =
	"configure the number of segments of an sis33 device.\n"
	" " PROGNAME " [-h] [-m<LUN>] [-n<NR_SEGMENTS>]";

static const char commands_string[] =
	"options:\n"
	" -h = show this help text\n"
	" -m = Module number (default: " my_stringify(MODULE_NR) ")\n"
	" -n = Number of segments";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;

	for (;;) {
		c = getopt(argc, argv, "hm:n:");
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'm':
			module_nr = strtol(optarg, NULL, 0);
			break;
		case 'n':
			segments = strtoul(optarg, NULL, 0);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	sis33_t *dev;
	unsigned int segments_max;
	unsigned int segments_min;

	parse_args(argc, argv);

	/* log error messages */
	sis33_loglevel(3);

	dev = sis33_open(module_nr);
	if (dev == NULL)
		exit(EXIT_FAILURE);

	if (segments && sis33_set_nr_segments(dev, segments) < 0)
		exit(EXIT_FAILURE);

	if (sis33_get_nr_segments(dev, &segments) < 0)
		exit(EXIT_FAILURE);
	if (sis33_get_max_nr_segments(dev, &segments_max) < 0)
		exit(EXIT_FAILURE);
	if (sis33_get_min_nr_segments(dev, &segments_min) < 0)
		exit(EXIT_FAILURE);
	printf("Segments: %d (max. %d min. %d)\n", segments, segments_max, segments_min);

	if (sis33_close(dev))
		exit(EXIT_FAILURE);

	return 0;
}
