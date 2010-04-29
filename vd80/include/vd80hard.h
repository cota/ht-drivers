/*
 * vd80.h - Register definitions for the VD80 Transient Recorder
 *
 * Copyright (c) 2009 Sebastien Dugue
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _VD80_H
#define _VD80_H

#define VD80_CHANNELS 16

/*
 * Configuration ROM Registers
 *
 * Register offsets are given relative to the CR/CSR base address.
 */
#define VD80_CR_CHKSUM			0x00
#define VD80_CR_ROM_LEN_LEN		3
#define VD80_CR_ROM_LEN			0x04 /* 0x04 - 0x0c */
#define VD80_CR_CR_DW			0x10
#define VD80_CR_CSR_DW			0x14
#define VD80_CR_SPACE_ID		0x18

#define VD80_CR_SIG1			0x1c
#define VD80_CR_SIG2			0x20

#define VD80_CR_MANUF_ID_LEN		3
#define VD80_CR_MANUF_ID		0x24 /* 0x24 - 0x2c */

#define VD80_CR_BOARD_ID_LEN		4
#define VD80_CR_BOARD_ID		0x30 /* 0x30 - 0x3c */

#define VD80_CR_REV_ID_LEN		4
#define VD80_CR_REV_ID			0x40 /* 0x40 - 0x4c */

#define VD80_CR_DESC_PTR_LEN		3
#define VD80_CR_DESC_PTR		0x50 /* 0x50 - 0x58 */

#define VD80_CR_PGM_ID			0x7c

#define VD80_CR_OFFSET_LEN		3
#define VD80_CR_BEG_USER_CR		0x80
#define VD80_CR_END_USER_CR		0x8c
#define VD80_CR_BEG_CRAM		0x98
#define VD80_CR_END_CRAM		0xa4
#define VD80_CR_BEG_USER_CSR		0xb0
#define VD80_CR_END_USER_CSR		0xbc
#define VD80_CR_BEG_SN			0xc8
#define VD80_CR_END_SN			0xd4

#define VD80_CR_SLAVE_PARAM		0xe0
#define VD80_CR_INTERRUPTER_CAP		0xf4
#define VD80_CR_CRAM_DW			0xfc
#define VD80_CR_FUNC0_DW		0x100
#define VD80_CR_FUNC1_DW		0x104
#define VD80_CR_FUNC2_DW		0x108
#define VD80_CR_FUNC3_DW		0x10c

#define VD80_CR_FUNC_AM_MASK_LEN	8
#define VD80_CR_FUNC0_AM_MASK		0x120
#define VD80_CR_FUNC1_AM_MASK		0x140
#define VD80_CR_FUNC2_AM_MASK		0x160
#define VD80_CR_FUNC3_AM_MASK		0x180

#define VD80_CR_FUNC_ADEM_LEN		4
#define VD80_CR_FUNC0_ADEM		0x620
#define VD80_CR_FUNC1_ADEM		0x630
#define VD80_CR_FUNC2_ADEM		0x640
#define VD80_CR_FUNC3_ADEM		0x650

/*
 * User Configuration ROM Registers
 *
 * Register offsets are given relative to the CR/CSR base address.
 */
#define VD80_UCR_BOARD_SERIAL_LEN	4
#define VD80_UCR_BOARD_SERIAL		0x1000

#define VD80_UCR_BOARD_DESC_LEN		0x140
#define VD80_UCR_BOARD_DESC		0x1040

/*
 * Control and Status Registers
 *
 * Register offsets are given relative to the CR/CSR base address.
 */

#define VD80_CSR_MBLT_ENDIAN            0x7fbf0
#define VD80_CSR_MBLT_BIG_ENDIAN        0x00
#define VD80_CSR_MBLT_LITTLE_ENDIAN     0xFF

#define VD80_CSR_IRQ_LEVEL		0x7fbf4
#define VD80_CSR_IRQ_VECTOR		0x7fbf8
#define VD80_CSR_FUNC_ACEN		0x7fbfc

#define VD80_CSR_ADER_LEN		4
#define VD80_CSR_ADER0			0x7ff60
#define VD80_CSR_ADER1			0x7ff70
#define VD80_CSR_ADER2			0x7ff80
#define VD80_CSR_ADER3			0x7ff90

#define VD80_CSR_CRAM_OWNER		0x7fff0

#define VD80_CSR_BITCLR			0x7fff4
#define VD80_CSR_BITSET			0x7fff8
#define VD80_CSR_RESET_MODULE           (1 << 7)
#define VD80_CSR_ENABLE_SYSFAIL		(1 << 6)
#define VD80_CSR_ENABLE_MODULE		(1 << 4)
#define VD80_CSR_BERR_FLAG		(1 << 3)
#define VD80_CSR_CRAM_OWNER_BIT		(1 << 2)

#define VD80_CSR_CRCSR_BASE		0x7fffc

/*
 * Operational Registers.
 *
 * Register offsets are given relative to the operational base address
 * setup at module initialization.
 */

/* General Control Register 1           offset 0x10 */
#define VD80_GCR1                       0x10

#define VD80_INTCLR			(1 << 9)
#define VD80_IOERRCLR			(1 << 8)

#define VD80_OPERANT_SHIFT		4
#define VD80_OPERANT_MASK		(0xf << VD80_OPERANT_SHIFT)

#define VD80_COMMAND_SHIFT		0
#define VD80_COMMAND_MASK		(0xf << VD80_COMMAND_SHIFT)
#define VD80_COMMAND_NONE		(0 << VD80_COMMAND_SHIFT)
#define VD80_COMMAND_SINGLE		(1 << VD80_COMMAND_SHIFT)
#define VD80_COMMAND_TRIG		(2 << VD80_COMMAND_SHIFT)
#define VD80_COMMAND_READ		(4 << VD80_COMMAND_SHIFT)
#define VD80_COMMAND_SUBSTOP		(8 << VD80_COMMAND_SHIFT)
#define VD80_COMMAND_SUBSTART		(9 << VD80_COMMAND_SHIFT)
#define VD80_COMMAND_START		(0xb << VD80_COMMAND_SHIFT)
#define VD80_COMMAND_CLEAR		(0xc << VD80_COMMAND_SHIFT)
#define VD80_COMMAND_STOP		(0xe << VD80_COMMAND_SHIFT)

/* General Control Register 1           offset 0x14 */
#define VD80_GCR2                       0x14

#define VD80_INTERRUPT_MASK             0x1f

#define VD80_BYTESWAP                   (1 << 7)
#define VD80_DBCNTSEL			(1 << 6)
#define VD80_BIGEND			(1 << 5)
#define VD80_CALINTEN			(1 << 4)
#define VD80_IOERRINTEN			(1 << 3)
#define VD80_SMPLINTEN			(1 << 2)
#define VD80_SHOTINTEN			(1 << 1)
#define VD80_TRIGINTEN			(1 << 0)

/* General Status Register              offset 0x18 */
#define VD80_GSR                        0x18

#define VD80_GSR_INTERRUPT_SHIFT        3

#define VD80_MEMOUTRDY			(1 << 10)
#define VD80_IOERRTRIG			(1 << 9)
#define VD80_IOERRCLK			(1 << 8)
#define VD80_CALINTEVENT		(1 << 7)
#define VD80_IOERRINTEVENT		(1 << 6)
#define VD80_SMPLINTEVENT		(1 << 5)
#define VD80_SHOTINTEVENT		(1 << 4)
#define VD80_TRIGINTEVENT		(1 << 3)

#define VD80_STATE_SHIFT		0
#define VD80_STATE_MASK			(7 << VD80_STATE_SHIFT)
#define VD80_STATE_IDLE			(1 << VD80_STATE_SHIFT)
#define VD80_STATE_PRETRIG		(2 << VD80_STATE_SHIFT)
#define VD80_STATE_POSTTRIG		(4 << VD80_STATE_SHIFT)

/* Clock Control Register               offset 0x1c */
#define VD80_CCR                        0x1c

#define VD80_CLKDIV_SHIFT		16
#define VD80_CLKDIV_MASK       		(0xffff << VD80_CLKDIV_SHIFT)

#define VD80_CLKTERM_SHIFT              9
#define VD80_CLKTERM_MASK               (1 << VD80_CLKTERM_SHIFT)
#define VD80_CLKTERM_50OHM              (1 << VD80_CLKTERM_SHIFT)
#define VD80_CLKTERM_NONE               (0 << VD80_CLKTERM_SHIFT)

#define VD80_CLKEDGE_SHIFT              8
#define VD80_CLKEDGE_MASK               (1 << VD80_CLKEDGE_SHIFT)
#define VD80_CLKEDGE_RISING             (0 << VD80_CLKEDGE_SHIFT)
#define VD80_CLKEDGE_FALLING            (1 << VD80_CLKEDGE_SHIFT)

#define VD80_CLKDIVMODE_SHIFT           7
#define VD80_CLKDIVMODE_MASK            (1 << VD80_CLKDIVMODE_SHIFT)
#define VD80_CLKDIVMODE_DIVIDE          (0 << VD80_CLKDIVMODE_SHIFT)
#define VD80_CLKDIVMODE_SUBSAMPLE       (1 << VD80_CLKDIVMODE_SHIFT)

#define VD80_CLKVXIOUT_SHIFT		4
#define VD80_CLKVXIOUT_MASK		(7 << VD80_CLKVXIOUT_SHIFT)
#define VD80_CLKVXIOUT_VXI_NONE		(0 << VD80_CLKVXIOUT_SHIFT)
#define VD80_CLKVXIOUT_VXI_TRIG0	(1 << VD80_CLKVXIOUT_SHIFT)
#define VD80_CLKVXIOUT_VXI_TRIG1	(2 << VD80_CLKVXIOUT_SHIFT)
#define VD80_CLKVXIOUT_VXI_TRIG2	(3 << VD80_CLKVXIOUT_SHIFT)
#define VD80_CLKVXIOUT_VXI_TRIG3	(4 << VD80_CLKVXIOUT_SHIFT)
#define VD80_CLKVXIOUT_VXI_TRIG4	(5 << VD80_CLKVXIOUT_SHIFT)
#define VD80_CLKVXIOUT_VXI_TRIG5	(6 << VD80_CLKVXIOUT_SHIFT)
#define VD80_CLKVXIOUT_VXI_TRIG6	(7 << VD80_CLKVXIOUT_SHIFT)

#define VD80_CLKSOURCE_SHIFT		0
#define VD80_CLKSOURCE_MASK		(0xf << VD80_CLKSOURCE_SHIFT)
#define VD80_CLKSOURCE_INT		(0 << VD80_CLKSOURCE_SHIFT)
#define VD80_CLKSOURCE_VXI_TRIG0	(1 << VD80_CLKSOURCE_SHIFT)
#define VD80_CLKSOURCE_VXI_TRIG1	(2 << VD80_CLKSOURCE_SHIFT)
#define VD80_CLKSOURCE_VXI_TRIG2	(3 << VD80_CLKSOURCE_SHIFT)
#define VD80_CLKSOURCE_VXI_TRIG3	(4 << VD80_CLKSOURCE_SHIFT)
#define VD80_CLKSOURCE_VXI_TRIG4	(5 << VD80_CLKSOURCE_SHIFT)
#define VD80_CLKSOURCE_VXI_TRIG5	(6 << VD80_CLKSOURCE_SHIFT)
#define VD80_CLKSOURCE_VXI_TRIG6	(7 << VD80_CLKSOURCE_SHIFT)
#define VD80_CLKSOURCE_EXT		(8 << VD80_CLKSOURCE_SHIFT)

/* Trigger Control Register 1           offset 0x20 */
#define VD80_TCR1                       0x20

#define VD80_TRIGSYNC			(1 << 10)

#define VD80_TRIG_TERM_SHIFT            9
#define VD80_TRIG_TERM_MASK             (1 << VD80_TRIG_TERM_SHIFT)
#define VD80_TRIG_TERM_50OHM            (1 << 9)
#define VD80_TRIG_TERM_NONE             (0 << 9)

#define VD80_TRIGEDGE_SHIFT             8
#define VD80_TRIGEDGE_MASK              (1 << VD80_TRIGEDGE_SHIFT)
#define VD80_TRIGEDGE_RISING            (0 << VD80_TRIGEDGE_SHIFT)
#define VD80_TRIGEDGE_FALLING           (1 << VD80_TRIGEDGE_SHIFT)

#define VD80_TRIGEXTOUT			(1 << 7)

#define VD80_TRIGVXIOUT_SHIFT		4
#define VD80_TRIGVXIOUT_MASK		(7 << VD80_TRIGVXIOUT_SHIFT)
#define VD80_TRIGVXIOUT_VXI_NONE	(0 << VD80_TRIGVXIOUT_SHIFT)
#define VD80_TRIGVXIOUT_VXI_TRIG0	(1 << VD80_TRIGVXIOUT_SHIFT)
#define VD80_TRIGVXIOUT_VXI_TRIG1	(2 << VD80_TRIGVXIOUT_SHIFT)
#define VD80_TRIGVXIOUT_VXI_TRIG2	(3 << VD80_TRIGVXIOUT_SHIFT)
#define VD80_TRIGVXIOUT_VXI_TRIG3	(4 << VD80_TRIGVXIOUT_SHIFT)
#define VD80_TRIGVXIOUT_VXI_TRIG4	(5 << VD80_TRIGVXIOUT_SHIFT)
#define VD80_TRIGVXIOUT_VXI_TRIG5	(6 << VD80_TRIGVXIOUT_SHIFT)
#define VD80_TRIGVXIOUT_VXI_TRIG6	(7 << VD80_TRIGVXIOUT_SHIFT)

#define VD80_TRIGSOURCE_SHIFT		0
#define VD80_TRIGSOURCE_MASK		(0xf << VD80_TRIGSOURCE_SHIFT)
#define VD80_TRIGSOURCE_INT		(0 << VD80_TRIGSOURCE_SHIFT)
#define VD80_TRIGSOURCE_VXI_TRIG0	(1 << VD80_TRIGSOURCE_SHIFT)
#define VD80_TRIGSOURCE_VXI_TRIG1	(2 << VD80_TRIGSOURCE_SHIFT)
#define VD80_TRIGSOURCE_VXI_TRIG2	(3 << VD80_TRIGSOURCE_SHIFT)
#define VD80_TRIGSOURCE_VXI_TRIG3	(4 << VD80_TRIGSOURCE_SHIFT)
#define VD80_TRIGSOURCE_VXI_TRIG4	(5 << VD80_TRIGSOURCE_SHIFT)
#define VD80_TRIGSOURCE_VXI_TRIG5	(6 << VD80_TRIGSOURCE_SHIFT)
#define VD80_TRIGSOURCE_VXI_TRIG6	(7 << VD80_TRIGSOURCE_SHIFT)
#define VD80_TRIGSOURCE_EXT		(8 << VD80_TRIGSOURCE_SHIFT)
#define VD80_TRIGSOURCE_ANALOG		(9 << VD80_TRIGSOURCE_SHIFT)

#define VD80_TRIGOUT_DELAY              (1 << 11)

/* Trigger Control Register 2           offset 0x24 */
#define VD80_TCR2                       0x24
#define VD80_POSTSAMPLES_SHIFT		0
#define VD80_POSTSAMPLES_MASK		(0x7ffff << VD80_POSTSAMPLES_SHIFT)

/* Trigger Control Register 3           offset 0x64 (New Trigger Delay feature) */
#define VD80_TCR3                       0x64

/* Trigger Status Register              offset 0x28 */
#define VD80_TSR                        0x28
#define VD80_ACTPOSTSAMPLES_SHIFT	0
#define VD80_ACTPOSTSAMPLES_MASK	(0x1ffffff << VD80_ACTPOSTSAMPLES_SHIFT)

/* Shot Status Register                 offset 0x2c */
#define VD80_SSR                        0x2c
#define VD80_SHOTLEN_SHIFT		0
#define VD80_SHOTLEN_MASK		(0x1ffffff << VD80_SHOTLEN_SHIFT)

/* ADC registers - channel 1 to 16      offset 0x30-0x4c */
#define VD80_ADCR1                      0x30 /* channels 2-1 */
#define VD80_ADCR2                      0x34 /* channels 4-3 */
#define VD80_ADCR3                      0x38 /* channels 6-5 */
#define VD80_ADCR4                      0x3c /* channels 8-7 */
#define VD80_ADCR5                      0x40 /* channels 10-9 */
#define VD80_ADCR6                      0x44 /* channels 12-11 */
#define VD80_ADCR7                      0x48 /* channels 14-13 */
#define VD80_ADCR8                      0x4c /* channels 16-15 */

/* Memory Control Register 1            offset 0x50 */
#define VD80_MCR1                       0x50
#define VD80_READSTART_SHIFT		0
#define VD80_READSTART_MASK		(0x7ffff << VD80_READSTART_SHIFT)

/* Memory Control Register 2            offset 0x54 */
#define VD80_MCR2                       0x54
#define VD80_READLEN_SHIFT		0
#define VD80_READLEN_MASK		(0x7ffff << VD80_READSTART_SHIFT)

/* Pre Trigger Status Register          offset 0x58 */
#define VD80_PTSR                       0x58
#define VD80_PRESAMPLES_SHIFT		0
#define VD80_PRESAMPLES_MASK		(0x1ffffff << VD80_PRESAMPLES_SHIFT)

/* Pre Trigger Control Register         offset 0x5c */
#define VD80_PTCR                       0x5c
#define VD80_PSCNTCLKONSMPL		(1 << 2)
#define VD80_PSREGCLRONREAD		(1 << 1)
#define VD80_PSCNTCLRONSTART		(1 << 0)

#define VD80_PRESAMPLESMIN_SHIFT        3
#define VD80_PRESAMPLESMIN_MASK         (0x1fffffff << VD80_PRESAMPLESMIN_SHIFT)

/* Memory Read/Write Register           offset 0x00-0x0c */
/* Locations 0x04-0x0c are mirrors of 0x00? TBC. */
#define VD80_MRWR                       0x00
/* More significant alias for reading */
#define VD80_MEMOUT                     0x00

/* Definitions for writing */
#define VD80_MEMSEG_SHIFT		16
#define VD80_MEMSEG_MASK		(0xf << VD80_MRWR_MEMSEG_SHIFT)

#define VD80_MEMIN_SHIFT		0
#define VD80_MEMIN_MASK			(0xffff << VD80_MRWR_MEMIN_SHIFT)

/* Calibration Register			offset 0x60-0x6c */
/* Locations 0x64-0x6c are mirrors of 0x60? TBC. */
#define VD80_CALR			0x60

#define VD80_CALKEY_SHIFT		24
#define VD80_CALKEY_MASK		(0xf << VD80_CALKEY_SHIFT)

#define VD80_CALMSHD			(1 << 23)

#define VD80_CALFUNC_SHIFT		21
#define VD80_CALFUNC_MASK		(3 << VD80_CALFUNC_SHIFT)

#define VD80_CALRDY			(1 << 20)

#define VD80_CALADDR_ROM_SHIFT		17
#define VD80_CALADDR_ROM_MASK		(3 << VD80_CALADDR_ROM_SHIFT)

#define VD80_CALADDR_CHAN_SHIFT		13
#define VD80_CALADDR_CHAN_MASK		(0xf << VD80_CALADDR_CHAN_SHIFT)

#define VD80_CALADDR_LSB_SHIFT		12
#define VD80_CALADDR_OFFSET		(0 << VD80_CALADDR_LSB_SHIFT)
#define VD80_CALADDR_GAIN		(1 << VD80_CALADDR_LSB_SHIFT)

#define VD80_CALDATA_SHIFT		0
#define VD80_CALDATA_MASK		(0xfff << VD80_CALDATA_SHIFT)

/* Sub Sample Control Register		0x70-0x7c */
/* Locations 0x74-0x7c are mirrors of 0x70. */
#define VD80_SUBSAMPCR			0x70

#define VD80_SUBDIV_SHIFT		16
#define VD80_SUBDIV_MASK		(0xffff << VD80_SUBDIV_SHIFT)

#define VD80_TRIGEN			(1 << 4)

#define VD80_SUBMODE_SHIFT		3
#define VD80_SUBMODE_FIFO		(0 << VD80_SUBMODE_SHIFT)
#define VD80_SUBMODE_RING		(1 << VD80_SUBMODE_SHIFT)

#define VD80_SUBSIZE_SHIFT		0
#define VD80_SUBSIZE_MASK		(7 << VD80_SUBSIZE_SHIFT)
#define VD80_SUBSIZE_1			(0 << VD80_SUBSIZE_SHIFT)
#define VD80_SUBSIZE_2			(1 << VD80_SUBSIZE_SHIFT)
#define VD80_SUBSIZE_4			(2 << VD80_SUBSIZE_SHIFT)
#define VD80_SUBSIZE_8			(3 << VD80_SUBSIZE_SHIFT)
#define VD80_SUBSIZE_16			(4 << VD80_SUBSIZE_SHIFT)
#define VD80_SUBSIZE_32			(5 << VD80_SUBSIZE_SHIFT)
#define VD80_SUBSIZE_64			(6 << VD80_SUBSIZE_SHIFT)

/* Sub Sample buffers, channel 1 to 16	0x80-0xbc */
#define VD80_SUBSAMP_CHAN1		0x80
#define VD80_SUBSAMP_CHAN2		0x84
#define VD80_SUBSAMP_CHAN3		0x88
#define VD80_SUBSAMP_CHAN4		0x8c
#define VD80_SUBSAMP_CHAN5		0x90
#define VD80_SUBSAMP_CHAN6		0x94
#define VD80_SUBSAMP_CHAN7		0x98
#define VD80_SUBSAMP_CHAN8		0x9c
#define VD80_SUBSAMP_CHAN9		0xa0
#define VD80_SUBSAMP_CHAN10		0xa4
#define VD80_SUBSAMP_CHAN11		0xa8
#define VD80_SUBSAMP_CHAN12		0xac
#define VD80_SUBSAMP_CHAN13		0xb0
#define VD80_SUBSAMP_CHAN14		0xb4
#define VD80_SUBSAMP_CHAN15		0xb8
#define VD80_SUBSAMP_CHAN16		0xbc

#define VD80_SUBSAMP_FULL		(1 << 17)
#define VD80_SUBSAMP_EMPTY		(1 << 16)
#define VD80_SUBSAMP_SAMP_SHIFT		0
#define VD80_SUBSAMP_SAMP_MASK		(0xffff << VD80_SUBSAMP_SAMP_SHIFT)

/* Analog Trigger Levels, channel 1 to 16	0xc0-0xfc */
#define VD80_ATRIG_CHAN1		0xc0
#define VD80_ATRIG_CHAN2		0xc4
#define VD80_ATRIG_CHAN3		0xc8
#define VD80_ATRIG_CHAN4		0xcc
#define VD80_ATRIG_CHAN5		0xc0
#define VD80_ATRIG_CHAN6		0xc4
#define VD80_ATRIG_CHAN7		0xc8
#define VD80_ATRIG_CHAN8		0xcc
#define VD80_ATRIG_CHAN9		0xc0
#define VD80_ATRIG_CHAN10		0xc4
#define VD80_ATRIG_CHAN11		0xc8
#define VD80_ATRIG_CHAN12		0xcc
#define VD80_ATRIG_CHAN13		0xc0
#define VD80_ATRIG_CHAN14		0xc4
#define VD80_ATRIG_CHAN15		0xc8
#define VD80_ATRIG_CHAN16		0xcc


#define VD80_ATRIG_LEVEL_ABOVE_SHIFT	17
#define VD80_ATRIG_LEVEL_ABOVE_MASK	(0xfff << VD80_ATRIG_LEVEL_ABOVE_SHIFT)

#define VD80_ATRIG_LEVEL_BELOW_SHIFT	5
#define VD80_ATRIG_LEVEL_BELOW_MASK	(0xfff << VD80_ATRIG_LEVEL_BELOW_SHIFT)

#define VD80_ATRIG_LOGIC_SHIFT		4
#define VD80_ATRIG_LOGIC_OR		(0 << VD80_ATRIG_LOGIC_SHIFT)
#define VD80_ATRIG_LOGIC_AND		(1 << VD80_ATRIG_LOGIC_SHIFT)

#define VD80_ATRIG_CHANGE_SHIFT         3
#define VD80_ATRIG_CHANGE_LEVEL		(0 << VD80_ATRIG_CHANGE_SHIFT)
#define VD80_ATRIG_CHANGE_EDGE		(1 << VD80_ATRIG_CHANGE_SHIFT)

#define VD80_ATRIG_WIN_SHIFT		0
#define VD80_ATRIG_WIN_MASK		(7 << VD80_ATRIG_WIN_SHIFT)
#define VD80_ATRIG_WIN_DISABLED		(0 << VD80_ATRIG_WIN_SHIFT)
#define VD80_ATRIG_WIN_BELOW_AND_ABOVE	(1 << VD80_ATRIG_WIN_SHIFT)
#define VD80_ATRIG_WIN_BELOW_OR_ABOVE	(2 << VD80_ATRIG_WIN_SHIFT)
#define VD80_ATRIG_WIN_BELOW		(3 << VD80_ATRIG_WIN_SHIFT)
#define VD80_ATRIG_WIN_ABOVE		(4 << VD80_ATRIG_WIN_SHIFT)


#endif /* _VD80_H */
