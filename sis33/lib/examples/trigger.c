/*
 * trigger.c
 *
 * configure and send triggers
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <libsis33.h>
#include "my_stringify.h"

#define PROGNAME	"trigger"

/* default module number (LUN) */
#define MODULE_NR	0

static int			module_nr = MODULE_NR;
static enum sis33_trigger	trigger = -1;
static int			enable_ext = -1;
extern char *optarg;

static const char usage_string[] =
	"configure and send triggers on an sis33 device.\n"
	" " PROGNAME " [-e<BOOL>] [-h] [-m<LUN>] [-s<TRIGGER>]";

static const char commands_string[] =
	"options:\n"
	" -e = enable/disable external trigger\n"
	" -d = delay\n"
	" -h = show this help text\n"
	" -m = Module number (default: " my_stringify(MODULE_NR) ")\n"
	" -s = Send trigger (valid: \"start\" and \"stop\")";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;

	for (;;) {
		c = getopt(argc, argv, "e:hm:s:");
		if (c < 0)
			break;
		switch (c) {
		case 'e':
			enable_ext = strtol(optarg, NULL, 0);
			break;
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'm':
			module_nr = strtol(optarg, NULL, 0);
			break;
		case 's':
			if (strcmp(optarg, "stop") == 0)
				trigger = SIS33_TRIGGER_STOP;
			else if (strcmp(optarg, "start") == 0)
				trigger = SIS33_TRIGGER_START;
			else {
				fprintf(stderr, "Unknown trigger '%s'\n", optarg);
				exit(EXIT_FAILURE);
			}
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	sis33_t *dev;

	parse_args(argc, argv);

	/* log error messages */
	sis33_loglevel(3);

	dev = sis33_open(module_nr);
	if (dev == NULL)
		exit(EXIT_FAILURE);

	if (trigger != -1) {
		if (sis33_trigger(dev, trigger) < 0)
			exit(EXIT_FAILURE);
		else
			printf("%s trigger sent\n", trigger == SIS33_TRIGGER_START ? "start" : "stop");
	}

	if (enable_ext != -1 && sis33_set_enable_external_trigger(dev, enable_ext) < 0)
		exit(EXIT_FAILURE);
	if (sis33_get_enable_external_trigger(dev, &enable_ext) < 0)
		exit(EXIT_FAILURE);
	printf("External: %s\n", enable_ext ? "enabled" : "disabled");

	if (sis33_close(dev))
		exit(EXIT_FAILURE);

	return 0;
}
