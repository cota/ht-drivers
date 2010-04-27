/*
 * cvorg_hw.h
 *
 * CVORG hardware description
 */

#ifndef _CVORG_HW_H_
#define _CVORG_HW_H_

#ifndef BIT
#define BIT(nr)		(1UL << (nr))
#endif

/*
 * Channel's offset
 */
#define CVORG_CHAN1_OFFSET	0x000
#define CVORG_CHAN2_OFFSET	0x200
#define CVORG_CHANNELS	2

/*
 * CVORG Registers (0x000-0x1ff per channel)
 * All the registers in this card are 32-bits long
 */

/*
 * Interrupt Source (Read only)
 * This register is Clear-On-Read
 */
#define CVORG_INTSR	0x000
#define CVORG_INTSR_ENDFUNC	BIT(0)	/**< End of waveform */
#define CVORG_INTSR_ENDSRAM	BIT(1)	/**< End of read from external SRAM */
#define CVORG_INTSR_CHANNEL	BIT(31)	/**< Does it come from chan0 or 1? */
#define CVORG_INTSR_BITMASK	0x3	/**< Interrupt bitmask */
#define CVORG_NR_INTERRUPTS	2	/**< number of interrupts */

/*
 * Interrupt Enable mask (Read/Write)
 */
#define CVORG_INTEN	0x004
#define CVORG_INTEN_ENDFUNC	BIT(0) /**< enable End of function */
#define CVORG_INTEN_ENDSRAM	BIT(1) /**< enable End of read from external SRAM */
#define CVORG_INTEN_VALID	(CVORG_INTEN_ENDFUNC | CVORG_INTEN_ENDSRAM)

/*
 * Interrupt Level (R/W)
 * Values: 2 or 3
 */
#define CVORG_INTLEVEL	0x008

/*
 * Interrupt vector (R/W)
 * Values: 0 to 2**32 - 1
 */
#define CVORG_INTVECT	0x00c

/*
 * VHDL Version (R)
 * Encoded in BCD -- Upper 16 bits are not used
 * |15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
 * |   Tens    |   Units   |   Tenth   |Hundredths |
 */
#define CVORG_VERSION	0x010
#define CVORG_VERSION_FW_SHIFT		16
#define CVORG_VERSION_FW_MASK		0xffff
#define CVORG_VERSION_TENS_SHIFT	12
#define CVORG_VERSION_UNITS_SHIFT	8
#define CVORG_VERSION_TENTHS_SHIFT	4
#define CVORG_VERSION_HUNDREDTHS_SHIFT	0

/*
 * PCB serial number MSBs (R)
 * Each board has a unique ID, given by a 64-bit serial number
 */
#define CVORG_PCB_MSB	0x014

/*
 * PCB serial number LSBs (R)
 */
#define CVORG_PCB_LSB	0x018

/*
 * Temperature (R)
 * Format: 12 bits signed fixed point (bits 0-10)
 * Range: -55 to 125 [C]
 * Bits 11-31: sign --> they all have the same value
 */
#define CVORG_TEMP	0x01C

/*
 * ADC value (R)
 */
#define CVORG_ADCVAL	0x020
#define CVORG_ADCVAL_UP2DATE	BIT(31)
#define CVORG_ADCVAL_READVAL	0xffff

/*
 * CVORG Software Control (W)
 * Note that only one bit can be written at a time on this register
 */
#define CVORG_CTL	0x024
#define CVORG_CTL_CHAN_RESET	BIT(0) /**< Channel software reset */
#define CVORG_CTL_READADC	BIT(1) /**< Read ADC of the channel@"CHANCFG" */
#define CVORG_CTL_ERR_CLEAR	BIT(2) /**< Clear all errors except ERR_CLK */
#define CVORG_CTL_FPGA_RESET	BIT(3) /**< FPGA hardware reset */
#define CVORG_CTL_CHAN_START	BIT(4) /**< Simulate Start trigger */
#define CVORG_CTL_CHAN_STOP	BIT(5) /**< Simulate Stop trigger */
#define CVORG_CTL_CHAN_EVSTOP	BIT(7) /**< Simulate Event Stop trigger */
#define CVORG_CTL_CHAN_LOADSEQ	BIT(8) /**< Start loading a sequence */

/*
 * Sampling clock frequency (R)
 * given in [Hz]
 */
#define CVORG_SAMPFREQ	0x028

/*
 * Clock generator control (R/W)
 * SPI Interface to the AD9516-O IC
 */
#define CVORG_CLKCTL	0x02C
#define CVORG_CLKCTL_DATA	(0xff<<0) /**< AD9516 register data (D7->D0) */
#define CVORG_CLKCTL_ADDR	(0x3ff<<8) /**< AD9156 register addr (D9->D0) */
#define CVORG_CLKCTL_RW		BIT(18) /**< AD9516 read/write (1/0) */
#define CVORG_CLKCTL_SENDSPI	BIT(19) /**< Send SPI frame (1) */
#define CVORG_CLKCTL_UP2DATE	BIT(20) /**< set when data in CLKCTL are
					       updated; cleared on SPI read */
/* bits 21-24 not used */
#define CVORG_CLKCTL_LOCKDETCT	BIT(25) /**< Lock detect (R/only) */
#define CVORG_CLKCTL_REFMON	BIT(26) /**< Reference monitor (R/only) */
#define CVORG_CLKCTL_STATUS	BIT(27) /**< Status (R/only) */
#define CVORG_CLKCTL_POWDOWN	BIT(28) /**< Power down (on 0) */
#define CVORG_CLKCTL_RESET9516	BIT(29) /**< Reset (on 0) */
#define CVORG_CLKCTL_SYNC	BIT(30) /**< Manual synchronisation (on 0) */
#define CVORG_CLKCTL_SELECT	BIT(31) /**< Select local/ext clock (0/1) */
/*
 * AD9516-O default Control Operation
 * Since the user basically wants to read/write from the AD at a certain
 * address, define a set of operations so that the caller just needs
 * to OR these opcodes with the address and desired value (if any).
 * (in case of doubt, check these opcodes against the definitions below)
 */
#define CVORG_AD9516_OP_WRITE	0x70080000
#define CVORG_AD9516_OP_READ	0x700c0000

/*
 * chunk of 4 bytes not used: from 0x0030 to 0x003c
 */

/*
 * Status (R)
 */
#define CVORG_STATUS	0x040
#define CVORG_STATUS_READY	BIT(0) /**< Ready to run (on 0) */
#define CVORG_STATUS_MODE	BIT(1) /**< 1->Normal, 0-> all other modes */
#define CVORG_STATUS_BUSY	BIT(2) /**< waveform is being played */
#define CVORG_STATUS_SRAM_BUSY	BIT(3) /**< SRAM being accessed by the module */
#define CVORG_STATUS_ERR	BIT(4) /**< module in error */
#define CVORG_STATUS_ERR_CLK	BIT(5) /**< No sampling clock/Clock unstable */
#define CVORG_STATUS_ERR_TRIG	BIT(6) /**< Start trigger before channel is ready */
#define CVORG_STATUS_ERR_SYNC	BIT(7) /**< Synchronization FIFO empty before end of waveform */
#define CVORG_STATUS_CLKOK	BIT(8) /**< CLK present and stable */
#define CVORG_STATUS_STARTABLE	BIT(9) /**< '1' when 'start' triggers can be sent */
/* remaining bits not used */

/*
 * Configuration (R/W)
 */
#define CVORG_CONFIG	0x044
#define CVORG_CONFIG_MODE	(0xf<<0)
/* if an undefined mode is selected, the channel is turned off */
#define CVORG_CONFIG_MODE_OFF		0x0 /**< off */
#define CVORG_CONFIG_MODE_NORMAL	0x1 /**< normal mode */
#define CVORG_CONFIG_MODE_DAC		0x3 /**< outputs DAC_VAL register */
#define CVORG_CONFIG_MODE_TEST1		0x8 /**< triangle waveform */
#define CVORG_CONFIG_MODE_TEST2		0x9 /**< saw-tooth waveform */
#define CVORG_CONFIG_MODE_TEST3		0xa /**< square waveform */
#define CVORG_CONFIG_SEQREADY	BIT(4)
#define CVORG_CONFIG_OUT_OFFSET	BIT(5)	/**< 0 - no offset, 1 - 2.5V */
#define CVORG_CONFIG_INPOL	BIT(6)	/**< 0 positive pulses, 1 negative */
#define CVORG_CONFIG_OUT_GAIN_SHIFT	7
#define CVORG_CONFIG_OUT_GAIN_MASK	(0x7<<7)
#define CVORG_CONFIG_OUT_ENABLE	BIT(10)
/* bits 11-23 not used */
#define CVORG_CONFIG_ADCSELECT_SHIFT	24
#define CVORG_CONFIG_ADCSELECT_MASK	(0x7<<24) /**< ADC channel sel: result @ ADCVAL */
#define CVORG_CONFIG_ADCSELECT_MONITOR	0x1
#define CVORG_CONFIG_ADCSELECT_REF	0x2
#define CVORG_CONFIG_ADCSELECT_5V	0x3
#define CVORG_CONFIG_ADCSELECT_GND	0x4
#define CVORG_CONFIG_ADCRANGE_SHIFT	27
#define CVORG_CONFIG_ADCRANGE_MASK	(0x3<<27) /**< ADC input range */
#define CVORG_CONFIG_ADCRANGE_5V_BIPOLAR	0x0
#define CVORG_CONFIG_ADCRANGE_5V_UNIPOLAR	0x2
#define CVORG_CONFIG_ADCRANGE_10V_BIPOLAR	0x1
#define CVORG_CONFIG_ADCRANGE_10V_UNIPOLAR	0x3

/*
 * DAC Value (R/W)
 * Used in CONFIG_MODE_DAC, allows the user to send directly a value (14 bits)
 * to the DAC
 */
#define CVORG_DACVAL	0x048
#define CVORG_DACVAL_MASK	0x3fff /**< 14 bits */

/*
 * SRAM start address (R/W)
 * This register is a pointer to the external SRAM. The pointer auto-increments
 * on read or write of SRAMDATA. External SRAM is 512k x 32 bits.
 */
#define CVORG_SRAMADDR	0x04c /**< Byte addresses must be written into this reg */

/*
 * SRAM data (R/W)
 * Used to read/write functions from/to SRAM
 * Note. SRAM cannot be directly mapped
 */
#define CVORG_SRAMDATA	0x050

/*
 * DAC Control (R/W)
 * SPI interface to AD9707
 */
#define CVORG_DACCTL	0x054
#define CVORG_DACCTL_DATA	(0xff<<0) /**< DAC register data */
#define CVORG_DACCTL_ADDR	(0x1ff<<8) /**< DAC register address */
/* bits 13-14: do not care about them */
#define CVORG_DACCTL_RW		BIT(15) /**< DAC register read/write (1/0) */
#define CVORG_DACCTL_UP2DATE	BIT(16) /**< set when data in DACCTL are
					       updated; cleared on SPI read */
/* bits 17-31: not used */

/*
 * Number of sequences (R/W)
 * Number of times that a waveform is played. 0 -> play infinitely
 */
#define CVORG_SEQNR	0x058

/*
 * DAC Correction (offset/gain)
 */
#define CVORG_DACCORR	0x05c
#define CVORG_DACCORR_GAIN_MASK		0xffff
#define CVORG_DACCORR_OFFSET_SHIFT	16
#define CVORG_DACCORR_OFFSET_MASK	(0x7fff<<16)


/*
 * Maximum number of sequences that can be stored in a channel
 */
#define CVORG_MAX_SEQUENCES	26

/*
 * Waveform Block
 * A waveform is defined in what it's called a block. A block defines the
 * waveform length, start address @ SRAM, number of recurrent cycles,
 * and number of the following block to be played (if any).
 */

/*
 * address of a block:
 * board address + CVORG_MODX + CVORG_BLKOFFSET + CVORG_BLKSIZE * block number
 * , where X={1,2}
 * Note that 0 <= block number <= 25 (last block starts at 0x1F0)
 */
#define CVORG_BLKOFFSET 0x60
#define CVORG_BLKSIZE	0x10

/*
 * Waveform start address (R/W)
 * Address @ SRAM of the first byte of the waveform
 * Note that the address is 21 bits long
 */
#define CVORG_WFSTART	0x00
#define CVORG_WFSTART_MASK	0x001fffff

/*
 * Waveform Length (R/W)
 * Number of (16-bit) points of the waveform
 * Note that the length is 20 bits long
 */
#define CVORG_WFLEN	0x04
#define CVORG_WFLEN_MASK	0x000fffff

/*
 * Waveform recurrence
 * Number of times a waveform has to be played consecutively.
 * Note that '0' means infinitely
 */
#define CVORG_WFRECURR	0x08

/*
 * Next waveform and output gain
 * Number of the next waveform block to be played.
 * Set to CVORG_WFNEXTBLK_LAST to end a sequence
 */
#define CVORG_WFNEXTBLK	0x0c
#define CVORG_WFNEXTBLK_NEXT_MASK	0x1f
#define CVORG_WFNEXTBLK_LAST	31
#define CVORG_WFNEXTBLK_CONF_GAIN	BIT(8)
#define CVORG_WFNEXTBLK_GAIN_SHIFT	9
#define CVORG_WFNEXTBLK_GAIN_MASK	(0x7<<(CVORG_WFNEXTBLK_GAIN_SHIFT))

#define CVORG_OUTGAIN_M22	0x0 /**< -22 / 0.08 */
#define CVORG_OUTGAIN_M16	0x1 /**< -16 / 0.16 */
#define CVORG_OUTGAIN_M10	0x2 /**< -10 / 0.32 */
#define CVORG_OUTGAIN_M4	0x3 /**<  -4 / 0.64 */
#define CVORG_OUTGAIN_2		0x4 /**<   2 / 1.26 */
#define CVORG_OUTGAIN_8		0x5 /**<   8 / 2.52 */
#define CVORG_OUTGAIN_14	0x6 /**< mandatory for unipolar mode */
#define CVORG_OUTGAIN_20	0x7 /**< for bipolar mode */

#endif /* _CVORG_HW_H_ */
