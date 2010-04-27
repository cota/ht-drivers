/*
 * outconf.c
 *
 * Change the output configuration of a CVORG channel
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
#define PROGNAME	"outconf"

static int module_nr = MODULE_NR;
static int channel_nr = CHANNEL_NR;
static enum cvorg_outoff outoff = -1;
static int32_t outgain;
/* this is non-zero when the user provides a value for the gain */
static int set_outgain;
static int out_enable = -1;

extern char *optarg;

static const char usage_string[] =
	"Change the output configuration of a CVORG channel\n"
	" " PROGNAME " [-deh] [-c<CHAN>] [-g<GAIN>] [-m<LUN>] [-o<OFFSET>]";

static const char commands_string[] =
	"options:\n"
	" -c = channel number (default: " my_stringify(CHANNEL_NR) ")\n"
	" -d = disable the output\n"
	" -e = enable the output\n"
	" -g = output gain in dB [-22, 20]. Note: might get rounded.\n"
	" -h = show this help text\n"
	" -m = module number (default: " my_stringify(MODULE_NR) ")\n"
	" -o = output offset (0 or 2.5 V)\n"
	"Note: The offset can only be set to 2.5V when the gain has previously "
	"been set to 14dB.\n";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;
	float input_outoff;

	for (;;) {
		c = getopt(argc, argv, "c:deg:hm:o:");
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
		case 'd':
			out_enable = 0;
			break;
		case 'e':
			out_enable = 1;
			break;
		case 'g':
			if (!sscanf(optarg, "%i", &outgain)) {
				fprintf(stderr, "Invalid output gain\n");
				exit(EXIT_FAILURE);
			}
			set_outgain++;
			break;
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'm':
			module_nr = atoi(optarg);
			break;
		case 'o':
			if (!sscanf(optarg, "%g", &input_outoff)) {
				fprintf(stderr, "Invalid output offset\n");
				exit(EXIT_FAILURE);
			}
			if (input_outoff == 0)
				outoff = CVORG_OUT_OFFSET_0V;
			else if (input_outoff == 2.5)
				outoff = CVORG_OUT_OFFSET_2_5V;
			else {
				fprintf(stderr, "Invalid output offset\n");
				exit(EXIT_FAILURE);
			}
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	cvorg_t *device;
	unsigned int chanstat;

	parse_args(argc, argv);

	/* get a handle */
	device = cvorg_open(module_nr, channel_nr);
	if (device == NULL) {
		cvorg_perror("cvorg_open");
		exit(EXIT_FAILURE);
	}

	/* set the output gain if the user provided one */
	if (set_outgain) {
		if (cvorg_channel_set_outgain(device, &outgain) < 0) {
			cvorg_perror("channel_set_outgain");
			exit(EXIT_FAILURE);
		}
	} else {
		if (cvorg_channel_get_outgain(device, &outgain) < 0) {
			cvorg_perror("channel_get_outgain");
			exit(EXIT_FAILURE);
		}
	}

	printf("outgain: %d dB\n", outgain);

	/* set the output gain if the user provided one */
	if (outoff != -1 && cvorg_channel_set_outoff(device, outoff) < 0) {
		cvorg_perror("channel_set_outoff");
		exit(EXIT_FAILURE);
	}

	/* get the output offset */
	if (cvorg_channel_get_outoff(device, &outoff) < 0) {
		cvorg_perror("channel_get_outoff");
		exit(EXIT_FAILURE);
	}

	if (outoff == CVORG_OUT_OFFSET_0V)
		printf("outoff: 0 V\n");
	else
		printf("outoff: 2.5 V\n");

	/* output enable/disable */
	if (out_enable == 1 && cvorg_channel_enable_output(device)) {
		cvorg_perror("channel_enable_output");
		exit(EXIT_FAILURE);
	} else if (out_enable == 0 && cvorg_channel_disable_output(device)) {
		cvorg_perror("channel_disable_output");
		exit(EXIT_FAILURE);
	}

	/* get the output enable/disable status */
	if (cvorg_channel_get_status(device, &chanstat) < 0) {
		cvorg_perror("channel_get_status");
		exit(EXIT_FAILURE);
	}
	if (chanstat & CVORG_CHANSTAT_OUT_ENABLED)
		printf("enabled\n");
	else
		printf("disabled\n");

	/* close the handle */
	if (cvorg_close(device)) {
		cvorg_perror("cvorg_close");
		exit(EXIT_FAILURE);
	}

	return 0;
}
