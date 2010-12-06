/*
 * dac.c
 *
 * Retrieve and set the DAC's configuration of a CVORG module
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <cvorg.h>
#include <libcvorg.h>
#include "my_stringify.h"

/* default module number (LUN) */
#define MODULE_NR	0

#define PROGNAME	"dac"

static int	module_nr = MODULE_NR;
static int	channel = CVORG_CHANNEL_A;
static int	offset = -1;
static int	gain = -1;
static cvorg_t *device;

extern char *optarg;

static const char usage_string[] =
	"Retrieve DAC's configuration of CVORG module"
	" and change the value of the offset\n"
	" " PROGNAME " [-h] [-m<LUN>] [-c<CHAN>] [-o<OFFSET>]";

static const char commands_string[] =
	"options:\n"
	" -c = Channel (A -> 1, B -> 2)\n"
	" -h = show this help text\n"
	" -m = Module number (default: " my_stringify(MODULE_NR) ")\n"
	" -o = Offset";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;

	for (;;) {
		c = getopt(argc, argv, "c:hg:m:o:");
		if (c < 0)
			break;
		switch (c) {
		case 'c':
			channel = atoi(optarg);
			break;
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'g':
			gain = atoi(optarg);
			break;
		case 'm':
			module_nr = atoi(optarg);
			break;
		case 'o':
			offset = atoi(optarg);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	struct cvorg_dac conf;
	int ret;

	parse_args(argc, argv);

	/* Get a handle. Don't care which channel we choose */
	device = cvorg_open(module_nr, channel);
	if (device == NULL) {
		cvorg_perror("cvorg_open");
		exit(EXIT_FAILURE);
	}

	ret = cvorg_dac_get_conf(device, &conf);
	if (ret) {
		cvorg_perror("cvorg_dac_get_conf");
		exit(EXIT_FAILURE);
	}
	printf("CVORG DAC channel %d, gain 0x%x, offset 0x%x \n",
			channel,
			conf.gain,
			conf.offset);

	if(offset >= 0)
		conf.offset = offset;

	if(gain >= 0)
		conf.gain = gain;

	ret = cvorg_dac_set_conf(device, conf);
	if (ret) {
		cvorg_perror("cvorg_dac_set_conf");
		exit(EXIT_FAILURE);
	}

	ret = cvorg_dac_get_conf(device, &conf);
	if (ret) {
		cvorg_perror("cvorg_dac_get_conf");
		exit(EXIT_FAILURE);
	}
	printf("CVORG DAC channel %d, gain 0x%x, offset 0x%x \n",
			channel,
			conf.gain,
			conf.offset);


	exit(EXIT_SUCCESS);
}
