/*
 * cvorg.h
 *
 * CVORG kernel/user-space interface
 */

#ifndef _CVORG_H_
#define _CVORG_H_

#if !defined(__KERNEL__)
#include <stdint.h>
#else
#include <linux/types.h>
#endif

/* 2MB of SRAM per channel --> 1M samples/channel (i.e. 2 bytes/sample) */
#define CVORG_SRAM_SIZE		0x200000
#define CVORG_SRAM_SIZE_LONGS	((CVORG_SRAM_SIZE)/sizeof(uint32_t))

/* 100MHz maximum sampling frequency */
#define CVORG_MAX_FREQ		100000000

/**
 * @brief CVORG channels
 */
enum cvorg_channelnr {
	CVORG_CHANNEL_A = 1,
	CVORG_CHANNEL_B
};

/**
 * @brief Analog output offset
 */
enum cvorg_outoff {
	CVORG_OUT_OFFSET_0V,
	CVORG_OUT_OFFSET_2_5V
};

/**
 * @brief CVORG Input (triggers) polarity
 */
enum cvorg_inpol {
	CVORG_POSITIVE_PULSE,
	CVORG_NEGATIVE_PULSE
};

/* channel status bitmask */
#define CVORG_CHANSTAT_READY		0x1
#define CVORG_CHANSTAT_BUSY		0x2
#define CVORG_CHANSTAT_ERR		0x4
#define CVORG_CHANSTAT_CLKOK		0x8
#define CVORG_CHANSTAT_ERR_CLK		0x10
#define CVORG_CHANSTAT_ERR_TRIG		0x20
#define CVORG_CHANSTAT_ERR_SYNC		0x40
#define CVORG_CHANSTAT_SRAM_BUSY	0x80
#define CVORG_CHANSTAT_STARTABLE	0x100
#define CVORG_CHANSTAT_OUT_ENABLED	0x200


/**
 * @brief CVORG triggers
 */
enum cvorg_trigger {
	CVORG_TRIGGER_START,
	CVORG_TRIGGER_STOP,
	CVORG_TRIGGER_EVSTOP
};

/**
 * @brief Operation modes
 */
enum cvorg_mode {
	CVORG_MODE_OFF,
	CVORG_MODE_NORMAL,
	CVORG_MODE_DAC,
	CVORG_MODE_TEST1,
	CVORG_MODE_TEST2,
	CVORG_MODE_TEST3
};

/**
 * @brief Waveform struct
 */
struct cvorg_wv {
	uint32_t	recurr;		/**< play this wv 'recurr' times */
	uint32_t	size;		/**< size (in bytes) of the waveform */
	uint32_t	dynamic_gain;	/**< 1 to use gain_val, 0 otherwise */
	int32_t		gain_val;	/**< [-22,20] dB */
	void		*form;		/**< address of the waveform */
};

/**
 * @brief write buffer
 */
struct cvorg_seq {
	uint32_t	nr;		/**< play this sequence nr times */
	uint32_t	n_waves;	/**< number of waveforms */
	struct cvorg_wv *waves;		/**< address of the waveforms */
};

/**
 * @brief SRAM entry
 * @param address - address of the SRAM entry
 * @param data - value of the SRAM entry
 */
struct cvorg_sram_entry {
	uint32_t	address;
	uint16_t	data[2];
};

enum cvorg_adc_channel {
	CVORG_ADC_CHANNEL_MONITOR,
	CVORG_ADC_CHANNEL_REF,
	CVORG_ADC_CHANNEL_5V,
	CVORG_ADC_CHANNEL_GND
};

enum cvorg_adc_range {
	CVORG_ADC_RANGE_5V_BIPOLAR,
	CVORG_ADC_RANGE_5V_UNIPOLAR,
	CVORG_ADC_RANGE_10V_BIPOLAR,
	CVORG_ADC_RANGE_10V_UNIPOLAR
};

/**
 * @brief ADC configuration struct
 * @param channel - ADC channel. See enum cvorg_adc_channel.
 * @param range - Input range. See enum cvorg_adc_range.
 * @param value - Read value of the selected ADC for the given number/range.
 */
struct cvorg_adc {
	uint32_t	channel;
	uint32_t	range;
	uint32_t	value;
};

#endif /* _CVORG_H_ */
