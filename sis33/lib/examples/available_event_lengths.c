/*
 * available_event_lengths.c
 *
 * Show the available event lengths of a given device
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libsis33.h>
#include "my_stringify.h"

/* default module number (LUN) */
#define MODULE_NR	0

#define PROGNAME	"available_event_lengths"

static int		module_nr = MODULE_NR;
static int		raw_values;
extern char *optarg;

static const char usage_string[] =
	"Show the available event lengths of an sis33 device\n"
	" " PROGNAME " [-h] [-m<LUN>]";

static const char commands_string[] =
	"options:\n"
	" -h = show this help text\n"
	" -m = Module number (default: " my_stringify(MODULE_NR) ")\n"
	" -r = show raw values (all values in bytes)";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;

	for (;;) {
		c = getopt(argc, argv, "hm:r");
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'm':
			module_nr = strtol(optarg, NULL, 0);
			break;
		case 'r':
			raw_values++;
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	unsigned int n_ev_lengths;
	unsigned int *ev_lengths;
	sis33_t *dev;
	int ret;
	int i;

	parse_args(argc, argv);

	dev = sis33_open(module_nr);
	if (dev == NULL) {
		fprintf(stderr, "Couldn't open sis33.%d\n", module_nr);
		exit(EXIT_FAILURE);
	}

	n_ev_lengths = sis33_get_nr_event_lengths(dev);
	ev_lengths = calloc(sizeof(*ev_lengths), n_ev_lengths);

	ret = sis33_get_available_event_lengths(dev, ev_lengths, n_ev_lengths);
	if (ret < 0) {
		fprintf(stderr, "sis33_get_available_event_lengths failed\n");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < n_ev_lengths; i++) {
		double mb = ev_lengths[i] / 1024 / 1024;
		double kb = ev_lengths[i] / 1024;

		if (raw_values) {
			printf("%10u\n", ev_lengths[i]);
			continue;
		}
		if (mb > 1)
			printf("%4g MB\n", mb);
		else if (kb > 1)
			printf("%4g KB\n", kb);
		else
			printf("%4u B\n", ev_lengths[i]);
	}

	free(ev_lengths);
	if (sis33_close(dev)) {
		fprintf(stderr, "sis33_close failed\n");
		exit(EXIT_FAILURE);
	}

	return 0;
}
