/*
 * tsi148.h - Low level support for the Tundra TSI148 PCI-VME Bridge Chip
 *
 * Copyright (c) 2009 Sebastien Dugue
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * Borrowed and modified from the tsi148.h file from:
 *   Tom Armistead, Updated and maintained by Ajit Prem and
 *   copyrighted 2004 Motorola Inc.
 *
 */

#ifndef _TSI148_H
#define _TSI148_H

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#endif /* CONFIG_PROC_FS */

#include <asm-generic/iomap.h>

#include "vme_dma.h"
#include "vmebus.h"

/*
 *  Define the number of each resources that the Tsi148 supports.
 */
#define TSI148_NUM_OUT_WINDOWS        8	/* Number of outbound windows */
#define TSI148_NUM_IN_WINDOWS         8	/* Nuber of inbound windows */
#define TSI148_NUM_DMA_CHANNELS       2	/* Number of DMA channels */
#define TSI148_NUM_MAILBOXES          4	/* Number of mailboxes */
#define TSI148_NUM_SEMAPHORES         8	/* Number of semaphores */

#define PCI_VENDOR_ID_TUNDRA		0x10e3
#define PCI_DEVICE_ID_TUNDRA_TSI148	0x0148

/*
 * VME constants for the TSI148
 */
/* Address mode */
enum tsi148_address_size {
	TSI148_A16		= 0,
	TSI148_A24		= 1,
	TSI148_A32		= 2,
	TSI148_A64		= 4,
	TSI148_CRCSR	= 5,
	TSI148_USER1	= 8,
	TSI148_USER2	= 9,
	TSI148_USER3	= 10,
	TSI148_USER4	= 11
};

/* VME access type definitions */
enum tsi148_user_access_type {
	TSI148_USER	= 0,
	TSI148_SUPER
};

enum tsi148_data_access_type {
	TSI148_DATA	= 0,
	TSI148_PROG
};

/* VME transfer mode */
enum tsi148_transfer_mode {
	TSI148_SCT = 0,
	TSI148_BLT,
	TSI148_MBLT,
	TSI148_2eVME,
	TSI148_2eSST,
	TSI148_2eSSTB
};

enum tsi148_data_width {
	TSI148_DW_16 = 0,
	TSI148_DW_32
};



/*
 *  TSI148 CRG registers structure.
 *
 *  The TSI148 Combined Register Group (CRG) consists of the following
 *  combination of registers:
 *
 *  0x000 PCFS    - PCI Configuration Space Registers
 *  0x100 LCSR    - Local Control and Status Registers
 *  0x600 GCSR    - Global Control and Status Registers
 *  0xff4 CR/CSR  - Subset of Configuration ROM / Control and Status Registers
 */


/*
 * Misc structures used in the LCSR registers description
 */

/* Outbound translation registers in LCSR group */
struct tsi148_otrans {
	unsigned int otsau;		/* Starting address */
	unsigned int otsal;
	unsigned int oteau;		/* Ending address */
	unsigned int oteal;
	unsigned int otofu;		/* Translation offset */
	unsigned int otofl;
	unsigned int otbs;		/* 2eSST broadcast select */
	unsigned int otat;		/* Attributes */
};

/* Inbound translation registers in LCSR group */
struct tsi148_itrans {
	unsigned int itsau;		/* Starting address */
	unsigned int itsal;
	unsigned int iteau;		/* Ending address */
	unsigned int iteal;
	unsigned int itofu;		/* Translation offset */
	unsigned int itofl;
	unsigned int itat;		/* Attributes */
	unsigned char reserved[4];
};

/* DMAC linked-list descriptor */
struct tsi148_dma_desc {
	unsigned int dsau;		/* Source Address */
	unsigned int dsal;
	unsigned int ddau;		/* Destination Address */
	unsigned int ddal;
	unsigned int dsat;		/* Source attributes */
	unsigned int ddat;		/* Destination attributes */
	unsigned int dnlau;		/* Next link address */
	unsigned int dnlal;
	unsigned int dcnt;		/* Byte count */
	unsigned int ddbs;		/* 2eSST Broadcast select */
};

/* DMAC registers in LCSR group */
struct tsi148_dma {
	unsigned int dctl;		/* Control */
	unsigned int dsta;		/* Status */
	unsigned int dcsau;		/* Current Source address */
	unsigned int dcsal;
	unsigned int dcdau;		/* Current Destination address */
	unsigned int dcdal;
	unsigned int dclau;		/* Current Link address */
	unsigned int dclal;
	struct tsi148_dma_desc dma_desc;
	unsigned char reserved[0x38];
};

/*
 *
 *  PCFS register group - PCI configuration space
 *
 */
struct tsi148_pcfs {
	unsigned short veni;
	unsigned short devi;
	unsigned short cmmd;
	unsigned short stat;
	unsigned int rev_class;
	unsigned char clsz;
	unsigned char mlat;
	unsigned char head;
	unsigned char reserved0[1];
	unsigned int mbarl;
	unsigned int mbaru;
	unsigned char reserved1[20];
	unsigned short subv;
	unsigned short subi;
	unsigned char reserved2[4];
	unsigned char capp;
	unsigned char reserved3[7];
	unsigned char intl;
	unsigned char intp;
	unsigned char mngn;
	unsigned char mxla;

	/*
	 * offset 0x40 - PCI-X Capp PCIXCD/NCAPP/PCIXCAP
	 * offset 0x44 - PCI-X Capp PCIXSTAT
	 */
	unsigned char pcix_cap_id;
	unsigned char pcix_next_cap_ptr;
	unsigned short pcix_command;
	unsigned int pcix_status;

	/*
	 * offset 0x48 to 0xff - Reserved
	 */
	unsigned char reserved4[0xb8];
};

/*
 *
 * LCSR registers group definition
 *
 * offsets are given relative to the Combined Register Group (CRG)
 *
 */
struct tsi148_lcsr {

	/* Outbound translations			offset 0x100	*/
	struct tsi148_otrans otrans[TSI148_NUM_OUT_WINDOWS];

	/*
	 * VME bus IACK registers			offset 0x200
	 *
	 * Note: Even though the registers are defined as 32-bits in
	 *       the spec, we only want to issue 8-bit IACK cycles on the bus.
	 */
	unsigned char viack[8 * 4];

	/* RMW registers				offset 0x220	*/
	unsigned int rmwau;
	unsigned int rmwal;
	unsigned int rmwen;
	unsigned int rmwc;
	unsigned int rmws;

	/* VMEbus control				offset 0x234	*/
	unsigned int vmctrl;
	unsigned int vctrl;
	unsigned int vstat;

	/* PCI status					offset 0x240	*/
	unsigned int pstat;
	unsigned char pstatrsvd[0xc];

	/* VME filter					offset 0x250	*/
	unsigned int vmefl;
	unsigned char vmeflrsvd[0xc];

	/* VME exception				offset 0x260	*/
	unsigned int veau;
	unsigned int veal;
	unsigned int veat;
	unsigned char vearsvd[0x4];

	/* PCI error					offset 0x270	*/
	unsigned int edpau;
	unsigned int edpal;
	unsigned int edpxa;
	unsigned int edpxs;
	unsigned int edpat;
	unsigned char edparsvd[0x7c];

	/* Inbound Translations				offset 0x300	*/
	struct tsi148_itrans itrans[TSI148_NUM_IN_WINDOWS];

	/* Inbound Translation for GCSR			offset 0x400 	*/
	unsigned int gbau;
	unsigned int gbal;
	unsigned int gcsrat;

	/* Inbound Translation for CRG			offset 0x40C 	*/
	unsigned int cbau;
	unsigned int cbal;
	unsigned int crgat;

	/* Inbound Translation for CR/CSR		offset 0x418	*/
	unsigned int crou;
	unsigned int crol;
	unsigned int crat;

	/* Inbound Translation for Location Monitor	offset 0x424 	*/
	unsigned int lmbau;
	unsigned int lmbal;
	unsigned int lmat;

	/* VME bus Interrupt Control			offset 0x430	*/
	unsigned int bcu;
	unsigned int bcl;
	unsigned int bpgtr;
	unsigned int bpctr;
	unsigned int vicr;

	unsigned char vicrsvd[0x4];

	/* Local Bus Interrupt Control			offset 0x448	*/
	unsigned int inten;
	unsigned int inteo;
	unsigned int ints;
	unsigned int intc;
	unsigned int intm1;
	unsigned int intm2;

	unsigned char reserved[0xa0];

	/* DMA Controllers				offset 500	*/
	struct tsi148_dma dma[TSI148_NUM_DMA_CHANNELS];
};

/*
 *
 * GCSR registers group definition
 *
 * offsets are given relative to the Combined Register Group (CRG)
 *
 */
struct tsi148_gcsr {
	/* Header					offset 0x600	*/
	unsigned short devi;
	unsigned short veni;

	/* Control					offset 0x604	*/
	unsigned short ctrl;
	unsigned char ga;
	unsigned char revid;

	/* Semaphores					offset 0x608
	 *
	 *  semaphores 3/2/1/0 are at offset 0x608
	 *             7/6/5/4 are at offset 0x60C
	 */
	unsigned char semaphore[TSI148_NUM_SEMAPHORES];

	/* Mail Boxes					offset 0x610	*/
	unsigned int mbox[TSI148_NUM_MAILBOXES];
};


/*
 *
 * CR/CSR registers group definition
 *
 * offsets are given relative to the Combined Register Group (CRG)
 *
 */
struct tsi148_crcsr {
	unsigned int csrbcr;	/* offset 0xff4		*/
	unsigned int csrbsr;	/* offset 0xff8		*/
	unsigned int cbar;	/* offset 0xffc		*/
};


/*
 *
 * TSI148 CRG definition
 *
 */
struct tsi148_chip {
	struct tsi148_pcfs pcfs;
	struct tsi148_lcsr lcsr;
	struct tsi148_gcsr gcsr;
	unsigned char reserved[0xFF4 - 0x61C - 4];
	struct tsi148_crcsr crcsr;
};


/*
 * TSI148 Register Bit Definitions
 */

/*
 *  PFCS Register Set
 */

/* Command/Status Registers (CRG + $004) */
#define TSI148_PCFS_CMMD_SERR          (1<<8)   /* SERR_L out pin ssys err */
#define TSI148_PCFS_CMMD_PERR          (1<<6)   /* PERR_L out pin  parity */
#define TSI148_PCFS_CMMD_MSTR          (1<<2)   /* PCI bus master */
#define TSI148_PCFS_CMMD_MEMSP         (1<<1)   /* PCI mem space access  */
#define TSI148_PCFS_CMMD_IOSP          (1<<0)   /* PCI I/O space enable */

#define TSI148_PCFS_STAT_DPE           (1<<15)  /* Detected Parity Error */
#define TSI148_PCFS_STAT_SIGSE         (1<<14)  /* Signalled System Error */
#define TSI148_PCFS_STAT_RCVMA         (1<<13)  /* Received Master Abort */
#define TSI148_PCFS_STAT_RCVTA         (1<<12)  /* Received Target Abort */
#define TSI148_PCFS_STAT_SIGTA         (1<<11)  /* Signalled Target Abort */
#define TSI148_PCFS_STAT_SELTIM        (3<<9)   /* DELSEL Timing */
#define TSI148_PCFS_STAT_DPED          (1<<8)   /* Data Parity Err Reported */
#define TSI148_PCFS_STAT_FAST          (1<<7)   /* Fast back-to-back Cap */
#define TSI148_PCFS_STAT_P66M          (1<<5)   /* 66 MHz Capable */
#define TSI148_PCFS_STAT_CAPL          (1<<4)   /* Capab List - address $34 */

/* Revision ID/Class Code Registers   (CRG +$008) */
#define TSI148_PCFS_BCLAS_M            (0xFF<<24)   /* Class ID */
#define TSI148_PCFS_SCLAS_M            (0xFF<<16)   /* Sub-Class ID */
#define TSI148_PCFS_PIC_M              (0xFF<<8)    /* Sub-Class ID */
#define TSI148_PCFS_REVID_M            (0xFF<<0)    /* Rev ID */

/* Cache Line Size/ Master Latency Timer/ Header Type Registers (CRG + $00C) */
#define TSI148_PCFS_HEAD_M             (0xFF<<16)   /* Master Lat Timer */
#define TSI148_PCFS_MLAT_M             (0xFF<<8)    /* Master Lat Timer */
#define TSI148_PCFS_CLSZ_M             (0xFF<<0)    /* Cache Line Size */

/* Memory Base Address Lower Reg (CRG + $010) */
#define TSI148_PCFS_MBARL_BASEL_M      (0xFFFFF<<12)  /* Base Addr Lower Mask */
#define TSI148_PCFS_MBARL_PRE          (1<<3)   /* Prefetch */
#define TSI148_PCFS_MBARL_MTYPE_M      (3<<1)   /* Memory Type Mask */
#define TSI148_PCFS_MBARL_IOMEM        (1<<0)   /* I/O Space Indicator */

/* PCI-X Capabilities Register (CRG + $040) */
#define TSI148_PCFS_PCIXCAP_MOST_M     (7<<4)   /* Max outstanding Split Tran */
#define TSI148_PCFS_PCIXCAP_MMRBC_M    (3<<2)   /* Max Mem Read byte cnt */
#define TSI148_PCFS_PCIXCAP_ERO        (1<<1)   /* Enable Relaxed Ordering */
#define TSI148_PCFS_PCIXCAP_DPERE      (1<<0)   /* Data Parity Recover Enable */

/* PCI-X Status Register (CRG +$054) */
#define TSI148_PCFS_PCIXSTAT_RSCEM     (1<<29)  /* Recieved Split Comp Error */
#define TSI148_PCFS_PCIXSTAT_DMCRS_M   (7<<26)  /* max Cumulative Read Size */
#define TSI148_PCFS_PCIXSTAT_DMOST_M   (7<<23)  /* max outstanding Split Tr */
#define TSI148_PCFS_PCIXSTAT_DMMRC_M   (3<<21)  /* max mem read byte count */
#define TSI148_PCFS_PCIXSTAT_DC        (1<<20)  /* Device Complexity */
#define TSI148_PCFS_PCIXSTAT_USC       (1<<19)  /* Unexpected Split comp */
#define TSI148_PCFS_PCIXSTAT_SCD       (1<<18)  /* Split completion discard */
#define TSI148_PCFS_PCIXSTAT_133C      (1<<17)  /* 133MHz capable */
#define TSI148_PCFS_PCIXSTAT_64D       (1<<16)  /* 64 bit device */
#define TSI148_PCFS_PCIXSTAT_BN_M      (0xFF<<8)    /* Bus number */
#define TSI148_PCFS_PCIXSTAT_DN_M      (0x1F<<3)    /* Device number */
#define TSI148_PCFS_PCIXSTAT_FN_M      (7<<0)   /* Function Number */

/*
 *  LCSR Registers
 */

/* Outbound Translation Starting Address Lower */
#define TSI148_LCSR_OTSAL_M            (0xFFFF<<16) /* Mask */

/* Outbound Translation Ending Address Lower */
#define TSI148_LCSR_OTEAL_M            (0xFFFF<<16) /* Mask */

/* Outbound Translation Offset Lower */
#define TSI148_LCSR_OTOFFL_M           (0xFFFF<<16) /* Mask */

/* Outbound Translation 2eSST Broadcast Select */
#define TSI148_LCSR_OTBS_M             (0xFFFFF<<0) /* Mask */

/* Outbound Translation Attribute */
#define TSI148_LCSR_OTAT_EN            (1<<31)  /* Window Enable */
#define TSI148_LCSR_OTAT_MRPFD         (1<<18)  /* Prefetch Disable */

#define TSI148_LCSR_OTAT_PFS_SHIFT     16
#define TSI148_LCSR_OTAT_PFS_M         (3<<16)  /* Prefetch Size Mask */
#define TSI148_LCSR_OTAT_PFS_2         (0<<16)  /* 2 Cache Lines P Size */
#define TSI148_LCSR_OTAT_PFS_4         (1<<16)  /* 4 Cache Lines P Size */
#define TSI148_LCSR_OTAT_PFS_8         (2<<16)  /* 8 Cache Lines P Size */
#define TSI148_LCSR_OTAT_PFS_16        (3<<16)  /* 16 Cache Lines P Size */

#define TSI148_LCSR_OTAT_2eSSTM_SHIFT  11
#define TSI148_LCSR_OTAT_2eSSTM_M      (7<<11)  /* 2eSST Xfer Rate Mask */
#define TSI148_LCSR_OTAT_2eSSTM_160    (0<<11)  /* 160MB/s 2eSST Xfer Rate */
#define TSI148_LCSR_OTAT_2eSSTM_267    (1<<11)  /* 267MB/s 2eSST Xfer Rate */
#define TSI148_LCSR_OTAT_2eSSTM_320    (2<<11)  /* 320MB/s 2eSST Xfer Rate */

#define TSI148_LCSR_OTAT_TM_SHIFT      8
#define TSI148_LCSR_OTAT_TM_M          (7<<8)   /* Xfer Protocol Mask */
#define TSI148_LCSR_OTAT_TM_SCT        (0<<8)   /* SCT Xfer Protocol */
#define TSI148_LCSR_OTAT_TM_BLT        (1<<8)   /* BLT Xfer Protocol */
#define TSI148_LCSR_OTAT_TM_MBLT       (2<<8)   /* MBLT Xfer Protocol */
#define TSI148_LCSR_OTAT_TM_2eVME      (3<<8)   /* 2eVME Xfer Protocol */
#define TSI148_LCSR_OTAT_TM_2eSST      (4<<8)   /* 2eSST Xfer Protocol */
#define TSI148_LCSR_OTAT_TM_2eSSTB     (5<<8)   /* 2eSST Bcast Xfer Protocol */

#define TSI148_LCSR_OTAT_DBW_SHIFT     6
#define TSI148_LCSR_OTAT_DBW_M         (3<<6)   /* Max Data Width */
#define TSI148_LCSR_OTAT_DBW_16        (0<<6)   /* 16-bit Data Width */
#define TSI148_LCSR_OTAT_DBW_32        (1<<6)   /* 32-bit Data Width */

#define TSI148_LCSR_OTAT_SUP           (1<<5)   /* Supervisory Access */
#define TSI148_LCSR_OTAT_PGM           (1<<4)   /* Program Access */

#define TSI148_LCSR_OTAT_AMODE_SHIFT   0
#define TSI148_LCSR_OTAT_AMODE_M       (0xf<<0) /* Address Mode Mask */
#define TSI148_LCSR_OTAT_AMODE_A16     (0<<0)   /* A16 Address Space */
#define TSI148_LCSR_OTAT_AMODE_A24     (1<<0)   /* A24 Address Space */
#define TSI148_LCSR_OTAT_AMODE_A32     (2<<0)   /* A32 Address Space */
#define TSI148_LCSR_OTAT_AMODE_A64     (4<<0)   /* A32 Address Space */
#define TSI148_LCSR_OTAT_AMODE_CRCSR   (5<<0)   /* CR/CSR Address Space */
#define TSI148_LCSR_OTAT_AMODE_USER1   (8<<0)   /* User1 Address Space */
#define TSI148_LCSR_OTAT_AMODE_USER2   (9<<0)   /* User2 Address Space */
#define TSI148_LCSR_OTAT_AMODE_USER3   (0xa<<0) /* User3 Address Space */
#define TSI148_LCSR_OTAT_AMODE_USER4   (0xb<<0) /* User4 Address Space */

/* VME Master Control Register  CRG+$234 */
#define TSI148_LCSR_VMCTRL_VSA         (1<<27)  /* VMEbus Stop Ack */
#define TSI148_LCSR_VMCTRL_VS          (1<<26)  /* VMEbus Stop */
#define TSI148_LCSR_VMCTRL_DHB         (1<<25)  /* Device Has Bus */
#define TSI148_LCSR_VMCTRL_DWB         (1<<24)  /* Device Wants Bus */

#define TSI148_LCSR_VMCTRL_RMWEN       (1<<20)  /* RMW Enable */
#define TSI148_LCSR_VMCTRL_A64DS       (1<<16)  /* A64 Data strobes */

#define TSI148_LCSR_VMCTRL_VTOFF_M     (7<<12)  /* VMEbus Master Time off */
#define TSI148_LCSR_VMCTRL_VTOFF_0     (0<<12)  /* 0us */
#define TSI148_LCSR_VMCTRL_VTOFF_1     (1<<12)  /* 1us */
#define TSI148_LCSR_VMCTRL_VTOFF_2     (2<<12)  /* 2us */
#define TSI148_LCSR_VMCTRL_VTOFF_4     (3<<12)  /* 4us */
#define TSI148_LCSR_VMCTRL_VTOFF_8     (4<<12)  /* 8us */
#define TSI148_LCSR_VMCTRL_VTOFF_16    (5<<12)  /* 16us */
#define TSI148_LCSR_VMCTRL_VTOFF_32    (6<<12)  /* 32us */
#define TSI148_LCSR_VMCTRL_VTOFF_64    (7<<12)  /* 64us */

#define TSI148_LCSR_VMCTRL_VTON_M      (7<<8)   /* VMEbus Master Time On */
#define TSI148_LCSR_VMCTRL_VTON_4      (0<<8)   /* 4us */
#define TSI148_LCSR_VMCTRL_VTON_8      (1<<8)   /* 8us */
#define TSI148_LCSR_VMCTRL_VTON_16     (2<<8)   /* 16us */
#define TSI148_LCSR_VMCTRL_VTON_32     (3<<8)   /* 32us */
#define TSI148_LCSR_VMCTRL_VTON_64     (4<<8)   /* 64us */
#define TSI148_LCSR_VMCTRL_VTON_128    (5<<8)   /* 128us */
#define TSI148_LCSR_VMCTRL_VTON_256    (6<<8)   /* 256us */
#define TSI148_LCSR_VMCTRL_VTON_512    (7<<8)   /* 512us */

#define TSI148_LCSR_VMCTRL_VREL_M      (3<<3)   /* Master Rel Mode Mask */
#define TSI148_LCSR_VMCTRL_VREL_T_D    (0<<3)   /* Time on or Done */
#define TSI148_LCSR_VMCTRL_VREL_T_R_D  (1<<3)   /* Time on and REQ or Done */
#define TSI148_LCSR_VMCTRL_VREL_T_B_D  (2<<3)   /* Time on and BCLR or Done */
#define TSI148_LCSR_VMCTRL_VREL_T_D_R  (3<<3)   /* Time on or Done and REQ */

#define TSI148_LCSR_VMCTRL_VFAIR       (1<<2)   /* VMEbus Master Fair Mode */
#define TSI148_LCSR_VMCTRL_VREQL_M     (3<<0)   /* VMEbus Master Req Level Mask */

/* VMEbus Control Register CRG+$238 */
#define TSI148_LCSR_VCTRL_DLT_M        (0xF<<24) /* Deadlock Timer */
#define TSI148_LCSR_VCTRL_DLT_OFF      (0<<24)   /* Deadlock Timer Off */
#define TSI148_LCSR_VCTRL_DLT_16       (1<<24)   /* 16 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_32       (2<<24)   /* 32 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_64       (3<<24)   /* 64 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_128      (4<<24)   /* 128 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_256      (5<<24)   /* 256 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_512      (6<<24)   /* 512 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_1024     (7<<24)   /* 1024 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_2048     (8<<24)   /* 2048 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_4096     (9<<24)   /* 4096 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_8192     (0xa<<24) /* 8192 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_16384    (0xb<<24) /* 16384 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_32768    (0xc<<24) /* 32768 VCLKS */

#define TSI148_LCSR_VCTRL_NERBB        (1<<20)  /* No Early Release of Bus Busy */

#define TSI148_LCSR_VCTRL_SRESET       (1<<17)  /* System Reset */
#define TSI148_LCSR_VCTRL_LRESET       (1<<16)  /* Local Reset */

#define TSI148_LCSR_VCTRL_SFAILAI      (1<<15)  /* SYSFAIL Auto Slot ID */
#define TSI148_LCSR_VCTRL_BID_M        (0x1F<<8)    /* Broadcast ID Mask */

#define TSI148_LCSR_VCTRL_ATOEN        (1<<7)   /* Arbiter Time-out Enable */
#define TSI148_LCSR_VCTRL_ROBIN        (1<<6)   /* VMEbus Round Robin */

#define TSI148_LCSR_VCTRL_GTO_M        (0xf<<0) /* VMEbus Global Time-out Mask */
#define TSI148_LCSR_VCTRL_GTO_8        (0<<0)   /* 8 us */
#define TSI148_LCSR_VCTRL_GTO_16       (1<<0)   /* 16 us */
#define TSI148_LCSR_VCTRL_GTO_32       (2<<0)   /* 32 us */
#define TSI148_LCSR_VCTRL_GTO_64       (3<<0)   /* 64 us */
#define TSI148_LCSR_VCTRL_GTO_128      (4<<0)   /* 128 us */
#define TSI148_LCSR_VCTRL_GTO_256      (5<<0)   /* 256 us */
#define TSI148_LCSR_VCTRL_GTO_512      (6<<0)   /* 512 us */
#define TSI148_LCSR_VCTRL_GTO_1024     (7<<0)   /* 1024 us */
#define TSI148_LCSR_VCTRL_GTO_2048     (7<<0)   /* 2048 us */

/* VMEbus Status Register  CRG + $23C */
#define TSI148_LCSR_VSTAT_CPURST       (1<<15)  /* Clear power up reset */
#define TSI148_LCSR_VSTAT_BDFAIL       (1<<14)  /* Board fail */
#define TSI148_LCSR_VSTAT_PURSTS       (1<<12)  /* Power up reset status */
#define TSI148_LCSR_VSTAT_BDFAILS      (1<<11)  /* Board Fail Status */
#define TSI148_LCSR_VSTAT_SYSFAILS     (1<<10)  /* System Fail Status */
#define TSI148_LCSR_VSTAT_ACFAILS      (1<<9)   /* AC fail status */
#define TSI148_LCSR_VSTAT_SCONS        (1<<8)   /* System Cont Status */
#define TSI148_LCSR_VSTAT_GAP          (1<<5)   /* Geographic Addr Parity */
#define TSI148_LCSR_VSTAT_GA_M         (0x1F<<0)    /* Geographic Addr Mask */

/* PCI Configuration Status Register CRG+$240 */
#define TSI148_LCSR_PCSR_SRTO         (7<<24)  /* */
#define TSI148_LCSR_PCSR_SRTT         (1<<22)  /* */
#define TSI148_LCSR_PCSR_CCTM         (1<<21)  /* */
#define TSI148_LCSR_PCSR_DRQ          (1<<20)  /* */
#define TSI148_LCSR_PCSR_DTTT         (1<<19)  /* */
#define TSI148_LCSR_PCSR_MRCT         (1<<18)  /* */
#define TSI148_LCSR_PCSR_MRC          (1<<17)  /* */
#define TSI148_LCSR_PCSR_SBH          (1<<16)  /* */
#define TSI148_LCSR_PCSR_SRTE         (1<<10)  /* */
#define TSI148_LCSR_PCSR_DTTE         (1<<9)   /* */
#define TSI148_LCSR_PCSR_MRCE         (1<<8)   /* */
#define TSI148_LCSR_PCSR_REQ64S       (1<<6)   /* Request 64 status set */
#define TSI148_LCSR_PCSR_M66ENS       (1<<5)   /* M66ENS 66Mhz enable */
#define TSI148_LCSR_PCSR_FRAMES       (1<<4)   /* Frame Status */
#define TSI148_LCSR_PCSR_IRDYS        (1<<3)   /* IRDY status */
#define TSI148_LCSR_PCSR_DEVSELS      (1<<2)   /* DEVL status */
#define TSI148_LCSR_PCSR_STOPS        (1<<1)   /* STOP status */
#define TSI148_LCSR_PCSR_TRDYS        (1<<0)   /* TRDY status */

/* VMEbus Exception Attributes Register CRG+0x268 */
#define TSI148_LCSR_VEAT_VES          (1<<31)
#define TSI148_LCSR_VEAT_VEOF         (1<<30)
#define TSI148_LCSR_VEAT_VESCL        (1<<29)
#define TSI148_LCSR_VEAT_2EOT         (1<<21)
#define TSI148_LCSR_VEAT_2EST         (1<<20)
#define TSI148_LCSR_VEAT_BERR         (1<<19)
#define TSI148_LCSR_VEAT_LWORD        (1<<18)
#define TSI148_LCSR_VEAT_WRITE        (1<<17)
#define TSI148_LCSR_VEAT_IACK         (1<<16)
#define TSI148_LCSR_VEAT_DS1          (1<<15)
#define TSI148_LCSR_VEAT_DS0          (1<<14)
#define TSI148_LCSR_VEAT_AM_SHIFT     8
#define TSI148_LCSR_VEAT_AM           (0x3f<<8)
#define TSI148_LCSR_VEAT_XAM          (0xff<<0)

/* PCI/X Error diagnostic attributes register CRG + 0x280 */
#define TSI148_LCSR_EDPAT_EDPST		(1<<31)
#define TSI148_LCSR_EDPAT_EDPOF		(1<<30)
#define TSI148_LCSR_EDPAT_EDPCL		(1<<29)
#define TSI148_LCSR_EDPAT_SCD		(1<<17)
#define TSI148_LCSR_EDPAT_USC		(1<<16)
#define TSI148_LCSR_EDPAT_SRT		(1<<15)
#define TSI148_LCSR_EDPAT_SCEM		(1<<14)
#define TSI148_LCSR_EDPAT_DPED		(1<<13)
#define TSI148_LCSR_EDPAT_DPE		(1<<12)
#define TSI148_LCSR_EDPAT_MRC		(1<<11)
#define TSI148_LCSR_EDPAT_RMA		(1<<10)
#define TSI148_LCSR_EDPAT_RTA		(1<<9)
#define TSI148_LCSR_EDPAT_DTT		(1<<8)
#define TSI148_LCSR_EDPAT_CBEA_M	(0xf<<4)
#define TSI148_LCSR_EDPAT_COMM_M	(0xf<<0)

/* Inbound Translation Starting Address Lower */
#define TSI148_LCSR_ITSAL6432_M        (0xFFFF<<16) /* Mask */
#define TSI148_LCSR_ITSAL24_M          (0x00FFF<<12)    /* Mask */
#define TSI148_LCSR_ITSAL16_M          (0x0000FFF<<4)   /* Mask */

/* Inbound Translation Ending Address Lower */
#define TSI148_LCSR_ITEAL6432_M        (0xFFFF<<16) /* Mask */
#define TSI148_LCSR_ITEAL24_M          (0x00FFF<<12)    /* Mask */
#define TSI148_LCSR_ITEAL16_M          (0x0000FFF<<4)   /* Mask */

/* Inbound Translation Offset Lower */
#define TSI148_LCSR_ITOFFL6432_M       (0xFFFF<<16) /* Mask */
#define TSI148_LCSR_ITOFFL24_M         (0xFFFFF<<12)    /* Mask */
#define TSI148_LCSR_ITOFFL16_M         (0xFFFFFFF<<4)   /* Mask */

/* Inbound Translation Attribute */
#define TSI148_LCSR_ITAT_EN            (1<<31)  /* Window Enable */
#define TSI148_LCSR_ITAT_TH            (1<<18)  /* Prefetch Threshold */

#define TSI148_LCSR_ITAT_VFS_M         (3<<16)  /* Virtual FIFO Size Mask */
#define TSI148_LCSR_ITAT_VFS_64        (0<<16)  /* 64 bytes Virtual FIFO Size */
#define TSI148_LCSR_ITAT_VFS_128       (1<<16)  /* 128 bytes Virtual FIFO Sz */
#define TSI148_LCSR_ITAT_VFS_256       (2<<16)  /* 256 bytes Virtual FIFO Sz */
#define TSI148_LCSR_ITAT_VFS_512       (3<<16)  /* 512 bytes Virtual FIFO Sz */

#define TSI148_LCSR_ITAT_2eSSTM_M      (7<<12)  /* 2eSST Xfer Rate Mask */
#define TSI148_LCSR_ITAT_2eSSTM_160    (0<<12)  /* 160MB/s 2eSST Xfer Rate */
#define TSI148_LCSR_ITAT_2eSSTM_267    (1<<12)  /* 267MB/s 2eSST Xfer Rate */
#define TSI148_LCSR_ITAT_2eSSTM_320    (2<<12)  /* 320MB/s 2eSST Xfer Rate */

#define TSI148_LCSR_ITAT_2eSSTB        (1<<11)  /* 2eSST Bcast Xfer Protocol */
#define TSI148_LCSR_ITAT_2eSST         (1<<10)  /* 2eSST Xfer Protocol */
#define TSI148_LCSR_ITAT_2eVME         (1<<9)   /* 2eVME Xfer Protocol */
#define TSI148_LCSR_ITAT_MBLT          (1<<8)   /* MBLT Xfer Protocol */
#define TSI148_LCSR_ITAT_BLT           (1<<7)   /* BLT Xfer Protocol */

#define TSI148_LCSR_ITAT_AS_M          (7<<4)   /* Address Space Mask */
#define TSI148_LCSR_ITAT_AS_A16        (0<<4)   /* A16 Address Space */
#define TSI148_LCSR_ITAT_AS_A24        (1<<4)   /* A24 Address Space */
#define TSI148_LCSR_ITAT_AS_A32        (2<<4)   /* A32 Address Space */
#define TSI148_LCSR_ITAT_AS_A64        (4<<4)   /* A64 Address Space */

#define TSI148_LCSR_ITAT_SUPR          (1<<3)   /* Supervisor Access */
#define TSI148_LCSR_ITAT_NPRIV         (1<<2)   /* Non-Priv (User) Access */
#define TSI148_LCSR_ITAT_PGM           (1<<1)   /* Program Access */
#define TSI148_LCSR_ITAT_DATA          (1<<0)   /* Data Access */

/* GCSR Base Address Lower Address  CRG +$404 */
#define TSI148_LCSR_GBAL_M             (0x7FFFFFF<<5)   /* Mask */

/* GCSR Attribute Register CRG + $408 */
#define TSI148_LCSR_GCSRAT_EN          (1<<7)   /* Enable access to GCSR */

#define TSI148_LCSR_GCSRAT_AS_M        (7<<4)   /* Address Space Mask */
#define TSI148_LCSR_GCSRAT_AS_16       (0<<4)   /* Address Space 16 */
#define TSI148_LCSR_GCSRAT_AS_24       (1<<4)   /* Address Space 24 */
#define TSI148_LCSR_GCSRAT_AS_32       (2<<4)   /* Address Space 32 */
#define TSI148_LCSR_GCSRAT_AS_64       (4<<4)   /* Address Space 64 */

#define TSI148_LCSR_GCSRAT_SUPR        (1<<3)   /* Sup set -GCSR decoder */
#define TSI148_LCSR_GCSRAT_NPRIV       (1<<2)   /* Non-Privliged set - CGSR */
#define TSI148_LCSR_GCSRAT_PGM         (1<<1)   /* Program set - GCSR decoder */
#define TSI148_LCSR_GCSRAT_DATA        (1<<0)   /* DATA set GCSR decoder */

/* CRG Base Address Lower Address  CRG + $410 */
#define TSI148_LCSR_CBAL_M             (0xFFFFF<<12)

/* CRG Attribute Register  CRG + $414 */
#define TSI148_LCSR_CRGAT_EN           (1<<7)   /* Enable PRG Access */

#define TSI148_LCSR_CRGAT_AS_M         (7<<4)   /* Address Space */
#define TSI148_LCSR_CRGAT_AS_16        (0<<4)   /* Address Space 16 */
#define TSI148_LCSR_CRGAT_AS_24        (1<<4)   /* Address Space 24 */
#define TSI148_LCSR_CRGAT_AS_32        (2<<4)   /* Address Space 32 */
#define TSI148_LCSR_CRGAT_AS_64        (4<<4)   /* Address Space 64 */

#define TSI148_LCSR_CRGAT_SUPR         (1<<3)   /* Supervisor Access */
#define TSI148_LCSR_CRGAT_NPRIV        (1<<2)   /* Non-Privliged(User) Access */
#define TSI148_LCSR_CRGAT_PGM          (1<<1)   /* Program Access */
#define TSI148_LCSR_CRGAT_DATA         (1<<0)   /* Data Access */

/* CR/CSR Offset Lower Register  CRG + $41C */
#define TSI148_LCSR_CROL_M             (0x1FFF<<19) /* Mask */

/* CR/CSR Attribute register  CRG + $420 */
#define TSI148_LCSR_CRAT_EN            (1<<7)   /* Enable access to CR/CSR */

/* Location Monitor base address lower register  CRG + $428 */
#define TSI148_LCSR_LMBAL_M            (0x7FFFFFF<<5)   /* Mask */

/* Location Monitor Attribute Register  CRG + $42C */
#define TSI148_LCSR_LMAT_EN            (1<<7)   /* Enable Location Monitor */

#define TSI148_LCSR_LMAT_AS_M          (7<<4)   /* Address Space MASK  */
#define TSI148_LCSR_LMAT_AS_16         (0<<4)   /* A16 */
#define TSI148_LCSR_LMAT_AS_24         (1<<4)   /* A24 */
#define TSI148_LCSR_LMAT_AS_32         (2<<4)   /* A32 */
#define TSI148_LCSR_LMAT_AS_64         (4<<4)   /* A64 */

#define TSI148_LCSR_LMAT_SUPR          (1<<3)   /* Supervisor Access */
#define TSI148_LCSR_LMAT_NPRIV         (1<<2)   /* Non-Priv (User) Access */
#define TSI148_LCSR_LMAT_PGM           (1<<1)   /* Program Access */
#define TSI148_LCSR_LMAT_DATA          (1<<0)   /* Data Access  */

/* Broadcast Pulse Generator Timer Register  CRG + $438 */
#define TSI148_LCSR_BPGTR_BPGT_M       (0xFFFF<<0)  /* Mask */

/* Broadcast Programmable Clock Timer Register  CRG + $43C */
#define TSI148_LCSR_BPCTR_BPCT_M       (0xFFFFFF<<0)    /* Mask */

/* VMEbus Interrupt Control Register           CRG + $440 */
#define TSI148_LCSR_VICR_CNTS_M        (3<<30)  /* Cntr Source MASK */
#define TSI148_LCSR_VICR_CNTS_DIS      (0<<30)  /* Cntr Disable */
#define TSI148_LCSR_VICR_CNTS_IRQ1     (1<<30)  /* IRQ1 to Cntr */
#define TSI148_LCSR_VICR_CNTS_IRQ2     (2<<30)  /* IRQ2 to Cntr */

#define TSI148_LCSR_VICR_EDGIS_M       (3<<28)  /* Edge interupt MASK */
#define TSI148_LCSR_VICR_EDGIS_DIS     (0<<28)  /* Edge interupt Disable */
#define TSI148_LCSR_VICR_EDGIS_IRQ1    (1<<28)  /* IRQ1 to Edge */
#define TSI148_LCSR_VICR_EDGIS_IRQ2    (2<<28)  /* IRQ2 to Edge */

#define TSI148_LCSR_VICR_IRQ1F_M       (3<<26)  /* IRQ1* Function MASK */
#define TSI148_LCSR_VICR_IRQ1F_NORM    (1<<26)  /* Normal */
#define TSI148_LCSR_VICR_IRQ1F_PULSE   (2<<26)  /* Pulse Generator */
#define TSI148_LCSR_VICR_IRQ1F_PROG    (3<<26)  /* Programmable Clock */
#define TSI148_LCSR_VICR_IRQ1F_1U      (4<<26)  /* 1.02us Clock */

#define TSI148_LCSR_VICR_IRQ2F_M       (3<<24)  /* IRQ2* Function MASK */
#define TSI148_LCSR_VICR_IRQ2F_NORM    (1<<24)  /* Normal */
#define TSI148_LCSR_VICR_IRQ2F_PULSE   (2<<24)  /* Pulse Generator */
#define TSI148_LCSR_VICR_IRQ2F_PROG    (3<<24)  /* Programmable Clock */
#define TSI148_LCSR_VICR_IRQ2F_1U      (4<<24)  /* 1us Clock */

#define TSI148_LCSR_VICR_BIP           (1<<23)  /* Broadcast Interrupt Pulse */
#define TSI148_LCSR_VICR_BIPS          (1<<22)  /* BIP status */

#define TSI148_LCSR_VICR_IRQC          (1<<15)  /* VMEbus IRQ Clear */

#define TSI148_LCSR_VICR_IRQLS         (7<<12)  /* VMEbus IRQ Level Status */


#define TSI148_LCSR_VICR_IRQS          (1<<11)  /* VMEbus IRQ Status */

#define TSI148_LCSR_VICR_IRQL_M        (7<<8)   /* VMEbus SW IRQ Level Mask */
#define TSI148_LCSR_VICR_IRQL_1        (1<<8)   /* VMEbus SW IRQ Level 1 */
#define TSI148_LCSR_VICR_IRQL_2        (2<<8)   /* VMEbus SW IRQ Level 2 */
#define TSI148_LCSR_VICR_IRQL_3        (3<<8)   /* VMEbus SW IRQ Level 3 */
#define TSI148_LCSR_VICR_IRQL_4        (4<<8)   /* VMEbus SW IRQ Level 4 */
#define TSI148_LCSR_VICR_IRQL_5        (5<<8)   /* VMEbus SW IRQ Level 5 */
#define TSI148_LCSR_VICR_IRQL_6        (6<<8)   /* VMEbus SW IRQ Level 6 */
#define TSI148_LCSR_VICR_IRQL_7        (7<<8)   /* VMEbus SW IRQ Level 7 */

#define TSI148_LCSR_VICR_STID_M        (0xFF<<0)    /* Status/ID Mask */

/* Interrupt Enable Register		CRG + $448 */
/* Interrupt Enable Out Register	CRG + $44c */
/* Interrupt Status Register		CRG + $450 */
/* Interrupt Clear Register		CRG + $454 */
/*  Note: IRQ1-7 bits are reserved in INTC     */
#define TSI148_LCSR_INT_DMA_SHIFT  24
#define TSI148_LCSR_INT_DMA_M      (3<<24)  /* DMAC mask */
#define TSI148_LCSR_INT_DMA1       (1<<25)  /* DMAC 1 */
#define TSI148_LCSR_INT_DMA0       (1<<24)  /* DMAC 0 */

#define TSI148_LCSR_INT_LM_SHIFT   20
#define TSI148_LCSR_INT_LM_M       (0xf<<20)
#define TSI148_LCSR_INT_LM3        (1<<23)  /* Location Monitor 3 */
#define TSI148_LCSR_INT_LM2        (1<<22)  /* Location Monitor 2 */
#define TSI148_LCSR_INT_LM1        (1<<21)  /* Location Monitor 1 */
#define TSI148_LCSR_INT_LM0        (1<<20)  /* Location Monitor 0 */

#define TSI148_LCSR_INT_MB_SHIFT   16
#define TSI148_LCSR_INT_MB_M       (0xf<<16)
#define TSI148_LCSR_INT_MB3        (1<<19)  /* Mail Box 3 */
#define TSI148_LCSR_INT_MB2        (1<<18)  /* Mail Box 2 */
#define TSI148_LCSR_INT_MB1        (1<<17)  /* Mail Box 1 */
#define TSI148_LCSR_INT_MB0        (1<<16)  /* Mail Box 0 */

#define TSI148_LCSR_INT_PERR       (1<<13)  /* VMEbus Error */
#define TSI148_LCSR_INT_VERR       (1<<12)  /* VMEbus Access Time-out */
#define TSI148_LCSR_INT_VIE        (1<<11)  /* VMEbus IRQ Edge */
#define TSI148_LCSR_INT_IACK       (1<<10)  /* IACK */
#define TSI148_LCSR_INT_SYSFL      (1<<9)   /* System Fail */
#define TSI148_LCSR_INT_ACFL       (1<<8)   /* AC Fail */

#define TSI148_LCSR_INT_IRQM       (0x7f<<1)
#define TSI148_LCSR_INT_IRQ7       (1<<7)   /* IRQ7 */
#define TSI148_LCSR_INT_IRQ6       (1<<6)   /* IRQ6 */
#define TSI148_LCSR_INT_IRQ5       (1<<5)   /* IRQ5 */
#define TSI148_LCSR_INT_IRQ4       (1<<4)   /* IRQ4 */
#define TSI148_LCSR_INT_IRQ3       (1<<3)   /* IRQ3 */
#define TSI148_LCSR_INT_IRQ2       (1<<2)   /* IRQ2 */
#define TSI148_LCSR_INT_IRQ1       (1<<1)   /* IRQ1 */

/* Interrupt Map Register 1 CRG + $458 */
#define TSI148_LCSR_INTM1_DMA1M_M      (3<<18)  /* DMA 1 */
#define TSI148_LCSR_INTM1_DMA0M_M      (3<<16)  /* DMA 0 */
#define TSI148_LCSR_INTM1_LM3M_M       (3<<14)  /* Location Monitor 3 */
#define TSI148_LCSR_INTM1_LM2M_M       (3<<12)  /* Location Monitor 2 */
#define TSI148_LCSR_INTM1_LM1M_M       (3<<10)  /* Location Monitor 1 */
#define TSI148_LCSR_INTM1_LM0M_M       (3<<8)   /* Location Monitor 0 */
#define TSI148_LCSR_INTM1_MB3M_M       (3<<6)   /* Mail Box 3 */
#define TSI148_LCSR_INTM1_MB2M_M       (3<<4)   /* Mail Box 2 */
#define TSI148_LCSR_INTM1_MB1M_M       (3<<2)   /* Mail Box 1 */
#define TSI148_LCSR_INTM1_MB0M_M       (3<<0)   /* Mail Box 0 */

/* Interrupt Map Register 2 CRG + $45C */
#define TSI148_LCSR_INTM2_PERRM_M      (3<<26)  /* PCI Bus Error */
#define TSI148_LCSR_INTM2_VERRM_M      (3<<24)  /* VMEbus Error */
#define TSI148_LCSR_INTM2_VIEM_M       (3<<22)  /* VMEbus IRQ Edge */
#define TSI148_LCSR_INTM2_IACKM_M      (3<<20)  /* IACK */
#define TSI148_LCSR_INTM2_SYSFLM_M     (3<<18)  /* System Fail */
#define TSI148_LCSR_INTM2_ACFLM_M      (3<<16)  /* AC Fail */
#define TSI148_LCSR_INTM2_IRQ7M_M      (3<<14)  /* IRQ7 */
#define TSI148_LCSR_INTM2_IRQ6M_M      (3<<12)  /* IRQ6 */
#define TSI148_LCSR_INTM2_IRQ5M_M      (3<<10)  /* IRQ5 */
#define TSI148_LCSR_INTM2_IRQ4M_M      (3<<8)   /* IRQ4 */
#define TSI148_LCSR_INTM2_IRQ3M_M      (3<<6)   /* IRQ3 */
#define TSI148_LCSR_INTM2_IRQ2M_M      (3<<4)   /* IRQ2 */
#define TSI148_LCSR_INTM2_IRQ1M_M      (3<<2)   /* IRQ1 */

/* DMA Control (0-1) Registers CRG + $500 */
#define TSI148_LCSR_DCTL_ABT           (1<<27)  /* Abort */
#define TSI148_LCSR_DCTL_PAU           (1<<26)  /* Pause */
#define TSI148_LCSR_DCTL_DGO           (1<<25)  /* DMA Go */

#define TSI148_LCSR_DCTL_MOD           (1<<23)  /* Mode */
#define TSI148_LCSR_DCTL_VFAR          (1<<17)  /* VME Flush on Aborted Read */
#define TSI148_LCSR_DCTL_PFAR          (1<<16)  /* PCI Flush on Aborted Read */

#define TSI148_LCSR_DCTL_VBKS_SHIFT    12
#define TSI148_LCSR_DCTL_VBKS_M        (7<<12)  /* VMEbus block Size MASK */
#define TSI148_LCSR_DCTL_VBKS_32       (0<<12)  /* VMEbus block Size 32 */
#define TSI148_LCSR_DCTL_VBKS_64       (1<<12)  /* VMEbus block Size 64 */
#define TSI148_LCSR_DCTL_VBKS_128      (2<<12)  /* VMEbus block Size 128 */
#define TSI148_LCSR_DCTL_VBKS_256      (3<<12)  /* VMEbus block Size 256 */
#define TSI148_LCSR_DCTL_VBKS_512      (4<<12)  /* VMEbus block Size 512 */
#define TSI148_LCSR_DCTL_VBKS_1024     (5<<12)  /* VMEbus block Size 1024 */
#define TSI148_LCSR_DCTL_VBKS_2048     (6<<12)  /* VMEbus block Size 2048 */
#define TSI148_LCSR_DCTL_VBKS_4096     (7<<12)  /* VMEbus block Size 4096 */

#define TSI148_LCSR_DCTL_VBOT_SHIFT    8
#define TSI148_LCSR_DCTL_VBOT_M        (7<<8)   /* VMEbus back-off MASK */
#define TSI148_LCSR_DCTL_VBOT_0        (0<<8)   /* VMEbus back-off  0us */
#define TSI148_LCSR_DCTL_VBOT_1        (1<<8)   /* VMEbus back-off 1us */
#define TSI148_LCSR_DCTL_VBOT_2        (2<<8)   /* VMEbus back-off 2us */
#define TSI148_LCSR_DCTL_VBOT_4        (3<<8)   /* VMEbus back-off 4us */
#define TSI148_LCSR_DCTL_VBOT_8        (4<<8)   /* VMEbus back-off 8us */
#define TSI148_LCSR_DCTL_VBOT_16       (5<<8)   /* VMEbus back-off 16us */
#define TSI148_LCSR_DCTL_VBOT_32       (6<<8)   /* VMEbus back-off 32us */
#define TSI148_LCSR_DCTL_VBOT_64       (7<<8)   /* VMEbus back-off 64us */

#define TSI148_LCSR_DCTL_PBKS_SHIFT    4
#define TSI148_LCSR_DCTL_PBKS_M        (7<<4)   /* PCI block size MASK */
#define TSI148_LCSR_DCTL_PBKS_32       (0<<4)   /* PCI block size 32 bytes */
#define TSI148_LCSR_DCTL_PBKS_64       (1<<4)   /* PCI block size 64 bytes */
#define TSI148_LCSR_DCTL_PBKS_128      (2<<4)   /* PCI block size 128 bytes */
#define TSI148_LCSR_DCTL_PBKS_256      (3<<4)   /* PCI block size 256 bytes */
#define TSI148_LCSR_DCTL_PBKS_512      (4<<4)   /* PCI block size 512 bytes */
#define TSI148_LCSR_DCTL_PBKS_1024     (5<<4)   /* PCI block size 1024 bytes */
#define TSI148_LCSR_DCTL_PBKS_2048     (6<<4)   /* PCI block size 2048 bytes */
#define TSI148_LCSR_DCTL_PBKS_4096     (7<<4)   /* PCI block size 4096 bytes */

#define TSI148_LCSR_DCTL_PBOT_SHIFT    0
#define TSI148_LCSR_DCTL_PBOT_M        (7<<0)   /* PCI back off MASK */
#define TSI148_LCSR_DCTL_PBOT_0        (0<<0)   /* PCI back off 0us */
#define TSI148_LCSR_DCTL_PBOT_1        (1<<0)   /* PCI back off 1us */
#define TSI148_LCSR_DCTL_PBOT_2        (2<<0)   /* PCI back off 2us */
#define TSI148_LCSR_DCTL_PBOT_4        (3<<0)   /* PCI back off 3us */
#define TSI148_LCSR_DCTL_PBOT_8        (4<<0)   /* PCI back off 4us */
#define TSI148_LCSR_DCTL_PBOT_16       (5<<0)   /* PCI back off 8us */
#define TSI148_LCSR_DCTL_PBOT_32       (6<<0)   /* PCI back off 16us */
#define TSI148_LCSR_DCTL_PBOT_64       (7<<0)   /* PCI back off 32us */

/* DMA Status Registers (0-1)  CRG + $504 */
#define TSI148_LCSR_DSTA_VBE           (1<<28)  /* Error */
#define TSI148_LCSR_DSTA_ABT           (1<<27)  /* Abort */
#define TSI148_LCSR_DSTA_PAU           (1<<26)  /* Pause */
#define TSI148_LCSR_DSTA_DON           (1<<25)  /* Done */
#define TSI148_LCSR_DSTA_BSY           (1<<24)  /* Busy */
#define TSI148_LCSR_DSTA_ERRS          (1<<20)  /* Error Source */
#define TSI148_LCSR_DSTA_ERT           (3<<16)  /* Error Type */

/* DMA Current Link Address Lower (0-1) */
#define TSI148_LCSR_DCLAL_M            (0x3FFFFFF<<6)   /* Mask */

/* DMA Source Attribute (0-1) Reg */
#define TSI148_LCSR_DSAT_TYP_SHIFT     28
#define TSI148_LCSR_DSAT_TYP_M         (3<<28)  /* Source Bus Type */
#define TSI148_LCSR_DSAT_TYP_PCI       (0<<28)  /* PCI Bus */
#define TSI148_LCSR_DSAT_TYP_VME       (1<<28)  /* VMEbus */
#define TSI148_LCSR_DSAT_TYP_PAT       (2<<28)  /* Data Pattern */

#define TSI148_LCSR_DSAT_PSZ           (1<<25)  /* Pattern Size */
#define TSI148_LCSR_DSAT_NIP           (1<<24)  /* No Increment */

#define TSI148_LCSR_DSAT_2eSSTM_SHIFT  11
#define TSI148_LCSR_DSAT_2eSSTM_M      (3<<11)  /* 2eSST Trans Rate Mask */
#define TSI148_LCSR_DSAT_2eSSTM_160    (0<<11)  /* 160 MB/s */
#define TSI148_LCSR_DSAT_2eSSTM_267    (1<<11)  /* 267 MB/s */
#define TSI148_LCSR_DSAT_2eSSTM_320    (2<<11)  /* 320 MB/s */

#define TSI148_LCSR_DSAT_TM_SHIFT      8
#define TSI148_LCSR_DSAT_TM_M          (7<<8)   /* Bus Transfer Protocol Mask */
#define TSI148_LCSR_DSAT_TM_SCT        (0<<8)   /* SCT */
#define TSI148_LCSR_DSAT_TM_BLT        (1<<8)   /* BLT */
#define TSI148_LCSR_DSAT_TM_MBLT       (2<<8)   /* MBLT */
#define TSI148_LCSR_DSAT_TM_2eVME      (3<<8)   /* 2eVME */
#define TSI148_LCSR_DSAT_TM_2eSST      (4<<8)   /* 2eSST */
#define TSI148_LCSR_DSAT_TM_2eSSTB     (5<<8)   /* 2eSST Broadcast */

#define TSI148_LCSR_DSAT_DBW_SHIFT     6
#define TSI148_LCSR_DSAT_DBW_M         (3<<6)   /* Max Data Width MASK */
#define TSI148_LCSR_DSAT_DBW_16        (0<<6)   /* 16 Bits */
#define TSI148_LCSR_DSAT_DBW_32        (1<<6)   /* 32 Bits */

#define TSI148_LCSR_DSAT_SUP           (1<<5)   /* Supervisory Mode */
#define TSI148_LCSR_DSAT_PGM           (1<<4)   /* Program Mode */

#define TSI148_LCSR_DSAT_AMODE_SHIFT   0
#define TSI148_LCSR_DSAT_AMODE_M       (0xf<<0) /* Address Space Mask */
#define TSI148_LCSR_DSAT_AMODE_16      (0<<0)   /* A16 */
#define TSI148_LCSR_DSAT_AMODE_24      (1<<0)   /* A24 */
#define TSI148_LCSR_DSAT_AMODE_32      (2<<0)   /* A32 */
#define TSI148_LCSR_DSAT_AMODE_64      (4<<0)   /* A64 */
#define TSI148_LCSR_DSAT_AMODE_CRCSR   (5<<0)   /* CR/CSR */
#define TSI148_LCSR_DSAT_AMODE_USER1   (8<<0)   /* User1 */
#define TSI148_LCSR_DSAT_AMODE_USER2   (9<<0)   /* User2 */
#define TSI148_LCSR_DSAT_AMODE_USER3   (0xa<<0) /* User3 */
#define TSI148_LCSR_DSAT_AMODE_USER4   (0xb<<0) /* User4 */

/* DMA Destination Attribute Registers (0-1) */
#define TSI148_LCSR_DDAT_TYP_SHIFT     28
#define TSI148_LCSR_DDAT_TYP_M         (1<<28)  /* Destination Bus Type */
#define TSI148_LCSR_DDAT_TYP_PCI       (0<<28)  /* Destination PCI Bus  */
#define TSI148_LCSR_DDAT_TYP_VME       (1<<28)  /* Destination VMEbus */

#define TSI148_LCSR_DDAT_2eSSTM_M      (3<<11)  /* 2eSST Transfer Rate Mask */
#define TSI148_LCSR_DDAT_2eSSTM_160    (0<<11)  /* 160 MB/s */
#define TSI148_LCSR_DDAT_2eSSTM_267    (1<<11)  /* 267 MB/s */
#define TSI148_LCSR_DDAT_2eSSTM_320    (2<<11)  /* 320 MB/s */

#define TSI148_LCSR_DDAT_TM_M          (7<<8)   /* Bus Transfer Protocol Mask */
#define TSI148_LCSR_DDAT_TM_SCT        (0<<8)   /* SCT */
#define TSI148_LCSR_DDAT_TM_BLT        (1<<8)   /* BLT */
#define TSI148_LCSR_DDAT_TM_MBLT       (2<<8)   /* MBLT */
#define TSI148_LCSR_DDAT_TM_2eVME      (3<<8)   /* 2eVME */
#define TSI148_LCSR_DDAT_TM_2eSST      (4<<8)   /* 2eSST */
#define TSI148_LCSR_DDAT_TM_2eSSTB     (5<<8)   /* 2eSST Broadcast */

#define TSI148_LCSR_DDAT_DBW_M         (3<<6)   /* Max Data Width MASK */
#define TSI148_LCSR_DDAT_DBW_16        (0<<6)   /* 16 Bits */
#define TSI148_LCSR_DDAT_DBW_32        (1<<6)   /* 32 Bits */

#define TSI148_LCSR_DDAT_SUP           (1<<5)   /* Supervisory/User Access */
#define TSI148_LCSR_DDAT_PGM           (1<<4)   /* Program/Data Access */

#define TSI148_LCSR_DDAT_AMODE_M       (0xf<<0) /* Address Space Mask */
#define TSI148_LCSR_DDAT_AMODE_16      (0<<0)   /* A16 */
#define TSI148_LCSR_DDAT_AMODE_24      (1<<0)   /* A24 */
#define TSI148_LCSR_DDAT_AMODE_32      (2<<0)   /* A32 */
#define TSI148_LCSR_DDAT_AMODE_64      (4<<0)   /* A64 */
#define TSI148_LCSR_DDAT_AMODE_CRCSR   (5<<0)   /* CRC/SR */
#define TSI148_LCSR_DDAT_AMODE_USER1   (8<<0)   /* User1 */
#define TSI148_LCSR_DDAT_AMODE_USER2   (9<<0)   /* User2 */
#define TSI148_LCSR_DDAT_AMODE_USER3   (0xa<<0) /* User3 */
#define TSI148_LCSR_DDAT_AMODE_USER4   (0xb<<0) /* User4 */

/* DMA Next Link Address Lower */
#define TSI148_LCSR_DNLAL_DNLAL_M      (0x1FFFFFFF<<3)   /* Address Mask */
#define TSI148_LCSR_DNLAL_LLA          (1<<0)   /* Last Link Address Indicator */

/* DMA 2eSST Broadcast Select */
#define TSI148_LCSR_DBS_M              (0x1FFFFF<<0)    /* Mask */

/*
 * GCSR Register Group
 */

/* GCSR Control and Status Register  CRG + $604 */
#define TSI148_GCSR_GCTRL_LRST         (1<<15)  /* Local Reset */
#define TSI148_GCSR_GCTRL_SFAILEN      (1<<14)  /* System Fail enable */
#define TSI148_GCSR_GCTRL_BDFAILS      (1<<13)  /* Board Fail Status */
#define TSI148_GCSR_GCTRL_SCON         (1<<12)  /* System Copntroller */
#define TSI148_GCSR_GCTRL_MEN          (1<<11)  /* Module Enable (READY) */

#define TSI148_GCSR_GCTRL_LMI3S        (1<<7)   /* Loc Monitor 3 Int Status */
#define TSI148_GCSR_GCTRL_LMI2S        (1<<6)   /* Loc Monitor 2 Int Status */
#define TSI148_GCSR_GCTRL_LMI1S        (1<<5)   /* Loc Monitor 1 Int Status */
#define TSI148_GCSR_GCTRL_LMI0S        (1<<4)   /* Loc Monitor 0 Int Status */
#define TSI148_GCSR_GCTRL_MBI3S        (1<<3)   /* Mail box 3 Int Status */
#define TSI148_GCSR_GCTRL_MBI2S        (1<<2)   /* Mail box 2 Int Status */
#define TSI148_GCSR_GCTRL_MBI1S        (1<<1)   /* Mail box 1 Int Status */
#define TSI148_GCSR_GCTRL_MBI0S        (1<<0)   /* Mail box 0 Int Status */

#define TSI148_GCSR_GAP                (1<<5)   /* Geographic Addr Parity */
#define TSI148_GCSR_GA_M               (0x1F<<0)    /* Geographic Address Mask */

/*
 * CR/CSR Register Group
 */

/* CR/CSR Bit Clear Register CRG + $FF4 */
#define TSI148_CRCSR_CSRBCR_LRSTC      (1<<7)   /* Local Reset Clear */
#define TSI148_CRCSR_CSRBCR_SFAILC     (1<<6)   /* System Fail Enable Clear */
#define TSI148_CRCSR_CSRBCR_BDFAILS    (1<<5)   /* Board Fail Status */
#define TSI148_CRCSR_CSRBCR_MENC       (1<<4)   /* Module Enable Clear */
#define TSI148_CRCSR_CSRBCR_BERRSC     (1<<3)   /* Bus Error Status Clear */

/* CR/CSR Bit Set Register CRG+$FF8 */
#define TSI148_CRCSR_CSRBSR_LRSTS      (1<<7)   /* Local Reset Set */
#define TSI148_CRCSR_CSRBSR_SFAILS     (1<<6)   /* System Fail Enable Set */
#define TSI148_CRCSR_CSRBSR_BDFAILS    (1<<5)   /* Board Fail Status */
#define TSI148_CRCSR_CSRBSR_MENS       (1<<4)   /* Module Enable Set */
#define TSI148_CRCSR_CSRBSR_BERRS      (1<<3)   /* Bus Error Status Set */

/* CR/CSR Base Address Register CRG + FFC */
#define TSI148_CRCSR_CBAR_M            (0x1F<<3)    /* Mask */


/* Low level misc inline stuff */
static inline int tsi148_get_slotnum(struct tsi148_chip *regs)
{
	return  ioread32be(&regs->lcsr.vstat) & 0x1f;
}

static inline int tsi148_get_syscon(struct tsi148_chip *regs)
{
	int syscon = 0;

	if (ioread32be(&regs->lcsr.vstat) & 0x100)
		syscon = 1;

	return syscon;
}

static inline int tsi148_set_interrupts(struct tsi148_chip *regs,
					unsigned int mask)
{
	iowrite32be(mask, &regs->lcsr.inteo);
	iowrite32be(mask, &regs->lcsr.inten);

	/* Quick sanity check */
	if ((ioread32be(&regs->lcsr.inteo) != mask) ||
	    (ioread32be(&regs->lcsr.inten) != mask))
		return -1;

	return 0;
}

static inline unsigned int tsi148_get_int_enabled(struct tsi148_chip *regs)
{
	return ioread32be(&regs->lcsr.inteo);
}

static inline unsigned int tsi148_get_int_status(struct tsi148_chip *regs)
{
	return ioread32be(&regs->lcsr.ints);
}

static inline void tsi148_clear_int(struct tsi148_chip *regs, unsigned int mask)
{
	iowrite32be(mask, &regs->lcsr.intc);
}

static inline int tsi148_iack8(struct tsi148_chip *regs, int irq)
{
	int vec = ioread8(&regs->lcsr.viack[(irq * 4) + 3]);

	return vec & 0xff;
}

static inline int tsi148_bus_error_chk(struct tsi148_chip *regs, int clear)
{
	unsigned int veat = ioread32be(&regs->lcsr.veat);

	if (veat & TSI148_LCSR_VEAT_VES) {
		if (clear)
			iowrite32be(TSI148_LCSR_VEAT_VESCL, &regs->lcsr.veat);
		return 1;
	}

	return 0;
}


extern void tsi148_handle_pci_error(void);
extern void tsi148_handle_vme_error(void);
extern int tsi148_generate_interrupt(int, int, signed long);

extern int tsi148_dma_get_status(struct dma_channel *);
extern int tsi148_dma_busy(struct dma_channel *);
extern int tsi148_dma_done(struct dma_channel *);
extern int tsi148_dma_setup(struct dma_channel *);
extern void tsi148_dma_start(struct dma_channel *);
extern void tsi148_dma_abort(struct dma_channel *);
extern void tsi148_dma_release(struct dma_channel *);
extern void __devexit tsi148_dma_exit(void);
extern int __devinit tsi148_dma_init(void);

extern void tsi148_get_window_attr(struct vme_mapping *);
extern int tsi148_create_window(struct vme_mapping *);
extern void tsi148_remove_window(struct vme_mapping *);

extern int __devinit tsi148_setup_crg(unsigned int, enum vme_address_modifier);
extern void __devexit tsi148_disable_crg(struct tsi148_chip *);

#ifdef CONFIG_PROC_FS
extern void tsi148_procfs_register(struct proc_dir_entry *);
extern void tsi148_procfs_unregister(struct proc_dir_entry *);
#endif /* CONFIG_PROC_FS */
extern void tsi148_quiesce(struct tsi148_chip *);
extern void tsi148_init(struct tsi148_chip *);

#endif /* _TSI148_H */

