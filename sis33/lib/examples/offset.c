/*
 * offset.c
 *
 * configure the channel offsets of an sis33 device
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <libsis33.h>
#include "my_stringify.h"

#define PROGNAME	"offset"

/* default module number (LUN) */
#define MODULE_NR	0

static int			module_nr = MODULE_NR;
static int			channel_nr = -2;
static int			offset = -1;
extern char *optarg;

static const char usage_string[] =
	"configure and send triggers on an sis33 device.\n"
	" " PROGNAME " [-c<CHANNEL>] [-h] [-m<LUN>] [o<OFFSET>]";

static const char commands_string[] =
	"options:\n"
	" -c = channel index (0 to n-1)\n"
	" -h = show this help text\n"
	" -m = Module number (default: " my_stringify(MODULE_NR) ")\n"
	" -o = offset";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;

	for (;;) {
		c = getopt(argc, argv, "c:hm:o:");
		if (c < 0)
			break;
		switch (c) {
		case 'c':
			channel_nr = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'm':
			module_nr = strtol(optarg, NULL, 0);
			break;
		case 'o':
			offset = strtoul(optarg, NULL, 0);
			break;
		}
	}
}

static int set_offset(sis33_t *dev, int channel, unsigned int offs)
{
	if (channel < -1 || channel >= sis33_get_nr_channels(dev)) {
		fprintf(stderr, "Invalid channel index %d\n", channel);
		return -1;
	}
	if (channel == -1)
		return sis33_channel_set_offset_all(dev, offs);
	else
		return sis33_channel_set_offset(dev, channel, offs);
}

int main(int argc, char *argv[])
{
	sis33_t *dev;
	unsigned int val;
	int i;

	parse_args(argc, argv);

	/* log error messages */
	sis33_loglevel(3);

	dev = sis33_open(module_nr);
	if (dev == NULL)
		exit(EXIT_FAILURE);

	if (offset != -1 && set_offset(dev, channel_nr, offset) < 0)
		exit(EXIT_FAILURE);

	for (i = 0; i < sis33_get_nr_channels(dev); i++) {
		if (sis33_channel_get_offset(dev, i, &val) < 0)
			exit(EXIT_FAILURE);
		printf("%d\t0x%-8x (%d)\n", i, val, val);
	}

	if (sis33_close(dev))
		exit(EXIT_FAILURE);

	return 0;
}
