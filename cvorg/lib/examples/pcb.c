/*
 * pcb.c
 *
 * Retrieve the pcb id of a CORG module
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <libcvorg.h>
#include "my_stringify.h"

/* default module number (LUN) */
#define MODULE_NR	0

#define PROGNAME	"pcb"

static int	module_nr = MODULE_NR;
static cvorg_t *device;

extern char *optarg;

static const char usage_string[] =
	"Retrieve the pcb id of a CVORG module\n"
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
			module_nr = atoi(optarg);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	uint64_t pcb_id = 0;

	parse_args(argc, argv);

	/* Get a handle. Don't care which channel we choose */
	device = cvorg_open(module_nr, CVORG_CHANNEL_B);
	if (device == NULL) {
		cvorg_perror("cvorg_open");
		exit(EXIT_FAILURE);
	}

	if(cvorg_get_pcb_id(device, &pcb_id) < 0) {
		cvorg_perror("cvorg_get_pcb_id");
		exit(EXIT_FAILURE);
	}
	printf("CVORG pcb id 0x%016llx\n", (unsigned long long)pcb_id);
	exit(EXIT_SUCCESS);
}
