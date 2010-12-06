/*
 * sis3320.c
 * sis33 driver for Struck's SIS3320, SIS3320-250 and SIS3302.
 *
 * Copyright (c) 2009-2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/err.h>

#include <asm/atomic.h>

#include <vmebus.h>

#include "sis33core.h"
#include "sis3320.h"

#define DRV_NAME		"sis3320"
#define DRV_MODULE_VERSION	"1.0"

#define SIS3320_MAX_NR_EVENTS	512
#define SIS3320_NR_CHANNELS	8

#define SIS3320_200_CLK_SOURCES	3
#define SIS3320_250_CLK_SOURCES	4
#define SIS3302_CLK_SOURCES	5

/*
 * 32 Msamples / 128 segments = 256Ksamples per segment.
 * We could use an arbitrary power of two for the maximum number of segments,
 * but really there's not much use in using a value greater than 2.
 */
#define SIS3320_MAX_NR_SEGMENTS	2

/**
 * struct sis3320 - sis3320 device
 * @vme_base:	VME base address of the module
 * @version:	board version number
 * @completion:	completion to signal when an acquisition has finished
 * @pdev:	parent (physical) device
 * @curr_page:	caches the current ADC memory page.
 *
 * NOTE: when printing to the kernel log, we use card->dev (the device that
 * user-space sees) only when we're sure that is already there. Otherwise,
 * we use pdev, which is the physical device and is always present.
 */
struct sis3320 {
	unsigned long		vme_base;
	unsigned int		version;
	struct completion	completion;
	struct device		*pdev;
	unsigned int		curr_page;
};

static long base[SIS33_MAX_DEVICES];
static unsigned int base_num;
static int vector[SIS33_MAX_DEVICES];
static unsigned int vector_num;
static int level[SIS33_MAX_DEVICES];
static unsigned int level_num;
static int index[SIS33_MAX_DEVICES] = SIS33_DEFAULT_IDX;

module_param_array(base, long, &base_num, 0444);
MODULE_PARM_DESC(base, "Base address of the sis3320 card");
module_param_array(vector, int, &vector_num, 0444);
MODULE_PARM_DESC(vector, "IRQ vector");
module_param_array(level, int, &level_num, 0444);
MODULE_PARM_DESC(level, "IRQ level");
module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for sis3320 card");

static const u32 sis3320_ev_lengths[SIS3320_EV_LENGTHS] = {
	[0]	= 33554432,
	[1]	= 16777216,
	[2]	= 4194304,
	[3]	= 1048576,
	[4]	= 262144,
	[5]	= 65536,
	[6]	= 16384,
	[7]	= 4096,
	[8]	= 1024,
	[9]	= 512,
	[10]	= 256,
	[11]	= 128,
	[12]	= 64,
};
#define SIS3320_NR_EV_LENGTHS	ARRAY_SIZE(sis3320_ev_lengths)

static const u32 sis3320_ev_length_bits[SIS3320_EV_LENGTHS] = {
	[0]	= 0, /* this corresponds to 32MB and should never be used */
	[1]	= EV_LENGTH_16M,
	[2]	= EV_LENGTH_4M,
	[3]	= EV_LENGTH_1M,
	[4]	= EV_LENGTH_256K,
	[5]	= EV_LENGTH_64K,
	[6]	= EV_LENGTH_16K,
	[7]	= EV_LENGTH_4K,
	[8]	= EV_LENGTH_1K,
	[9]	= EV_LENGTH_512,
	[10]	= EV_LENGTH_256,
	[11]	= EV_LENGTH_128,
	[12]	= EV_LENGTH_64,
};

/*
 * NOTE:
 * in the arrays below, frequencies must be ordered from higher to lower.
 */
static const u32 sis3320_200_freqs[SIS3320_200_CLK_SOURCES] = {
	[0]	= 200000000,
	[1]	= 100000000,
	[2]	= 50000000,
};
#define SIS3320_200_NR_FREQS	ARRAY_SIZE(sis3320_200_freqs)

static const u32 sis3320_200_freq_bits[SIS3320_200_NR_FREQS] = {
	[0]	= CLKSRC_200,
	[1]	= CLKSRC_100,
	[2]	= CLKSRC_50,
};

static const u32 sis3320_250_freqs[SIS3320_250_CLK_SOURCES] = {
	[0]	= 250000000,
	[1]	= 200000000,
	[2]	= 100000000,
	[3]	= 50000000,
};
#define SIS3320_250_NR_FREQS	ARRAY_SIZE(sis3320_250_freqs)

static const u32 sis3320_250_freq_bits[SIS3320_250_NR_FREQS] = {
	[0]	= CLKSRC_250,
	[1]	= CLKSRC_200,
	[2]	= CLKSRC_100,
	[3]	= CLKSRC_50,
};

static const u32 sis3302_freqs[SIS3302_CLK_SOURCES] = {
	[0]	= 100000000,
	[1]	= 50000000,
	[2]	= 25000000,
	[3]	= 10000000,
	[4]	= 1000000,
};
#define SIS3302_NR_FREQS	ARRAY_SIZE(sis3302_freqs)

static const u32 sis3302_freq_bits[SIS3302_NR_FREQS] = {
	[0]	= CLKSRC_3302_100,
	[1]	= CLKSRC_3302_50,
	[2]	= CLKSRC_3302_25,
	[3]	= CLKSRC_3302_10,
	[4]	= CLKSRC_3302_1,
};

static inline unsigned int sis3320_readw(struct sis3320 *sis, ssize_t offset)
{
	unsigned int val;

	val = sis33_dma_read(sis->pdev, sis->vme_base + offset);
	dev_dbg(sis->pdev, "readw:\t0x%8x\tval: 0x%8x\n", (unsigned int)offset, val);

	return val;
}

static inline void sis3320_writew(struct sis3320 *sis, ssize_t offset, u32 val)
{
	dev_dbg(sis->pdev, "writew:\t\t0x%8x\tval: 0x%8x\n", (unsigned int)offset, (unsigned int)val);
	sis33_dma_write(sis->pdev, sis->vme_base + offset, val);
}

/* Note: Event descriptors of the same channel are contiguous */
static inline struct sis33_event *sis3320_get_event(struct sis33_segment *segment, int channel, int event_nr)
{
	return &segment->events[channel * segment->nr_events + event_nr];
}

/* return the initial address (in samples, 4-byte aligned) of the segment */
static ssize_t sis3320_get_segment_base(const struct sis33_card *card, int segment)
{
	return (segment * (card->ev_lengths[0] / card->n_segments)) & 0x01fffffc;
}

static void
sis3320_process_event(struct sis33_card *card, int segment_nr, int channel,
		      u32 *raw_events, int event_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];
	struct sis33_event *event;
	unsigned int next_sample;
	u32 reg;

	event = sis3320_get_event(segment, channel, event_nr);
	reg = raw_events[event_nr];

	next_sample = reg & EV_DIR_ADDR;
	/* convert to an offset relative to the first sample */
	next_sample -= event_nr * segment->nr_samp_per_ev;
	next_sample -= sis3320_get_segment_base(card, segment_nr);

	/* acquisitions are done in groups of 4 samples */
	next_sample &= ~0x3;
	if (reg & EV_DIR_WRAP) {
		event->nr_samples = segment->nr_samp_per_ev;
		event->first_samp = next_sample;
	} else {
		event->nr_samples = next_sample;
		event->first_samp = 0;
	}
}

static int
sis3320_read_events_dir(struct sis33_card *card, int segment_nr, int channel,
			u32 *raw_events, int nr_events)
{
	struct sis3320 *priv = card->private_data;
	unsigned int vme_addr;
	int ret;
	int i;

	/*
	 * All the event descriptors for this channel are fetched in a single
	 * BLT read and stored in the raw_events array.
	 * This is much quicker than doing an SCT access per event.
	 * Note that the boards don't support MBLT on the event directory.
	 */
	vme_addr = priv->vme_base + SIS3320_EV_DIR(channel);
	ret = sis33_dma_read32be_blt(card->dev, vme_addr, raw_events, nr_events);
	if (ret)
		return ret;

	for (i = 0; i < nr_events; i++)
		sis3320_process_event(card, segment_nr, channel, raw_events, i);
	return 0;
}

static void sis3320_read_segment(struct sis33_card *card, int segment_nr)
{
	struct sis3320 *priv = card->private_data;
	struct sis33_segment *segment = &card->segments[segment_nr];
	u32 *raw_events;
	int i;

	if (segment->events && !IS_ERR(segment->events)) {
		kfree(segment->events);
		segment->events = NULL;
		segment->nr_events = 0;
	}

	segment->nr_events = sis3320_readw(priv, SIS3320_EV_COUNTER);
	segment->nr_events &= EV_COUNTER_MASK;
	if (!segment->nr_events)
		return;

	raw_events = kmalloc(segment->nr_events * sizeof(*raw_events), GFP_KERNEL);
	if (raw_events == NULL) {
		segment->events = ERR_PTR(-ENOMEM);
		dev_warn(card->dev, "Couldn't allocate raw events' buffer for handling the ev. directory\n");
		return;
	}

	segment->events = kcalloc(segment->nr_events * card->n_channels, sizeof(struct sis33_event), GFP_KERNEL);
	if (segment->events == NULL) {
		segment->events = ERR_PTR(-ENOMEM);
		dev_warn(card->dev, "Couldn't allocate memory for handling the acquisition\n");
		goto events_alloc_failed;
	}

	for (i = 0; i < card->n_channels; i++) {
		if (sis3320_read_events_dir(card, segment_nr, i, raw_events, segment->nr_events)) {
			dev_warn(card->dev, "Failed to read the channel's %i events' directory\n", i);
			goto read_events_failed;
		}
	}
	kfree(raw_events);
	return;

 read_events_failed:
	kfree(segment->events);
	segment->events = NULL;
	segment->nr_events = 0;
 events_alloc_failed:
	kfree(raw_events);
}

/*
 * For consistency with the user-space API, the first prevticks value
 * must be zero, and the counting starts from then on.
 */
static void sis3302_pt_cache_normalize(u64 *pt_cache, int n)
{
	int i;

	for (i = 0; i < n; i++)
		pt_cache[i] -= pt_cache[0];
}


static void sis3302_pt_cache_fill(u64 *pt_cache, const u32 *raw, int n)
{
	u64 upper;
	u32 lower;
	int i;

	/* we keep two u32's per timestamp, the upper u32 goes first */
	for (i = 0; i < n; i++) {
		upper = raw[i * 2] & SIS3302_TSTAMP_UPPER_MASK;
		lower = raw[i * 2 + 1];
		pt_cache[i] = (upper << 32) | lower;
	}
}

static int sis3302_pt_cache_rawfill(struct sis33_card *card, int segment_nr, u32 *rawbuf, int n)
{
	struct sis3320 *priv = card->private_data;
	unsigned int vme_addr;

	vme_addr = priv->vme_base + SIS3302_TSTAMP_DIR;
	return sis33_dma_read32be_blt(card->dev, vme_addr, rawbuf, n);
}

static void sis3320_read_prevticks(struct sis33_card *card, int segment_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];
	struct sis3320 *priv = card->private_data;
	u32 *raw_pt_cache;
	u64 *pt_cache;
	int elems;
	int err;

	/* only the sis3302 supports event timestamping */
	if (priv->version != 3302 || !segment->nr_events)
		return;

	if (segment->prevticks && !IS_ERR(segment->prevticks))
		kfree(segment->prevticks);
	segment->prevticks = NULL;

	elems = segment->nr_events;
	/* NOTE: on the sis3302, timestamps are 48 bits long */
	raw_pt_cache = kmalloc(elems * 2 * sizeof(u32), GFP_KERNEL);
	if (raw_pt_cache == NULL) {
		segment->prevticks = ERR_PTR(-ENOMEM);
		dev_info(card->dev, "Cannot allocate raw prevtick's cache buffer\n");
		return;
	}

	pt_cache = kmalloc(elems * sizeof(u64), GFP_KERNEL);
	if (pt_cache == NULL) {
		segment->prevticks = ERR_PTR(-ENOMEM);
		dev_info(card->dev, "Cannot allocate prevtick's cache buffer\n");
		goto pt_cache_alloc_failed;
	}

	err = sis3302_pt_cache_rawfill(card, segment_nr, raw_pt_cache, elems * 2);
	if (err) {
		dev_info(card->dev, "Failed to fill segment's %u prevtick's cache buffer, err %d\n", segment_nr, err);
		goto fill_pt_cache_failed;
	}

	sis3302_pt_cache_fill(pt_cache, raw_pt_cache, elems);
	sis3302_pt_cache_normalize(pt_cache, elems);

	segment->prevticks = pt_cache;
	kfree(raw_pt_cache);
	return;

 fill_pt_cache_failed:
	kfree(pt_cache);
 pt_cache_alloc_failed:
	kfree(raw_pt_cache);
}

/*
 * This is called when an acq is cancelled or when handling the "ACQ finished"
 * IRQ.
 * NOTE: This function may be called twice--first from acq_cancel and then
 * inside the IRQ handler--; hence the check of segment->acquiring to avoid
 * treating the (cancelled) acquisition twice.
 */
static void sis3320_acq_complete(struct sis33_card *card)
{
	struct sis3320 *priv = card->private_data;
	struct sis33_segment *segment;

	segment = &card->segments[card->curr_segment];
	atomic_set(&segment->acquiring, 0);
	complete_all(&priv->completion);
}

static void sis3320_irq_work(struct work_struct *work)
{
	struct sis33_card *card = container_of(work, struct sis33_card, irq_work);
	struct sis3320 *priv = card->private_data;
	struct sis33_segment *segment;
	unsigned int status;

	status = sis3320_readw(priv, SIS3320_INTCTL);

	/*
	 * No need to take the card's lock: if a job is sent before the previous
	 * job has been processed, then we've made a mistake.
	 */
	if (status & INTCTL_ST_LEV) {
		sis3320_writew(priv, SIS3320_INTCTL, INTCTL_DICL_LEV);
		segment = &card->segments[card->curr_segment];
		/* BUG_ON(!atomic_read(&segment->acquiring)); */
		do_gettimeofday(&segment->endtime);
		sis3320_read_segment(card, card->curr_segment);
		sis3320_read_prevticks(card, card->curr_segment);
		sis3320_acq_complete(card);
		sis3320_writew(priv, SIS3320_INTCTL, INTCTL_EN_LEV);
	}
}

static int sis3320_irq(void *arg)
{
	struct sis33_card *card = arg;

	queue_work(sis33_workqueue, &card->irq_work);
	return 0;
}

static void __devinit
sis3320_spi_sleep(const struct sis33_card *card, unsigned long vme_spi)
{
	u32 val;
	int i;

	for (i = 0; i < 3; i++) {
		udelay(5);
		val = sis33_dma_read(card->pdev, vme_spi);
		if (!(val & SPI_REG_BUSY))
			return;
	}
	dev_warn(card->pdev, "SPI interface busy for at least 15us\n");
}

static void __devinit
sis3320_spi_write(const struct sis33_card *card, int group, int adc, int addr, int data)
{
	struct sis3320 *priv = card->private_data;
	unsigned long vme_spi = priv->vme_base + SIS3320_GREG(group, SPI_REG);
	int second_adc = adc == 2 ? 1 : 0;
	u32 reg;

	addr &= SPI_REG_ADDR_MASK;
	data &= SPI_REG_DATA_MASK;

	reg = second_adc ? SPI_REG_SEL : 0;
	reg |= addr << SPI_REG_ADDR_SHIFT;
	reg |= data << SPI_REG_DATA_SHIFT;

	reg &= ~SPI_REG_RD;
	sis33_dma_write(card->pdev, vme_spi, reg);
	sis3320_spi_sleep(card, vme_spi);

	reg = second_adc ? SPI_REG_SEL : 0;
	reg |= AD9230_DEVICE_UPDATE << SPI_REG_ADDR_SHIFT;
	reg |= 1 << SPI_REG_DATA_SHIFT;
	sis33_dma_write(card->pdev, vme_spi, reg);
	sis3320_spi_sleep(card, vme_spi);
}

/* SPI write for both channels on the same ADC group */
static void __devinit
sis3320_spi_write_both(const struct sis33_card *card, int group, int addr, int data)
{
	int adc;

	for (adc = 1; adc < 3; adc++)
		sis3320_spi_write(card, group, adc, addr, data);
}

/* SPI write for all ADC groups */
static void __devinit
sis3320_spi_write_all(const struct sis33_card *card, int addr, int data)
{
	int group;

	for (group = 1; group < 5; group++)
		sis3320_spi_write_both(card, group, addr, data);
}

static void __devinit sis3320_reset(struct sis3320 *priv)
{
	sis3320_writew(priv, SIS3320_RESET, 1);
	udelay(5);
}

static int __devinit
sis3320_request_irq(struct sis33_card *card, int vector, int level)
{
	struct sis3320 *priv = card->private_data;
	const char *name;
	unsigned int reg;
	int err;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
	name = card->pdev->bus_id;
#else
	name = dev_name(card->pdev);
#endif
	vector	&= INTCONF_VECTOR_MASK;
	level	&= INTCONF_LEVEL_MASK;

	INIT_WORK(&card->irq_work, sis3320_irq_work);
	err = vme_request_irq(vector, sis3320_irq, card, name);
	if (err)
		return err;

	reg = INTCONF_ENABLE | INTCONF_ROAK;
	reg |= level << INTCONF_LEVEL_SHIFT;
	reg |= vector;
	sis3320_writew(priv, SIS3320_INTCONF, reg);
	sis3320_writew(priv, SIS3320_INTCTL, INTCTL_EN_LEV);

	return 0;
}

static void sis3320_free_irq(struct sis33_card *card, int vector, int level)
{
	struct sis3320 *priv = card->private_data;
	unsigned int reg;
	int ret;

	/* quiesce the module and unregister the irq */
	reg = sis3320_readw(priv, SIS3320_INTCONF);
	reg &= ~INTCONF_ENABLE;
	sis3320_writew(priv, SIS3320_INTCONF, reg);
	sis3320_writew(priv, SIS3320_INTCTL, 0);
	ret = vme_free_irq(vector);
	if (ret)
		dev_warn(card->pdev, "Cannot free irq %d, err %d\n", vector, ret);
}

static int __devinit sis3320_device_init(struct sis33_card *card, int ndev)
{
	struct sis3320 *priv = card->private_data;
	u32 reg;
	int err;

	priv->vme_base = base[ndev];

	/* identify the module version */
	reg = sis3320_readw(priv, SIS3320_FW);
	card->id = (reg & FW_MODID) >> FW_MODID_SHIFT;
	card->major_rev = (reg & FW_MAJOR) >> FW_MAJOR_SHIFT;
	card->minor_rev = (reg & FW_MINOR) >> FW_MINOR_SHIFT;

	if (card->id == 0x3320) {
		if (card->major_rev >= 0x20)
			priv->version = 250;
		else
			priv->version = 200;
	} else if (card->id == 0x3302) {
		priv->version = 3302;
	} else {
		dev_err(card->pdev, "device not present at base address 0x%08lx. "
			"version read back 0x%04x expected 0x33{02,20}\n",
			priv->vme_base, card->id);
		return -ENODEV;
	}

	sis3320_reset(priv);
	/* make sure the initial page is 0 */
	sis3320_writew(priv, SIS3320_ADC_MEM_PAGE, 0);
	priv->curr_page = 0;

	if (priv->version == 250) {
		/*
		 * This enables on the AD9230 the Common Mode Ouput that feeds
		 * the AD8138 pre-amplifier.
		 * This is necessary to achieve the specified input range.
		 * There's a Note about this in the documentation, just after
		 * the SPI register is described.
		 */
		sis3320_spi_write_all(card, AD9230_AIN_CONFIG, 0x2);
	}

	init_completion(&priv->completion);

	err = sis3320_request_irq(card, vector[ndev], level[ndev]);
	if (err)
		return err;
	return 0;
}

static inline void sis3320_device_exit(struct sis33_card *card, int ndev)
{
	sis3320_free_irq(card, vector[ndev], level[ndev]);
	flush_workqueue(sis33_workqueue);
}

static int __devinit sis3320_match(struct device *devp, unsigned int ndev)
{
	if (ndev >= base_num)
		return 0;
	if (ndev >= vector_num || ndev >= level_num) {
		dev_warn(devp, "irq vector/level missing\n");
		return 0;
	}
	return 1;
}

static u32 sis3320_get_clk_bits(struct sis33_card *card, unsigned int freq)
{
	struct sis3320 *priv = card->private_data;
	const u32 *freqs;
	const u32 *bits;
	int n;
	int i;

	switch (priv->version) {
	case 3302:
		n = SIS3302_NR_FREQS;
		freqs = &sis3302_freqs[0];
		bits = &sis3302_freq_bits[0];
		break;
	case 250:
		n = SIS3320_250_NR_FREQS;
		freqs = &sis3320_250_freqs[0];
		bits = &sis3320_250_freq_bits[0];
		break;
	case 200:
	default:
		n = SIS3320_200_NR_FREQS;
		freqs = &sis3320_200_freqs[0];
		bits = &sis3320_200_freq_bits[0];
	}

	for (i = 0; i < n; i++) {
		if (freqs[i] == freq)
			break;
	}

	if (i == n) {
		dev_warn(card->pdev, "invalid clkfreq %dHz, using %dHz\n",
			freq, freqs[0]);
		return bits[0];
	}

	return bits[i];
}

static u32 sis3320_acq_clk(struct sis33_card *card, int clksrc, unsigned int clkfreq)
{
	u32 ret;

	switch (clksrc) {
	case SIS33_CLKSRC_INTERNAL:
		ret = sis3320_get_clk_bits(card, clkfreq) << ACQ_EN_CLKSRC_SHIFT;
		break;
	case SIS33_CLKSRC_EXTERNAL:
		ret = CLKSRC_EXT << ACQ_EN_CLKSRC_SHIFT;
		break;
	default:
		dev_warn(card->pdev, "invalid clksrc, using external\n");
		ret = CLKSRC_EXT << ACQ_EN_CLKSRC_SHIFT;
	}

	return ret;
}

static u32 sis3320_get_ev_length(struct sis33_card *card, u32 size)
{
	int i;

	for (i = 0; i < SIS3320_NR_EV_LENGTHS; i++) {
		if (sis3320_ev_lengths[i] == size)
			break;
	}

	if (i == SIS3320_NR_EV_LENGTHS) {
		dev_warn(card->pdev, "invalid event length 0x%x, using 0x%x\n",
			size, sis3320_ev_lengths[7]);
		return sis3320_ev_length_bits[7];
	}

	return sis3320_ev_length_bits[i];
}

static int sis3320_conf_acq(struct sis33_card *card, struct sis33_cfg *cfg)
{
	struct sis3320 *priv = card->private_data;
	u32 val;

	/* clear the clocksource bits before updating them */
	sis3320_writew(priv, SIS3320_ACQ, ACQ_CL_CLKSRC);

	/*
	 * Acquisition Control Register
	 */
	val = 0;

	/* get clock settings */
	val |= sis3320_acq_clk(card, cfg->clksrc, cfg->clkfreq);

	/* enable/disable auto-start mode */
	val |= cfg->start_auto ? ACQ_EN_AUTOST : ACQ_DI_AUTOST;

	/* enable/disable front panel */
	val |= cfg->trigger_ext_en ? ACQ_EN_FPANEL : 0;

	sis3320_writew(priv, SIS3320_ACQ, val);

	/*
	 * Start/Stop Delay
	 */
	sis3320_writew(priv, SIS3320_EXT_START_DELAY, cfg->start_delay);
	sis3320_writew(priv, SIS3320_EXT_STOP_DELAY, cfg->stop_delay);

	return 0;
}

static void sis3320_dac_write(struct sis3320 *sis, u32 val)
{
	val &= DAC_DATA_WR;
	sis3320_writew(sis, SIS3320_DAC_DATA, val);
}

static void sis3320_dac_poll(struct sis33_card *card)
{
	struct sis3320 *sis = card->private_data;
	u32 val;
	int i;

	for (i = 0; i < 1000; i++) {
		val = sis3320_readw(sis, SIS3320_DAC_CTL);
		if (!(val & DAC_CTL_BUSY))
			return;
		udelay(1);
	}

	dev_warn(card->pdev, "After ~1ms, the DAC is still busy. Continuing.\n");
}

static void sis3320_dac_shift(struct sis33_card *card)
{
	struct sis3320 *sis = card->private_data;

	sis3320_writew(sis, SIS3320_DAC_CTL, DACCMD_LOAD_SH);
	sis3320_dac_poll(card);
}

static void
sis3320_dac_cmd_channel(struct sis33_card *card, int cmd, int channel_index)
{
	struct sis3320 *sis = card->private_data;
	u32 val;

	val = cmd;
	val |= channel_index << DAC_CTL_SELECT_SHIFT;

	sis3320_writew(sis, SIS3320_DAC_CTL, val);
	sis3320_dac_poll(card);
}

static void sis3320_dac_load(struct sis33_card *card)
{
	struct sis3320 *sis = card->private_data;

	sis3320_writew(sis, SIS3320_DAC_CTL, DACCMD_LOAD_DAC);
	sis3320_dac_poll(card);
}

static void __sis3320_apply_channels_offset(struct sis33_card *card)
{
	struct sis3320 *priv = card->private_data;
	u32 val;
	int i;

	for (i = 0; i < SIS3320_NR_CHANNELS; i++) {
		val = card->channels[i].offset;

		sis3320_dac_write(priv, val);
		sis3320_dac_cmd_channel(card, DACCMD_LOAD_SH, i);
		sis3320_dac_cmd_channel(card, DACCMD_LOAD_DAC, i);
	}
}

static void __sis3320_apply_channels_offset_daisychain(struct sis33_card *card)
{
	struct sis3320 *priv = card->private_data;
	u32 val;
	int i, j;

	/*
	 * This is daisy chained, i.e. the last element has to
	 * be loaded first.
	 */
	for (i = 0; i < SIS3320_NR_CHANNELS; i++) {
		j = SIS3320_NR_CHANNELS - 1 - i;
		val = card->channels[j].offset;

		sis3320_dac_write(priv, val);
		sis3320_dac_shift(card);
		sis3320_dac_load(card);
	}
}

static void sis3320_apply_channels_offset(struct sis33_card *card)
{
	struct sis3320 *priv = card->private_data;

	switch (priv->version) {
	case 3302:
	case 250:
		__sis3320_apply_channels_offset(card);
		break;
	case 200:
	default:
		__sis3320_apply_channels_offset_daisychain(card);
	}
}

static int sis3320_conf_channels(struct sis33_card *card, struct sis33_channel *channels)
{
	sis3320_apply_channels_offset(card);
	return 0;
}

/*
 * By using the cached page number, we save many superfluous writes.
 * No locking is needed because simultaneous transfers from the same device
 * aren't allowed.
 */
static void sis3320_put_page(struct sis3320 *priv, unsigned int page_nr)
{
	page_nr &= ADC_MEM_PAGE_MASK;

	if (page_nr == priv->curr_page)
		return;
	priv->curr_page = page_nr;
	sis3320_writew(priv, SIS3320_ADC_MEM_PAGE, page_nr);
}

static unsigned int
sis3320_get_offset(struct sis33_card *card, int segment_nr, int event_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];
	struct sis3320 *priv = card->private_data;
	unsigned int event_size;
	unsigned int membase;
	unsigned int offset;

	event_size = segment->nr_samp_per_ev * sizeof(u16);
	membase = sis3320_get_segment_base(card, segment_nr) * sizeof(u16);
	offset = membase + event_nr * event_size;
	sis3320_put_page(priv, offset >> SIS3320_PAGESHIFT);
	return offset & SIS3320_PAGEMASK;
}

/*
 * The length and start address of the event fetched by this function is assumed
 * to be a multiple of the ADC memory window size. This is one of the reasons
 * for not supporting segment sizes which aren't a power of two.
 */
static int
sis3320_get_data_bigpage(struct sis33_card *card, int segment_nr, struct sis33_acq *acq, int channel, int event_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];
	struct sis3320 *priv = card->private_data;
	unsigned int nr_pages;
	unsigned int page0;
	unsigned int i;

	nr_pages = segment->nr_samp_per_ev / SIS3320_PAGESIZE_SAMPLES;
	page0 = sis3320_get_segment_base(card, segment_nr);
	page0 += event_nr * segment->nr_samp_per_ev;
	page0 *= sizeof(u16);
	page0 >>= SIS3320_PAGESHIFT;

	for (i = 0; i < nr_pages; i++) {
		u8 __user *addr = (u8 __user *)acq->data + i * SIS3320_PAGESIZE;
		unsigned int vme_addr;
		int ret;

		sis3320_put_page(priv, page0 + i);
		vme_addr = priv->vme_base + SIS3320_MEM_ADC(channel);
		ret = sis33_dma_read_mblt_user(card->dev, vme_addr, addr, SIS3320_PAGESIZE);
		if (ret)
			return ret;
	}
	return 0;
}

static int
sis3320_get_data_smallpage(struct sis33_card *card, int segment_nr, struct sis33_acq *acq, int channel, int event_nr)
{
	struct sis3320 *priv = card->private_data;
	unsigned int vme_addr;
	unsigned int offset;

	offset = sis3320_get_offset(card, segment_nr, event_nr);
	vme_addr = priv->vme_base + SIS3320_MEM_ADC(channel) + offset;
	return sis33_dma_read_mblt_user(card->dev, vme_addr, acq->data, acq->size);
}

static int
sis3320_get_data(struct sis33_card *card, int segment_nr, struct sis33_acq *acq, int channel, int event_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];

	acq->be = 1;
	if (segment->nr_samp_per_ev <= SIS3320_PAGESIZE_SAMPLES)
		return sis3320_get_data_smallpage(card, segment_nr, acq, channel, event_nr);
	else
		return sis3320_get_data_bigpage(card, segment_nr, acq, channel, event_nr);
}

static int
sis3320_read_event(struct sis33_card *card, int segment_nr, struct sis33_acq *acq, int channel, int event_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];
	struct sis33_event *event;

	if (!segment->nr_events || segment->events == NULL)
		return -ENODATA;
	if (IS_ERR(segment->events))
		return PTR_ERR(segment->events);
	if (segment->prevticks) {
		const u64 *pt_cache = segment->prevticks;

		if (IS_ERR(segment->prevticks))
			return PTR_ERR(segment->prevticks);
		acq->prevticks = pt_cache[event_nr];
	} else {
		acq->prevticks = 0;
	}
	event = sis3320_get_event(segment, channel, event_nr);
	acq->nr_samples = event->nr_samples;
	acq->first_samp = event->first_samp;
	return sis3320_get_data(card, segment_nr, acq, channel, event_nr);
}

static int
sis3320_fetch(struct sis33_card *card, int segment_nr, int channel_nr, struct sis33_acq *acqs, int nr_events)
{
	int ret;
	int i;

	for (i = 0; i < nr_events; i++) {
		ret = sis3320_read_event(card, segment_nr, &acqs[i], channel_nr, i);
		if (ret)
			return ret;
	}
	return i;
}

static int sis3320_trigger(struct sis33_card *card, u32 trigger)
{
	struct sis3320 *priv = card->private_data;

	switch (trigger) {
	case SIS33_TRIGGER_START:
		sis3320_writew(priv, SIS3320_START_SAMP, 1);
		return 0;

	case SIS33_TRIGGER_STOP:
		sis3320_writew(priv, SIS3320_STOP_SAMP, 1);
		return 0;

	default:
		dev_warn(card->pdev, "Invalid trigger %d\n", trigger);
		return -EINVAL;
	}
}

static int sis3320_conf_ev(struct sis33_card *card, struct sis33_acq_desc *desc)
{
	struct sis3320 *priv = card->private_data;
	struct sis33_cfg *cfg = &card->cfg;
	u32 val;

	/* single or multi-event mode */
	val = desc->nr_events == 1 ? ACQ_DI_MEV : ACQ_EN_MEV;
	/*
	 * the rest of the acquisition register should have been filled in
	 * by conf_acq, and shouldn't change here. However, some hardware
	 * revisions (notably sis3320) take a 0 on one of the 'enable' bits
	 * as if we put a 1 on the corresponding 'disable' bit. To work around
	 * it we simply set everything up again.
	 */
	val |= sis3320_acq_clk(card, cfg->clksrc, cfg->clkfreq);
	val |= cfg->start_auto ? ACQ_EN_AUTOST : ACQ_DI_AUTOST;
	val |= cfg->trigger_ext_en ? ACQ_EN_FPANEL : 0;
	sis3320_writew(priv, SIS3320_ACQ, val);

	/* set the max number of events */
	sis3320_writew(priv, SIS3320_NR_EVENTS, desc->nr_events);

	/*
	 * Event Configuration
	 * Note: this applies to all channels
	 */
	val = card->cfg.stop_auto ? EV_EN_SAMPLEN_STOP : 0;
	/* wrap around full memory (32MSamples) or by pages */
	if (desc->ev_length != card->ev_lengths[0]) {
		val |= EV_EN_WRAP_MODE;
		val |= sis3320_get_ev_length(card, desc->ev_length);
	}
	sis3320_writew(priv, SIS3320_GALL(EV_CONF), val);

	/* Sample Length */
	if (card->cfg.stop_auto) {
		/*
		 * The Sampling Length is in fact a mask with the lower
		 * two bits unused.  From the manual:
		 * "Desired sample length 0x100 -> set the register to 0xfc."
		 */
		val = (desc->ev_length - 4) & 0x01fffffc;
		sis3320_writew(priv, SIS3320_GALL(SAMP_LEN), val);
	}

	/* set initial sampling address, in samples */
	sis3320_writew(priv, SIS3320_GALL(SAMP_STADDR), sis3320_get_segment_base(card, desc->segment));

	return 0;
}

static void sis3320_acq_start(struct sis33_card *card)
{
	struct sis3320 *priv = card->private_data;

	INIT_COMPLETION(priv->completion);
	if (priv->version == 3302)
		sis3320_writew(priv, SIS3302_TSTAMP_CLEAR, 1);
	sis3320_writew(priv, SIS3320_ARM, 1);
}

static int sis3320_acq_wait(struct sis33_card *card, struct timespec *timeout)
{
	struct sis3320 *priv = card->private_data;
	int wait;
	int ret = 0;

	if (timeout) {
		unsigned long expires = timespec_to_jiffies(timeout);

		wait = wait_for_completion_interruptible_timeout(&priv->completion, expires);
		if (wait < 0)
			ret = -EINTR;
		else if (wait == 0)
			ret = -ETIME;
	} else {
		wait = wait_for_completion_interruptible(&priv->completion);
		if (wait < 0)
			ret = -EINTR;
	}
	return ret;
}

static int sis3320_acq_cancel(struct sis33_card *card)
{
	struct sis3320 *priv = card->private_data;

	sis3320_writew(priv, SIS3320_DISARM, 1);
	sis3320_acq_complete(card);
	return 0;
}

static void sis3320_config_init(struct sis33_card *card)
{
	struct sis33_cfg *cfg = &card->cfg;
	struct sis3320 *priv = card->private_data;
	unsigned int offset;
	int i;

	cfg->start_delay	= 0;
	cfg->stop_delay		= 0;
	cfg->start_auto		= 1;
	cfg->stop_auto		= 1;
	cfg->trigger_ext_en	= 1;
	cfg->clksrc		= SIS33_CLKSRC_INTERNAL;
	cfg->ev_tstamp_divider	= 1; /* Note: not supported */

	if (priv->version == 200) {
		cfg->clkfreq = 200000000;
		offset = 0x99f2;
	} else if (priv->version == 3302) {
		cfg->clkfreq = 100000000;
		offset = 0x91be; /* @todo: test if this value is reasonable */
	} else {
		cfg->clkfreq = 250000000;
		offset = 0x8dd6;
	}
	for (i = 0; i < card->n_channels; i++) {
		card->channels[i].show_attrs = 1;
		card->channels[i].offset = offset;
	}
}

static struct sis33_card_ops sis3320_ops = {
	.conf_acq		= sis3320_conf_acq,
	.conf_ev		= sis3320_conf_ev,
	.conf_channels		= sis3320_conf_channels,
	.fetch			= sis3320_fetch,
	.trigger		= sis3320_trigger,
	.acq_start		= sis3320_acq_start,
	.acq_wait		= sis3320_acq_wait,
	.acq_cancel		= sis3320_acq_cancel,
	.config_init		= sis3320_config_init,
};

static int __devinit sis3320_probe(struct device *pdev, unsigned int ndev)
{
	struct sis33_card *card;
	struct sis3320 *priv;
	struct sis33_channel *channels;
	struct sis33_segment *segments;
	int error;

	error = sis33_card_create(index[ndev], THIS_MODULE, &card, pdev);
	if (error)
		return error;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (priv == NULL) {
		error = -ENOMEM;
		goto alloc_priv_failed;
	}
	priv->pdev = pdev;

	channels = kcalloc(SIS3320_NR_CHANNELS, sizeof(*channels), GFP_KERNEL);
	if (channels == NULL) {
		error = -ENOMEM;
		goto alloc_channels_failed;
	}

	segments = kcalloc(SIS3320_MAX_NR_SEGMENTS, sizeof(*segments), GFP_KERNEL);
	if (segments == NULL) {
		error = -ENOMEM;
		goto alloc_segments_failed;
	}

	card->pdev = pdev;
	card->private_data = priv;

	error = sis3320_device_init(card, ndev);
	if (error)
		goto sis3320_device_init_failed;

	/* fill in the module-specific part of struct sis33_card */
	strlcpy(card->driver, DRV_NAME, sizeof(card->driver));
	if (priv->version == 3302) {
		snprintf(card->description, sizeof(card->description),
			"Struck SIS3302v0x%02x%02x at VME-A32 0x%08lx irqv %d irql %d",
			card->major_rev, card->minor_rev, priv->vme_base,
			vector[ndev], level[ndev]);
	} else {
		snprintf(card->description, sizeof(card->description),
			"Struck SIS3320-%dv%02x at VME-A32 0x%08lx irqv %d irql %d",
			priv->version, card->minor_rev, priv->vme_base,
			vector[ndev], level[ndev]);
	}

	if (priv->version == 3302) {
		card->ev_tstamp_support = 1;
		card->ev_tstamp_max_ticks_log2 = 48;
		card->n_bits = 16;
	} else {
		card->ev_tstamp_support = 0;
		card->ev_tstamp_max_ticks_log2 = 0;
		card->n_bits = 12;
	}
	card->ev_tstamp_divider_max = 1;

	card->n_channels = SIS3320_NR_CHANNELS;
	card->channels = channels;

	card->max_nr_events = SIS3320_MAX_NR_EVENTS;
	card->max_delay	= SIS3320_MAX_DELAY;

	card->n_ev_lengths = SIS3320_NR_EV_LENGTHS;
	card->ev_lengths = &sis3320_ev_lengths[0];

	switch (priv->version) {
	case 3302:
		card->n_freqs = SIS3302_NR_FREQS;
		card->freqs = &sis3302_freqs[0];
		break;
	case 250:
		card->n_freqs = SIS3320_250_NR_FREQS;
		card->freqs = &sis3320_250_freqs[0];
		break;
	case 200:
	default:
		card->n_freqs = SIS3320_200_NR_FREQS;
		card->freqs = &sis3320_200_freqs[0];
	}

	card->n_segments_max = SIS3320_MAX_NR_SEGMENTS;
	card->n_segments_min = 1;
	card->n_segments = 2;
	card->segments = segments;
	card->curr_segment = 0;

	error = sis33_card_register(card, &sis3320_ops);
	if (!error)
		goto out;

	sis3320_device_exit(card, ndev);
 sis3320_device_init_failed:
	kfree(segments);
 alloc_segments_failed:
	kfree(channels);
 alloc_channels_failed:
	kfree(priv);
 alloc_priv_failed:
	sis33_card_free(card);
 out:
	return error;
}

static int __devexit sis3320_remove(struct device *pdev, unsigned int ndev)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	int i;

	sis3320_device_exit(card, ndev);
	for (i = 0; i < SIS3320_MAX_NR_SEGMENTS; i++) {
		struct sis33_segment *segment = &card->segments[i];

		if (segment->events && !IS_ERR(segment->events))
			kfree(segment->events);
		if (segment->prevticks && !IS_ERR(segment->prevticks))
			kfree(segment->prevticks);
	}
	kfree(card->segments);
	kfree(card->channels);
	kfree(card->private_data);
	sis33_card_free(card);
	return 0;
}

static struct vme_driver sis3320_driver = {
	.match		= sis3320_match,
	.probe		= sis3320_probe,
	.remove		= __devexit_p(sis3320_remove),
	.driver		= {
		.name	= DRV_NAME,
	},
};

static int __init sis3320_init(void)
{
	return vme_register_driver(&sis3320_driver, SIS33_MAX_DEVICES);
}

static void __exit sis3320_exit(void)
{
	vme_unregister_driver(&sis3320_driver);
}

module_init(sis3320_init)
module_exit(sis3320_exit)

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sis3320 ADC driver");
MODULE_VERSION(DRV_MODULE_VERSION);
