/*
 * inconf.c
 *
 * Change the input configuration of a CVORG channel
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libcvorg.h>
#include "my_stringify.h"

/* default module number (LUN) */
#define MODULE_NR	0

/* default channel number */
#define CHANNEL_NR	CVORG_CHANNEL_A

/* name of this program */
#define PROGNAME	"inconf"

static int module_nr = MODULE_NR;
static int channel_nr = CHANNEL_NR;
static enum cvorg_inpol inpol = -1;

extern char *optarg;

static const char usage_string[] =
	"Change the input configuration of a CVORG channel\n"
	" " PROGNAME " [-h] [-c<CHAN>] [-p<POLARITY>] [-m<LUN>]";

static const char commands_string[] =
	"options:\n"
	" -c = channel number (default: " my_stringify(CHANNEL_NR) ")\n"
	" -h = show this help text\n"
	" -m = module number (default: " my_stringify(MODULE_NR) ")\n"
	" -p = input polarity (0 -> Positive pulses, 1 -> Negative)\n";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;
	int input_inpol;

	for (;;) {
		c = getopt(argc, argv, "c:hm:p:");
		if (c < 0)
			break;
		switch (c) {
		case 'c':
			channel_nr = atoi(optarg);
			if (!channel_nr) {
				fprintf(stderr, "Invalid channel_nr\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'm':
			module_nr = atoi(optarg);
			break;
		case 'p':
			input_inpol = atoi(optarg);
			if (input_inpol == 0)
				inpol = CVORG_POSITIVE_PULSE;
			else if (input_inpol == 1)
				inpol = CVORG_NEGATIVE_PULSE;
			else {
				fprintf(stderr, "Invalid input polarity\n");
				exit(EXIT_FAILURE);
			}
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	cvorg_t *device;

	parse_args(argc, argv);

	/* get a handle */
	device = cvorg_open(module_nr, channel_nr);
	if (device == NULL) {
		cvorg_perror("cvorg_open");
		exit(EXIT_FAILURE);
	}

	/* set the input polarity if the user provided one */
	if (inpol != -1 && cvorg_channel_set_inpol(device, inpol) < 0) {
		cvorg_perror("channel_set_inpol");
		exit(EXIT_FAILURE);
	}

	/* get the input polarity */
	if (cvorg_channel_get_inpol(device, &inpol) < 0) {
		cvorg_perror("channel_get_inpol");
		exit(EXIT_FAILURE);
	}

	if (inpol == CVORG_POSITIVE_PULSE)
		printf("polarity: Positive\n");
	else
		printf("polarity: Negative\n");

	/* close the handle */
	if (cvorg_close(device)) {
		cvorg_perror("cvorg_close");
		exit(EXIT_FAILURE);
	}

	return 0;
}
