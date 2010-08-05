/*
 * sis3300.h
 * SIS3300 hardware description
 *
 * Copyright (c) 2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _SIS3300_H_
#define _SIS3300_H_

#include <linux/bitops.h>

/* Control/status (R/W) */
#define SIS3300_CONTROL_STATUS	0x0
#define USER_LED_ON		BIT(0)
#define USER_LED_OFF		BIT(16)
#define USER_OUTPUT_ON		BIT(1)
#define USER_OUTPUT_CLEAR	BIT(17)
#define EN_TRIG_OUTPUT		BIT(2)
#define DI_TRIG_OUTPUT		BIT(18)
#define INVERT_TRIG		BIT(4)
#define NON_INVERT_TRIG		BIT(20)
#define EN_TRIG_ON_ARMED	BIT(5)
#define DI_TRIG_ON_ARMED	BIT(21)
#define EN_INT_TRIG_ROUTING	BIT(6)
#define DI_INT_TRIG_ROUTING	BIT(22)
#define SET_BANKFULL_OUT1	BIT(8)
#define SET_BANKFULL_OUT2	BIT(9)
#define SET_BANKFULL_OUT3	BIT(10)
#define CLEAR_BANKFULL_OUT1	BIT(24)
#define CLEAR_BANKFULL_OUT2	BIT(25)
#define CLEAR_BANKFULL_OUT3	BIT(26)
#define SET_DONT_CLEAR_TSTAMP	BIT(12)
#define CLEAR_DONT_CLEAR_TSTAMP	BIT(28)

/* Module Id/Firmware (R/O) */
#define SIS3300_FW	0x4
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
#define SIS3300_INTCONF	0x8
#define INTCONF_ROAK	BIT(12)
#define INTCONF_ENABLE	BIT(11)
#define INTCONF_LEVEL_MASK	0x7
#define INTCONF_LEVEL_SHIFT	8
#define INTCONF_VECTOR_MASK	0xff

/*
 * Interrupt control register (R/W)
 */
#define SIS3300_INTCTL	0xc
#define INTCTL_EN_EV	BIT(0)
#define INTCTL_DI_EV	BIT(16)
#define INTCTL_CL_EV	BIT(20)
#define INTCTL_ST_EV	BIT(28)

#define INTCTL_EN_LEV	BIT(1)
#define INTCTL_DI_LEV	BIT(17)
#define INTCTL_CL_LEV	BIT(21)
#define INTCTL_ST_LEV	BIT(29)

#define INTCTL_EN_UIN	BIT(3)
#define INTCTL_DI_UIN	BIT(19)
#define INTCTL_CL_UIN	BIT(23)
#define INTCTL_ST_UIN	BIT(31)

#define INTCTL_ST_MASK	0xb0000000

/*
 * Acquisition control/status (R/W)
 */
#define SIS3300_ACQ	0x10
#define ACQ_ARM1	BIT(0)
#define ACQ_ARM2	BIT(1)
#define ACQ_EN_AUTOSWITCH	BIT(2)
#define ACQ_EN_DELAY_LL	BIT(3)	/* sis3301 03 06 only */
#define ACQ_EN_AUTOST	BIT(4)
#define ACQ_EN_MEV	BIT(5)
#define ACQ_EN_STARTD	BIT(6)
#define ACQ_EN_STOPD	BIT(7)
#define ACQ_EN_FPANEL	BIT(8)
#define ACQ_EN_P2_LOGIC	BIT(9)
#define ACQ_EN_GATEMODE	BIT(10)
#define ACQ_EN_RANDCLK	BIT(11)
#define ACQ_CLKSRC	0x7000
#define ACQ_CLKSRC_SHIFT	12
#define ACQ_EN_MPLEX	BIT(15)
#define ACQ_DISARM1	BIT(16)
#define ACQ_BUSY	BIT(16)
#define ACQ_DISARM2	BIT(17)
#define ACQ_DI_AUTOSWITCH	BIT(18)
#define ACQ_DI_DELAY_LL	BIT(19)	/* sis3301 03 06 only */
#define ACQ_DI_AUTOST	BIT(20)
#define ACQ_DI_MEV	BIT(21)
#define ACQ_DI_STARTD	BIT(22)
#define ACQ_DI_STOPD	BIT(23)
#define ACQ_DI_FPANEL	BIT(24)
#define ACQ_DI_P2_LOGIC	BIT(25)
#define ACQ_DI_GATEMODE	BIT(26)
#define ACQ_DI_RANDCLK	BIT(27)
#define ACQ_CL_CLKSRC	0x70000000
#define ACQ_CL_CLKSRC_SHIFT	28
#define ACQ_EN_MPLEX	BIT(15)

/*
 * clock sources
 * Different versions have different available clock sources
 */
#define SIS3300_CLKSRC_100	0x0
#define SIS3300_CLKSRC_50	0x1
#define SIS3300_CLKSRC_25	0x2
#define SIS3300_CLKSRC_12M5	0x3
#define SIS3300_CLKSRC_6M25	0x4
#define SIS3300_CLKSRC_3M125	0x5
#define SIS3300_CLKSRCS	6

#define SIS3300_CLKSRC_EXT	0x6
#define SIS3300_CLKSRC_P2	0x7


/*
 * External start/stop delay (R/W)
 */
#define SIS3300_EXT_START_DELAY	0x14
#define SIS3300_EXT_STOP_DELAY	0x18

#define SIS3300_MAX_DELAY	0xffff

/*
 * time stamp predivider register (R/W)
 */
#define SIS3300_TSTAMP_PREDIV	0x1c
#define SIS3300_TSTAMP_PREDIV_MASK	0xffff

/*
 * Reset (Key Address)
 */
#define SIS3300_RESET		0x20

/*
 * VME start sampling on the current bank, if armed. (KA)
 */
#define SIS3300_START_SAMP	0x30

/*
 * VME stop sampling (KA)
 */
#define SIS3300_STOP_SAMP	0x34

/*
 * Start/stop auto bank switch mode (KA)
 */
#define SIS3300_AUTO_BANK_SWITCH_START	0x40
#define SIS3300_AUTO_BANK_SWITCH_STOP	0x44

/*
 * Clear bank full flags (KA)
 */
#define SIS3300_CLEAR_BANK_FULL1	0x48
#define SIS3300_CLEAR_BANK_FULL2	0x4c

/*
 * one-wire id register (SIS3301 v3.05 only) (R/W)
 */
#define SIS3300_ONE_WIRE_ID	0x60

/*
 * event timestamp directories (R/O)
 */
#define SIS3300_EV_TSTAMP1	0x1000
#define SIS3300_EV_TSTAMP2	0x2000
#define SIS3300_EV_TSTAMP(nr)	(0x1000 << (nr))
#define SIS3300_EV_TSTAMP_MASK	0x00ffffff

#define SIS3300_GALL_OFFSET	0x100000
#define SIS3300_GALL(reg)	((SIS3300_GALL_OFFSET) + (reg))

/*
 * event configuration (R/W)
 */
#define EV_CONF	0x0
#define EV_CONF_EV_LEN_MASK	0x7
#define EV_CONF_EN_WRAP_MODE	BIT(3)
#define EV_CONF_GROUPID0	BIT(8)
#define EV_CONF_GROUPID1	BIT(9)
#define EV_CONF_RANDCLK_MODE	BIT(11)
#define EV_CONF_MULTIPLEX_MODE	BIT(15)
#define EV_CONF_AVERAGE_MASK	0x70000
#define EV_CONF_EV_LEN_MAP	BIT(20)	/* firmware 03 08 onwards */

/* event length map select bit == 0 */
#define EV_LENGTH_128K	0x0
#define EV_LENGTH_16K	0x1
#define EV_LENGTH_4K	0x2
#define EV_LENGTH_2K	0x3
#define EV_LENGTH_1K	0x4
#define EV_LENGTH_512	0x5
#define EV_LENGTH_256	0x6
#define EV_LENGTH_128	0x7

#define SIS3300_EV_LENGTHS	8

/* event length map select bit == 1 */
#define EV_LENGTH_32K	0x1
#define EV_LENGTH_8K	0x2

#define SIS3300_08_EV_LENGTHS	10

/*
 * threshold registers
 */
#define TRIG_THRESHOLD	0x4

/*
 * trigger flag clear counter (R/W)
 */
#define TRIG_FLAG_CLEAR_COUNTER	0x1

/*
 * clock predivider
 */
#define CLOCK_PREDIVIDER	0x20
#define CLOCK_PREDIVIDER_MASK	0xff

/*
 * Number of samples
 * This is used in multiplexer mode only
 */
#define NR_OF_SAMPLES	0x24

/*
 * Trigger output
 */
#define TRIG_OUTPUT	0x28

/*
 * Maximum number of events
 * Used in gate chaining mode only
 */
#define MAX_NR_EVENTS	0x2c
#define MAX_NR_EVENTS_MASK	0xffff

/*
 * Event directories
 * (bank valid from 0 to 1)
 */
#define EV_DIR0	0x101000
#define EV_DIR(bank) ((EV_DIR0)+(bank)*0x1000)

#define EV_DIR_WRAP	BIT(19)
#define EV_DIR_ADDR_MASK	0x1ffff

/*
 * The particular event directories and bank address counters
 * are left out of this file because they duplicate the functionality
 * of the event directories above
 */

/*
 * event counter
 * valid bank values range from 0 to n-1.
 */
#define EV_COUNTER0	0x200010
#define EV_COUNTER(bank)	((EV_COUNTER0) + (bank) * 0x4)
#define EV_COUNTER_MASK	0xffff


/*
 * Current sample value
 * The group parameter goes from 0 to 3.
 */
#define CURRSAMP_0	0x200018
#define CURRSAMP(channel) (CURRSAMP_0 + (group) * 0x80000)

/*
 * The sis3300 is a 12-bit adc and the sis3301 is a 14-bit one. The register
 * layout below is used for reading from both CURRSAMP and BANKMEM.
 */
#define ADC0_DATA_MASK	0xfff0000
#define ADC0_DATA_SHIFT	16
#define ADC0_OUTRANGE	BIT(28)
#define ADC1_DATA_MASK	0xfff
#define ADC1_DATA_SHIFT	0
#define ADC1_OUTRANGE	BIT(12)

#define SIS3301_ADC0_DATA_MASK	0x3fff0000
#define SIS3301_ADC0_OUTRANGE	BIT(30)
#define SIS3301_ADC1_DATA_MASK	0x3fff
#define SIS3301_ADC1_OUTRANGE	BIT(14)

/*
 * Memory pages -- accessed through DMA
 * valid bank and channel numbers: 0 to n-1
 */
#define MEMBANK1_ADC_0	0x400000
#define MEMBANK(bank,chan)	((MEMBANK1_ADC_0)+(bank)*0x200000+((chan)/2)*0x80000)

#endif /* _SIS3300_H_ */
