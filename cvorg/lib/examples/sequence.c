/*
 * waveform.c
 *
 * Plays a sequence for a few seconds on a given module and channel.
 * The parameters of the sequence are configured through the constants below.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <libcvorg.h>

#include "my_stringify.h"
#include "wv_square.h"
#include "wv_sine.h"
#include "wv_center.h"

/* module number (LUN) */
#define MODULE_NR	0

/* CVORG channel */
#define CHANNEL_NR	CVORG_CHANNEL_A

/* play the sequence 'SEQ_NR' times--0 to play it forever*/
#define SEQ_NR		0

/* length of the waveform, in points */
#define LEN1		40000
#define LEN2		60000

/* amplitude of the waveform (max: 0xffff) */
#define AMP1		0xffff
#define AMP2		0xffff

/* play the waveform 'RECURR' times--0 will play it forever */
#define RECURR1		1
#define RECURR2		1

/*
 * gain: set to 1 to use 'GAIN_VAL' for the gain of the waveform.
 * When set to 0, the value in 'GAIN_VAL' is ignored.
 */
#define GAIN1		0
#define GAIN2		0

/* gain value: in dB. */
#define GAIN_VAL1	14
#define GAIN_VAL2	14

/* play the sequence for a certain number of seconds */
#define PLAY_SECONDS	4

#define PROGNAME	"sequence"

static int module_nr = MODULE_NR;
static int channel_nr = CHANNEL_NR;
static int play_seconds = PLAY_SECONDS;

extern char *optarg;

static const char usage_string[] =
	"Play a CVORG sequence\n"
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

static void
init_wv(struct cvorg_wv *wv, int len, int recurr, int gain, int gain_val)
{
	/* allocate memory to store the waveform */
	wv->form = malloc(len * sizeof(uint16_t));
	if (wv->form == NULL) {
		fprintf(stderr, "Not enough memory for the waveform\n");
		exit(EXIT_FAILURE);
	}

	/* fill in the rest of the struct */
	wv->recurr		= recurr;
	wv->size		= len * sizeof(uint16_t);
	wv->dynamic_gain	= gain;
	wv->gain_val		= gain_val;
}

int main(int argc, char *argv[])
{
	cvorg_t *device;
	struct cvorg_seq seq;
	struct cvorg_wv *wv1;
	struct cvorg_wv *wv2;
	int loglevel;

	parse_args(argc, argv);

	memset(&seq, 0, sizeof(seq));
	seq.nr = SEQ_NR;
	seq.n_waves = 2;
	seq.waves = malloc(seq.n_waves * sizeof(struct cvorg_wv));
	if (seq.waves == NULL) {
		fprintf(stderr, "Not enough memory for the sequence\n");
		exit(EXIT_FAILURE);
	}

	wv1 = &seq.waves[0];
	wv2 = &seq.waves[1];

	init_wv(wv1, LEN1, RECURR1, GAIN1, GAIN_VAL1);
	init_wv(wv2, LEN2, RECURR2, GAIN2, GAIN_VAL2);

	/* fill in the waveforms and center them */
	cvorg_square_init(wv1->form, LEN1, AMP1);
	cvorg_center_output(wv1->form, LEN1, AMP1);

	cvorg_sine_init	(wv2->form, LEN2, AMP2);
	cvorg_center_output(wv2->form, LEN2, AMP2);

	/* get a handle */
	device = cvorg_open(module_nr, channel_nr);
	if (device == NULL) {
		cvorg_perror("cvorg_open");
		goto out_err;
	}

	/* trace errors; keep the previous value if it's more verbose than 3 */
	loglevel = cvorg_loglevel(3);
	if (loglevel > 3)
		cvorg_loglevel(loglevel);

	/* stop whatever the module is playing */
	if (cvorg_channel_set_trigger(device, CVORG_TRIGGER_STOP) < 0)
		goto out_err;

	/* store the waveform in the module */
	if (cvorg_channel_set_sequence(device, &seq) < 0)
		goto out_err;

	/* enable the output */
	if (cvorg_channel_enable_output(device))
		goto out_err;

	/* start playing the function */
	if (cvorg_channel_set_trigger(device, CVORG_TRIGGER_START) < 0)
		goto out_err;

	sleep(play_seconds);

	/* stop playing the function */
	if (cvorg_channel_set_trigger(device, CVORG_TRIGGER_STOP) < 0)
		goto out_err;

	/* close the handle */
	if (cvorg_close(device))
		goto out_err;

	free(wv1->form);
	free(wv2->form);
	free(seq.waves);
	exit(EXIT_SUCCESS);

 out_err:
	free(wv1->form);
	free(wv2->form);
	free(seq.waves);
	exit(EXIT_FAILURE);
}
