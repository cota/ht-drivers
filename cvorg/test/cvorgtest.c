/**
 * cvorgtest.c
 *
 * CVORG driver's test program
 * Copyright (c) 2009 Emilio G. Cota <cota@braap.org>
 *
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <sys/ioctl.h>
#include <curses.h>
#include <readline/readline.h>

#include <general_usr.h>

#include <extest.h>
#include "cvorgtest.h"

#include <libad9516.h>

/* this compilation flag enables printing the test waveform before it's sent */
/* #define DEBUG_CVORG_WVSHAPES */

int use_builtin_cmds = 1;
char xmlfile[128] = "cvorg.xml";

static int twv_len = 50000;
static const struct cvorg_memtest_op cvorg_memtest_ops[] = {
	{ 0, CVORG_SRAM_SIZE_LONGS, 1, 0xffffffff, CVORG_MEMTEST_FIXED },
	{ 0, CVORG_SRAM_SIZE_LONGS, 1, 0x55555555, CVORG_MEMTEST_FIXED },
	{ 0, CVORG_SRAM_SIZE_LONGS, 1, 0xaaaaaaaa, CVORG_MEMTEST_FIXED },
	{ 0, CVORG_SRAM_SIZE_LONGS, 2, 0x55555555, CVORG_MEMTEST_FIXED },
	{ 1, CVORG_SRAM_SIZE_LONGS, 2, 0xaaaaaaaa, CVORG_MEMTEST_FIXED },
	{ 0, CVORG_SRAM_SIZE_LONGS, 2, 0x55555555, CVORG_MEMTEST_READONLY },
	{ 1, CVORG_SRAM_SIZE_LONGS, 2, 0xaaaaaaaa, CVORG_MEMTEST_READONLY },
	{ 0, CVORG_SRAM_SIZE_LONGS, 1, 0x00000000, CVORG_MEMTEST_INCREMENT },
	{ 0, CVORG_SRAM_SIZE_LONGS, 1, 0x00000000, CVORG_MEMTEST_FIXED }
};
#define CVORG_NR_MEMTEST_OPS	ARRAY_SIZE(cvorg_memtest_ops)

/*
 * ratio: 2 for 1 period, bigger for higher freqs
 */
static inline double testsine(int len, int i, double ratio)
{
	return (sin(i * M_PI / (len / ratio)) + 1) / 2.0;
}

static void twv_sine_init(uint16_t *wv, int len, int fullscale)
{
	int i;

	for (i = 0; i < len; i++)
		wv[i] = (uint16_t)(testsine(len, i, 2.0) * fullscale);
}

static void twv_square_init(uint16_t *wv, int len, int fullscale)
{
	int i;

	/* initialise the square test-wave */
	for (i = 0; i < len / 2; i++)
		wv[i] = fullscale;
	for (; i < len; i++)
		wv[i] = 0x0000;
}

static void twv_ramp_init(uint16_t *wv, int len, int fullscale)
{
	int i;

	for (i = 0; i < len; i++)
		wv[i] = ((double)fullscale * i / (len - 1));
}

/*
 * Move up a wave so that its middle point is at the middle of the
 * resulting output.
 */
static void twv_center_output(uint16_t *wv, int len, int fullscale)
{
	int i;

	if (fullscale == 0xffff)
		return;

	for (i = 0; i < len; i++)
		wv[i] += (0xffff - fullscale) / 2;
}

static void *alloc_twv(int wavenr, int len, int fullscale)
{
	void *wv;

	wv = malloc(len * sizeof(uint16_t));
	if (wv == NULL) {
		printf("Not enough memory for allocating the waveform\n");
		return NULL;
	}

	switch (wavenr) {
	case TWV_SQUARE:
		twv_square_init(wv, len, fullscale);
		break;
	case TWV_SINE:
		twv_sine_init(wv, len, fullscale);
		break;
	case TWV_RAMP:
		twv_ramp_init(wv, len, fullscale);
		break;
	default:
		printf("Invalid waveform type");
		free(wv);
		return NULL;
	}

	twv_center_output(wv, len, fullscale);

	return wv;
}

static int count_waves(struct atom *atoms)
{
	int count = 0;
	struct atom *p = atoms;

	if (p->type == Terminator)
		return 0;

	while (p->type != Terminator) {
		if (p->type == String)
			count++;
		p++;
	}

	if (count > CVORG_MAX_NR_SEQUENCES) {
		printf("The number of sequences exceeds the maximum %d.\n",
			CVORG_MAX_NR_SEQUENCES);
		return 0;
	}
	printf("waves: %d\n", count);

	return count;
}

static int get_testwave(struct atom *atoms, int *wavenr)
{
	if (!strncmp(atoms->text, "square", MAX_ARG_LENGTH))
		*wavenr = TWV_SQUARE;
	else if (!strncmp(atoms->text, "sine", MAX_ARG_LENGTH))
		*wavenr = TWV_SINE;
	else if (!strncmp(atoms->text, "ramp", MAX_ARG_LENGTH))
		*wavenr = TWV_RAMP;
	else {
		printf("Huh? (%s)\n", atoms->text);
		return -1;
	}
	return 0;
}

/*
 * Mark the gain with this magic number to point out that no specific
 * gain has been provided.
 */
#define CVORG_GAIN_MAGIC	0xdeadface

static int fill_waves(struct atom *atoms, struct cvorg_seq *seq)
{
	struct cvorg_wv *current;
	struct atom *p = atoms;
	int recurr;
	int gain;
	int wavenr;
	int len;
	int fullscale;
	int i = 0;
	int j;

	while (p->type != Terminator) {
		if (p->type != String)
			return -1;
		if (get_testwave(p, &wavenr))
			return -1;
		p++;

		/* default values */
		recurr = 1;
		gain = CVORG_GAIN_MAGIC;
		len = twv_len;
		fullscale = 0xffff;

		/* parse all the optional numeric arguments */
		j = 0;
		while (p->type == Numeric) {
			switch (j) {
			case 0:
				recurr = p->val;
				break;
			case 1:
				if (!WITHIN_RANGE(2, p->val, TEST_WV_MAXLEN)) {
					printf("The waveform length should be "
						"between 2 and %d.\n",
						TEST_WV_MAXLEN);
					return -1;
				}
				len = p->val;
				break;
			case 2:
				fullscale = 0xffff * p->val / 100;
				fullscale &= 0xffff;
				break;
			case 3:
				gain = p->val;
				break;
			default:
				break;
			}
			j++;
			p++;
		}

		current = &seq->waves[i++];

		current->recurr	= recurr;
		if (gain == CVORG_GAIN_MAGIC) {
			current->dynamic_gain	= 0;
			current->gain_val	= 0;
		} else {
			current->dynamic_gain	= 1;
			current->gain_val	= gain;
		}
		current->size	= len * 2;
		current->form	= alloc_twv(wavenr, len, fullscale);
		if (current->form == NULL)
			return -1;
	}

	return 0;
}

#ifdef DEBUG_CVORG_WVSHAPES
static void print_waveform(struct cvorg_wv *wave)
{
	uint16_t *form = wave->form;
	int len = wave->size / sizeof(uint16_t) + wave->size % sizeof(uint16_t);
	int i;

	for (i = 0; i < len; i += 1)
		printf("\twaveform[%d] = 0x%04x\t%d\n", i, form[i], form[i]);
}
#else
static void print_waveform(struct cvorg_wv *wave)
{
}
#endif


static void print_seq(struct cvorg_seq *seq)
{
	int i;
	struct cvorg_wv *wave;

	printf("sequence: seqnr=%d, waves=%d\n", seq->nr, seq->n_waves);
	for (i = 0; i < seq->n_waves; i++) {
		printf("\twave %d\n", i);
		wave = &seq->waves[i];
		printf("\t\trecurr: %d, size(bytes): %d, dyn_gain: %d, "
			"gain_val: %d\n", wave->recurr, (int)wave->size,
			wave->dynamic_gain, wave->gain_val);
		print_waveform(wave);
	}
}

static int play_seq(struct cvorg_seq *seq)
{
	struct timespec timestart, timeend;
	double delta;

	if (clock_gettime(CLOCK_REALTIME, &timestart)) {
		mperr("clock_gettime() failed for timestart");
		return -TST_ERR_SYSCALL;
	}

	if (ioctl(_DNFD, CVORG_IOCSLOADSEQ, seq) < 0) {
		mperr("Write sequence");
		return -TST_ERR_IOCTL;
	}

	if (clock_gettime(CLOCK_REALTIME, &timeend)) {
		mperr("clock_gettime() failed for timeend");
		return -TST_ERR_SYSCALL;
	}

	print_seq(seq);

	delta = (double)(timeend.tv_sec - timestart.tv_sec) * 1000000 +
		(double)(timeend.tv_nsec - timestart.tv_nsec) / 1000.;

	printf("Time elapsed: %g us\n", delta);

	return 0;
}

static void free_seq(struct cvorg_seq *seq)
{
	struct cvorg_wv *wv;
	int i;

	if (!seq->waves)
		return;

	for (i = 0; i < seq->n_waves; i++) {
		wv = &seq->waves[i];

		if (wv->form)
			free(wv->form);
	}

	free(seq->waves);
}

int h_playtest(struct cmd_desc *cmdd, struct atom *atoms)
{
	struct cvorg_seq seq;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - play a predefined test waveform\n"
			"%s [seqnr] \"wave\" [times] [length] [scale(%%)] "
			"[gain(dB)] [...]\n"
			"seqnr is the number of times the following sequence"
			" has to be played (default: 1, 0 means forever)\n"
			"type is a string representing the waveform type:\n"
			"\t\"sine\": sine wave (default: \"sine 1\")\n"
			"\t\"square\": square wave\n"
			"\t\"ramp\": ramp wave\n"
			"length is the number of points of this waveform\n"
			"\tValid values: (2-%d). The default is set with the "
			"'len' command.\n"
			"scale is the percentage over the full scale to use in this "
			"waveform.\n"
			"\tValid values: [0-100]. Default: 100.\n"
			"The gain is specified in dB.\n"
			"\tvalid gain values: [-22, 20].\n"
			"Example:\n%s 5 \"ramp\" 2 \"sine\" 1 187 80 -22 \"square\" 3\n"
			"will play 5 times the sequence formed by 2 ramps, "
			"one sine and 3 square waves.\n",
			cmdd->name, cmdd->name, TEST_WV_MAXLEN, cmdd->name);
		goto out;
	}

	/* default number of sequences */
	seq.nr = 1;

	if ((++atoms)->type == Numeric) {
		seq.nr = atoms->val;
		atoms++;
	}

	seq.n_waves = count_waves(atoms);
	if (!seq.n_waves) {
		printf("Invalid wave descriptors\n");
		return -TST_ERR_WRONG_ARG;
	}

	seq.waves = malloc(sizeof(struct cvorg_wv) * seq.n_waves);
	if (seq.waves == NULL) {
		mperr("malloc fails for allocating the waveforms' descriptors");
		return -ENOMEM;
	}

	memset(seq.waves, 0, sizeof(struct cvorg_wv) * seq.n_waves);

	if (fill_waves(atoms, &seq))
		goto out_err;

	play_seq(&seq);

	free_seq(&seq);

out:
	return 1;
out_err:
	free_seq(&seq);
	return -TST_ERR_WRONG_ARG;
}

int h_ch(struct cmd_desc *cmdd, struct atom *atoms)
{
	int channel = 0;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - select the channel (1 or 2) to work on\n"
			"%s - show the channel we're working on\n"
			"%s [nr] - select channel 'nr' (default: nr=1)\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type == Terminator)
		goto out;

	if (atoms->type == Numeric)
		channel = atoms->val;

	if (ioctl(_DNFD, CVORG_IOCSCHANNEL, &channel) < 0) {
		mperr("set channel");
		return -TST_ERR_IOCTL;
	}
out:
	if (ioctl(_DNFD, CVORG_IOCGCHANNEL, &channel) < 0) {
		mperr("get channel");
		return -TST_ERR_IOCTL;
	}
	printf("current channel: %d\n", channel);

	return 1;
}

int h_len(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - select length (in points) for each testwave\n"
			"%s - show the current length\n"
			"%s [nr] - set the length to nr (max: %d)\n",
			cmdd->name, cmdd->name, cmdd->name, (int)TEST_WV_MAXLEN);
		goto out;
	}

	if ((++atoms)->type == Terminator) {
		printf("current length (in points): %d\n", twv_len);
		goto out;
	}

	if (atoms->type == Numeric) {
		if (!WITHIN_RANGE(2, atoms->val, TEST_WV_MAXLEN)) {
			printf("The waveform length should be between 2 and "
				"%d.\n", TEST_WV_MAXLEN);
			return -TST_ERR_WRONG_ARG;
		}
		twv_len = atoms->val;
	} else {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}
out:
	return 1;
}

int h_outoff(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint32_t offset;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get/set the analog output's offset"
			"%s - show the current offset\n"
			"%s (0|1) - set the offset to:\n"
			"\t0: 0 V (default)\n"
			"\t1: 2.5 V\n",
			cmdd->name, cmdd->name, cmdd->name);
		return 1;
	}

	++atoms;
	if (atoms->type == Terminator)
		goto out;

	if (atoms->type == Numeric) {
		offset = !!atoms->val;
		if (ioctl(_DNFD, CVORG_IOCSOUTOFFSET, &offset) < 0) {
			mperr("set analog output offset");
			return -TST_ERR_IOCTL;
		}
	} else {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}

 out:
	if (ioctl(_DNFD, CVORG_IOCGOUTOFFSET, &offset) < 0) {
		mperr("get analog output offset");
		return -TST_ERR_IOCTL;
	}

	if (offset == CVORG_OUT_OFFSET_2_5V)
		printf("2.5 V\n");
	else
		printf("0 V\n");

	return 1;
}

int h_inpol(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get/set input's pulses' polarity\n"
			"%s - show the current polarity\n"
			"%s (0|1) - set the polarity to:\n"
			"\t0: positive pulses (default)\n"
			"\t1: negative pulses\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type == Terminator) {
		uint32_t polarity;

		if (ioctl(_DNFD, CVORG_IOCGINPOLARITY, &polarity) < 0) {
			mperr("get input polarity");
			return -TST_ERR_IOCTL;
		}
		printf("%s%s", polarity ? "Nega" : "Posi", "tive pulses\n");
		goto out;
	}

	if (atoms->type == Numeric) {
		uint32_t polarity = atoms->val;

		if (ioctl(_DNFD, CVORG_IOCSINPOLARITY, &polarity) < 0) {
			mperr("set input polarity");
			return -TST_ERR_IOCTL;
		}
	} else {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}
out:
	return 1;
}

static void show_mode(uint32_t status, uint32_t config)
{
	unsigned int chanmode = config & 0xf;

	printf("Mode: ");
	switch (chanmode) {
	case 0:
		printf("Off");
		break;
	case 0x1:
		printf("Normal");
		break;
	case 0x3:
		printf("DAC mode");
		break;
	case 0x8:
		printf("Test mode 1 (triangle)");
		break;
	case 0x9:
		printf("Test mode 2 (sawtooth)");
		break;
	case 0xa:
		printf("Test mode 3 (square)");
		break;
	case 0xb:
		printf("Test mode 4 (square)");
		break;
	case 0xc:
		printf("Test mode 5 (triangle, DAC clock dep)");
		break;
	default:
		printf("undef (default to off)");
		break;
	}
	printf("\n");
}

static int do_rawio(unsigned int offset, uint32_t *val)
{
	SkelDrvrRawIoBlock rio;

	memset(&rio, 0, sizeof(rio));

	rio.SpaceNumber	= 0x29;
	rio.DataWidth	= 32;
	rio.Offset	= offset;

	if (ioctl(_DNFD, SkelDrvrIoctlRAW_READ, &rio) < 0) {
		mperr("RAW_READ ioctl fails:\n"
			"space #: %d; data width:%d, offset: %d\n",
			rio.SpaceNumber, rio.DataWidth, rio.Offset);
		return -TST_ERR_IOCTL;
	}

	*val = rio.Data;

	return 0;
}

int h_chanstat(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint32_t status;
	uint32_t status_reg;
	uint32_t config_reg;
	int32_t temperature;
	int ret;


	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get the current channel's status\n", cmdd->name);
		goto out;
	}

	if (ioctl(_DNFD, CVORG_IOCGCHANSTAT, &status) < 0) {
		mperr("get_chanstat");
		return -TST_ERR_IOCTL;
	}

	ret = do_rawio(CVORG_STATUS_REG, &status_reg);
	if (ret)
		return ret;

	ret = do_rawio(CVORG_CONFIG_REG, &config_reg);
	if (ret)
		return ret;

	printf("status reg: 0x%08x\n", status_reg);

	if (status & CVORG_CHANSTAT_READY)
		printf("\tReady\n");

	show_mode(status_reg, config_reg);

	if (status & CVORG_CHANSTAT_BUSY)
		printf("\tBusy\n");
	if (status & CVORG_CHANSTAT_SRAM_BUSY)
		printf("\tSRAM Busy (SRAM being accessed by the module)\n");
	if (status & CVORG_CHANSTAT_ERR)
		printf("\tModule in Error\n");
	if (status & CVORG_CHANSTAT_ERR_CLK)
		printf("\t\tSampling clock not present or unstable\n");
	if (status & CVORG_CHANSTAT_ERR_TRIG)
		printf("\t\tStart trigger before channel is ready\n");
	if (status & CVORG_CHANSTAT_ERR_SYNC)
		printf("\t\tSynchronization FIFO empty before end of waveform\n");
	if (status & CVORG_CHANSTAT_CLKOK)
		printf("\tSampling Clock present and stable\n");
	if (status & CVORG_CHANSTAT_STARTABLE)
		printf("\tReady to accept a 'Start' trigger\n");

	if (status & CVORG_CHANSTAT_OUT_ENABLED)
		printf("\tOutput enabled\n");
	else
		printf("\tOutput disabled\n");

	printf("config reg: 0x%08x\n", config_reg);

	if (ioctl(_DNFD, CVORG_IOCGTEMPERATURE, &temperature) < 0) {
		mperr("get_temperature");
		return -TST_ERR_IOCTL;
	}
	printf("On-board Temperature: %d C\n", temperature);
out:
	return 1;
}

int h_trigger(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint32_t trig;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - send a software trigger to a channel\n"
			"%s \"string\" - send the trigger given by \"string\":\n"
			"\t\"start\": Start\n"
			"\t\"stop\": Stop\n"
			"\t\"evstop\": Event Stop\n",
			cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type != String)
		goto out;

	if (!strncmp(atoms->text, "start", MAX_ARG_LENGTH))
		trig = CVORG_TRIGGER_START;
	else if (!strncmp(atoms->text, "stop", MAX_ARG_LENGTH))
		trig = CVORG_TRIGGER_STOP;
	else if (!strncmp(atoms->text, "evstop", MAX_ARG_LENGTH))
		trig = CVORG_TRIGGER_EVSTOP;
	else {
		printf("Invalid trigger event\n");
		return -TST_ERR_WRONG_ARG;
	}

	if (ioctl(_DNFD, CVORG_IOCSTRIGGER, &trig) < 0) {
		mperr("send software trigger");
		return -TST_ERR_IOCTL;
	}
out:
	return 1;
}

static void check_pll_dividers(struct ad9516_pll *pll)
{
	if (!pll->r)
		pll->r = 1;
	if (!pll->dvco)
		pll->dvco = 1;
	if (!pll->d1)
		pll->d1 = 1;
	if (!pll->d2)
		pll->d2 = 1;
}

static void print_pll(struct ad9516_pll *pll)
{
	unsigned long n = pll->p * pll->b + pll->a;
	double fvco;

	fvco = (double)AD9516_OSCILLATOR_FREQ * n / pll->r;

	printf("debug: fref=%g, n=%lu, r=%d\n", (double)AD9516_OSCILLATOR_FREQ,
		n, pll->r);

	printf("PLL params: [P=%d, B=%d, A=%d, R = %d]\n"
		"\tdvco=%d, d1=%d, d2=%d\n",
		pll->p, pll->b, pll->a, pll->r, pll->dvco, pll->d1, pll->d2);

	printf("f_vco = f_ref * N / R, where N = PxB + A\n"
		"\tf_ref = %g Hz\n"
		"\tN = %lu\n"
		"\tf_vco = %g Hz\n",
		(double)AD9516_OSCILLATOR_FREQ, n, fvco);

	if (pll->external) {
		printf("f_out: external\n");
	} else {
		double sampfreq = fvco / pll->dvco / pll->d1 / pll->d2;

		printf("f_out: ");
		if (sampfreq < 1000000)
			printf("%g KHz", sampfreq / 1e3);
		else
			printf("%g MHz", sampfreq / 1e6);
		printf("\n");
	}

}

int h_pll(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get/set the PLL configuration\n"
			"%s - print the current configuration\n"
			"%s a b p dvco d1 d2 - configure the PLL, where:\n"
			"\ta is A in the formula below\n"
			"\tb is B in the formula below\n"
			"\tp is the pre-scaler P in the formula below\n"
			"\tdvco is the divider of the VCO\n"
			"\td1 is the first divider of the output\n"
			"\td2 is the second divider of the output\n"
			"Note: N = (PxB + A), f_vco = f_ref * (N/R), where "
			"f_ref = 40MHz.\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type == Terminator) {
		struct ad9516_pll pll;

		if (ioctl(_DNFD, CVORG_IOCGPLL, &pll) < 0) {
			mperr("get PLL config");
			return -TST_ERR_IOCTL;
		}

		check_pll_dividers(&pll);
		print_pll(&pll);
		goto out;
	}
out:
	return 1;
}

int h_sampfreq(struct cmd_desc *cmdd, struct atom *atoms)
{
	struct ad9516_pll pll;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get/set the sampling frequency\n"
			"%s - print the current sampling frequency\n"
			"%s n:\n"
			"\tif n == 0, the sampling frequency is EXTCLK\n"
			"\tif n != 0, set the sampling frequency to "
			"(approx.) n Hz (using the internal PLL)\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type == Terminator) {
		if (ioctl(_DNFD, CVORG_IOCGPLL, &pll) < 0) {
			mperr("get PLL config");
			return -TST_ERR_IOCTL;
		}
		check_pll_dividers(&pll);
		print_pll(&pll);
		goto out;
	}

	if (atoms->type != Numeric) {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}

	if (atoms->val > CVORG_MAX_FREQ) {
		printf("Out of range sampfreq. Max: %d\n", CVORG_MAX_FREQ);
		return -TST_ERR_WRONG_ARG;
	}

	if (ad9516_fill_pll_conf(atoms->val, &pll)) {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}

	if (ioctl(_DNFD, CVORG_IOCSPLL, &pll) < 0) {
		mperr("set PLL config");
		return -TST_ERR_IOCTL;
	}
	print_pll(&pll);
out:
	return 1;
}

int h_rsf(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint32_t sampfreq;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Read the module's current Sampling Frequency\n"
			"Note that the current sampling frequency might differ "
			"from the one queued issuing 'sampfreq'\n"
			"If there's no valid sampling clock, the value "
			"read is 0\n", cmdd->name);
		goto out;
	}

	if (ioctl(_DNFD, CVORG_IOCGSAMPFREQ, &sampfreq) < 0) {
		mperr("get_sampfreq");
		return -TST_ERR_IOCTL;
	}

	if (sampfreq < 1000000)
		printf("%g KHz\n", (double)sampfreq / 1e3);
	else
		printf("%g MHz\n", (double)sampfreq / 1e6);

out:
	return 1;
}

int h_sram(struct cmd_desc *cmdd, struct atom *atoms)
{
	struct cvorg_sram_entry entry;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Read/Write from/to SRAM\n"
			"%s offset [value]\n"
			"\toffset: offset within the SRAM\n"
			"\tvalue: value to be written, uint_32\n",
			cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type != Numeric) {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}

	/* sanity check */
	if (atoms->val >= CVORG_SRAM_SIZE) {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}

	entry.address = atoms->val * 4;

	if ((atoms + 1)->type != Numeric) {
		/* read from SRAM */
		if (ioctl(_DNFD, CVORG_IOCGSRAM, &entry) < 0) {
			mperr("Read from SRAM");
			return -TST_ERR_IOCTL;
		}

		printf("value[0]=0x%04x\nvalue[1]=0x%04x\n",
			entry.data[0], entry.data[1]);
		goto out;
	} else {
		/* write to SRAM */
		entry.data[0] = (++atoms)->val >> 16;
		entry.data[1] = atoms->val & 0xffff;

		if (ioctl(_DNFD, CVORG_IOCSSRAM, &entry) < 0) {
			mperr("Write to SRAM");
			return -TST_ERR_IOCTL;
		}

		printf("written 0x%04x%04x to address 0x%x OK\n",
			entry.data[0], entry.data[1], entry.address);
		goto out;
	}
out:
	return 1;
}

int h_lock(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Lock Module\n", cmdd->name);
		return 1;
	}

	if (ioctl(_DNFD, CVORG_IOCTLOCK, NULL) < 0) {
		mperr("lock_module");
		return 1;
	}

	printf("Module locked OK\n");
	return 1;
}

int h_unlock(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Unlock Module\n"
			"%s [force]\n"
			"\tforce: [bool]. Force the unlocking of the module\n",
			cmdd->name, cmdd->name);
		return 1;
	}

	++atoms;

	if (atoms->type == Numeric && atoms->val) {
		if (ioctl(_DNFD, CVORG_IOCTUNLOCK_FORCE, NULL) < 0) {
			mperr("force unlock_module");
			return -TST_ERR_IOCTL;
		}
		printf("Module unlocked (forced)\n");
		return 1;
	}

	if (ioctl(_DNFD, CVORG_IOCTUNLOCK, NULL) < 0) {
		mperr("unlock_module");
		return 1;
	}
	printf("Module unlocked\n");
	return 1;
}

static void print_mode(uint32_t mode)
{
	switch (mode) {
	case CVORG_MODE_OFF:
		printf("Off\n");
		break;
	case CVORG_MODE_NORMAL:
		printf("Normal\n");
		break;
	case CVORG_MODE_DAC:
		printf("DAC\n");
		break;
	case CVORG_MODE_TEST1:
		printf("Test 1\n");
		break;
	case CVORG_MODE_TEST2:
		printf("Test 2\n");
		break;
	case CVORG_MODE_TEST3:
		printf("Test 3\n");
		break;
	default:
		printf("Unknown mode (0x%x)\n", mode);
		break;
	}
}

int h_mode(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint32_t mode;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get/set operation mode on the current channel\n"
			"%s \"string\" - set mode to:\n"
			"\t\"off\": Off\n"
			"\t\"normal\": Normal\n"
			"\t\"DAC\": DAC\n"
			"\t\"test1\": Test mode 1\n"
			"\t\"test2\": Test mode 2\n"
			"\t\"test3\": Test mode 3\n",
			cmdd->name, cmdd->name);
		return 1;
	}

	if ((++atoms)->type != String)
		goto out;

	if (!strncmp(atoms->text, "off", MAX_ARG_LENGTH))
		mode = CVORG_MODE_OFF;
	else if (!strncmp(atoms->text, "normal", MAX_ARG_LENGTH))
		mode = CVORG_MODE_NORMAL;
	else if (!strncmp(atoms->text, "DAC", MAX_ARG_LENGTH))
		mode = CVORG_MODE_DAC;
	else if (!strncmp(atoms->text, "test1", MAX_ARG_LENGTH))
		mode = CVORG_MODE_TEST1;
	else if (!strncmp(atoms->text, "test2", MAX_ARG_LENGTH))
		mode = CVORG_MODE_TEST2;
	else if (!strncmp(atoms->text, "test3", MAX_ARG_LENGTH))
		mode = CVORG_MODE_TEST3;
	else {
		printf("Invalid mode provided\n");
		return -TST_ERR_WRONG_ARG;
	}

	if (ioctl(_DNFD, CVORG_IOCSMODE, &mode) < 0) {
		mperr("set mode");
		return -TST_ERR_IOCTL;
	}
out:
	if (ioctl(_DNFD, CVORG_IOCGMODE, &mode) < 0) {
		mperr("get mode");
		return -TST_ERR_IOCTL;
	}

	print_mode(mode);

	return 1;
}

static int write_channel(int channel, unsigned int offset, uint32_t value)
{
	SkelDrvrRawIoBlock io;

	io.SpaceNumber	= 0x29;
	io.DataWidth	= 32;
	io.Offset	= channel ? offset + CVORG_CHAN2_OFFSET : offset;
	io.Data		= value;

	if (ioctl(_DNFD, SkelDrvrIoctlRAW_WRITE, &io) < 0) {
		mperr("RAW_WRITE ioctl fails:\n"
			"channel: %d, offset: 0x%x, data: 0x%x\n",
			channel + 1, offset, value);
		return -TST_ERR_IOCTL;
	}
	return 0;
}

static int read_channel(int channel, unsigned int offset, uint32_t *value)
{
	SkelDrvrRawIoBlock io;

	io.SpaceNumber	= 0x29;
	io.DataWidth	= 32;
	io.Offset	= channel ? offset + CVORG_CHAN2_OFFSET : offset;
	io.Data		= 0;

	if (ioctl(_DNFD, SkelDrvrIoctlRAW_READ, &io) < 0) {
		mperr("RAW_READ ioctl fails:\n"
			"channel: %d, offset: 0x%x, data: 0x%x\n",
			channel + 1, offset, io.Data);
		return -TST_ERR_IOCTL;
	}

	*value = io.Data;
	return 0;
}

static int memtest_write(int channel, const struct cvorg_memtest_op *op)
{
	uint32_t pattern = op->pattern;
	int ret;
	int i;

	if (op->type == CVORG_MEMTEST_READONLY)
		return 0;

	for (i = op->start; i < op->end; i += op->skip) {
		ret = write_channel(channel, CVORG_SRAMADDR, i * sizeof(uint32_t));
		if (ret)
			return ret;

		if (op->type == CVORG_MEMTEST_INCREMENT)
			pattern = op->pattern + i * sizeof(uint32_t);

		ret = write_channel(channel, CVORG_SRAMDATA, pattern);
		if (ret)
			return ret;
	}

	return 0;
}

static int memtest_block(int channel, const struct cvorg_memtest_op *op)
{
	uint32_t expected = op->pattern;
	uint32_t read;
	int errors = 0;
	int ret;
	int i;

	ret = memtest_write(channel, op);
	if (ret)
		return ret;

	for (i = op->start; i < op->end; i += op->skip) {
		if (op->type == CVORG_MEMTEST_INCREMENT)
			expected = op->pattern + i * sizeof(uint32_t);

		ret = write_channel(channel, CVORG_SRAMADDR, i * sizeof(uint32_t));
		if (ret)
			return ret;

		ret = read_channel(channel, CVORG_SRAMDATA, &read);
		if (ret)
			return ret;

		if (read != expected) {
			printf("channel %d: error @ 0x%x Read 0x%x Expected 0x%x\n",
				channel + 1, i * sizeof(uint32_t), read, expected);
			errors++;
		}

		if (errors >= CVORG_MEMTEST_MAX_ERRORS) {
			printf("channel %d: memtest terminating after %d errors",
				channel + 1, errors);
			return 1;
		}
	}

	return 0;
}

int memtest_channel(int channel)
{
	const struct cvorg_memtest_op *op;
	uint32_t chanstat;
	int ret;
	int i;

	if (ioctl(_DNFD, CVORG_IOCGCHANSTAT, &chanstat) < 0) {
		mperr("get_chanstat");
		return -TST_ERR_IOCTL;
	}
	if (chanstat & CVORG_CHANSTAT_BUSY ||
		chanstat & CVORG_CHANSTAT_SRAM_BUSY) {
		printf("channel %d: channel is busy\n", channel + 1);
		return 1;
	}

	for (i = 0; i < CVORG_NR_MEMTEST_OPS; i++) {
		op = &cvorg_memtest_ops[i];

		printf("channel %d Memory[i]=0x%08x %s%sTest: ",
			channel + 1, op->pattern,
			op->skip > 1 ? "SKIP " : "",
			op->type == CVORG_MEMTEST_READONLY ? "ReadBack " : "");

		ret = memtest_block(channel, &cvorg_memtest_ops[i]);
		if (ret) {
			printf("Failed\n");
			return ret;
		} else {
			printf("OK\n");
		}
	}

	return 0;
}

int h_memtest(struct cmd_desc *cmdd, struct atom *atoms)
{
	int ret;
	int i;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Perform a memory test on the current channel\n"
			"\tNote that the memory test cannot be launched when "
			"the channel is busy\n",
			cmdd->name);
		return 1;
	}

	for (i = 0; i < 2; i++) {
		ret = memtest_channel(i);
		if (ret) {
			printf("channel %d: memtest failed\n", i + 1);
			return ret;
		} else {
			printf("channel %d: memtest OK\n", i + 1);
		}
	}
	printf("Memory test OK\n");
	return 1;
}

static void print_adc_value_volts(struct cvorg_adc *adc)
{
	double volts;
	unsigned int factor;
	unsigned int shift;

	switch (adc->range) {
	case CVORG_ADC_RANGE_5V_UNIPOLAR:
		factor = 5;
		shift = 16;
		break;
	case CVORG_ADC_RANGE_10V_UNIPOLAR:
		factor = 10;
		shift = 16;
		break;
	case CVORG_ADC_RANGE_5V_BIPOLAR:
		factor = 5;
		shift = 15;
		break;
	case CVORG_ADC_RANGE_10V_BIPOLAR:
	default:
		factor = 10;
		shift = 15;
		break;
	}

	volts = adc->value;
	volts *= factor;
	volts /= (1 << shift) - 1;

	printf("%g V\n", volts);
}

int h_adcread(struct cmd_desc *cmdd, struct atom *atoms)
{
	struct cvorg_adc adc;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Read from a channel's ADC\n"
			"%s \"channel\" \"range\"\n"
			"\tchannel: ADC channel. Valid values:\n"
			"\t\t\"mon\": Monitor Output\n"
			"\t\t\"ref\": Reference Output\n"
			"\t\t\"5v\": 5V reference\n"
			"\t\t\"gnd\": GND reference\n"
			"\trange: ADC Input range. Valid values:\n"
			"\t\t\"5b\": 5V bipolar\n"
			"\t\t\"5u\": 5V unipolar\n"
			"\t\t\"10b\": 10V bipolar\n"
			"\t\t\"10u\": 10V unipolar\n",
			cmdd->name, cmdd->name);
		return 1;
	}

	memset(&adc, 0, sizeof(adc));

	if ((++atoms)->type != String) {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}

	if (!strncmp(atoms->text, "mon", MAX_ARG_LENGTH))
		adc.channel = CVORG_ADC_CHANNEL_MONITOR;
	else if (!strncmp(atoms->text, "ref", MAX_ARG_LENGTH))
		adc.channel = CVORG_ADC_CHANNEL_REF;
	else if (!strncmp(atoms->text, "5v", MAX_ARG_LENGTH))
		adc.channel = CVORG_ADC_CHANNEL_5V;
	else if (!strncmp(atoms->text, "gnd", MAX_ARG_LENGTH))
		adc.channel = CVORG_ADC_CHANNEL_GND;
	else {
		printf("Invalid range provided\n");
		return -TST_ERR_WRONG_ARG;
	}

	if ((++atoms)->type != String) {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}

	if (!strncmp(atoms->text, "5b", MAX_ARG_LENGTH))
		adc.range = CVORG_ADC_RANGE_5V_BIPOLAR;
	else if (!strncmp(atoms->text, "5u", MAX_ARG_LENGTH))
		adc.range = CVORG_ADC_RANGE_5V_UNIPOLAR;
	else if (!strncmp(atoms->text, "10b", MAX_ARG_LENGTH))
		adc.range = CVORG_ADC_RANGE_10V_BIPOLAR;
	else if (!strncmp(atoms->text, "10u", MAX_ARG_LENGTH))
		adc.range = CVORG_ADC_RANGE_10V_UNIPOLAR;
	else {
		printf("Invalid range provided\n");
		return -TST_ERR_WRONG_ARG;
	}

	if (ioctl(_DNFD, CVORG_IOCGADC_READ, &adc) < 0) {
		mperr("adc_read");
		return -TST_ERR_IOCTL;
	}

	print_adc_value_volts(&adc);
	return 1;
}

static int dac_print_header(void)
{
	uint32_t val;
	uint16_t gain;
	int16_t offset;

	if (ioctl(_DNFD, CVORG_IOCGDAC_VAL, &val) < 0) {
		mperr("get dac val");
		return -TST_ERR_IOCTL;
	}

	if (ioctl(_DNFD, CVORG_IOCGDAC_GAIN, &gain) < 0) {
		mperr("get dac gain");
		return -TST_ERR_IOCTL;
	}

	if (ioctl(_DNFD, CVORG_IOCGDAC_OFFSET, &offset) < 0) {
		mperr("get dac offset");
		return -TST_ERR_IOCTL;
	}

	printf("  DAC Value: 0x%04x\tgain corr: 0x%04x (%d)\toffset: 0x%04x\n",
		val, gain, gain, offset);

	return 0;
}

static void dac_print_cmdlist(void)
{
	printf("*** Commands ***\n"
		"  [v]alue\t[g]ain\t[o]ffset\t[q]uit\n"
		"What Now> ");
}

static int dac_get_input(int *val, const char *prompt)
{
	char *line;
	int ret = 0;

	line = readline(prompt);

	if (line == NULL)
		return 1;

	if (!sscanf(line, "%i", val))
		ret = 1;
	free(line);

	return ret;
}

static int dac_set_val(int val)
{
	uint32_t value = val;

	if (ioctl(_DNFD, CVORG_IOCSDAC_VAL, &value) < 0) {
		mperr("set dac val");
		return -TST_ERR_IOCTL;
	}
	return 0;
}

static int dac_set_gain(int val)
{
	uint16_t value = (uint16_t)val;

	if (ioctl(_DNFD, CVORG_IOCSDAC_GAIN, &value) < 0) {
		mperr("set dac gain");
		return -TST_ERR_IOCTL;
	}
	return 0;
}

static int dac_set_offset(int val)
{
	int16_t value = (uint16_t)val;

	if (ioctl(_DNFD, CVORG_IOCSDAC_OFFSET, &value) < 0) {
		mperr("set dac offset");
		return -TST_ERR_IOCTL;
	}
	return 0;
}

static int __dac(void)
{
	int ret;
	unsigned int c;
	int val;
	const char *val_prompt		= "DAC Value >> ";
	const char *gain_prompt		= "Gain Correction >> ";
	const char *offset_prompt	= "Offset Correction >> ";

 init:
	printf("\n");
	ret = dac_print_header();
	if (ret)
		return ret;
 get_cmd:
	dac_print_cmdlist();
	c = getchar();
	if (c == EOF) {
		return 1;
	} else if (c != '\n') {
		/* escape subsequent characters and the ending '\n' */
		while (getchar() != '\n')
			;
	}

	switch (c) {
	case 'v':
		ret = dac_get_input(&val, val_prompt);
		if (ret)
			goto get_cmd;
		ret = dac_set_val(val);
		if (ret)
			return ret;
		goto init;
	case 'g':
		ret = dac_get_input(&val, gain_prompt);
		if (ret)
			goto get_cmd;
		ret = dac_set_gain(val);
		if (ret)
			return ret;
		goto init;
	case 'o':
		ret = dac_get_input(&val, offset_prompt);
		if (ret)
			goto get_cmd;
		ret = dac_set_offset(val);
		if (ret)
			return ret;
		goto init;
	case 'q':
		return 1;
	case '\n':
		goto init;
	default:
		printf("Huh (%c)?\n", c);
		goto get_cmd;
		break;
	}
}

int h_dac(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint32_t mode;
	int ret;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Configure the channel's DAC\n", cmdd->name);
		return 1;
	}

	mode = CVORG_MODE_DAC;
	if (ioctl(_DNFD, CVORG_IOCSMODE, &mode) < 0) {
		mperr("set DAC mode");
		return -TST_ERR_IOCTL;
	}

	ret = __dac();

	mode = CVORG_MODE_OFF;
	if (ioctl(_DNFD, CVORG_IOCSMODE, &mode) < 0) {
		mperr("set mode to OFF");
		return -TST_ERR_IOCTL;
	}

	return ret;
}

int h_outgain(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint32_t gain;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get/set the analog output gain"
			"%s - show the current gain\n"
			"%s n - set the offset to n dB [-22,20].\n",
			cmdd->name, cmdd->name, cmdd->name);
		return 1;
	}

	++atoms;
	if (atoms->type == Terminator)
		goto out;

	if (atoms->type == Numeric) {
		gain = atoms->val;
		if (ioctl(_DNFD, CVORG_IOCSOUTGAIN, &gain) < 0) {
			mperr("set analog output gain");
			return -TST_ERR_IOCTL;
		}
	} else {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}

 out:
	if (ioctl(_DNFD, CVORG_IOCGOUTGAIN, &gain) < 0) {
		mperr("get analog output gain");
		return -TST_ERR_IOCTL;
	}

	printf("%d dB\n", gain);

	return 1;
}

int h_starten(struct cmd_desc *cmdd, struct atom *atoms)
{
	int32_t enable;
	uint32_t status;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - enable Start\n"
			"%s - show the current output enable\n"
			"%s 1/0 - enable/disable listening to start pulses on "
			"the current channel\n",
			cmdd->name, cmdd->name, cmdd->name);
		return 1;
	}

	++atoms;
	if (atoms->type == Terminator)
		goto out;

	if (atoms->type == Numeric) {
		enable = atoms->val;
		if (ioctl(_DNFD, CVORG_IOCSOUT_ENABLE, &enable) < 0) {
			mperr("set output enable");
			return -TST_ERR_IOCTL;
		}
	} else {
		printf("Invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}

 out:
	if (ioctl(_DNFD, CVORG_IOCGCHANSTAT, &status) < 0) {
		mperr("get_chanstat");
		return -TST_ERR_IOCTL;
	}
	enable = status & CVORG_CHANSTAT_OUT_ENABLED;
	printf("%s\n", enable ? "Enabled" : "Disabled");

	return 1;
}

int h_pcb(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint64_t pcb_id;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Print the PCB Serial Number of the current device\n",
			cmdd->name);
		return 1;
	}

	if (ioctl(_DNFD, CVORG_IOCGPCB_ID, &pcb_id) < 0) {
		mperr("get_pcb_id");
		return -TST_ERR_IOCTL;
	}
	printf("0x%016llx\n", (unsigned long long)pcb_id);

	return 1;
}

int main(int argc, char *argv[], char *envp[])
{
	return extest_init(argc, argv);

}

struct cmd_desc user_cmds[] = {

	{ 1, CmdPLAY,	"playtest", "Play a waveform", "waveform type", 1,
			h_playtest },

	{ 2, CmdCH,	"ch", "Select channel", "channel number (1 or 2)", 0,
			h_ch },

	{ 3, CmdLEN,	"len", "Test-waves length", "(in points)", 0,
			h_len },

	{ 4, CmdOUTOFF,	"outoff", "Analog Output's Offset", "0/2.5V", 0,
			h_outoff },

	{ 5, CmdINPOL,	"inpol", "Input Pulses' Polarity", "(pos/neg)-ive)", 0,
			h_inpol },

	{ 6, CmdCHANSTAT,	"chanstat", "Channel's Status", "", 0,
				h_chanstat },

	{ 7, CmdTRIGGER, "trigger", "Send a software Trigger", "", 0,
			h_trigger },

	{ 8, CmdPLL,	"pll", "Get/Set PLL config", "", 0,
			h_pll },

	{ 9, CmdSAMPFREQ,	"sampfreq", "Get/Set the sampling frequency",
	  "", 0,		h_sampfreq },

	{10, CmdRSF,	"rsf", "Read the module's Sampling Frequency", "", 0,
			h_rsf },

	{11, CmdSRAM,	"sram", "Read/Write from/to SRAM", "", 0,
			h_sram },

	{12, CmdLOCK,	"lock", "Lock Module", "", 0,
			h_lock },

	{13, CmdUNLOCK,	"unlock", "Unlock Module", "[force]", 0,
			h_unlock },

	{14, CmdMODE,	"mode", "Get/Set Mode", "", 0,
			h_mode },

	{15, CmdMEMTEST, "memtest", "Memory Test", "", 0,
			h_memtest },

	{16, CmdADCREAD, "adcread", "ADC Read", "\"channel\" \"range\"", 2,
			h_adcread },

	{17, CmdDAC,	"dac", "Configure DAC", "", 0,
			h_dac },

	{18, CmdOUTGAIN, "outgain", "Output Gain (dB)", "", 0,
			h_outgain },

	{19, CmdSTARTEN, "starten", "Enable Start", "", 0,
			h_starten },

	{20, CmdPCB,	"pcb", "Get the PCB Serial Number", "", 0,
			h_pcb },

	{ 0, }
};
