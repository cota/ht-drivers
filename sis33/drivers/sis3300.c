/*
 * sis3300.c
 * sis33 driver for the sis3300 family.
 * This doesn't support any of the sis3301-x, but it should be very
 * easy to make them work.
 *
 * Copyright (c) 2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/err.h>
#include <vmebus.h>

#include <asm/uaccess.h>

#include "sis33core.h"
#include "sis3300.h"

#define DRV_NAME		"sis3300"
#define DRV_MODULE_VERSION	"1.0"

#define SIS3300_MAX_NR_EVENTS	1024

#define SIS3300_NR_CHANNELS	8
#define SIS3300_NR_ADCGROUPS	((SIS3300_NR_CHANNELS)/2)
#define SIS3300_MAX_NR_SEGMENTS	2

/**
 * struct sis3300 - sis3300 device
 * @vme_base:	VME base address of the module
 * @pdev:	parent (physical) device
 * @req_events:	number of requested events
 * @completion:	signals when an acquisition has finished
 * @sampcache:	cache to store the interleaved data coming from each ADC group
 *
 * NOTE: when printing to the kernel log, we use card->dev (the device that
 * user-space sees) only when we're sure that is already there. Otherwise,
 * we use pdev, which is the always-present physical device.
 */
struct sis3300 {
	unsigned long		vme_base;
	struct device		*pdev;
	int			req_events;
	struct completion	completion;
	u32			*sampcache[SIS3300_MAX_NR_SEGMENTS][SIS3300_NR_ADCGROUPS];
};

static long base[SIS33_MAX_DEVICES];
static unsigned int base_num;
static int vector[SIS33_MAX_DEVICES];
static unsigned int vector_num;
static int level[SIS33_MAX_DEVICES];
static unsigned int level_num;
static int index[SIS33_MAX_DEVICES] = SIS33_DEFAULT_IDX;

module_param_array(base, long, &base_num, 0444);
MODULE_PARM_DESC(base, "Base address of the sis3300 card");
module_param_array(vector, int, &vector_num, 0444);
MODULE_PARM_DESC(vector, "IRQ vector");
module_param_array(level, int, &level_num, 0444);
MODULE_PARM_DESC(level, "IRQ level");
module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for sis3300 card");

static const u32 sis3300_ev_lengths[SIS3300_EV_LENGTHS] = {
	[0]	= 131072,
	[1]	= 16384,
	[2]	= 4096,
	[3]	= 2048,
	[4]	= 1024,
	[5]	= 512,
	[6]	= 256,
	[7]	= 128,
};
#define SIS3300_NR_EV_LENGTHS	ARRAY_SIZE(sis3300_ev_lengths)

static const u32 sis3300_ev_length_bits[SIS3300_EV_LENGTHS] = {
	[0]	= EV_LENGTH_128K,
	[1]	= EV_LENGTH_16K,
	[2]	= EV_LENGTH_4K,
	[3]	= EV_LENGTH_2K,
	[4]	= EV_LENGTH_1K,
	[5]	= EV_LENGTH_512,
	[6]	= EV_LENGTH_256,
	[7]	= EV_LENGTH_128,
};

static const u32 sis3300_08_ev_lengths[SIS3300_08_EV_LENGTHS] = {
	[0]	= 131072,
	[1]	= 32768,
	[2]	= 16384,
	[3]	= 8192,
	[4]	= 4096,
	[5]	= 2048,
	[6]	= 1024,
	[7]	= 512,
	[8]	= 256,
	[9]	= 128,
};
#define SIS3300_08_NR_EV_LENGTHS	ARRAY_SIZE(sis3300_08_ev_lengths)

static const u32 sis3300_08_ev_length_bits[SIS3300_08_EV_LENGTHS] = {
	[0]	= EV_LENGTH_128K,
	[1]	= EV_CONF_EV_LEN_MAP | EV_LENGTH_32K,
	[2]	= EV_LENGTH_16K,
	[3]	= EV_CONF_EV_LEN_MAP | EV_LENGTH_8K,
	[4]	= EV_LENGTH_4K,
	[5]	= EV_LENGTH_2K,
	[6]	= EV_LENGTH_1K,
	[7]	= EV_LENGTH_512,
	[8]	= EV_LENGTH_256,
	[9]	= EV_LENGTH_128,
};

static const u32 sis3300_freqs[SIS3300_CLKSRCS] = {
	[0]	= 100000000,
	[1]	= 50000000,
	[2]	= 25000000,
	[3]	= 12500000,
	[4]	= 6250000,
	[5]	= 3125000,
};
#define SIS3300_NR_FREQS	ARRAY_SIZE(sis3300_freqs)

static const u32 sis3300_freq_bits[SIS3300_CLKSRCS] = {
	[0]	= SIS3300_CLKSRC_100,
	[1]	= SIS3300_CLKSRC_50,
	[2]	= SIS3300_CLKSRC_25,
	[3]	= SIS3300_CLKSRC_12M5,
	[4]	= SIS3300_CLKSRC_6M25,
	[5]	= SIS3300_CLKSRC_3M125,
};

static inline unsigned int sis3300_readw(struct sis3300 *sis, ssize_t offset)
{
	unsigned int val;

	val = sis33_dma_read(sis->pdev, sis->vme_base + offset);
	dev_dbg(sis->pdev, "readw:\t0x%8x\tval: 0x%8x\n", (unsigned int)offset, val);

	return val;
}

static inline void sis3300_writew(struct sis3300 *sis, ssize_t offset, u32 val)
{
	dev_dbg(sis->pdev, "writew:\t\t0x%8x\tval: 0x%8x\n", (unsigned int)offset, (unsigned int)val);
	sis33_dma_write(sis->pdev, sis->vme_base + offset, val);
}

static inline unsigned int sis3300_rev(const struct sis33_card *card)
{
	return card->major_rev << 8 | card->minor_rev;
}

static u32 sis3300_get_clk_bits(struct sis33_card *card, unsigned int freq)
{
	int i;

	for (i = 0; i < SIS3300_NR_FREQS; i++) {
		if (sis3300_freqs[i] == freq)
			break;
	}
	if (i == SIS3300_NR_FREQS) {
		dev_warn(card->pdev, "invalid clkfreq %dHz, using %dHz\n",
			freq, sis3300_freqs[0]);
		return sis3300_freq_bits[0];
	}
	return sis3300_freq_bits[i];
}

static u32 sis3300_acq_clk(struct sis33_card *card, int clksrc, unsigned int clkfreq)
{
	u32 val;

	switch (clksrc) {
	case SIS33_CLKSRC_INTERNAL:
		val = sis3300_get_clk_bits(card, clkfreq);
		break;
	case SIS33_CLKSRC_EXTERNAL:
		val = SIS3300_CLKSRC_EXT;
		break;
	default:
		dev_warn(card->pdev, "invalid clksrc, using external\n");
		val = SIS3300_CLKSRC_EXT;
	}
	return val << ACQ_CLKSRC_SHIFT;
}

static int sis3300_conf_acq(struct sis33_card *card, struct sis33_cfg *cfg)
{
	struct sis3300 *priv = card->private_data;
	u32 val;

	/*
	 * these bits need to be cleared; otherwise the clock bitmask we write
	 * will effectively be ORed to the existing one.
	 */
	sis3300_writew(priv, SIS3300_ACQ, ACQ_CL_CLKSRC);

	val = 0;
	val |= sis3300_acq_clk(card, cfg->clksrc, cfg->clkfreq);
	val |= cfg->start_auto ? ACQ_EN_AUTOST : ACQ_DI_AUTOST;
	val |= cfg->trigger_ext_en ? ACQ_EN_FPANEL : ACQ_DI_FPANEL;
	val |= cfg->start_delay ? ACQ_EN_STARTD : ACQ_DI_STARTD;
	val |= cfg->stop_delay ? ACQ_EN_STOPD : ACQ_DI_STOPD;
	sis3300_writew(priv, SIS3300_ACQ, val);

	sis3300_writew(priv, SIS3300_EXT_START_DELAY, cfg->start_delay);
	sis3300_writew(priv, SIS3300_EXT_STOP_DELAY, cfg->stop_delay);
	sis3300_writew(priv, SIS3300_TSTAMP_PREDIV, cfg->ev_tstamp_divider);

	return 0;
}

static u32 sis3300_get_ev_len_bits(struct sis33_card *card, u32 length)
{
	const u32 *lengths;
	const u32 *bits;
	int n;
	int i;

	if (sis3300_rev(card) >= 0x0308) {
		lengths	= sis3300_08_ev_lengths;
		bits	= sis3300_08_ev_length_bits;
		n	= SIS3300_08_NR_EV_LENGTHS;
	} else {
		lengths	= sis3300_ev_lengths;
		bits	= sis3300_ev_length_bits;
		n	= SIS3300_NR_EV_LENGTHS;
	}

	for (i = 0; i < n; i++) {
		if (lengths[i] == length)
			break;
	}
	if (i == n) {
		dev_warn(card->pdev, "invalid event length 0x%x, using 0x%x\n",
			length, lengths[3]);
		return bits[3];
	}
	return bits[i];
}

static int sis3300_conf_ev(struct sis33_card *card, struct sis33_acq_desc *desc)
{
	struct sis3300 *priv = card->private_data;
	u32 val;

	/* single or multi-event mode */
	val = desc->nr_events == 1 ? ACQ_DI_MEV : ACQ_EN_MEV;
	sis3300_writew(priv, SIS3300_ACQ, val);
	/*
	 * Note: the MAX_NR_EVENTS register is useless; it only works
	 * in GATE mode. Thus we just save in the device's struct the requested
	 * number of events, and will check if we've reached that figure
	 * after each 'event acquired' interrupt.
	 */
	priv->req_events = desc->nr_events;

	/* event config register */
	val = card->cfg.stop_auto ? 0 : EV_CONF_EN_WRAP_MODE;
	val |= sis3300_get_ev_len_bits(card, desc->ev_length);
	sis3300_writew(priv, SIS3300_GALL(EV_CONF), val);

	return 0;
}

static void
sis3300_process_raw_event(struct sis33_card *card, int segment_nr, u32 *raw_events,
			  struct sis33_event *events, int event_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];
	struct sis33_event *event = &events[event_nr];
	u32 raw_event = raw_events[event_nr];
	unsigned int next_sample;

	next_sample = raw_event & EV_DIR_ADDR_MASK;
	next_sample -= event_nr * segment->nr_samp_per_ev;

	if (raw_event & EV_DIR_WRAP) {
		event->nr_samples = segment->nr_samp_per_ev;
		event->first_samp = next_sample;
	} else {
		event->nr_samples = next_sample;
		event->first_samp = 0;
	}
}

static int
sis3300_read_events_dir(struct sis33_card *card, int segment_nr, u32 *raw_events,
			struct sis33_event *events, int nr_events)
{
	struct sis3300 *priv = card->private_data;
	unsigned int vme_addr;
	int ret;
	int i;

	vme_addr = priv->vme_base + EV_DIR(segment_nr);
	ret = sis33_dma_read32be_blt(card->dev, vme_addr, raw_events, nr_events);
	if (ret)
		return ret;
	for (i = 0; i < nr_events; i++)
		sis3300_process_raw_event(card, segment_nr, raw_events, events, i);
	return 0;
}

static void sis3300_u32_swap(u32 *buf, int n)
{
	int i;

	for (i = 0; i < n; i++)
		buf[i] = be32_to_cpu(buf[i]);
}

static int
sis3300_read_event(struct sis33_acq *acq, int channel, u32 *raw_data, u64 prevticks, struct sis33_event *event)
{
	u16 data;
	int ret;
	int i;

	acq->nr_samples = event->nr_samples;
	acq->first_samp = event->first_samp;
	acq->prevticks = prevticks;
	acq->be = 0;

	/* NOTE: raw_data buffer is already in the cpu's endianness */
	for (i = 0; i < acq->nr_samples; i++) {
		if (channel & 1)
			data = (raw_data[i] & ADC1_DATA_MASK) >> ADC1_DATA_SHIFT;
		else
			data = (raw_data[i] & ADC0_DATA_MASK) >> ADC0_DATA_SHIFT;
		ret = put_user(data, &acq->data[i]);
		if (ret)
			return ret;
	}
	return 0;
}

static void sis3300_evcache_free(struct sis33_card *card, int segment_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];

	if (segment->events) {
		kfree(segment->events);
		segment->events = NULL;
	}
}

static void sis3300_evcache_free_all(struct sis33_card *card)
{
	int i;

	for (i = 0; i < SIS3300_MAX_NR_SEGMENTS; i++)
		sis3300_evcache_free(card, i);
}

static struct sis33_event *sis3300_evcache_get(struct sis33_card *card, int segment_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];
	u32 *raw_events;
	int err;

	if (segment->events)
		return segment->events;
	/*
	 * we use segment->nr_events instead of nr_events because nr_events may
	 * be smaller than segment->nr_events since it comes from user-space.
	 */
	segment->events = kcalloc(segment->nr_events, sizeof(*segment->events), GFP_KERNEL);
	if (segment->events == NULL) {
		dev_info(card->dev, "Couldn't allocate memory for handling the acquisition\n");
		return ERR_PTR(-ENOMEM);
	}
	raw_events = kmalloc(segment->nr_events * sizeof(*raw_events), GFP_KERNEL);
	if (raw_events == NULL) {
		dev_info(card->dev, "Couldn't allocate raw events' buffer for handling the ev. directory\n");
		err = -ENOMEM;
		goto raw_events_alloc_failed;
	}
	err = sis3300_read_events_dir(card, segment_nr, raw_events, segment->events, segment->nr_events);
	if (err) {
		dev_info(card->dev, "Failed to read segment's %i event directory\n", segment_nr);
		goto read_events_failed;
	}
	kfree(raw_events);
	return segment->events;

 read_events_failed:
	kfree(raw_events);
 raw_events_alloc_failed:
	sis3300_evcache_free(card, segment_nr);
	return ERR_PTR(err);
}

static int
sis3300_fill_bouncebuf(struct sis33_card *card, int segment_nr, int channel_nr, u32 *bouncebuf, ssize_t size)
{
	struct sis3300 *priv = card->private_data;
	unsigned int vme_addr;

	vme_addr = priv->vme_base + MEMBANK(segment_nr, channel_nr);
	return sis33_dma_read_mblt(card->dev, vme_addr, bouncebuf, size);
}

static void sis3300_sampcache_free(struct sis3300 *priv, int segment_nr)
{
	u32 *sampcache;
	int i;

	for (i = 0; i < SIS3300_NR_ADCGROUPS; i++) {
		sampcache = priv->sampcache[segment_nr][i];

		if (sampcache) {
			kfree(sampcache);
			priv->sampcache[segment_nr][i] = NULL;
		}
	}
}

static void sis3300_sampcache_free_all(struct sis3300 *priv)
{
	int i;

	for (i = 0; i < SIS3300_MAX_NR_SEGMENTS; i++)
		sis3300_sampcache_free(priv, i);
}

/*
 * The acquisition by the device is done in ADC groups of 2 channels. The two
 * channels' sampled data are interleaved in the on-board memory banks.
 * We first copy into the bounce buffer the data acquired by the ADC group
 * and then copy the given channel's samples from the bounce buffer to
 * user-space. In other words, in the bounce buffer we effectively need 32 bits
 * per sample.
 */
static u32 *sis3300_sampcache_get(struct sis33_card *card, int segment_nr, int channel_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];
	struct sis3300 *priv = card->private_data;
	ssize_t bouncesize;
	u32 *bouncebuf;
	int err;

	if (priv->sampcache[segment_nr][channel_nr/2])
		return priv->sampcache[segment_nr][channel_nr/2];

	bouncesize = segment->nr_events * segment->nr_samp_per_ev * sizeof(u32);
	bouncebuf = kmalloc(bouncesize, GFP_KERNEL);
	if (bouncebuf == NULL) {
		dev_info(card->dev, "Couldn't allocate bounce buffer to treat the sampled data\n");
		return ERR_PTR(-ENOMEM);
	}
	err = sis3300_fill_bouncebuf(card, segment_nr, channel_nr, bouncebuf, bouncesize);
	if (err) {
		dev_info(card->dev, "Failed to fill segment's %i bounce buffer (err %d)\n", segment_nr, err);
		goto fill_bouncebuf_failed;
	}
	sis3300_u32_swap(bouncebuf, segment->nr_events * segment->nr_samp_per_ev);

	priv->sampcache[segment_nr][channel_nr/2] = bouncebuf;
	return bouncebuf;

 fill_bouncebuf_failed:
	kfree(bouncebuf);
	return ERR_PTR(err);
}

static void sis3300_pt_cache_free(struct sis33_card *card, int segment_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];

	if (segment->prevticks) {
		kfree(segment->prevticks);
		segment->prevticks = NULL;
	}
}

static void sis3300_pt_cache_free_all(struct sis33_card *card)
{
	int i;

	for (i = 0; i < SIS3300_MAX_NR_SEGMENTS; i++)
		sis3300_pt_cache_free(card, i);
}

static int sis3300_fill_pt_cache(struct sis33_card *card, int segment_nr, u32 *buf, int n)
{
	struct sis3300 *priv = card->private_data;
	unsigned int vme_addr;
	int ret;
	int i;

	vme_addr = priv->vme_base + SIS3300_EV_TSTAMP(segment_nr);
	ret = sis33_dma_read32be_blt(card->dev, vme_addr, buf, n);
	if (ret)
		return ret;
	for (i = 0; i < n; i++)
		buf[i] &= SIS3300_EV_TSTAMP_MASK;
	return 0;
}

static u32 *sis3300_pt_cache_get(struct sis33_card *card, int segment_nr)
{
	struct sis33_segment *segment = &card->segments[segment_nr];
	u32 *pt_cache;
	int err;

	if (segment->prevticks)
		return segment->prevticks;

	pt_cache = kmalloc(segment->nr_events * sizeof(u32), GFP_KERNEL);
	if (pt_cache == NULL) {
		dev_info(card->dev, "Couldn't allocate prevtick's cache buffer\n");
		return ERR_PTR(-ENOMEM);
	}
	err = sis3300_fill_pt_cache(card, segment_nr, pt_cache, segment->nr_events);
	if (err) {
		dev_info(card->dev, "Failed to fill segment's %i prevtick's cache buffer (err %d)\n", segment_nr, err);
		goto fill_pt_cache_failed;
	}
	segment->prevticks = pt_cache;
	return pt_cache;

 fill_pt_cache_failed:
	kfree(pt_cache);
	return ERR_PTR(err);
}

static int
sis3300_fetch(struct sis33_card *card, int segment_nr, int channel_nr, struct sis33_acq *acqs, int nr_events)
{
	struct sis33_segment *segment = &card->segments[segment_nr];
	struct sis33_event *events;
	u32 *bouncebuf;
	u32 *pt_cache;
	int ret;
	int i;

	events = sis3300_evcache_get(card, segment_nr);
	if (IS_ERR(events))
		return PTR_ERR(events);
	bouncebuf = sis3300_sampcache_get(card, segment_nr, channel_nr);
	if (IS_ERR(bouncebuf))
		return PTR_ERR(bouncebuf);
	pt_cache = sis3300_pt_cache_get(card, segment_nr);
	if (IS_ERR(pt_cache))
		return PTR_ERR(pt_cache);

	for (i = 0; i < nr_events; i++) {
		u32 *raw_event = &bouncebuf[i * segment->nr_samp_per_ev];

		ret = sis3300_read_event(&acqs[i], channel_nr, raw_event, pt_cache[i], &events[i]);
		if (ret)
			goto out;
	}
	ret = i;
 out:
	return ret;
}

static int sis3300_trigger(struct sis33_card *card, u32 trigger)
{
	struct sis3300 *priv = card->private_data;

	switch (trigger) {
	case SIS33_TRIGGER_START:
		sis3300_writew(priv, SIS3300_START_SAMP, 1);
		return 0;
	case SIS33_TRIGGER_STOP:
		sis3300_writew(priv, SIS3300_STOP_SAMP, 1);
		return 0;
	default:
		dev_warn(card->pdev, "Invalid trigger %d\n", trigger);
		return -EINVAL;
	}
}

/*
 * Arm bank 1 or 2
 * No need to acquire the lock because the segment is in the 'acquiring' state
 */
static void sis3300_acq_start(struct sis33_card *card)
{
	struct sis3300 *priv = card->private_data;

	sis3300_evcache_free(card, card->curr_segment);
	sis3300_sampcache_free(priv, card->curr_segment);
	sis3300_pt_cache_free(card, card->curr_segment);

	INIT_COMPLETION(priv->completion);
	sis3300_writew(priv, SIS3300_ACQ, 1 << card->curr_segment);
	/*
	 * when start_auto is enabled, the module should start sampling
	 * immediately after being armed
	 */
	if (card->cfg.start_auto)
		sis3300_writew(priv, SIS3300_START_SAMP, 1);
}

static void sis3300_finish_acquisition(struct sis33_card *card)
{
	struct sis3300 *priv = card->private_data;
	struct sis33_segment *segment = &card->segments[card->curr_segment];

	do_gettimeofday(&segment->endtime);
	sis3300_writew(priv, SIS3300_ACQ, ACQ_DISARM1 | ACQ_DISARM2);
	atomic_set(&segment->acquiring, 0);
	complete_all(&priv->completion);
}

static int sis3300_acq_wait(struct sis33_card *card, struct timespec *timeout)
{
	struct sis3300 *priv = card->private_data;
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

/*
 * The device doesn't interrupt after we cancel an ongoing acquisition,
 * therefore we must cancel the acquisition and then tell the core about it.
 */
static int sis3300_acq_cancel(struct sis33_card *card)
{
	struct sis33_segment *segment;

	/*
	 * here we don't use mutex_lock_interruptible; we must handle this right
	 * now, otherwise we risk leaving the device in a permanent 'busy' state
	 */
	mutex_lock(&card->lock);
	segment = &card->segments[card->curr_segment];
	/*
	 * If the device is sampling, before cancelling we check which event is
	 * being acquired. Then we know that all the events up to that one have
	 * already been acquired, and we note that value in segment->nr_events.
	 * This is especially useful when cancelling an acquisition that takes
	 * too long to end (imagine autostop is disabled and the stop triggers
	 * arrive very rarely); after losing our patience we'd at least be able
	 * to read back the events that had been acquired up to that moment.
	 */
	if (atomic_read(&segment->acquiring)) {
		struct sis3300 *priv = card->private_data;
		int curr_event;
		int acquiring;

		curr_event = sis3300_readw(priv, EV_COUNTER(card->curr_segment));
		acquiring = sis3300_readw(priv, SIS3300_ACQ) & ACQ_BUSY;
		sis3300_finish_acquisition(card);
		segment->nr_events = acquiring ? curr_event - 1 : curr_event;
		if (segment->nr_events < 0)
			segment->nr_events = 0;
	}
	mutex_unlock(&card->lock);
	return 0;
}

static void sis3300_config_init(struct sis33_card *card)
{
	struct sis33_cfg *cfg = &card->cfg;
	int i;

	cfg->start_delay	= 0;
	cfg->stop_delay		= 0;
	cfg->start_auto		= 1;
	cfg->stop_auto		= 1;
	cfg->trigger_ext_en	= 1;
	cfg->clksrc		= SIS33_CLKSRC_INTERNAL;
	cfg->clkfreq		= 100000000;
	cfg->ev_tstamp_divider	= 1;
	for (i = 0; i < card->n_channels; i++)
		card->channels[i].show_attrs = 0;
}

static struct sis33_card_ops sis3300_ops = {
	.conf_acq		= sis3300_conf_acq,
	.conf_ev		= sis3300_conf_ev,
	.fetch			= sis3300_fetch,
	.trigger		= sis3300_trigger,
	.acq_start		= sis3300_acq_start,
	.acq_wait		= sis3300_acq_wait,
	.acq_cancel		= sis3300_acq_cancel,
	.config_init		= sis3300_config_init,
};

static int __devinit sis3300_match(struct device *devp, unsigned int ndev)
{
	if (ndev >= base_num)
		return 0;
	if (ndev >= vector_num || ndev >= level_num) {
		dev_warn(devp, "irq vector/level missing\n");
		return 0;
	}
	return 1;
}

static void __devinit sis3300_reset(struct sis3300 *priv)
{
	sis3300_writew(priv, SIS3300_RESET, 1);
	udelay(5);
}

static int sis3300_irq(void *arg)
{
	struct sis33_card *card = arg;

	queue_work(sis33_workqueue, &card->irq_work);
	return 0;
}

/*
 * The device does not stop sampling in multievent mode unless a bank fills
 * up. This means we need to do some gymnastics to behave as user-space
 * expects, which is to stop sampling after a certain number of events
 * have been acquired, no matter whether that number of events would fill up
 * a bank or not.
 * Thus, every time we receive an 'event acquired' interrupt, we check the
 * EV_COUNTER register, and when we have gone beyond the requested number
 * of events we stop the acquisition.
 */
static inline void sis3300_do_irq(struct sis33_card *card, u32 status)
{
	struct sis33_segment *segment = &card->segments[card->curr_segment];
	struct sis3300 *priv = card->private_data;
	int event_threshold;
	int curr_event;
	int acquiring;
	/*
	 * Reading the acquisition state before reading what the current segment
	 * is would be racy. Imagine we read that the module isn't acquiring,
	 * and immediately after that a start arrives and the device starts
	 * acquiring the next event. Then we'd go and read that the current
	 * event is that 'next' event; we'd conclude that 'next' has been
	 * acquired, which is wrong.
	 * Therefore it's safer to read first what the current event is and
	 * then read whether the device is acquiring or not.
	 */
	curr_event = sis3300_readw(priv, EV_COUNTER(card->curr_segment));

	if (status & INTCTL_ST_EV) {
		sis3300_writew(priv, SIS3300_INTCTL, INTCTL_DI_EV | INTCTL_CL_EV);
		acquiring = sis3300_readw(priv, SIS3300_ACQ) & ACQ_BUSY;
		/*
		 * If it's acquiring, the device is in multi-event mode, and
		 * we shouldn't stop if it's sampling the last requested event.
		 * If it isn't acquiring, the event in curr_event has already
		 * been acquired.
		 */
		if (acquiring)
			event_threshold = priv->req_events + 1;
		else
			event_threshold = priv->req_events;
		/*
		 * checking that segment->acquiring is set avoids races between
		 * consecutive interrupts
		 */
		if (curr_event >= event_threshold && atomic_read(&segment->acquiring)) {
			segment->nr_events = priv->req_events;
			sis3300_finish_acquisition(card);
		}
		sis3300_writew(priv, SIS3300_INTCTL, INTCTL_EN_EV);
	}
	if (status & INTCTL_ST_LEV) {
		/*
		 * We double check here that we counted the events correctly.
		 * Imagine we were waiting for more events than what could be
		 * stored in a bank!
		 */
		sis3300_writew(priv, SIS3300_INTCTL, INTCTL_DI_LEV | INTCTL_CL_LEV);
		if (atomic_read(&segment->acquiring)) {
			segment->nr_events = curr_event;
			sis3300_finish_acquisition(card);
		}
		sis3300_writew(priv, SIS3300_INTCTL, INTCTL_EN_LEV);
	}
}

static void sis3300_irq_work(struct work_struct *work)
{
	struct sis33_card *card = container_of(work, struct sis33_card, irq_work);
	struct sis3300 *priv = card->private_data;
	u32 status;

	/*
	 * acquiring the mutex we make sure that the card's state is consistent
	 * throughout the handling of the interrupt
	 */
	mutex_lock(&card->lock);
	/*
	 * Overlapping interrupts aren't queued because we're clearing them in
	 * ROAK mode--the interrupt source is disabled when acknowledged by
	 * the CPU, not by accessing a particular register. This means that
	 * while handling an interrupt some others may arrive, and for this
	 * reason we must check the interrupt register in a loop.
	 */
	while ((status = sis3300_readw(priv, SIS3300_INTCTL) & INTCTL_ST_MASK))
		sis3300_do_irq(card, status);
	mutex_unlock(&card->lock);
}

static int __devinit
sis3300_request_irq(struct sis33_card *card, int vector, int level)
{
	struct sis3300 *priv = card->private_data;
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

	INIT_WORK(&card->irq_work, sis3300_irq_work);
	err = vme_request_irq(vector, sis3300_irq, card, name);
	if (err)
		return err;

	reg = INTCONF_ENABLE | INTCONF_ROAK;
	reg |= level << INTCONF_LEVEL_SHIFT;
	reg |= vector;
	sis3300_writew(priv, SIS3300_INTCONF, reg);
	sis3300_writew(priv, SIS3300_INTCTL, INTCTL_EN_EV);
	sis3300_writew(priv, SIS3300_INTCTL, INTCTL_EN_LEV);

	return 0;
}

static int __devinit sis3300_device_init(struct sis33_card *card, int ndev)
{
	struct sis3300 *priv = card->private_data;
	u32 reg;
	int err;

	priv->vme_base = base[ndev];
	reg = sis3300_readw(priv, SIS3300_FW);
	card->id = (reg & FW_MODID) >> FW_MODID_SHIFT;
	card->major_rev = (reg & FW_MAJOR) >> FW_MAJOR_SHIFT;
	card->minor_rev = (reg & FW_MINOR) >> FW_MINOR_SHIFT;

	if (card->id != 0x3300) {
		dev_err(card->pdev, "device not present at base address 0x%08lx. "
			"version read back 0x%04x expected 0x3300\n",
			priv->vme_base, card->id);
		return -ENODEV;
	}
	sis3300_reset(priv);

	init_completion(&priv->completion);
	err = sis3300_request_irq(card, vector[ndev], level[ndev]);
	if (err)
		return err;
	return 0;
}

static void sis3300_free_irq(struct sis33_card *card, int vector, int level)
{
	struct sis3300 *priv = card->private_data;
	unsigned int reg;
	int ret;

	/* quiesce the module and unregister the irq */
	reg = sis3300_readw(priv, SIS3300_INTCONF);
	reg &= ~INTCONF_ENABLE;
	sis3300_writew(priv, SIS3300_INTCONF, reg);
	sis3300_writew(priv, SIS3300_INTCTL, 0);
	ret = vme_free_irq(vector);
	if (ret)
		dev_warn(card->pdev, "Cannot free irq %d, err %d\n", vector, ret);
}

static void sis3300_device_exit(struct sis33_card *card, int ndev)
{
	sis3300_free_irq(card, vector[ndev], level[ndev]);
	flush_workqueue(sis33_workqueue);
}

static int __devinit sis3300_probe(struct device *pdev, unsigned int ndev)
{
	struct sis33_channel *channels;
	struct sis33_segment *segments;
	struct sis33_card *card;
	struct sis3300 *priv;
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

	channels = kcalloc(SIS3300_NR_CHANNELS, sizeof(*channels), GFP_KERNEL);
	if (channels == NULL) {
		error = -ENOMEM;
		goto alloc_channels_failed;
	}

	segments = kcalloc(SIS3300_MAX_NR_SEGMENTS, sizeof(*segments), GFP_KERNEL);
	if (segments == NULL) {
		error = -ENOMEM;
		goto alloc_segments_failed;
	}

	card->pdev = pdev;
	card->private_data = priv;

	error = sis3300_device_init(card, ndev);
	if (error)
		goto device_init_failed;

	strlcpy(card->driver, DRV_NAME, sizeof(card->driver));
	snprintf(card->description, sizeof(card->description),
		"Struck SIS%xv0x%02x%02x at VME-A32 0x%08lx irqv %d irql %d",
		card->id, card->major_rev, card->minor_rev, priv->vme_base,
		vector[ndev], level[ndev]);
	card->ev_tstamp_support = 1;
	card->ev_tstamp_max_ticks_log2 = 24;
	card->ev_tstamp_divider_max = SIS3300_TSTAMP_PREDIV_MASK;
	card->n_bits = 12;

	card->n_channels = SIS3300_NR_CHANNELS;
	card->channels = channels;

	card->max_nr_events = SIS3300_MAX_NR_EVENTS;
	card->max_delay	= SIS3300_MAX_DELAY;

	if (sis3300_rev(card) >= 0x0308) {
		card->n_ev_lengths = SIS3300_08_NR_EV_LENGTHS;
		card->ev_lengths = &sis3300_08_ev_lengths[0];
	} else {
		card->n_ev_lengths = SIS3300_NR_EV_LENGTHS;
		card->ev_lengths = &sis3300_ev_lengths[0];
	}

	card->n_freqs = SIS3300_NR_FREQS;
	card->freqs = &sis3300_freqs[0];

	card->n_segments_max = SIS3300_MAX_NR_SEGMENTS;
	card->n_segments_min = SIS3300_MAX_NR_SEGMENTS;
	card->n_segments = 2;
	card->segments = segments;
	card->curr_segment = 0;

	error = sis33_card_register(card, &sis3300_ops);
	if (!error)
		goto out;

	sis3300_device_exit(card, ndev);
 device_init_failed:
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

static int __devexit sis3300_remove(struct device *pdev, unsigned int ndev)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	sis3300_device_exit(card, ndev);
	sis3300_evcache_free_all(card);
	sis3300_sampcache_free_all(card->private_data);
	sis3300_pt_cache_free_all(card);
	kfree(card->segments);
	kfree(card->channels);
	kfree(card->private_data);
	sis33_card_free(card);
	return 0;
}

static struct vme_driver sis3300_driver = {
	.match		= sis3300_match,
	.probe		= sis3300_probe,
	.remove		= __devexit_p(sis3300_remove),
	.driver		= {
		.name	= DRV_NAME,
	},
};

static int __init sis3300_init(void)
{
	return vme_register_driver(&sis3300_driver, SIS33_MAX_DEVICES);
}

static void __exit sis3300_exit(void)
{
	vme_unregister_driver(&sis3300_driver);
}

module_init(sis3300_init)
module_exit(sis3300_exit)

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sis3300 ADC driver");
MODULE_VERSION(DRV_MODULE_VERSION);
