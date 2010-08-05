/*
 * sis33core.h
 * sis33 driver core
 *
 * Copyright (c) 2009-2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _SIS33CORE_H_
#define _SIS33CORE_H_

#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/time.h>

#include <asm/atomic.h>

#include <sis33.h>

#define SIS33_DEFAULT_IDX	{ [0 ... (SIS33_MAX_DEVICES-1)] = -1 }

extern struct workqueue_struct *sis33_workqueue;

struct sis33_card_ops;

/**
 * struct sis33_channel - descriptor of an sis33's channel
 * @kobject:	kobject to represent the channel
 * @show_attrs:	export default attributes through sysfs
 * @offset:	ADC offset (16-bit value)
 */
struct sis33_channel {
	struct kobject		kobj;
	int			show_attrs;
	unsigned int		offset;
};

/**
 * struct sis33_cfg - configuration of an sis33
 * @start_delay:	start delay (in samples)
 * @stop_delay:		stop delay (in samples)
 * @start_auto:		enable autostart mode
 * @stop_auto:		enable sample length stop
 * @trigger_ext_en:	enable external trigger
 * @clksrc:		clock source, enum sis33_clk
 * @clkfreq:		clock frequency, in Hz
 * @ev_tstamp_divider:	divider of the sampling clock to generate the timestamping clock
 */
struct sis33_cfg {
	int			start_delay;
	int			stop_delay;
	int			start_auto;
	int			stop_auto;
	int			trigger_ext_en;
	int			clksrc;
	unsigned int		clkfreq;
	int			ev_tstamp_divider;
};

/**
 * struct sis33_event - attributes of an sis33 event
 * @nr_samples:		number of samples acquired
 * @first_samp:		index of the first valid sample
 */
struct sis33_event {
	int		nr_samples;
	int		first_samp;
};

/**
 * struct sis33_segment - descriptor of an sis33 segment
 * @acquiring:		1 when the segment is busy (acquiring), 0 otherwise.
 * @transferring:	1 when there is an ongoing transfer to main memory. 0 otherwise.
 * @nr_samp_per_ev:	number of samples per event
 * @nr_events:		number of events
 * @events:		array of events
 * @prevticks:		array of prevticks, number of clock ticks after the first event stop
 * @endtime:		local time when the last event was acquired
 */
struct sis33_segment {
	atomic_t		acquiring;
	atomic_t		transferring;
	unsigned int		nr_samp_per_ev;
	unsigned int		nr_events;
	struct sis33_event	*events;
	void			*prevticks;
	struct timeval		endtime;
};

/**
 * struct sis33_card - description of an sis33 card
 * @number:		card number in the global sis33_card_lock. from 0 to n-1.
 * @lock:		mutex to serialise access to the device
 * @cdev:		associated character device
 * @dev:		associated device
 * @module:		module that manages the device
 * @ops:		sis33 operations to act on the hardware
 * @pdev:		parent device
 * @channels_dir:	channels' directory kobject
 * @sysfs_files_created:set to 1 when the sysfs files have been created
 * @cfg:		configuration of the board
 * @private_data:	pointer to some module-specific data
 * @driver:		driver name
 * @id:			hardware id
 * @major_rev:		hardware major revision
 * @minor_rev:		hardware minor revision
 * @description:	device description
 * @irq_work:		workqueue to handle interrupts
 * @max_nr_events:	maximum number of events of this board
 * @max_delay:		maximum delay of this board
 * @ev_tstamp_support:	1 or 0 to announce whether or not the device supports event timestamping
 * @ev_tstamp_max_ticks_log2:	log2 of the number of clock ticks at which the timestamp counter overflows
 * @ev_tstamp_divider_max:	max. value of the timestamp divider
 * @n_bits:		number of bits of the ADC conversion
 * @n_channels:		number of channels available
 * @channels:		pointer to the array of channels
 * @n_ev_lengths:	number of page sizes available
 * @ev_lengths:		pointer to the array of page sizes
 * @n_freqs:		number of frequencies available
 * @freqs:		pointer to the array of frequencies
 * @n_segments_max:	maximum number of segments
 * @n_segments_min:	minimum number of segments
 * @n_segments:		number of segments
 * @segments:		array of sis33 segments
 * @curr_segment:	current segment from the array of segments
 *
 * NOTE:
 * The values in @ev_lengths and @freqs must be ordered from higher to lower.
 */
struct sis33_card {
	/* device handling */
	int			number;
	struct mutex		lock;
	struct cdev		cdev;
	struct device		*dev;
	struct module		*module;
	struct sis33_card_ops	*ops;
	struct device		*pdev;
	struct kobject		channels_dir;
	int			sysfs_files_created;

	/* card configuration */
	struct sis33_cfg	cfg;

	/* the below are all set by the particular sis33xx modules */
	void			*private_data;
	char			driver[16];
	int			id;
	int			major_rev;
	int			minor_rev;
	char			description[80];
	struct work_struct	irq_work;
	unsigned int		max_nr_events;
	unsigned int		max_delay;
	int			ev_tstamp_support;
	int			ev_tstamp_max_ticks_log2;
	int			ev_tstamp_divider_max;
	int			n_bits;
	int			n_channels;
	struct sis33_channel	*channels;
	int			n_ev_lengths;
	const u32		*ev_lengths;
	int			n_freqs;
	const u32		*freqs;
	int			n_segments_max;
	int			n_segments_min;
	int			n_segments;
	struct sis33_segment	*segments;
	int			curr_segment;
};

/**
 * struct sis33_card_ops - available operations on an sis33 module
 * @conf_acq:		configure acquisition
 * @conf_ev:		configure events
 * @conf_channels:	configure channels
 * @fetch:		retrieve acquisition data
 * @trigger:		send software trigger to the module
 * @acq_start:		start acquisition
 * @acq_wait:		wait for an acquisition to finish
 * @acq_cancel:		cancel acquisition
 * @config_init:	fill in the default config
 */
struct sis33_card_ops {
	int (*conf_acq)		(struct sis33_card *card, struct sis33_cfg *cfg);
	int (*conf_ev)		(struct sis33_card *card, struct sis33_acq_desc *desc);
	int (*conf_channels)	(struct sis33_card *card, struct sis33_channel *channels);
	int (*fetch)		(struct sis33_card *card, int segment_nr, int channel_nr, struct sis33_acq *acqs, int nr_events);
	int (*trigger)		(struct sis33_card *card, u32 trigger);
	void (*acq_start)	(struct sis33_card *card);
	int (*acq_wait)		(struct sis33_card *card, struct timespec *timeout);
	int (*acq_cancel)	(struct sis33_card *card);
	void (*config_init)	(struct sis33_card *card);
};

int sis33_card_create(int idx, struct module *module,
		struct sis33_card **card_ret, struct device *pdev);
int sis33_card_register(struct sis33_card *card, struct sis33_card_ops *ops);
void sis33_card_free(struct sis33_card *card);
int sis33_dma_read_mblt(struct device *dev, unsigned int vme_addr, void __kernel *addr, ssize_t size);
int sis33_dma_read_mblt_user(struct device *dev, unsigned int vme_addr, void __user *addr, ssize_t size);
int sis33_dma_read32be_blt(struct device *dev, unsigned int vme_addr, u32 __kernel *addr, unsigned int elems);
unsigned int sis33_dma_read(struct device *dev, unsigned int vme_addr);
void sis33_dma_write(struct device *dev, unsigned int vme_addr, unsigned int val);

#endif /* _SIS33CORE_H_ */
