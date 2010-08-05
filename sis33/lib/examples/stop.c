/*
 * stop.c
 *
 * configure the 'stop' of an sis33 device.
 *
 * Note: this file is start.c with s/start/stop/ applied on it.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <libsis33.h>
#include "my_stringify.h"

#define PROGNAME	"stop"

/* default module number (LUN) */
#define MODULE_NR	0

static int			module_nr = MODULE_NR;
static int			auto_mode = -1;
static int			delay = -1;
extern char *optarg;

static const char usage_string[] =
	"configure the 'stop' of an sis33 device.\n"
	" " PROGNAME " [-a] [-d] [-h] [-m<LUN>]";

static const char commands_string[] =
	"options:\n"
	" -a = enable/disable auto mode (bool)\n"
	" -d = delay\n"
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
		c = getopt(argc, argv, "a:d:hm:");
		if (c < 0)
			break;
		switch (c) {
		case 'a':
			auto_mode = strtol(optarg, NULL, 0);
			break;
		case 'd':
			delay = strtol(optarg, NULL, 0);
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
	sis33_t *dev;
	int ret;

	parse_args(argc, argv);

	/* log error messages */
	sis33_loglevel(3);

	dev = sis33_open(module_nr);
	if (dev == NULL)
		exit(EXIT_FAILURE);

	if (auto_mode != -1 && sis33_set_stop_auto(dev, auto_mode) < 0)
		exit(EXIT_FAILURE);

	ret = sis33_get_stop_auto(dev, &auto_mode);
	if (ret)
		exit(EXIT_FAILURE);
	printf("Auto: %d\n", auto_mode);

	if (delay != -1 && sis33_set_stop_delay(dev, delay) < 0)
		exit(EXIT_FAILURE);

	ret = sis33_get_stop_delay(dev, &delay);
	if (ret)
		exit(EXIT_FAILURE);
	printf("Delay: %d\n", delay);

	if (sis33_close(dev))
		exit(EXIT_FAILURE);

	return 0;
}
