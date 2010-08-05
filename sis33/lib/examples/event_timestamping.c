/*
 * event_timestamping.c
 *
 * Operate on the event timestamping configuration of an sis33 device.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <libsis33.h>
#include "my_stringify.h"

/* default module number (LUN) */
#define MODULE_NR	0

#define PROGNAME	"event_timestamping"

static int			module_nr = MODULE_NR;
static int			set_divider;
static unsigned int		divider_val;
extern char *optarg;

static const char usage_string[] =
	"Operate on the event timestamping configuration of an sis33 device\n"
	" " PROGNAME " [-d<DIVIDER>] [-h] [-m<LUN>]";

static const char commands_string[] =
	"options:\n"
	" -d = Set event timestamping divider\n"
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
		c = getopt(argc, argv, "d:hm:");
		if (c < 0)
			break;
		switch (c) {
		case 'd':
			set_divider++;
			divider_val = strtoul(optarg, NULL, 0);
			break;
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
	unsigned int maxticks_log2;
	unsigned int divider_max;
	unsigned int divider;
	sis33_t *dev;

	parse_args(argc, argv);

	/* log error messages */
	sis33_loglevel(3);

	dev = sis33_open(module_nr);
	if (dev == NULL)
		exit(EXIT_FAILURE);

	if (!sis33_event_timestamping_is_supported(dev)) {
		fprintf(stderr, "This device does not support event timestamping\n");
		exit(EXIT_FAILURE);
	}

	if (sis33_get_max_event_timestamping_divider(dev, &divider_max))
		exit(EXIT_FAILURE);
	if (sis33_get_max_event_timestamping_clock_ticks(dev, &maxticks_log2))
		exit(EXIT_FAILURE);

	if (set_divider) {
		if (divider_val > divider_max) {
			fprintf(stderr, "warning: divider %u greater than divider_max %u\n",
				divider_val, divider_max);
		}
		if (sis33_set_event_timestamping_divider(dev, divider_val))
			exit(EXIT_FAILURE);
	}

	if (sis33_get_event_timestamping_divider(dev, &divider))
		exit(EXIT_FAILURE);

	printf("divider:         %u\n", divider);
	printf("max. divider:    %u\n", divider_max);
	printf("max. clockticks: 2**%u (0x%llx)\n", maxticks_log2, (unsigned long long)1 << maxticks_log2);

	if (sis33_close(dev))
		exit(EXIT_FAILURE);

	return 0;
}
