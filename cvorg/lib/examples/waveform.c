/*
 * waveform.c
 *
 * Plays a waveform for a few seconds on a given module and channel.
 * The parameters of the waveform (sampling frequency, amplitude, etc.)
 * are configured through the constants below.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libcvorg.h>

#include "my_stringify.h"
#include "wv_square.h"
#include "wv_center.h"

/* module number (LUN) */
#define MODULE_NR	0

/* CVORG channel */
#define CHANNEL_NR	CVORG_CHANNEL_A

/* length of the waveform, in points */
#define LEN		35000

/* play the waveform 'RECURR' times--0 will play it forever */
#define RECURR		0

/*
 * gain: set to 1 to use 'GAIN_VAL' for the gain of the waveform.
 * When set to 0, the value in 'GAIN_VAL' is ignored.
 */
#define GAIN		0

/* gain value: in dB. */
#define GAIN_VAL	14

/* amplitude of the waveform (max: 0xffff) */
#define AMPLITUDE	0xffff

/* play the function for a certain number of seconds */
#define PLAY_SECONDS	4

#define PROGNAME	"waveform"

static int module_nr = MODULE_NR;
static int channel_nr = CHANNEL_NR;
static int play_seconds = PLAY_SECONDS;

extern char *optarg;

static const char usage_string[] =
	"Play a CVORG waveform\n"
	" " PROGNAME " [-h] [-c<CHAN>] [-m<LUN>] [-s<SECS>]";

static const char commands_string[] =
	"options:\n"
	" -c = channel number (default: " my_stringify(CHANNEL_NR) ")\n"
	" -h = show this help text\n"
	" -m = Module number (default: " my_stringify(MODULE_NR) ")\n"
	" -s = Seconds to play (default: " my_stringify(PLAY_SECONDS) ")";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;

	for (;;) {
		c = getopt(argc, argv, "c:hm:s:");
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
		case 's':
			play_seconds = atoi(optarg);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	cvorg_t *cvorg;
	struct cvorg_wv wv;
	int loglevel;

	parse_args(argc, argv);

	memset(&wv, 0, sizeof(wv));

	/* allocate memory to store the waveform */
	wv.form = malloc(LEN * sizeof(uint16_t));
	if (wv.form == NULL) {
		fprintf(stderr, "Not enough memory for the waveform\n");
		exit(EXIT_FAILURE);
	}

	/* fill in the waveform */
	cvorg_square_init(wv.form, LEN, AMPLITUDE);

	/* center the waveform */
	cvorg_center_output(wv.form, LEN, AMPLITUDE);

	/* configure the rest of the waveform descriptor */
	wv.recurr	= RECURR;
	wv.size		= LEN * sizeof(uint16_t);
	wv.dynamic_gain = GAIN;
	wv.gain_val	= GAIN_VAL;

	/* obtain a CVORG handle */
	cvorg = cvorg_open(module_nr, channel_nr);
	if (cvorg == NULL) {
		cvorg_perror("cvorg_open");
		exit(EXIT_FAILURE);
	}

	/* trace errors; keep the previous value if it's more verbose than 3 */
	loglevel = cvorg_loglevel(3);
	if (loglevel > 3)
		cvorg_loglevel(loglevel);

	/* stop whatever the module is playing */
	if (cvorg_channel_set_trigger(cvorg, CVORG_TRIGGER_STOP) < 0)
		goto out_err;

	/* store the waveform in the module */
	if (cvorg_channel_set_waveform(cvorg, &wv) < 0)
		goto out_err;

	/* enable the output */
	if (cvorg_channel_enable_output(cvorg))
		goto out_err;

	/* start playing the function */
	if (cvorg_channel_set_trigger(cvorg, CVORG_TRIGGER_START) < 0)
		goto out_err;

	sleep(play_seconds);

	/* stop playing the function */
	if (cvorg_channel_set_trigger(cvorg, CVORG_TRIGGER_STOP) < 0)
		goto out_err;

	/* close the handle */
	if (cvorg_close(cvorg))
		goto out_err;

	free(wv.form);
	exit(EXIT_SUCCESS);

 out_err:
	free(wv.form);
	exit(EXIT_FAILURE);
}
