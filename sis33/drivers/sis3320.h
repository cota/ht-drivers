/*
 * sis3320.h
 * SIS3320 Hardware description
 *
 * Copyright (c) 2009-10 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _SIS3320_H_
#define _SIS3320_H_

#include <linux/bitops.h>

/*
 * SIS3320 registers.
 * All of them are Big-Endian 32-bit registers.
 *
 * Note: write access to a key address (KA) with arbitrary data invokes
 * the respective action.
 */

/*
 * Control/Status register (Read/Write)
 */
#define SIS3320_CTL		0x000
#define CTL_LED_SET	BIT(0)
#define CTL_LED_CLR	BIT(16)

/*
 * Module Id/Firmware (Read only)
 */
#define SIS3320_FW		0x004
#define FW_MODID	0xffff0000
#define FW_MODID_SHIFT	16
#define FW_MAJOR	0xff00
#define FW_MAJOR_SHIFT	8
#define FW_MINOR	0x00ff
#define FW_MINOR_SHIFT	0

/*
 * Interrupt configuration register (R/W)
 * INTCONF_ROAK can be either RORA (0) or ROAK (1):
 * - RORA: Release on register access: the interrupt will be pending
 *   until the IRQ source is cleared by specific access to the corresponding
 *   disable VME IRQ source bit. Default.
 * - ROAK: Release on acknowledge: the interrupt condition will be cleared (and
 *   the IRQ source disabled) as soon as the interrupt is acknowledged by
 *   the CPU.
 * In both modes, a given interrupt is disabled after being serviced.
 */
#define SIS3320_INTCONF		0x008
#define INTCONF_ROAK		BIT(12)
#define INTCONF_ENABLE		BIT(11)
#define INTCONF_LEVEL_MASK	0x7
#define INTCONF_LEVEL_SHIFT	8
#define INTCONF_VECTOR_MASK	0xff

/*
 * Interrupt control register (R/W)
 */
#define SIS3320_INTCTL	0x00c
#define INTCTL_EN_EV	BIT(0)
#define INTCTL_EN_LEV	BIT(1)
#define INTCTL_DICL_EV	BIT(16)
#define INTCTL_DICL_LEV	BIT(17)
#define INTCTL_ST_EV	BIT(24)
#define INTCTL_ST_LEV	BIT(25)
#define INTCTL_ST_MASK	0x03000000

/*
 * Acquisition control/status (R/W)
 */
#define SIS3320_ACQ	0x010
#define ACQ_EN_AUTOST	BIT(4)
#define ACQ_EN_MEV	BIT(5)
#define ACQ_EN_IST	BIT(6)
#define ACQ_EN_FPANEL	BIT(8)
#define ACQ_EN_CLKSRC	0x7000
#define ACQ_EN_CLKSRC_SHIFT	12
#define ACQ_ARMED	BIT(16)
#define ACQ_BUSY	BIT(17)
#define ACQ_DI_AUTOST	BIT(20)
#define ACQ_DI_MEV	BIT(21)
#define ACQ_DI_IST	BIT(22)
#define ACQ_DI_FPANEL	BIT(24)
#define ACQ_CL_CLKSRC	0x70000000

/* Clock source bit fields */
#define CLKSRC_200	0x0
#define CLKSRC_100	0x1
#define CLKSRC_50	0x2
#define CLKSRC_EXT5	0x3
#define CLKSRC_EXT2	0x4
#define CLKSRC_RANDOM	0x5
#define CLKSRC_EXT	0x6
/* SIS3320-250 only */
#define CLKSRC_250	0x7

/* SIS3302 only */
#define CLKSRC_3302_100		0x0
#define CLKSRC_3302_50		0x1
#define CLKSRC_3302_25		0x2
#define CLKSRC_3302_10		0x3
#define CLKSRC_3302_1		0x4
#define CLKSRC_3302_RANDOM	0x5
#define CLKSRC_3302_EXT		0x6

/*
 * External start delay (R/W)
 */
#define SIS3320_EXT_START_DELAY	0x014

/*
 * External stop delay (R/W)
 */
#define SIS3320_EXT_STOP_DELAY	0x018

/* this is valid for both START and STOP */
#define SIS3320_MAX_DELAY		0xffffff

/*
 * Max. Number of Events (R/W)
 */
#define SIS3320_NR_EVENTS	0x020

/*
 * Event Counter (R/o)
 */
#define SIS3320_EV_COUNTER	0x024
#define EV_COUNTER_MASK	0xfffff

/*
 * Broadcast Setup register (R/W)
 */
#define SIS3320_BCAST	0x030
#define BCAST_EN	BIT(4)
#define BCAST_EN_MASTER	BIT(5)
#define BCAST_ADDR	0xff000000

/*
 * ADC Memory page (R/W)
 * valid page numbers: 0-7
 */
#define SIS3320_ADC_MEM_PAGE	0x034
#define ADC_MEM_PAGE_MASK	0x7

#define SIS3320_PAGESIZE_SAMPLES	0x400000
#define SIS3320_PAGESIZE		0x800000
#define SIS3320_PAGEMASK	((SIS3320_PAGESIZE)-1)
#define SIS3320_PAGESHIFT	23

/*
 * DAC control status (R/W)
 */
#define SIS3320_DAC_CTL	0x050
#define DAC_CTL_CMD		0x3
/* SELECT is 250-only */
#define DAC_CTL_SELECT_MASK	0x70
#define DAC_CTL_SELECT_SHIFT	4
#define DAC_CTL_BUSY		BIT(15)

/* DAC Command bit fields */
#define DACCMD_NOFUNC	0x0
#define DACCMD_LOAD_SH	0x1
#define DACCMD_LOAD_DAC	0x2
#define DACCMD_CLEARALL	0x3

/*
 * DAC Data (R/W)
 */
#define SIS3320_DAC_DATA	0x054
#define DAC_DATA_WR		0x0000ffff
#define DAC_DATA_WR_SHIFT	0
#define DAC_DATA_RD		0xffff0000
#define DAC_DATA_RD_SHIFT	16

/*
 * ADC gain register
 */
#define SIS3320_ADC_GAINCTL	0x058

/*
 * Xilinx JTAG test / JTAG Data in (R/W)
 */
#define SIS3320_JTAG_TEST_IN	0x060

/*
 * Xilinx JTAG Control (W/o)
 */
#define SIS3320_JTAG_CTL	0x064

/*
 * Reset (KA)
 */
#define SIS3320_RESET		0x400

/*
 * Arm sampling logic (KA)
 */
#define SIS3320_ARM		0x410

/*
 * Disarm sampling logic (KA)
 */
#define SIS3320_DISARM		0x414

/*
 * VME start sampling (KA)
 */
#define SIS3320_START_SAMP	0x418

/*
 * VME stop sampling (KA)
 */
#define SIS3320_STOP_SAMP	0x41c

/*
 * Address Timestamp clear (KA) (sis3302 only)
 */
#define SIS3302_TSTAMP_CLEAR	0x42c

/*
 * Reset DDR2 memory logic (KA)
 */
#define SIS3320_RESET_DDR2	0x428

/*
 * Event timestamp directory (sis3302 only)
 */
#define SIS3302_TSTAMP_DIR	0x10000
#define SIS3302_TSTAMP_UPPER_MASK	0xffff

/*
 * Event information for all ADC groups
 * The ADCs are grouped in pairs. These pairs can be configured separately.
 */

/* offset of the first ADC group */
#define SIS3320_G_OFFSET	0x2000000

/* offset of a given group. Note that only 1-4 are valid groups */
#define SIS3320_G(grp)	((SIS3320_G_OFFSET)+((grp)-1)*0x800000)

/* offset of a given register within a group */
#define SIS3320_GREG(grp,reg)	((SIS3320_G(grp)) + (reg))

/* offset of the common registers */
#define SIS3320_GALL_OFFSET	0x1000000

/* registers common to all ADCs */
#define SIS3320_GALL(reg)	((SIS3320_GALL_OFFSET) + (reg))

/*
 * Some registers in the group apply only to one of the two ADCs. 'num'
 * (valid values: 1-2) specifies which of the ADCs is to be affected.
 */
#define SIS3320_GREG_SP(grp,reg,num)	((SIS3320_GREG(grp,reg))+0x4*((num)-1))

/*
 * Event configuration (R/W)
 */
#define EV_CONF		0x00
#define EV_WRAPSIZ		0xf
#define EV_EN_WRAP_MODE		BIT(4)
#define EV_EN_SAMPLEN_STOP	BIT(5)
#define EV_EN_CTL1_ACC		BIT(10)
#define EV_EN_CTL1_DSTREAM	BIT(12)

/* samples per event bit fields */
#define EV_LENGTH_16M	0x0
#define EV_LENGTH_4M	0x1
#define EV_LENGTH_1M	0x2
#define EV_LENGTH_256K	0x3
#define EV_LENGTH_64K	0x4
#define EV_LENGTH_16K	0x5
#define EV_LENGTH_4K	0x6
#define EV_LENGTH_1K	0x7
#define EV_LENGTH_512	0x8
#define EV_LENGTH_256	0x9
#define EV_LENGTH_128	0xa
#define EV_LENGTH_64	0xb

/* these are the page sizes defined above + 32M (wrap around full memory) */
#define SIS3320_EV_LENGTHS	13

/*
 * Sample length (R/W)
 */
#define SAMP_LEN	0x04

/* maximum of 32MSamples */
#define SIS3320_MAX_SAMPLE_LEN		0x2000000

/*
 * Sample start address (R/W)
 */
#define SAMP_STADDR	0x08

/*
 * ADC input mode (R/W)
 */
#define INPUT_MODE	0x0c

/* ADC Input mode bit fields */
#define INPUT_TEST_ST	0xffff
#define INPUT_SIM_TEST	BIT(16)
#define INPUT_TEST_MODE	BIT(17)

/*
 * Next sample address ADC 1 and 2(R/o)
 */
#define SAMPLE_NEXT1	0x10
#define SAMPLE_NEXT2	0x14
#define SAMPLE_NEXT(n)	((SAMPLE_NEXT1)+(n-1)*4)

/*
 * Actual (I think they meant current) sample value
 */
#define SAMPLE_ACT	0x20
#define SAMPLE_ACT_ODD	0x0fff0000
#define SAMPLE_ACT_EVEN	0x00000fff

/*
 * Internal test register (R/o)
 */
#define INTERN_TEST	0x24

/*
 * DDR2 memory logic verification (R/o)
 * Hopefully we won't need to deal with these.
 */
#define DDR2_VERIF	0x28

/*
 * Trigger flag clear counter (R/W)
 */
#define TRIG_CL_COUNT	0x2c

/*
 * ADC1/2 Trigger setup (R/W)
 */
#define TRIG_SETUP1	0x30
#define TRIG_SETUP2	0x38
#define TRIG_SETUP_P	0x00001f
#define TRIG_SETUP_SUMG	0x001f00
#define TRIG_SETUP_PLEN	0xff0000

/*
 * ADC1/2 Trigger threshold (R/W)
 */
#define TRIG_THRESH1		0x34
#define TRIG_THRESH2		0x3C
#define TRIG_THRESH_MODE	BIT(26)
#define TRIG_THRESH_GTGE	BIT(25)
#define TRIG_THRESH_LT		BIT(24)
#define TRIG_THRESH_TRAPZD	0xffff

/*
 * SPI register
 */
#define SPI_REG		0x40
#define SPI_REG_DATA_MASK	0xff
#define SPI_REG_DATA_SHIFT	0
#define SPI_REG_ADDR_MASK	0x1fff
#define SPI_REG_ADDR_SHIFT	8
#define SPI_REG_RD		BIT(21)
#define SPI_REG_SEL		BIT(22)
#define SPI_REG_BUSY		BIT(23)

/*
 * AD9230: names taken from the AD9230's datasheet
 */
#define AD9230_CHIP_PORT_CONFIG		0x00
#define AD9230_CHIP_ID			0x01
#define AD9230_CHIP_GRADE		0x02
#define AD9230_ADC_MODES		0x08
#define AD9230_ADC_CLOCK		0x09
#define AD9230_TEST_IO			0x0d
#define AD9230_AIN_CONFIG		0x0f
#define AD9230_OUTPUT_MODE		0x14
#define AD9230_OUTPUT_ADJUST		0x15
#define AD9230_OUTPUT_PHASE		0x16
#define AD9230_FLEX_OUTPUT_DELAY	0x17
#define AD9230_FLEX_VREF		0x18
#define AD9230_OVR_CONFIG		0x2a
#define AD9230_DEVICE_UPDATE		0xff

/*
 * Event directory ADC1
 */
#define SIS3320_EV_DIR1	0x2010000

/* see scripts/sis3320_event_dirs.pl for a test of this calculation */
#define SIS3320_EV_DIR(n) ((SIS3320_EV_DIR1)+((n)/2)*0x800000+((n)%2)*0x8000)

#define EV_DIR_TRIG	BIT(29)
#define EV_DIR_WRAP	BIT(28)
#define EV_DIR_ADDR	0x1ffffff

/*
 * Memory pages -- accessed through DMA
 */
#define SIS3320_MEM_OFFSET	0x4000000

/* valid ADC numbers: 0-7 */
#define SIS3320_MEM_ADC(num)	((SIS3320_MEM_OFFSET)+(num)*0x800000)

#endif /* _SIS3320_H_ */
