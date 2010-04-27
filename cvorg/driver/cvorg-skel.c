/**
 * cvorg-skel.c
 *
 * skel-based driver for the CVORG
 * Copyright (c) 2009 Emilio G. Cota <cota@braap.org>
 *
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <cdcm/cdcm.h>
#include <skeldrvr.h>
#include <skeldrvrP.h>
#include <skeluser_ioctl.h>

#include <cvorg.h>
#include <cvorg_priv.h>
#include <cvorg_hw.h>

static char *cvorg_ioctl_names[] = {
	[_IOC_NR(CVORG_IOCSCHANNEL)]	= "Set Channel",
	[_IOC_NR(CVORG_IOCGCHANNEL)]	= "Get Channel",

	[_IOC_NR(CVORG_IOCGOUTOFFSET)]	= "Get Analog Output Offset",
	[_IOC_NR(CVORG_IOCSOUTOFFSET)]	= "Set Analog Output Offset",

	[_IOC_NR(CVORG_IOCGINPOLARITY)]	= "Get Input Pulse Polarity",
	[_IOC_NR(CVORG_IOCSINPOLARITY)]	= "Set Input Pulse Polarity",

	[_IOC_NR(CVORG_IOCGPLL)]	= "Get PLL Configuration",
	[_IOC_NR(CVORG_IOCSPLL)]	= "Set PLL Configuration",

	[_IOC_NR(CVORG_IOCGSRAM)]	= "Get SRAM",
	[_IOC_NR(CVORG_IOCSSRAM)]	= "Set SRAM",

	[_IOC_NR(CVORG_IOCSTRIGGER)]	= "Send a Software Trigger",

	[_IOC_NR(CVORG_IOCSLOADSEQ)]	= "Load a Waveform Sequence",

	[_IOC_NR(CVORG_IOCGCHANSTAT)]	= "Get Channel Status",

	[_IOC_NR(CVORG_IOCTUNLOCK_FORCE)] = "Unlock (force) module",
	[_IOC_NR(CVORG_IOCTLOCK)]	= "Lock Module",
	[_IOC_NR(CVORG_IOCTUNLOCK)]	= "Unlock Module",

	[_IOC_NR(CVORG_IOCGMODE)]	= "Get Channel Mode",
	[_IOC_NR(CVORG_IOCSMODE)]	= "Set Channel Mode",

	[_IOC_NR(CVORG_IOCGOUTGAIN)]	= "Get Channel Output Gain",
	[_IOC_NR(CVORG_IOCSOUTGAIN)]	= "Set Channel Output Gain",

	[_IOC_NR(CVORG_IOCGADC_READ)]	= "Channel ADC Read",

	[_IOC_NR(CVORG_IOCGDAC_VAL)]	= "Get Channel's DAC's Value",
	[_IOC_NR(CVORG_IOCSDAC_VAL)]	= "Set Channel's DAC's Value",

	[_IOC_NR(CVORG_IOCGDAC_GAIN)]	= "Get Channel's DAC's Gain Correction",
	[_IOC_NR(CVORG_IOCSDAC_GAIN)]	= "Set Channel's DAC's Gain Correction",

	[_IOC_NR(CVORG_IOCGDAC_OFFSET)]	= "Get Channel's DAC's Offset Correction",
	[_IOC_NR(CVORG_IOCSDAC_OFFSET)]	= "Set Channel's DAC's Offset Correction",

	[_IOC_NR(CVORG_IOCGTEMPERATURE)] = "Get On-Board Temperature (in C)",

	[_IOC_NR(CVORG_IOCSOUT_ENABLE)]	= "Enable/Disable Channel's Output",
};

static const int32_t cvorg_gains[] = {
	[0]	= -22,
	[1]	= -16,
	[2]	= -10,
	[3]	= -4,
	[4]	= 2,
	[5]	= 8,
	[6]	= 14,
	[7]	= 20,
};
#define CVORG_NR_GAINS	ARRAY_SIZE(cvorg_gains)

struct cvorg_limit {
	uint32_t	freq;
	uint32_t	len;
};

/* we use MHz here to avoid floating point arithmetic */
static const struct cvorg_limit cvorg_freqlimits[] = {
	{ 1,	16 },
	{ 10,	60 },
	{ 25,	1250 },
	{ 50,	3300 },
	{ 60,	4700 },
	{ 80,	9300 },
	{ 100,	23200 }
};
#define CVORG_NR_FREQLIMITS	ARRAY_SIZE(cvorg_freqlimits)

static inline int __cvorg_channel_busy(struct cvorg_channel *channel)
{
	return cvorg_rchan(channel, CVORG_STATUS) & CVORG_STATUS_BUSY;
}

static inline int __cvorg_channel_ready(struct cvorg_channel *channel)
{
	uint32_t status = cvorg_rchan(channel, CVORG_STATUS);

	/* ready to run? */
	return    status & CVORG_STATUS_READY &&
		!(status & CVORG_STATUS_BUSY) &&
		!(status & CVORG_STATUS_ERR) &&
		  status & CVORG_STATUS_CLKOK;
}

static inline int __cvorg_busy(struct cvorg *cvorg)
{
	return __cvorg_channel_busy(&cvorg->channels[0]) ||
	       __cvorg_channel_busy(&cvorg->channels[1]);
}

/* Note: ERR_CLK isn't (and can't be) cleared */
static inline void __cvorg_channel_clear_errors(struct cvorg_channel *channel)
{
	cvorg_wchan(channel, CVORG_CTL, CVORG_CTL_ERR_CLEAR);
}

char *SkelUserGetIoctlName(int cmd)
{
	return cvorg_ioctl_names[_IOC_NR(cmd)];
}

/*
 * Module and driver cannot read at the same time from the module's SRAM.
 * Since the module has priority over the requests coming from the
 * driver, we must always check that the module is not accessing it
 * before going ahead. Otherwise we risk waiting for too long while
 * waiting for the module to finish, causing a bus error in the VME bus.
 */
static int __cvorg_sram_busy(struct cvorg_channel *channel)
{
	uint32_t regval;
	int i;

	for (i = 0; i < 10; i++) {
		regval = cvorg_rchan(channel, CVORG_STATUS);
		if (!(regval & CVORG_STATUS_SRAM_BUSY))
			return 0;
		usec_sleep(10);
	}
	return 1;
}

static uint32_t cvorg_absdiff(int32_t s1, int32_t s2)
{
	if (s1 > s2)
		return s1 - s2;
	return s2 - s1;
}

static int32_t cvorg_nearest(const int32_t *array, int n, int32_t val)
{
	int i;
	uint32_t diff, mindiff;
	int result = 0;

	mindiff = ~0;

	for (i = 0; i < n; i++) {
		diff = cvorg_absdiff(array[i], val);
		if (diff < mindiff) {
			mindiff = diff;
			result = i;
		}
	}
	return array[result];
}

static int32_t cvorg_gain_approx(int32_t gain)
{
	if (gain < cvorg_gains[0])
		return cvorg_gains[0];

	if (gain > cvorg_gains[CVORG_NR_GAINS - 1])
		return cvorg_gains[CVORG_NR_GAINS - 1];

	return cvorg_nearest(&cvorg_gains[0], CVORG_NR_GAINS, gain);
}

static unsigned int cvorg_gain_to_hw(int32_t gain)
{
	switch (gain) {
	case -22:
		return CVORG_OUTGAIN_M22;
	case -16:
		return CVORG_OUTGAIN_M16;
	case -10:
		return CVORG_OUTGAIN_M10;
	case -4:
		return CVORG_OUTGAIN_M4;
	case 2:
		return CVORG_OUTGAIN_2;
	case 8:
		return CVORG_OUTGAIN_8;
	case 14:
		return CVORG_OUTGAIN_14;
	case 20:
	default:
		return CVORG_OUTGAIN_20;
	}
}

static inline int cvorg_invalid_gain_offset(int32_t gain, uint32_t offset)
{
	return offset == CVORG_OUT_OFFSET_2_5V && gain != 14;
}

static int
cvorg_check_gain_offset_wv(struct cvorg_channel *channel, struct cvorg_wv *wv)
{
	if (wv->dynamic_gain &&
		cvorg_invalid_gain_offset(wv->gain_val, channel->outoff))
		return -1;
	return 0;
}

/*
 * The way waveforms are played consecutively is as follows: There are
 * two alternating waveform players. Just after the first loads a
 * waveform, the second starts reading the following from the FPGA's SRAM.
 * For this to work, the time elapsed while playing the first waveform
 * has to be greater than the time it takes the second player to grab
 * the second waveform.
 */
static inline int cvorg_must_check_freqlimit(struct cvorg_seq *seq)
{
	return seq->nr == 0 || seq->n_waves > 2;
}

/*
 * Returns the i-th rank of the fixed set of lengths that we've got
 * given a sampling frequency.
 * Note that given n (freq, length) pairs, there are n - 1 ranges
 * in between them, i.e. i => [0,n-2].
 */
static int cvorg_freqlimit_range(uint32_t mhz)
{
	uint32_t x2, x1;
	int i;

	for (i = 0; i < CVORG_NR_FREQLIMITS - 1; i++) {
		x1 = cvorg_freqlimits[i].freq;
		x2 = cvorg_freqlimits[i + 1].freq;

		if (WITHIN_RANGE(x1, mhz, x2))
			return i;
	}

	/* handle the out of range cases here */
	if (mhz <= cvorg_freqlimits[0].freq)
		return 0;
	else
		return CVORG_NR_FREQLIMITS - 2;
}

/*
 * Make a linear interpolation to find the length limit for the given
 * frequency and then check if we're over this limit or not.
 *
 * The interpolated curve is y = m*x + b, where m is the slope.
 * Note: we use MHz to avoid floating point arithmetic. For safety,
 * we round up the input in Hz to the nearest MHz unit.
 */
static int cvorg_freqlimit_check(uint32_t length, uint32_t hz)
{
	uint32_t y1, y2, x1, x2;
	uint32_t minimum;
	uint32_t m;
	uint32_t mhz = hz / 1000000 + !!(hz % 1000000);
	int i;

	i = cvorg_freqlimit_range(mhz);

	x1 = cvorg_freqlimits[i].freq;
	y1 = cvorg_freqlimits[i].len;

	x2 = cvorg_freqlimits[i + 1].freq;
	y2 = cvorg_freqlimits[i + 1].len;

	m = (y2 - y1) / (x2 - x1);
	minimum = m == 0 ? y2 : m * (mhz - x1) + y1;

	if (length < minimum) {
		SK_INFO("Minimum number of points at %d MHz: %d. Requested: %d",
			mhz, minimum, length);
	}

	return length < minimum;
}

static int cvorg_invalid_freqlimit(struct cvorg_channel *channel,
				struct cvorg_seq *seq, struct cvorg_wv *wv)
{
	uint32_t length;
	uint32_t sampfreq;

	/*
	 * If recurr is not set, we depend on the external Event Stop
	 * to switch to another function, which means we can do nothing
	 * to prevent our users from causing an error on the module
	 * if the two triggers come very close in time.
	 */
	if (!cvorg_must_check_freqlimit(seq) || !wv->recurr)
		return 0;

	length = wv->recurr * wv->size / sizeof(uint16_t);
	sampfreq = cvorg_rchan(channel, CVORG_SAMPFREQ);

	return cvorg_freqlimit_check(length, sampfreq);
}

static void cvorg_pll_cfg_init(struct ad9516_pll *pll)
{
	/*
	 * PLL defaults: fvco = 2.7GHz, f_output = 100MHz, for the given
	 * fref = 40MHz
	 */
	pll->a		= 0;
	pll->b		= 4375;
	pll->p		= 16;
	pll->r		= 1000;
	pll->dvco	= 2;
	pll->d1		= 14;
	pll->d2		= 1;
	pll->external	= 0;
}

#define CVORG_FREQ_DIV	10000
/*
 * Sleep until the configured frequency is available at the output.
 * The frequencies in this function are determined by the divider above.
 *
 * This divider is used to avoid floating point arithmetic when
 * calculating the resulting frequency given a particular PLL configuration.
 */
static int cvorg_poll_sampfreq(struct cvorg *cvorg)
{
	struct ad9516_pll *pll = &cvorg->pll;
	unsigned int n = pll->p * pll->b + pll->a;
	int32_t sampfreq;
	int32_t freq;
	int i;

	/* When external, we cannot possibly know the desired frequency */
	if (pll->external)
		return 0;

	/* Note: some dividers may be bypassed (ie set to zero) */
	freq = AD9516_OSCILLATOR_FREQ / CVORG_FREQ_DIV * n;
	if (pll->r)
		freq /= pll->r;
	if (pll->dvco)
		freq /= pll->dvco;
	if (pll->d1)
		freq /= pll->d1;
	if (pll->d2)
		freq /= pll->d2;

	sampfreq = cvorg_readw(cvorg, CVORG_SAMPFREQ) / CVORG_FREQ_DIV;
	i = 0;
	/*
	 * The loop below converges when the difference between the read
	 * frequency and the desired frequency is below CVORG_FREQ_DIV Hz.
	 */
	while (cvorg_absdiff(sampfreq, freq) > 1) {
		/* if it hasn't settled in 10 secs, return an error */
		if (i >= 100) {
			pseterr(ETIME);
			SK_INFO("Timeout expired for changing the frequency");
			return -1;
		}

		/* sleep for approximately a hundred msecs */
		tswait(NULL, SEM_SIGIGNORE, 10);

		/* read again */
		sampfreq = cvorg_readw(cvorg, CVORG_SAMPFREQ) / CVORG_FREQ_DIV;
		i++;
	}

	return 0;
}

static void __cvorg_apply_channel_inpol(struct cvorg_channel *channel)
{
	if (channel->inpol == CVORG_NEGATIVE_PULSE)
		cvorg_ochan(channel, CVORG_CONFIG, CVORG_CONFIG_INPOL);
	else
		cvorg_achan(channel, CVORG_CONFIG, ~CVORG_CONFIG_INPOL);
}

static void __cvorg_apply_channel_outoff(struct cvorg_channel *channel)
{
	if (channel->outoff == CVORG_OUT_OFFSET_0V)
		cvorg_achan(channel, CVORG_CONFIG, ~CVORG_CONFIG_OUT_OFFSET);
	else
		cvorg_ochan(channel, CVORG_CONFIG, CVORG_CONFIG_OUT_OFFSET);
}

static void __cvorg_apply_channel_outgain(struct cvorg_channel *channel)
{
	uint32_t val = cvorg_gain_to_hw(channel->outgain);

	val <<= CVORG_CONFIG_OUT_GAIN_SHIFT;
	cvorg_uchan(channel, CVORG_CONFIG, val, CVORG_CONFIG_OUT_GAIN_MASK);
}

static void __cvorg_apply_channel_out_enable(struct cvorg_channel *channel)
{
	if (channel->out_enabled)
		cvorg_ochan(channel, CVORG_CONFIG, CVORG_CONFIG_OUT_ENABLE);
	else
		cvorg_achan(channel, CVORG_CONFIG, ~CVORG_CONFIG_OUT_ENABLE);
}

static void __cvorg_apply_channel_cfg(struct cvorg_channel *channel)
{
	__cvorg_apply_channel_inpol(channel);
	__cvorg_apply_channel_outoff(channel);
	__cvorg_apply_channel_outgain(channel);
	__cvorg_apply_channel_out_enable(channel);
}

static int __cvorg_apply_cfg(struct cvorg *cvorg)
{
	if (clkgen_apply_pll_conf(cvorg))
		SK_WARN("PLL config failed. Continuing, though..");

	__cvorg_apply_channel_cfg(&cvorg->channels[0]);
	__cvorg_apply_channel_cfg(&cvorg->channels[1]);

	return 0;
}

SkelUserReturn SkelUserGetModuleVersion(SkelDrvrModuleContext *mcon, char *mver)
{
	struct cvorg *cvorg = mcon->UserData;
	uint32_t ver;
	unsigned int vhdl_tens, vhdl_units, vhdl_tenths, vhdl_hundredths;

	/* read VHDL revision */
	ver = cvorg_readw(cvorg, CVORG_VERSION);
	vhdl_tens	= (ver >> CVORG_VERSION_TENS_SHIFT)		& 0xf;
	vhdl_units	= (ver >> CVORG_VERSION_UNITS_SHIFT)		& 0xf;
	vhdl_tenths	= (ver >> CVORG_VERSION_TENTHS_SHIFT)		& 0xf;
	vhdl_hundredths	= (ver >> CVORG_VERSION_HUNDREDTHS_SHIFT)	& 0xf;

	ksprintf(mver, "v%d%d.%d%d",
		vhdl_tens, vhdl_units, vhdl_tenths, vhdl_hundredths);

	return SkelUserReturnOK;
}

/*
 * Since there are two channels per module, this function is not used.
 * Use the 'get channel status' IOCTL instead.
 */
SkelUserReturn SkelUserGetHardwareStatus(SkelDrvrModuleContext *mcon,
					 uint32_t *hsts)
{
	*hsts = mcon->StandardStatus;
	return SkelUserReturnOK;
}

SkelUserReturn SkelUserGetUtc(SkelDrvrModuleContext *mcon, SkelDrvrTime *utc)
{
	utc->NanoSecond = nanotime((unsigned long *)utc->Second);

	return SkelUserReturnOK;
}

static void channel_reset(struct cvorg_channel *channel)
{
	cvorg_wchan(channel, CVORG_CTL, CVORG_CTL_CHAN_RESET);
	usec_sleep(5);
}

static void disable_interrupts(struct cvorg *cvorg)
{
	/* disable interrupts */
	cvorg_writew(cvorg, CVORG_INTEN, 0);

	/* clear interrupts */
	cvorg_readw(cvorg, CVORG_INTSR);
}

static void config_interrupts(SkelDrvrModuleContext *mcon)
{
	struct cvorg *cvorg = mcon->UserData;
	InsLibIntrDesc *interrupt = mcon->Modld->Isr;

	if (interrupt == NULL) {
		SK_INFO("cvorg%d No interrupts configured", mcon->ModuleNumber);
		disable_interrupts(cvorg);
		return;
	}
	cvorg_writew(cvorg, CVORG_INTVECT, interrupt->Vector);
	cvorg_writew(cvorg, CVORG_INTLEVEL, interrupt->Level);
}

static void enable_interrupts(struct cvorg *cvorg)
{
	uint32_t mask = CVORG_INTEN_ENDFUNC;

	cvorg_writew(cvorg, CVORG_INTEN, mask & CVORG_INTEN_VALID);
}

static void cvorg_channel_cfg_init(struct cvorg_channel *channel)
{
	channel->inpol	= CVORG_POSITIVE_PULSE;
	channel->outoff = CVORG_OUT_OFFSET_0V;
	channel->outgain = 20;
	channel->out_enabled = 0;
}

static void cvorg_cfg_init(struct cvorg *cvorg)
{
	cvorg_pll_cfg_init(&cvorg->pll);
	cvorg_channel_cfg_init(&cvorg->channels[0]);
	cvorg_channel_cfg_init(&cvorg->channels[1]);
}

static SkelUserReturn __cvorg_reset(SkelDrvrModuleContext *mcon)
{
	struct cvorg *cvorg = mcon->UserData;
	SkelUserReturn ret;

	/* reset the FPGA */
	cvorg_writew(cvorg, CVORG_CTL, CVORG_CTL_FPGA_RESET);
	usec_sleep(5);

	/* reset both channels */
	channel_reset(&cvorg->channels[0]);
	channel_reset(&cvorg->channels[1]);

	/* load the default config */
	cvorg_cfg_init(cvorg);

	/* start-up clkgen */
	clkgen_startup(cvorg);

	/* apply the config */
	ret = __cvorg_apply_cfg(cvorg);

	/* initialise interrupts */
	config_interrupts(mcon);
	enable_interrupts(cvorg);

	return ret;
}

SkelUserReturn SkelUserHardwareReset(SkelDrvrModuleContext *mcon)
{
	struct cvorg *cvorg = mcon->UserData;
	SkelUserReturn ret;

	cdcm_mutex_lock(&cvorg->lock);
	ret = __cvorg_reset(mcon);
	cdcm_mutex_unlock(&cvorg->lock);

	return ret;
}

/* Note: dummy function: we use our own interrupt handler */
SkelUserReturn SkelUserGetInterruptSource(SkelDrvrModuleContext *mcon,
					  SkelDrvrTime *itim, uint32_t *isrc)
{
	bzero((void *)itim, sizeof(SkelDrvrTime));
	*isrc = 0;
	return SkelUserReturnNOT_IMPLEMENTED;
}

SkelUserReturn SkelUserEnableInterrupts(SkelDrvrModuleContext *mcon,
					uint32_t imsk)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

SkelUserReturn SkelUserHardwareEnable(SkelDrvrModuleContext *mcon, uint32_t flag)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

SkelUserReturn SkelUserJtagReadByte(SkelDrvrModuleContext *mcon, uint32_t *byte)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

SkelUserReturn SkelUserJtagWriteByte(SkelDrvrModuleContext *mcon,
				     uint32_t byte)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

static void cvorg_init_channel(struct cvorg *cvorg, int index)
{
	struct cvorg_channel *channel = &cvorg->channels[index];

	channel->parent = cvorg;

	if (index == 0)
		channel->reg_offset = CVORG_CHAN1_OFFSET;
	else
		channel->reg_offset = CVORG_CHAN2_OFFSET;
}

static void cvorg_init_channels(struct cvorg *cvorg)
{
	cvorg_init_channel(cvorg, 0);
	cvorg_init_channel(cvorg, 1);
}

static int cvorg_version_check(struct cvorg *cvorg)
{
	uint32_t val;

	if (!recoset())
		val = cvorg_readw(cvorg, CVORG_VERSION);
	else
		val = 0xffffffff;
	noreco();

	/* store the hardware revision number */
	cvorg->hw_rev = val & CVORG_VERSION_FW_MASK;

	val >>= CVORG_VERSION_FW_SHIFT;
	val &= 0xff;

	return val != 'G';
}

SkelUserReturn SkelUserModuleInit(SkelDrvrModuleContext *mcon)
{
	struct cvorg *cvorg;
	SkelUserReturn ret;

	cvorg = (void *)sysbrk(sizeof(*cvorg));

	if (cvorg == NULL) {
		SK_ERROR("Not enough memory for the module's data");
		pseterr(ENOMEM);
		return SkelUserReturnFAILED;
	}

	mcon->UserData = cvorg;

	bzero((void *)cvorg, sizeof(*cvorg));

	cvorg->iomap = get_vmemap_addr(mcon, 0x29, 32);
	if (cvorg->iomap == NULL) {
		SK_ERROR("Can't find virtual address of the module's registers");
		pseterr(EINVAL);
		goto out_free;
	}

	if (cvorg_version_check(cvorg)) {
		SK_ERROR("cvorg%d: device not present", mcon->ModuleNumber);
		pseterr(ENODEV);
		goto out_free;
	}

	cdcm_mutex_init(&cvorg->lock);
	cvorg_init_channels(cvorg);

	ret = SkelUserHardwareReset(mcon);
	if (ret)
		goto out_free;

	return 0;

 out_free:
	if (cvorg) {
		sysfree((void *)cvorg, sizeof(*cvorg));
		mcon->UserData = NULL;
	}
	return SkelUserReturnFAILED;
}

SkelUserReturn SkelUserClientInit(SkelDrvrClientContext *ccon)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

static void cvorg_quiesce_channel(struct cvorg_channel *channel)
{
	/* disable waveform playing */
	cvorg_achan(channel, CVORG_CONFIG, ~CVORG_CONFIG_SEQREADY);

	/* make sure there's nothing being played right now */
	cvorg_wchan(channel, CVORG_CTL, CVORG_CTL_CHAN_STOP);
}

void SkelUserModuleRelease(SkelDrvrModuleContext *mcon)
{
	int i;
	struct cvorg *cvorg = mcon->UserData;

	if (cvorg == NULL)
		return;

	/* disable channels */
	for (i = 0; i < CVORG_CHANNELS; i++)
		cvorg_quiesce_channel(cvorg_get_channel(mcon, i));

	/* disable interrupts */
	disable_interrupts(cvorg);

	/* free userdata */
	sysfree((void *)cvorg, sizeof(*cvorg));
}

void SkelUserClientRelease(SkelDrvrClientContext *ccon)
{
	struct cvorg *cvorg;
	int i;

	/* make sure that no modules are left owned by this client */
	for (i = 0; i < Wa->InstalledModules; i++) {
		cvorg = Wa->Modules[i].UserData;

		if (cvorg == NULL)
			continue;
		/*
		 * Using the interruptible lock here might seem more elegant,
		 * but in fact we'd screwed anyway if it was interrupted.
		 * So either we do the below or we use no locking whatsoever.
		 */
		cdcm_mutex_lock(&cvorg->lock);
		if (cvorg->owner == ccon->cdcmf)
			cvorg->owner = NULL;
		cdcm_mutex_unlock(&cvorg->lock);
	}
}

static inline uint32_t get_first_set_bit(uint32_t mask)
{
	int i;
	int retval;

	for (i = 0; i < CVORG_NR_INTERRUPTS; i++) {
		retval = 1 << i;
		if (retval & mask)
			return retval; /* only one bit per interrupt */
	}
	return 0;
}

/* Note: An interrupt is triggered from a single channel at a time. */
int cvorg_isr(void *cookie)
{
	SkelDrvrModuleContext *mcon = cookie;
	struct cvorg *cvorg = mcon->UserData;
	struct cvorg_channel *channel;
	uint32_t mask, source;

	mask = cvorg_readw(cvorg, CVORG_INTSR);

	if (!(mask & CVORG_INTSR_BITMASK))
		return SYSERR; /* no ISR coming from this module */

	/* check which channel it comes from */
	channel = cvorg_get_channel(mcon, mask & CVORG_INTSR_CHANNEL);
	source = get_first_set_bit(mask);

	switch (source) {
	case CVORG_INTSR_ENDFUNC:
	default:
		/* do nothing for the time being */
		break;
	}

	return OK;
}

int cvorg_read(void *wa, struct cdcm_file *flp, char *u_buf, int len)
{
	pseterr(ENOSYS);
	return SYSERR;
}

int cvorg_write(void *wa, struct cdcm_file *flp, char *u_buf, int len)
{
	pseterr(ENOSYS);
	return SYSERR;
}

static void free_wv(struct cvorg_wv *wv)
{
	if (!wv)
		return;
	if (!wv->form || !wv->size)
		return;
	sysfree((void *)wv->form, wv->size);
}

static void free_seq(struct cvorg_seq *seq)
{
	int i;

	if (!seq)
		return;
	if (!seq->n_waves || !seq->waves)
		goto out;
	for (i = 0; i < seq->n_waves; i++)
		free_wv(&seq->waves[i]);

 out:
	sysfree((void *)seq->waves, seq->n_waves * sizeof(struct cvorg_wv));
}

static int copy_wv_from_user(struct cvorg_wv *to, struct cvorg_wv *from)
{
	/* copy the waveform descriptor */
	if (cdcm_copy_from_user(to, from, sizeof(struct cvorg_wv))) {
		SK_ERROR("cannot copy the waveform descriptor");
		goto out_perm_err;
	}

	/* allocate space for the waveform */
	to->form = (void *)sysbrk(to->size);
	if (to->form == NULL) {
		SK_ERROR("Not enough memory to allocate the waveform");
		goto out_err;
	}

	/* copy the waveform */
	if (cdcm_copy_from_user(to->form, from->form, to->size)) {
		SK_ERROR("cannot copy the waveform from user-space");
		goto out_perm_err;
	}

	return 0;
 out_err:
	pseterr(ENOMEM);
	return -1;
 out_perm_err:
	pseterr(EFAULT);
	return -1;
}

/**
 * seq_init - initialise a sequence from a user's descriptor
 *
 * @seq:	sequence descriptor to be initialised
 * @user_seq:	user's sequence descriptor
 *
 * Note that @seq is in kernel space. However the contents of
 * any of the pointers it may have are pointing to user-space addresses.
 * This function sets errno upon failure.
 *
 * @return 0 - on success
 * @return -1 - on failure
 */
static int seq_init(struct cvorg_seq *seq, struct cvorg_seq *user_seq)
{
	int i;

	memcpy(seq, user_seq, sizeof(struct cvorg_seq));

	/* allocate space for the waveform descriptors */
	seq->waves = (void *)sysbrk(seq->n_waves * sizeof(struct cvorg_wv));
	if (seq->waves == NULL) {
		SK_ERROR("ENOMEM for the waveform descriptors (%d waves)",
			seq->n_waves);
		goto out_mem_err;
	}

	/* copy the descriptors into kernel space */
	for (i = 0; i < seq->n_waves; i++)
		if (copy_wv_from_user(&seq->waves[i], &user_seq->waves[i])) {
			SK_ERROR("Copying the waveform from user-space failed");
			goto out_err;
		}
	return 0;

 out_mem_err:
	free_seq(seq);
	pseterr(ENOMEM);
	return -1;
 out_err:
	free_seq(seq);
	return -1;
}

static ssize_t seq_size(struct cvorg_seq *seq)
{
	ssize_t size = 0;
	int i;

	for (i = 0; i < seq->n_waves; i++)
		size += seq->waves[i].size;
	return size;
}

static ssize_t avail_sram(struct cvorg_channel *channel)
{
	return CVORG_SRAM_SIZE;
}

/*
 * Get the offset on mapped memory of a descriptor block
 * Note that the blocks are written contiguously
 */
static inline unsigned int block_offset(int blocknr)
{
	return CVORG_BLKOFFSET + CVORG_BLKSIZE * blocknr;
}

/* get the offset of the previous descriptor block on mapped memory */
static inline unsigned int prev_block_offset(int blocknr)
{
	return CVORG_BLKOFFSET + CVORG_BLKSIZE * (blocknr - 1);
}

/*
 * Given two 16-bit long values, convert to a 32-bit SRAM entry
 * Note: the module plays first the rightmost u16 of a 32-bit SRAM entry
 */
static inline uint32_t cvorg_cpu_to_hw(uint16_t *data)
{
	uint32_t first = cdcm_cpu_to_be16(data[0]);
	uint32_t second = cdcm_cpu_to_be16(data[1]);

	if (Wa->Endian == InsLibEndianLITTLE)
		return (first << 16) | second;
	else
		return (second << 16) | first;
}

/*
 * Given a 32-bit long SRAM entry, convert to two 16-bit long values
 */
static inline void cvorg_hw_to_cpu(uint32_t hwval, uint16_t *data)
{
	uint32_t first, second;

	if (Wa->Endian == InsLibEndianLITTLE) {
		first = hwval >> 16;
		second = hwval & 0xffff;
	} else {
		first = hwval & 0xffff;
		second = hwval >> 16;
	}

	data[0] = cdcm_be16_to_cpu(first);
	data[1] = cdcm_be16_to_cpu(second);
}

static inline uint32_t cvorg_cpu_to_hw_single(uint16_t *data)
{
	uint32_t pointval = cdcm_cpu_to_be16(data[0]);

	if (Wa->Endian == InsLibEndianLITTLE)
		return pointval << 16;
	else
		return pointval;
}

/*
 * The two LSBs are discarded by the module.
 * Since they might be of some use in the future, we set them
 * to zero here, rounding up 3 to 4.
 */
static void normalize_wv(struct cvorg_wv *wave)
{
	uint16_t *data = (uint16_t *)wave->form;
	int len = wave->size / sizeof(uint16_t);
	int i;

	for (i = 0; i < len; i++) {
		/* round up making sure there's no overflow */
		if (data[i] & 0x3 && data[i] != 0xffff)
			data[i] += 1;

		data[i] &= ~0x3;
	}
}

/*
 * Note. the caller has to make sure that he's going to write to a valid
 * location
 */
static void
__sram(struct cvorg_channel *chan, struct cvorg_wv *wave, unsigned dest)
{
	uint16_t *data = (uint16_t *)wave->form;
	int len = wave->size / 2;
	int i;

	/* the address auto-increments on every write */
	cvorg_wchan(chan, CVORG_SRAMADDR, dest);

	/* each write stores two points */
	for (i = 0; i < len - 1; i += 2)
		cvorg_wchan_noswap(chan, CVORG_SRAMDATA, cvorg_cpu_to_hw(&data[i]));

	if (len % 2) {
		uint32_t lastpoint = cvorg_cpu_to_hw_single(&data[i]);

		cvorg_wchan_noswap(chan, CVORG_SRAMDATA, lastpoint);
	}
}

static inline void
cvorg_wv_mark_prev(struct cvorg_channel *channel, unsigned int prevblock,
		unsigned int currblock_index)
{
	unsigned int offset = prevblock + CVORG_WFNEXTBLK;

	cvorg_uchan(channel, offset, currblock_index, CVORG_WFNEXTBLK_NEXT_MASK);
}

static inline void
cvorg_wv_mark_last(struct cvorg_channel *channel, unsigned int block)
{
	unsigned int offset = block + CVORG_WFNEXTBLK;

	/* write the magic number which marks the end of the sequence */
	cvorg_uchan(channel, offset, CVORG_WFNEXTBLK_LAST, CVORG_WFNEXTBLK_NEXT_MASK);
}

static inline void
cvorg_wv_gain_store(struct cvorg_channel *channel, unsigned int block,
		struct cvorg_wv *wv)
{
	unsigned int offset = block + CVORG_WFNEXTBLK;
	unsigned int val;

	/* set to the default gain (unset the CONF_GAIN bit) */
	cvorg_achan(channel, offset, ~CVORG_WFNEXTBLK_CONF_GAIN);

	/* exit if the user didn't explicitly set the gain */
	if (!wv->dynamic_gain)
		return;

	wv->gain_val = 	cvorg_gain_approx(wv->gain_val);
	val = cvorg_gain_to_hw(wv->gain_val);

	/* set the CONF_GAIN bit */
	cvorg_ochan(channel, offset, CVORG_WFNEXTBLK_CONF_GAIN);

	/* and write the specified gain to the module */
	val <<= CVORG_WFNEXTBLK_GAIN_SHIFT;
	cvorg_uchan(channel, offset, val, CVORG_WFNEXTBLK_GAIN_MASK);
}

static inline void
cvorg_wv_store(struct cvorg_channel *channel, unsigned int block,
	struct cvorg_wv *wv)
{
	cvorg_wchan(channel, block + CVORG_WFLEN, wv->size / 2);
	cvorg_wchan(channel, block + CVORG_WFRECURR, wv->recurr);
	cvorg_wv_gain_store(channel, block, wv);
}

/*
 * Note: The caller has to make sure that there's enough memory in the channel
 * and that the SRAM can be safely accessed.
 */
static void __cvorg_storeseq(struct cvorg_channel *channel,
			struct cvorg_seq *seq)
{
	unsigned int currblock;
	unsigned int ramaddr = 0;
	struct cvorg_wv *wv;
	int i;

	/*
	 * If there's only one waveform in the sequence, the hardware
	 * only accepts 1 as the number of sequence cycles.
	 * To loop we have then to put the number of iterations in
	 * the number of waveform cycles--hence the multiplication below.
	 */
	if (seq->n_waves == 1) {
		wv = &seq->waves[0];

		wv->recurr *= seq->nr;
		seq->nr = 1;
	}

	cvorg_wchan(channel, CVORG_SEQNR, seq->nr);

	for (i = 0; i < seq->n_waves; i++) {
		wv = &seq->waves[i];

		normalize_wv(wv);

		/* write to sram */
		__sram(channel, wv, ramaddr);

		/* get the offset of the current block */
		currblock = block_offset(i);

		/* point the previous block to the current one */
		if (i > 0)
			cvorg_wv_mark_prev(channel, prev_block_offset(i), i);

		/* point the block to the address in SRAM */
		cvorg_wchan(channel, currblock + CVORG_WFSTART, ramaddr);

		/* fill the rest of the block descriptor */
		cvorg_wv_store(channel, currblock, wv);

		/* mark the last item of the sequence as such */
		if (i == seq->n_waves - 1)
			cvorg_wv_mark_last(channel, currblock);

		ramaddr += wv->size;
	}
}

/**
 * cvorg_storeseq - store a sequence
 *
 * @channel:	channel to write it to
 * @seq:	sequence to be written
 *
 * @seq is a descriptor containing the set of waveforms to be written. This
 * function checks if there's enough room in the module to host the waveforms
 * and if that's the case, the set of waveforms are written to the module.
 *
 * return 0	- on success
 * return -1	- on failure
 */
static int cvorg_storeseq(struct cvorg_channel *channel, struct cvorg_seq *seq)
{
	/* check there's enough space on SRAM */
	if (seq_size(seq) > avail_sram(channel)) {
		SK_WARN("Not enough memory on the channel for the desired "
			"waveform. Available/Requested: %zd/%zd",
			avail_sram(channel), seq_size(seq));
		goto mem_err;
	}

	if (seq->n_waves <= 0 || seq->n_waves > CVORG_MAX_SEQUENCES) {
		SK_WARN("Invalid number of desired blocks. Maximum=%d",
			CVORG_MAX_SEQUENCES);
		goto val_err;
	}

	if (__cvorg_sram_busy(channel)) {
		SK_WARN("Access to the module's SRAM timed out");
		goto sram_err;
	}

	__cvorg_storeseq(channel, seq);

	return 0;

 sram_err:
	pseterr(EAGAIN);
	return -1;
 mem_err:
	pseterr(ENOMEM);
	return -1;
 val_err:
	pseterr(EINVAL);
	return -1;
}

static void cvorg_poll_startable(struct cvorg_channel *channel)
{
	uint32_t reg;
	int i;

	reg = cvorg_rchan(channel, CVORG_STATUS);
	i = 0;
	while (reg & CVORG_STATUS_SRAM_BUSY ||
		!(reg & CVORG_STATUS_STARTABLE)) {
		if (i > 20) {
			SK_INFO("Timeout expired: module not startable or RAM busy");
			return;
		}
		usec_sleep(10);
		reg = cvorg_rchan(channel, CVORG_STATUS);
		i++;
	}
}

static void play_wv(struct cvorg_channel *channel)
{
	uint32_t cmd;

	if (!__cvorg_channel_ready(channel))
		SK_WARN("Channel not ready");

	/* Mark the sequence in SRAM as valid and set to normal mode */
	cmd = CVORG_CONFIG_MODE_NORMAL | CVORG_CONFIG_SEQREADY;
	cvorg_uchan(channel, CVORG_CONFIG, cmd, CVORG_CONFIG_MODE);

	/* Go read the sequence from SRAM */
	cvorg_wchan(channel, CVORG_CTL, CVORG_CTL_CHAN_LOADSEQ);

	/* sleep until the module can accept START triggers */
	cvorg_poll_startable(channel);
}

static int __cvorg_loadseq(struct cvorg_channel *channel, struct cvorg_seq *seq)
{
	/* not even an external trigger will disturb us from now on */
	cvorg_quiesce_channel(channel);

	/* write sequence to SRAM */
	if (cvorg_storeseq(channel, seq))
		return -1;

	/* issue the write */
	play_wv(channel);

	return 0;
}

static SkelUserReturn
cvorg_invalid_seq(struct cvorg_channel *channel, struct cvorg_seq *seq)
{
	int i;
	struct cvorg_wv *wv;

	if (!seq->n_waves) {
		SK_INFO("seq->n_waves invalid");
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	if (!seq->waves) {
		SK_INFO("empty seq->waves");
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	for (i = 0; i < seq->n_waves; i++) {
		wv = &seq->waves[i];

		if (cvorg_check_gain_offset_wv(channel, wv)) {
			SK_INFO("wave %d: Invalid gain", i);
			pseterr(EINVAL);
			return SkelUserReturnFAILED;
		}

		if (cvorg_invalid_freqlimit(channel, seq, wv)) {
			SK_INFO("wave %d: not enough points", i);
			pseterr(EINVAL);
			return SkelUserReturnFAILED;
		}

		if (cvorg_rchan(channel, CVORG_SAMPFREQ) > 50000000 &&
			wv->size / sizeof(uint16_t) < 2) {
			SK_INFO("wave %d: at least 2 points are required "
				"at frequencies higher than 50MHz", i);
			pseterr(EINVAL);
			return SkelUserReturnFAILED;
		}
	}

	return SkelUserReturnOK;
}

/*
 * The waveforms in the sequence might have been updated by the driver.
 * Think, for instance, of fields that can be rounded, such as the
 * value of the dynamic gain.
 * This function writes back the updated values of these fields into user-space.
 */
static int seq_user_writeback(struct cvorg_seq *useq, struct cvorg_seq *kseq)
{
	int i;
	struct cvorg_wv *uwv;
	struct cvorg_wv *kwv;

	for (i = 0; i < kseq->n_waves; i++) {
		kwv = &kseq->waves[i];
		uwv = &useq->waves[i];

		if (!kwv->dynamic_gain)
			continue;

		if (cdcm_copy_to_user(&uwv->gain_val, &kwv->gain_val,
					sizeof(&kwv->gain_val))) {
			SK_ERROR("Copying the gain value to user-space failed");
			pseterr(EFAULT);
			return -1;
		}
	}
	return 0;
}

/* write a sequence to a channel and play it */
static SkelUserReturn cvorg_loadseq(struct cvorg_channel *channel, void *arg)
{
	struct cvorg_seq *user_seq = arg;
	struct cvorg_seq seq;
	SkelUserReturn ret = SkelUserReturnOK;

	/*
	 * We're going to load a new configuration. We don't care if a
	 * previous faulty config caused an error, so we wipe it out.
	 */
	__cvorg_channel_clear_errors(channel);

	if (!__cvorg_channel_ready(channel)) {
		pseterr(EBUSY);
		return SkelUserReturnFAILED;
	}

	ret = cvorg_invalid_seq(channel, user_seq);
	if (ret)
		return ret;

	if (seq_init(&seq, user_seq)) {
		SK_INFO("Initialisation of the sequence failed");
		return SkelUserReturnFAILED;
	}

	if (__cvorg_loadseq(channel, &seq)) {
		SK_INFO("__cvorg_loadseq failed");
		ret = SkelUserReturnFAILED;
		goto out;
	}

	if (seq_user_writeback(user_seq, &seq)) {
		SK_INFO("seq_copy_to_user failed");
		ret = SkelUserReturnFAILED;
		goto out;
	}

 out:
	free_seq(&seq);
	return ret;
}

static SkelUserReturn
gs_channel_ioctl(SkelDrvrClientContext *ccon, void *arg, int set)
{
	int *argp = arg;

	if (set) {
		if (!WITHIN_RANGE(CVORG_CHANNEL_A, *argp, CVORG_CHANNEL_B)) {
			pseterr(EINVAL);
			return SkelUserReturnFAILED;
		}
		ccon->ChannelNr = *argp - 1;
		return SkelUserReturnOK;
	}
	*argp = ccon->ChannelNr + 1;
	return SkelUserReturnOK;
}

static SkelUserReturn
gs_outoff_ioctl(struct cvorg_channel *channel, void *arg, int set)
{
	uint32_t *offset = arg;

	if (set) {
		if (cvorg_invalid_gain_offset(channel->outgain, *offset)) {
			SK_INFO("With an offset of 2.5V, a gain of 14dB must "
				"be applied.");
			pseterr(EINVAL);
			return SkelUserReturnFAILED;
		}

		switch (*offset) {
		case CVORG_OUT_OFFSET_2_5V:
			channel->outoff = CVORG_OUT_OFFSET_2_5V;
			break;
		case CVORG_OUT_OFFSET_0V:
		default:
			channel->outoff = CVORG_OUT_OFFSET_0V;
			break;
		}

		__cvorg_apply_channel_outoff(channel);
		return SkelUserReturnOK;
	}

	*offset = channel->outoff;
	return SkelUserReturnOK;
}

static SkelUserReturn
gs_inpol_ioctl(struct cvorg_channel *channel, void *arg, int set)
{
	enum cvorg_inpol *polarity = arg;

	if (set) {
		if (__cvorg_channel_busy(channel)) {
			pseterr(EBUSY);
			return SkelUserReturnFAILED;
		}

		if (*polarity)
			channel->inpol = CVORG_NEGATIVE_PULSE;
		else
			channel->inpol = CVORG_POSITIVE_PULSE;
		__cvorg_apply_channel_inpol(channel);

		return SkelUserReturnOK;
	}

	*polarity = channel->inpol;
	return SkelUserReturnOK;
}

/* Software triggers: start, stop, event stop. */
static SkelUserReturn s_trig_ioctl(struct cvorg_channel *channel, void *arg)
{
	uint32_t *trig = arg;

	switch (*trig) {
	case CVORG_TRIGGER_START:
		cvorg_wchan(channel, CVORG_CTL, CVORG_CTL_CHAN_START);
		break;
	case CVORG_TRIGGER_STOP:
		cvorg_wchan(channel, CVORG_CTL, CVORG_CTL_CHAN_STOP);
		break;
	case CVORG_TRIGGER_EVSTOP:
		cvorg_wchan(channel, CVORG_CTL, CVORG_CTL_CHAN_EVSTOP);
		break;
	default:
		SK_INFO("Unknown trigger event (0x%x)", *trig);
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}
	return SkelUserReturnOK;
}

/* Note: the PLL is common to both channels */
static int __cvorg_apply_pll(struct cvorg *cvorg)
{
	SkelUserReturn ret = SkelUserReturnOK;

	if (clkgen_apply_pll_conf(cvorg)) {
		ret = SkelUserReturnFAILED;
		goto out;
	}

	if (cvorg_poll_sampfreq(cvorg)) {
		ret = SkelUserReturnFAILED;
		goto out;
	}

 out:
	return ret;
}

/*
 * each client has its own's PLL configuration. When the client issues a write,
 * his particular configuration is set in the hardware.
 */
static SkelUserReturn gs_pll_ioctl(struct cvorg *cvorg, void *arg, int set)
{
	struct ad9516_pll *pll = arg;

	if (!set) {
		memcpy(pll, &cvorg->pll, sizeof(*pll));
		return SkelUserReturnOK;
	}

	if (__cvorg_busy(cvorg)) {
		pseterr(EBUSY);
		return SkelUserReturnFAILED;
	}

	/* sanity check */
	if (clkgen_check_pll(pll))
		return SkelUserReturnFAILED;

	memcpy(&cvorg->pll, pll, sizeof(*pll));
	return __cvorg_apply_pll(cvorg);

	return SkelUserReturnOK;
}

static SkelUserReturn
gs_sram_ioctl(struct cvorg_channel *channel, void *arg, int set)
{
	struct cvorg_sram_entry *entry = arg;

	if (set && __cvorg_channel_busy(channel)) {
		pseterr(EBUSY);
		return SkelUserReturnFAILED;
	}

	/* sanity check */
	if (entry->address >= CVORG_SRAM_SIZE) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	if (__cvorg_sram_busy(channel)) {
		SK_INFO("Access to SRAM timed out");
		pseterr(EAGAIN);
		return SkelUserReturnFAILED;
	}

	/* avoid unaligned accesses */
	cvorg_wchan(channel, CVORG_SRAMADDR, entry->address & ~0x3);

	if (!set) {
		uint32_t rval = cvorg_rchan_noswap(channel, CVORG_SRAMDATA);

		cvorg_hw_to_cpu(rval, entry->data);
		return SkelUserReturnOK;
	}

	cvorg_wchan_noswap(channel, CVORG_SRAMDATA, cvorg_cpu_to_hw(entry->data));

	return SkelUserReturnOK;
}

static SkelUserReturn cvorg_g_chanstat(struct cvorg_channel *channel, void *arg)
{
	uint32_t *status = arg;
	uint32_t val;

	*status = 0;

	val = cvorg_rchan(channel, CVORG_STATUS);

	if (val & CVORG_STATUS_READY)
		*status |= CVORG_CHANSTAT_READY;

	if (val & CVORG_STATUS_BUSY)
		*status |= CVORG_CHANSTAT_BUSY;

	if (val & CVORG_STATUS_SRAM_BUSY)
		*status |= CVORG_CHANSTAT_SRAM_BUSY;

	if (val & CVORG_STATUS_ERR)
		*status |= CVORG_CHANSTAT_ERR;

	if (val & CVORG_STATUS_ERR_CLK)
		*status |= CVORG_CHANSTAT_ERR_CLK;

	if (val & CVORG_STATUS_ERR_TRIG)
		*status |= CVORG_CHANSTAT_ERR_TRIG;

	if (val & CVORG_STATUS_ERR_SYNC)
		*status |= CVORG_CHANSTAT_ERR_SYNC;

	if (val & CVORG_STATUS_CLKOK)
		*status |= CVORG_CHANSTAT_CLKOK;

	if (val & CVORG_STATUS_STARTABLE)
		*status |= CVORG_CHANSTAT_STARTABLE;

	val = cvorg_rchan(channel, CVORG_CONFIG);
	if (val & CVORG_CONFIG_OUT_ENABLE)
		*status |= CVORG_CHANSTAT_OUT_ENABLED;

	return SkelUserReturnOK;
}

static SkelUserReturn cvorg_g_sampfreq(struct cvorg *cvorg, void *arg)
{
	uint32_t *freq = arg;

	*freq = cvorg_readw(cvorg, CVORG_SAMPFREQ);
	return SkelUserReturnOK;
}

static SkelUserReturn cvorg_lock(struct cvorg *cvorg, struct cdcm_file *cdcmf)
{
	cvorg->owner = cdcmf;
	return SkelUserReturnOK;
}

static SkelUserReturn cvorg_unlock(struct cvorg *cvorg)
{
	cvorg->owner = NULL;
	return SkelUserReturnOK;
}

static enum cvorg_mode cvorg_hw_to_mode(uint32_t regval)
{
	switch (regval) {
	case CVORG_CONFIG_MODE_OFF:
		return CVORG_MODE_OFF;

	case CVORG_CONFIG_MODE_NORMAL:
		return CVORG_MODE_NORMAL;

	case CVORG_CONFIG_MODE_DAC:
		return CVORG_MODE_DAC;

	case CVORG_CONFIG_MODE_TEST1:
		return CVORG_MODE_TEST1;

	case CVORG_CONFIG_MODE_TEST2:
		return CVORG_MODE_TEST2;

	case CVORG_CONFIG_MODE_TEST3:
		return CVORG_MODE_TEST3;

	default:
		SK_WARN("Unknown operation mode. Returning OFF");
		return CVORG_MODE_OFF;
	}
}

static int cvorg_mode_is_valid(uint32_t mode)
{
	switch (mode) {
	case CVORG_MODE_OFF:
	case CVORG_MODE_NORMAL:
	case CVORG_MODE_DAC:
	case CVORG_MODE_TEST1:
	case CVORG_MODE_TEST2:
	case CVORG_MODE_TEST3:
		return 1;
	default:
		return 0;
	}
}

static uint32_t cvorg_mode_to_hw(uint32_t mode)
{
	switch (mode) {
	case CVORG_MODE_OFF:
		return CVORG_CONFIG_MODE_OFF;

	case CVORG_MODE_NORMAL:
		return CVORG_CONFIG_MODE_NORMAL;

	case CVORG_MODE_DAC:
		return CVORG_CONFIG_MODE_DAC;

	case CVORG_MODE_TEST1:
		return CVORG_CONFIG_MODE_TEST1;

	case CVORG_MODE_TEST2:
		return CVORG_CONFIG_MODE_TEST2;

	case CVORG_MODE_TEST3:
		return CVORG_CONFIG_MODE_TEST3;

	default:
		SK_WARN("Unknown mode (0x%x). Setting to NORMAL", mode);
		return CVORG_CONFIG_MODE_NORMAL;
	}
}

static void cvorg_mode_set(struct cvorg_channel *channel, enum cvorg_mode mode)
{
	uint32_t regval;

	regval = cvorg_rchan(channel, CVORG_CONFIG);
	regval &= ~CVORG_CONFIG_MODE;
	regval |= cvorg_mode_to_hw(mode);

	cvorg_wchan(channel, CVORG_CONFIG, regval);
}

static enum cvorg_mode cvorg_mode_get(struct cvorg_channel *channel)
{
	uint32_t reg = cvorg_rchan(channel, CVORG_CONFIG);

	return cvorg_hw_to_mode(reg & CVORG_CONFIG_MODE);
}

static SkelUserReturn
cvorg_gs_mode_ioctl(struct cvorg_channel *channel, void *arg, int set)
{
	uint32_t *val = arg;

	if (!set) {
		*val = cvorg_mode_get(channel);
		return SkelUserReturnOK;
	}

	if (!cvorg_mode_is_valid(*val)) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}
	/*
	 * we don't check here if the channel is busy because this is only used
	 * by experts when testing the module.
	 */
	cvorg_mode_set(channel, *val);

	return SkelUserReturnOK;
}

static SkelUserReturn
cvorg_gs_outgain_ioctl(struct cvorg_channel *channel, void *arg, int set)
{
	int32_t *outgain = arg;

	if (set) {
		int32_t gain = cvorg_gain_approx(*outgain);

		if (__cvorg_channel_busy(channel)) {
			pseterr(EBUSY);
			return SkelUserReturnFAILED;
		}

		if (cvorg_invalid_gain_offset(gain, channel->outoff)) {
			SK_INFO("With an offset of 2.5V, only a gain of 14dB "
				"can be applied.");
			pseterr(EINVAL);
			return SkelUserReturnFAILED;
		}

		channel->outgain = gain;
		__cvorg_apply_channel_outgain(channel);
		*outgain = gain;
		return SkelUserReturnOK;
	}

	*outgain = channel->outgain;
	return SkelUserReturnOK;
}

static int cvorg_adc_channel_to_hw(enum cvorg_adc_channel channel, uint32_t *reg)
{
	uint32_t val;

	switch (channel) {
	case CVORG_ADC_CHANNEL_MONITOR:
		val = CVORG_CONFIG_ADCSELECT_MONITOR;
		break;
	case CVORG_ADC_CHANNEL_REF:
		val = CVORG_CONFIG_ADCSELECT_REF;
		break;
	case CVORG_ADC_CHANNEL_5V:
		val = CVORG_CONFIG_ADCSELECT_5V;
		break;
	case CVORG_ADC_CHANNEL_GND:
		val = CVORG_CONFIG_ADCSELECT_GND;
		break;
	default:
		return -1;
	}

	*reg &= ~CVORG_CONFIG_ADCSELECT_MASK;
	*reg |= val << CVORG_CONFIG_ADCSELECT_SHIFT;
	return 0;
}

static int cvorg_adc_range_to_hw(enum cvorg_adc_range range, uint32_t *reg)
{
	uint32_t val;

	switch (range) {
	case CVORG_ADC_RANGE_5V_BIPOLAR:
		val = CVORG_CONFIG_ADCRANGE_5V_BIPOLAR;
		break;
	case CVORG_ADC_RANGE_5V_UNIPOLAR:
		val = CVORG_CONFIG_ADCRANGE_5V_UNIPOLAR;
		break;
	case CVORG_ADC_RANGE_10V_BIPOLAR:
		val = CVORG_CONFIG_ADCRANGE_10V_BIPOLAR;
		break;
	case CVORG_ADC_RANGE_10V_UNIPOLAR:
		val = CVORG_CONFIG_ADCRANGE_10V_UNIPOLAR;
		break;
	default:
		return -1;
	}

	*reg &= ~CVORG_CONFIG_ADCRANGE_MASK;
	*reg |= val << CVORG_CONFIG_ADCRANGE_SHIFT;
	return 0;
}

static SkelUserReturn
cvorg_adc_read_ioctl(struct cvorg_channel *channel, void *arg)
{
	struct cvorg_adc *adc = arg;
	uint32_t reg;
	int i;

	if (__cvorg_channel_busy(channel)) {
		pseterr(EBUSY);
		return SkelUserReturnFAILED;
	}

	/* select the ADC channel and range */
	reg = 0;

	if (cvorg_adc_channel_to_hw(adc->channel, &reg)) {
		SK_INFO("Invalid ADC Channel");
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	if (cvorg_adc_range_to_hw(adc->range, &reg)) {
		SK_INFO("Invalid ADC Range");
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}
	cvorg_uchan(channel, CVORG_CONFIG, reg,
		CVORG_CONFIG_ADCSELECT_MASK | CVORG_CONFIG_ADCRANGE_MASK);

	/* trigger an ADC read on the configured ADC channel */
	cvorg_wchan(channel, CVORG_CTL, CVORG_CTL_READADC);

	/* wait for the ADC value to be up to date */
	reg = cvorg_rchan(channel, CVORG_ADCVAL);
	i = 0;
	while (!(reg & CVORG_ADCVAL_UP2DATE)) {
		if (i > 10) {
			SK_INFO("Timeout expired for reading the ADC Value");
			pseterr(EAGAIN);
			return SkelUserReturnFAILED;
		}
		usec_sleep(10);
		reg = cvorg_rchan(channel, CVORG_ADCVAL);
		i++;
	}

	/* fetch the valid ADC value */
	adc->value = reg & CVORG_ADCVAL_READVAL;

	return SkelUserReturnOK;
}

/* experts-only function: no need to check if the module's busy */
static SkelUserReturn
cvorg_dac_val_ioctl(struct cvorg_channel *channel, void *arg, int set)
{
	uint32_t *val = arg;

	if (set)
		cvorg_wchan(channel, CVORG_DACVAL, *val & CVORG_DACVAL_MASK);
	else
		*val = cvorg_rchan(channel, CVORG_DACVAL);

	return SkelUserReturnOK;
}

/* experts-only function: no need to check if the module's busy */
static SkelUserReturn
cvorg_dac_gain_ioctl(struct cvorg_channel *channel, void *arg, int set)
{
	uint16_t *val = arg;

	if (set) {
		cvorg_uchan(channel, CVORG_DACCORR, *val & CVORG_DACCORR_GAIN_MASK,
			CVORG_DACCORR_GAIN_MASK);
	} else {
		uint32_t regval = cvorg_rchan(channel, CVORG_DACCORR);

		regval &= CVORG_DACCORR_GAIN_MASK;
		*val = (uint16_t)regval;
	}

	return SkelUserReturnOK;
}

/* experts-only function: no need to check if the module's busy */
static SkelUserReturn
cvorg_dac_offset_ioctl(struct cvorg_channel *channel, void *arg, int set)
{
	int16_t *offset = arg;
	uint32_t regval;

	if (set) {
		/* the offset is represented by 15 bits in 2's complement */
		if (!WITHIN_RANGE(-(1 << 14), *offset, (1 << 14) - 1)) {
			SK_INFO("Gain correction out of range");
			pseterr(EINVAL);
			return SkelUserReturnFAILED;
		}
		/*
		 * Once we know it cannot overflow, converting to 15 bits
		 * in 2's complement is just a matter of trimming it down.
		 */
		regval = (*offset & 0x7fff) << CVORG_DACCORR_OFFSET_SHIFT;

		cvorg_uchan(channel, CVORG_DACCORR, regval,
			CVORG_DACCORR_OFFSET_MASK);
	} else {
		uint16_t val;

		regval = cvorg_rchan(channel, CVORG_DACCORR);

		regval &= CVORG_DACCORR_OFFSET_MASK;
		regval >>= CVORG_DACCORR_OFFSET_SHIFT;

		val = (int16_t)regval;
		/* sign extension because of two's complement in 15 bits */
		if (val & 0x4000)
			regval |= 0x8000;

		*offset = val;
	}

	return SkelUserReturnOK;
}

static SkelUserReturn cvorg_get_temperature(struct cvorg *cvorg, void *arg)
{
	int32_t *temperature = arg;
	uint32_t regtemp = cvorg_readw(cvorg, CVORG_TEMP);
	int temp;

	temp = (regtemp & 0x7ff) >> 4;
	if (regtemp & 0x800) /* sign bit */
		temp *= -1;

	*temperature = temp;
	return SkelUserReturnOK;
}

/*
 * Calling this function while playing a waveform only affects subsequent
 * start pulses. This means the current sequence will go on normally;
 * however, if the output has been disabled, start pulses arriving
 * after the sequence has finished will be ignored.
 */
static SkelUserReturn
cvorg_out_enable_ioctl(struct cvorg_channel *channel, void *arg)
{
	int32_t *enable = arg;

	channel->out_enabled = !!*enable;
	__cvorg_apply_channel_out_enable(channel);

	return SkelUserReturnOK;
}

SkelUserReturn SkelUserIoctls(SkelDrvrClientContext *ccon,
			SkelDrvrModuleContext *mcon, int cm, char *u_arg)
{
	void *arg = u_arg;
	struct cvorg *cvorg = cvorg_get(ccon);
	struct cvorg_channel *channel = cvorg_get_channel_ccon(ccon);
	SkelUserReturn ret;

	/*
	 * By being clever with the IOCTL numbers, we can concentrate all
	 * the locking/ownership code here.
	 */
	if (_IOC_NR(cm) > CVORG_IOCTL_NO_LOCKING) {
		if (cdcm_mutex_lock_interruptible(&cvorg->lock)) {
			pseterr(EINTR);
			return SkelUserReturnFAILED;
		}
		if (_IOC_NR(cm) > CVORG_IOCTL_LOCKING) {
			if (__cvorg_check_perm(cvorg, ccon->cdcmf)) {
				pseterr(EACCES);
				ret = SkelUserReturnFAILED;
				goto out;
			}
		}
	}

	switch (cm) {
	case CVORG_IOCSCHANNEL:
		ret = gs_channel_ioctl(ccon, arg, 1);
		break;
	case CVORG_IOCGCHANNEL:
		ret = gs_channel_ioctl(ccon, arg, 0);
		break;

	case CVORG_IOCSOUTOFFSET:
		ret = gs_outoff_ioctl(channel, arg, 1);
		break;
	case CVORG_IOCGOUTOFFSET:
		ret = gs_outoff_ioctl(channel, arg, 0);
		break;

	case CVORG_IOCSINPOLARITY:
		ret = gs_inpol_ioctl(channel, arg, 1);
		break;
	case CVORG_IOCGINPOLARITY:
		ret = gs_inpol_ioctl(channel, arg, 0);
		break;

	case CVORG_IOCSTRIGGER:
		ret = s_trig_ioctl(channel, arg);
		break;

	case CVORG_IOCSPLL:
		ret = gs_pll_ioctl(cvorg, arg, 1);
		break;
	case CVORG_IOCGPLL:
		ret = gs_pll_ioctl(cvorg, arg, 0);
		break;

	case CVORG_IOCSSRAM:
		ret = gs_sram_ioctl(channel, arg, 1);
		break;
	case CVORG_IOCGSRAM:
		ret = gs_sram_ioctl(channel, arg, 0);
		break;

	case CVORG_IOCSLOADSEQ:
		ret = cvorg_loadseq(channel, arg);
		break;

	case CVORG_IOCGCHANSTAT:
		ret = cvorg_g_chanstat(channel, arg);
		break;

	case CVORG_IOCGSAMPFREQ:
		ret = cvorg_g_sampfreq(cvorg, arg);
		break;

	case CVORG_IOCTLOCK:
		ret = cvorg_lock(cvorg, ccon->cdcmf);
		break;
	case CVORG_IOCTUNLOCK:
	case CVORG_IOCTUNLOCK_FORCE:
		/* Note that with _FORCE set, cvorg->owner hasn't been checked */
		ret = cvorg_unlock(cvorg);
		break;

	case CVORG_IOCSMODE:
		ret = cvorg_gs_mode_ioctl(channel, arg, 1);
		break;
	case CVORG_IOCGMODE:
		ret = cvorg_gs_mode_ioctl(channel, arg, 0);
		break;

	case CVORG_IOCSOUTGAIN:
		ret = cvorg_gs_outgain_ioctl(channel, arg, 1);
		break;
	case CVORG_IOCGOUTGAIN:
		ret = cvorg_gs_outgain_ioctl(channel, arg, 0);
		break;

	case CVORG_IOCGADC_READ:
		ret = cvorg_adc_read_ioctl(channel, arg);
		break;

	case CVORG_IOCSDAC_VAL:
		ret = cvorg_dac_val_ioctl(channel, arg, 1);
		break;
	case CVORG_IOCGDAC_VAL:
		ret = cvorg_dac_val_ioctl(channel, arg, 0);
		break;

	case CVORG_IOCSDAC_GAIN:
		ret = cvorg_dac_gain_ioctl(channel, arg, 1);
		break;
	case CVORG_IOCGDAC_GAIN:
		ret = cvorg_dac_gain_ioctl(channel, arg, 0);
		break;

	case CVORG_IOCSDAC_OFFSET:
		ret = cvorg_dac_offset_ioctl(channel, arg, 1);
		break;
	case CVORG_IOCGDAC_OFFSET:
		ret = cvorg_dac_offset_ioctl(channel, arg, 0);
		break;

	case CVORG_IOCGTEMPERATURE:
		ret = cvorg_get_temperature(cvorg, arg);
		break;

	case CVORG_IOCSOUT_ENABLE:
		ret = cvorg_out_enable_ioctl(channel, arg);
		break;

	default:
		pseterr(ENOTTY);
		ret = SkelUserReturnNOT_IMPLEMENTED;
		break;
	}

 out:
	if (_IOC_NR(cm) > CVORG_IOCTL_NO_LOCKING)
		cdcm_mutex_unlock(&cvorg->lock);

	return ret;
}

struct skel_conf SkelConf = {
	.read = cvorg_read,
	.write = cvorg_write,
	.intrhandler = cvorg_isr
};
