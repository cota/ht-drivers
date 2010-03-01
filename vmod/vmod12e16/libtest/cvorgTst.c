/**
 * @file cvorgTst.c
 *
 * @brief CVORG driver test program
 *
 * Copyright (c) 2009 CERN
 * @author Emilio G. Cota <emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <unistd.h>
#include <math.h>
#include <time.h>


#include <general_usr.h>  /* for handy definitions (mperr etc..) */

#include <extest.h>
#include "cvorgTst.h"

/* this compilation flag enables printing the test waveform before it's sent */
/* #define DEBUG_CVORG_WVSHAPES */

int use_builtin_cmds = 1;
char xmlfile[128] = "cvorg.xml";

struct cmd_desc user_cmds[] = {

	{ 1, CmdPLAY, "playtest", "Play a test waveform", "waveform type",
	  1, h_playtest },
	{ 1, CmdCHAN, "chan", "Select channel", "channel number (1 or 2)", 0,
	  h_chan },
	{ 1, CmdLEN, "len", "Test-waves length", "(in points)", 0, h_len },
	{ 1, CmdOUTPOL, "outpol", "Output's Polarity", "(Uni|Bi)Polar", 0,
	  h_outpol },
	{ 1, CmdINPOL, "inpol", "Input's Pulses' Polarity", "(pos/neg)-ive)",
	  0, h_inpol },
	{ 1, CmdCHANSTAT, "chanstat", "Channel's Status", "", 0, h_chanstat },
	{ 1, CmdPULSE, "pulse", "Send a software Pulse", "", 0, h_pulse },
	{ 1, CmdPLL, "pll", "Get/Set PLL config", "", 0, h_pll },
	{ 1, CmdSAMPFREQ, "sampfreq", "Get/Set the sampling frequency", "", 0,
	  h_sampfreq },
	{ 1, CmdRSF, "rsf", "Read the module's Sampling Frequency", "", 0,
	  h_rsf },
	{ 1, CmdSRAM, "sram", "Read/Write from/to SRAM", "", 0, h_sram },
	{ 0, }
};

static uint16_t	twaves[4][TEST_WV_MAXLEN];
static int 	twv_len = 500;

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

	printf("waves: %d\n", count);

	return count;
}

static int get_testwave(struct atom *atoms, int *wavenr)
{
	if (!strncmp(atoms->text, "square", MAX_ARG_LENGTH))
		*wavenr = TWV_SQUARE;
	else if (!strncmp(atoms->text, "sine", MAX_ARG_LENGTH))
		*wavenr = TWV_SINE;
	else if (!strncmp(atoms->text, "hsine", MAX_ARG_LENGTH))
		*wavenr = TWV_HSINE;
	else if (!strncmp(atoms->text, "ramp", MAX_ARG_LENGTH))
		*wavenr = TWV_RAMP;
	else
		return -1;
	return 0;
}

static int fill_waves(struct atom *atoms, struct cvorg_seq *seq)
{
	struct cvorg_wv *current;
	struct atom *p = atoms;
	int recurr;
	int wavenr;
	int i = 0;

	while (p->type != Terminator) {
		if (p->type != String)
			return -1;
		if (get_testwave(p, &wavenr))
			return -1;
		p++;
		recurr = 1;
		if (p->type == Numeric) {
			recurr = p->val;
			p++;
		}
		current = &seq->waves[i++];
		current->recurr = recurr;
		current->size = twv_len * 2;
		current->form = twaves[wavenr];
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

	printf("sequence: seqnr=%d, elems=%d\n", seq->nr, seq->elems);
	for (i = 0; i < seq->elems; i++) {
		printf("\twave %d\n", i);
		wave = &seq->waves[i];
		printf("\t\trecurr: %d, size(bytes): %d\n", wave->recurr,
			(int)wave->size);
		print_waveform(wave);
	}
}

static int play_seq(struct cvorg_seq *seq)
{
	struct timespec timestart, timeend;
	double delta;
	int rc;

	print_seq(seq);

	if (clock_gettime(CLOCK_REALTIME, &timestart)) {
		mperr("clock_gettime() failed for timestart");
		return -TST_ERR_SYSCALL;
	}

	rc = write(_DNFD, seq, sizeof(struct cvorg_seq));

	if (clock_gettime(CLOCK_REALTIME, &timeend)) {
		mperr("clock_gettime() failed for timeend");
		return -TST_ERR_SYSCALL;
	}

	delta = (double)(timeend.tv_sec - timestart.tv_sec) * 1000000 +
		(double)(timeend.tv_nsec - timestart.tv_nsec) / 1000.;

	printf("Time ellapsed: %g us\n", delta);

	if (rc != sizeof(struct cvorg_seq)) {
		mperr("write operation failed");
		return -TST_ERR_SYSCALL;
	}

	return 0;
}

int h_playtest(struct cmd_desc *cmdd, struct atom *atoms)
{
	struct cvorg_seq seq;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - play a predefined test waveform\n"
			"%s [seqnr] \"wave\" [times] [...]\n"
			"seqnr is the number of times the following sequence"
			" has to be played (default: 1, 0 means forever)\n"
			"type is a string representing the waveform type:\n"
			"\t\"sine\": sine wave (default: \"sine 1\")\n"
			"\t\"hsine\": high-freq sine wave\n"
			"\t\"square\": square wave\n"
			"\t\"ramp\": ramp wave\n"
			"Example:\n%s 5 \"hsine\" 2 \"sine\" 1 \"square\" 3\n"
			"will play 5 times the sequence formed by 2 high-freq "
			"sines, one sine and 3 square waves.\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}

	seq.nr = 1;
	if ((++atoms)->type == Numeric) {
		seq.nr = atoms->val;
		atoms++;
	}

	seq.elems = count_waves(atoms);
	if (!seq.elems) {
		mperr("Invalid wave descriptors");
		return -TST_ERR_WRONG_ARG;
	}

	seq.waves = malloc(sizeof(struct cvorg_wv) * seq.elems);
	if (seq.waves == NULL) {
		mperr("malloc fails for allocating the waveforms' descriptors");
		return -ENOMEM;
	}

	if (fill_waves(atoms, &seq))
		goto out_err;

	play_seq(&seq);

out:
	return 1;
out_err:
	if (seq.waves)
		free(seq.waves);
	return -TST_ERR_WRONG_ARG;
}

int h_chan(struct cmd_desc *cmdd, struct atom *atoms)
{
	int chan = 0;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - select the channel (1 or 2) to work on\n"
			"%s - show the channel we're working on\n"
			"%s [nr] - select channel 'nr' (default: nr=1)\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type == Terminator) {
		if (ioctl(_DNFD, CVORG_IOCGCHANNEL, &chan) < 0)
			return -TST_ERR_IOCTL;
		printf("current channel: %d\n", chan);
		goto out;
	}

	if (atoms->type == Numeric)
		chan = atoms->val;

	if (ioctl(_DNFD, CVORG_IOCSCHANNEL, &chan) < 0)
		return -TST_ERR_IOCTL;
out:
	return 1;
}

/*
 * ratio: 2 for 1 period, bigger for higher freqs
 */
static inline double testsine(int i, double ratio)
{
	return (sin(i * M_PI / (twv_len / ratio)) + 1) / 2.0;
}

static void twaves_init(void)
{
	int i;

	/* initialise the square test-wave */
	for (i = 0; i < twv_len / 2; i++)
		twaves[TWV_SQUARE][i] = 0xffff;
	for (; i < twv_len; i++)
		twaves[TWV_SQUARE][i] = 0x0000;

	/* initialise the sine test-wave (0xFFFF is the top of the scale) */
	for (i = 0; i < twv_len; i++)
		twaves[TWV_SINE][i] = (uint16_t)(testsine(i, 2.0) * 0xffff);

	/* high-freq sine test-wave */
	for (i = 0; i < twv_len; i++)
		twaves[TWV_HSINE][i] = (uint16_t)(testsine(i, 20.0) * 0xffff);

	for (i = 0; i < twv_len; i++)
		twaves[TWV_RAMP][i] = ((double)0xffff * i / (twv_len - 1));
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
		twaves_init();
	}
	else
		return -TST_ERR_WRONG_ARG;
out:
	return 1;
}

int h_outpol(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get/set the output's polarity"
			"%s - show the current polarity\n"
			"%s (0|1) - set the polarity to:\n"
			"\t0: unipolar (default)\n"
			"\t1: bipolar\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type == Terminator) {
		uint32_t polarity;

		if (ioctl(_DNFD, CVORG_IOCGOUTPOLARITY, &polarity) < 0)
			return -TST_ERR_IOCTL;
		printf("%s%s", polarity ? "Bi" : "Uni", "polar\n");
		goto out;
	}

	if (atoms->type == Numeric) {
		uint32_t polarity = atoms->val;

		if (ioctl(_DNFD, CVORG_IOCSOUTPOLARITY, &polarity) < 0)
			return -TST_ERR_IOCTL;
	} else
		return -TST_ERR_WRONG_ARG;
out:
	return 1;
}

int h_inpol(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get/set input's pulses' polarity"
			"%s - show the current polarity\n"
			"%s (0|1) - set the polarity to:\n"
			"\t0: positive pulses (default)\n"
			"\t1: negative pulses\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type == Terminator) {
		uint32_t polarity;

		if (ioctl(_DNFD, CVORG_IOCGINPOLARITY, &polarity) < 0)
			return -TST_ERR_IOCTL;
		printf("%s%s", polarity ? "Posi" : "Nega", "tive pulses\n");
		goto out;
	}

	if (atoms->type == Numeric) {
		uint32_t polarity = atoms->val;

		if (ioctl(_DNFD, CVORG_IOCSINPOLARITY, &polarity) < 0)
			return -TST_ERR_IOCTL;
	} else
		return -TST_ERR_WRONG_ARG;
out:
	return 1;
}

int h_chanstat(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint32_t status;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get the current channel's status\n"
			"The status is defined by the following bitmask:\n"
			"\t[0] 1 == Channel is ready to run\n"
			"\t[1] 1 == Normal Mode\n"
			"\t[2] 1 == Channel is busy playing a function\n"
			"\t[3] (not used)\n"
			"\t[4] 1 == Clock error\n"
			"\t[5] (not used)\n"
			"\t[6-7] (tbd)\n"
			"\t[8] 1 == sampling clock present and stable\n"
			"\t[9-31] (not used)\n", cmdd->name);
		goto out;
	}

	if (ioctl(_DNFD, CVORG_IOCGCHANSTAT, &status) < 0)
		return -TST_ERR_IOCTL;

	printf("0x%x\n", (int)status);
	if (status & CVORG_STATUS_READY)
		printf("\tReady\n");
	if (status & CVORG_STATUS_MODE_NORMAL)
		printf("\tNormal Mode\n");
	if (status & CVORG_STATUS_BUSY)
		printf("\tBusy\n");
	if (status & CVORG_STATUS_CLKERROR)
		printf("\tClock Error (either not present or unstable)\n");
	if (status & CVORG_STATUS_CLKOK)
		printf("\tClock OK\n");
out:
	return 1;
}

int h_pulse(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint32_t pulse;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - send a software pulse to a channel\n"
			"%s \"string\" - send the pulse given by \"string\":\n"
			"\t\"start\": Start pulse\n"
			"\t\"evstart\": Event Start pulse\n"
			"\t\"stop\": Stop pulse\n"
			"\t\"evstop\": Event Stop pulse\n"
			"\t\"resetdac\": Reset DAC to 0V\n",
			cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type != String)
		goto out;

	if (!strncmp(atoms->text, "start", MAX_ARG_LENGTH))
		pulse = CVORG_START;
	else if (!strncmp(atoms->text, "evstart", MAX_ARG_LENGTH))
		pulse = CVORG_EVSTART;
	else if (!strncmp(atoms->text, "stop", MAX_ARG_LENGTH))
		pulse = CVORG_STOP;
	else if (!strncmp(atoms->text, "evstop", MAX_ARG_LENGTH))
		pulse = CVORG_EVSTOP;
	else if (!strncmp(atoms->text, "resetdac", MAX_ARG_LENGTH))
		pulse = CVORG_RESETDAC;
	else
		return -TST_ERR_WRONG_ARG;

	if (ioctl(_DNFD, CVORG_IOCSPULSE, &pulse) < 0)
		return -TST_ERR_IOCTL;
out:
	return 1;
}

static void check_pll_dividers(struct cvorg_pll *pll)
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

static void print_pll(struct cvorg_pll *pll)
{
	unsigned long n = pll->p * pll->b + pll->a;
	double fvco;

	fvco = (double)CVORG_OSCILLATOR_FREQ * n / pll->r;

	printf("debug: fref=%g, n=%lu, r=%d\n", (double)CVORG_OSCILLATOR_FREQ,
		n, pll->r);

	printf("PLL params: [P=%d, B=%d, A=%d, R = %d]\n"
		"\tdvco=%d, d1=%d, d2=%d\n",
		pll->p, pll->b, pll->a, pll->r, pll->dvco, pll->d1, pll->d2);

	printf("f_vco = f_ref * N / R, where N = PxB + A\n"
		"\tf_ref = %g Hz\n"
		"\tN = %lu\n"
		"\tf_vco = %g Hz\n",
		(double)CVORG_OSCILLATOR_FREQ, n, fvco);

	printf("f_out: %g Hz\n", fvco / pll->dvco / pll->d1 / pll->d2);
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
		struct cvorg_pll pll;

		if (ioctl(_DNFD, CVORG_IOCGPLL, &pll) < 0)
			return -TST_ERR_IOCTL;

		check_pll_dividers(&pll);
		print_pll(&pll);
		goto out;
	}
out:
	return 1;
}

/*
 * given a desired output frequency and constant f_vco, check if just playing
 * with the output dividers can suffice. If it does, then return 0. Otherwise,
 * return -1, keeping the best dividers found.
 */
static int get_dividers(int freq, struct cvorg_pll *pll)
{
	double diff;
	double mindiff = 1e10;
	int dvco, d1, d2;
	int i, j, k;

	/*
	 * Note: this loop is sub-optimal, since sometimes combinations
	 * that lead to the same result are tried.
	 * For more info on this, check the octave (.m) files in /scripts.
	 */
	for (i = 2; i <= 6; i++) {
		for (j = 1; j <= 32; j++) {
			for (k = j; k <= 32; k++) {
				diff = (double)TEST_VCO_FREQ / i / j / k - freq;
				if (fabs(diff) <= (double)TEST_MAXDIFF)
					goto found;
				if (fabs(diff) < fabs(mindiff)) {
					mindiff = diff;
					dvco = i;
					d1 = j;
					d2 = k;
				}
			}
		}
	}
	pll->dvco = dvco;
	pll->d1 = d1;
	pll->d2 = d2;
	return -1;
found:
	pll->dvco = i;
	pll->d1 = j;
	pll->d2 = k;
	return 0;
}

/*
 * Here we consider the dividers fixed, and play with N, R to calculate
 * a satisfactory f_vco, given a certain required output frequency @freq
 */
static void calc_pll_params(int freq, struct cvorg_pll *pll)
{
	double n, nr;
	/*
	 * What's the required N / R ?
	 * N / R = Dividers * freq / fref, where Dividers = dvco * d1 * d1
	 * and fref = CVORG_OSCILLATOR_FREQ
	 */
	nr = (double)pll->dvco * pll->d1 * pll->d2 * freq / CVORG_OSCILLATOR_FREQ;

	n = nr * pll->r;
	/*
	 * No need to touch R or P; let's just re-calculate B and A
	 */
	pll->b = ((int)n) / pll->p;
	pll->a = ((int)n) % pll->p;
}

static int check_pll(struct cvorg_pll *pll)
{
	int err = 0;
	double fvco = (double)CVORG_OSCILLATOR_FREQ *
		(pll->p * pll->b + pll->a) / pll->r;

	if (pll->a > pll->b) {
		printf("Warning: pll->a > pll->b\n");
		err = -1;
	}

	if (pll->a > 63) {
		printf("Warning: pll->a out of range\n");
		err = -1;
	}

	if (pll->p != 16 && pll->p != 32) {
		printf("Warning: pll-> p != {16,32}\n");
		err = -1;
	}

	if (pll->b <= 0 || pll->b > 8191) {
		printf("Warning: b out of range\n");
		err = -1;
	}

	if (pll->r <= 0 || pll->r > 16383) {
		printf("Warning: r out of range\n");
		err = -1;
	}

	if (fvco < 2.55e9 || fvco > 2.95e9) {
		printf("Warning: fvco (%g) out of range\n", fvco);
		err = -1;
	}

	return err;
}

static int fill_pll_conf(int freq, struct cvorg_pll *pll)
{
	if (!freq) {
		pll->a = 1;
		pll->b = 1;
		pll->p = 1;
		pll->r = 1;
		pll->dvco = 1;
		pll->d1 = 1;
		pll->d2 = 1;

		pll->external = 1;
		return 0;
	}

	/* initial sanity check */
	if (freq < 416000) {
		printf("The minimum sampling freq is 416KHz\n");
		return -TST_ERR_WRONG_ARG;
	}

	/*
	 * Algorithm for adjusting the PLL
	 * 1. set f_vco = 2.8 GHz
	 * 	N = (p * b) + a -> N = 70000
	 * 	f_vco = f_ref * N / R = 40e6 * 70000 = 2.8e9 Hz
	 */
	pll->a = 0;
	pll->b = 4375;
	pll->p = 16;
	pll->r = 1000;
	pll->external = 0;

	/*
	 * 2. check if playing with the dividers we can achieve the required
	 *    output frequency (@freq) without changing the f_vco above.
	 *    If a exact match is not possible, then return the closest result.
	 */
	if (get_dividers(freq, pll)) {
		/*
		 * 2.1 Playing with the dividers is not enough: tune the VCO
		 *     by adjusting the relation N/R.
		 */
		calc_pll_params(freq, pll);
	}

	/* 3. Check the calculated PLL configuration is feasible */
	return check_pll(pll);
}

int h_sampfreq(struct cmd_desc *cmdd, struct atom *atoms)
{
	struct cvorg_pll pll;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - get/set the sampling frequency\n"
			"%s - print the current sampling frequency\n"
			"%s n [force]:\n"
			"\tif n == 0, the sampling frequency is EXTCLK\n"
			"\tif n != 0, set the sampling frequency to "
			"(approx.) n Hz (using the internal PLL)\n"
			"\tforce == 0: if the other channel is playing a "
			"waveform when we configure the sampling clock (which "
			"happens just before playing), the sampling frequency "
			"will be kept as it is (this is the default)\n"
			"\tforce == 1: force the change of the sampling "
			"frequency on the module, even if the other channel "
			"is playing a waveform\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}

	if ((++atoms)->type == Terminator) {
		if (ioctl(_DNFD, CVORG_IOCGPLL, &pll) < 0)
			return -TST_ERR_IOCTL;
		/* @todo check for EXT CLK */
		check_pll_dividers(&pll);
		print_pll(&pll);
		goto out;
	}

	if (atoms->type != Numeric)
		return -TST_ERR_WRONG_ARG;
	if (fill_pll_conf(atoms->val, &pll))
		return -TST_ERR_WRONG_ARG;

	pll.force = 0;
	if ((++atoms)->type == Numeric)
		pll.force = !!atoms->val;

	if (ioctl(_DNFD, CVORG_IOCSPLL, &pll) < 0)
		return -TST_ERR_IOCTL;
	print_pll(&pll);
out:
	return 1;
}

int h_rsf(struct cmd_desc *cmdd, struct atom *atoms)
{
	uint32_t rsf;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Read the module's current Sampling Frequency\n"
			"Note that the current sampling frequency might differ "
			"from the one queued issuing 'sampfreq'\n"
			"If there's no valid sampling clock, the value "
			"read is 0\n", cmdd->name);
		goto out;
	}

	if (ioctl(_DNFD, CVORG_IOCGSAMPFREQ, &rsf) < 0)
		return -TST_ERR_IOCTL;

	if (rsf < 1000000)
		printf("%g KHz\n", (double)rsf / 1e3);
	else
		printf("%g MHz\n", (double)rsf / 1e6);

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

	if ((++atoms)->type != Numeric)
		return -TST_ERR_WRONG_ARG;

	/* sanity check */
	if (atoms->val >= CVORG_SRAM_SIZE)
		return -TST_ERR_WRONG_ARG;

	entry.address = atoms->val * 4;

	if ((atoms + 1)->type != Numeric) {
		/* read from SRAM */
		if (ioctl(_DNFD, CVORG_IOCGSRAM, &entry) < 0)
			return -TST_ERR_IOCTL;

		printf("value[0]=0x%04x\nvalue[1]=0x%04x\n",
			entry.data[0], entry.data[1]);
		goto out;
	} else {
		/* write to SRAM */
		entry.data[0] = (++atoms)->val >> 16;
		entry.data[1] = atoms->val & 0xffff;

		if (ioctl(_DNFD, CVORG_IOCSSRAM, &entry) < 0)
			return -TST_ERR_IOCTL;

		printf("written 0x%04x%04x to address 0x%x OK\n",
			entry.data[0], entry.data[1], entry.address);
		goto out;
	}
out:
	return 1;
}


int main(int argc, char *argv[], char *envp[])
{
	twaves_init();
	return extest_init(argc, argv);

}
