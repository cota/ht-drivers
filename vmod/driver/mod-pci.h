/**
 * @file mod-pci.h
 *
 * @brief MOD-PCI ADC definitions
 *
 * The Janz MOD-PCI module is a PCI carrier board
 * handling up to 2 mezzanine cards. 
 *
 * This file provides the definitions for low-level access to 
 * its registers
 *
 * Copyright (c) 2009 CERN
 * @author Juan David Gonzalez Cobas <dcobas@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifndef _MOD_PCI_H_
#define _MOD_PCI_H_

/* Vendor/Device IDs for JANZ MOD-PCI */
#define PCI_VENDOR_ID_JANZ		0x10b5
#define PCI_DEVICE_ID_JANZ_MOD_PCI_1	0x9030	/* PLX9030-based */
#define PCI_DEVICE_ID_JANZ_MOD_PCI_2	0x9050	/* PLX9050-based */

#define PCI_SUBSYSTEM_VENDOR_ID_JANZ	0x13c3
#define PCI_SUBSYSTEM_ID_MOD_PCI_1_0	0x0200  /* MOD-PCI revisions */
#define PCI_SUBSYSTEM_ID_MOD_PCI_2_0	0x0201
#define PCI_SUBSYSTEM_ID_MOD_PCI_2_1	0x0201
#define PCI_SUBSYSTEM_ID_MOD_PCI_3_0	0x0202

/* MOD-PCI BARs base address registers */
#define MOD_PCI_CONFIG_MMIO_BAR		0
#define MOD_PCI_CONFIG_IO_BAR		1
#define MOD_PCI_LITTLE_BAR		2
#define MOD_PCI_BIG_BAR			3
#define MOD_PCI_ONBOARD_REGS_BAR	4
#define MOD_PCI_UNUSED_BAR		5

#define	MOD_PCI_BIG_BAR_OFFSET	\
	(sizeof(u32)*MOD_PCI_BIG_BAR)

#define	MOD_PCI_CONFIG_MMIO_BAR_SIZE	128
#define	MOD_PCI_CONFIG_IO_BAR_SIZE	128
#define	MOD_PCI_LITTLE_BAR_SIZE		4096
#define	MOD_PCI_BIG_BAR_SIZE		4096
#define	MOD_PCI_ONBOARD_REGS_BAR_SIZE	4096
#define	MOD_PCI_UNUSED_BAR_SIZE		0

/* MODULbus memory space offsets */
#define MOD_PCI_SLOTS			2
#define MOD_PCI_SLOT0_OFFSET	0x000
#define MOD_PCI_SLOT1_OFFSET	0x200
#define MOD_PCI_WIDTH		16
#define MOD_PCI_WINDOW_SIZE	2048

/* On-board registers offsets */
/* WARNING: all byte-only accesses */
#define MOD_PCI_INT_STAT	0x1	/* read access */
#define MOD_PCI_INT_DISABLE	0x1	/* write access */
#define MOD_PCI_MBUS_NUM	0x3	/* read access */
#define MOD_PCI_INT_ENABLE	0x3	/* write access */
#define MOD_PCI_RESET_ASSERT	0x5	/* write access */
#define MOD_PCI_RESET_DEASSERT	0x7	/* write access */
#define MOD_PCI_EEP		0x9	/* r/w access */
#define MOD_PCI_ENID		0xb	/* write/access */

/** map of the onboard registers at BAR#4 */
struct onboard {
	unsigned char	unused1;
	union {
	unsigned char	int_stat;
	unsigned char	int_disable;
	};
	unsigned char	unused2;
	union {
	unsigned char	mbus_num;
	unsigned char	int_enable;
	};
	unsigned char	unused3;
	unsigned char	reset_assert;
	unsigned char	unused4;
	unsigned char	reset_deassert;
	unsigned char	unused5;
	unsigned char	eep;
	unsigned char	unused6;
	unsigned char	unused7;
	unsigned char	enid;
};

/** description of a mod-pci module */
struct mod_pci {
	int		lun;		/* logical unit number */
	int		bus_number;	/* pci bus number */
	int		slot_number;	/* pci slot number */
	void __iomem	*vaddr;		/* virtual address of MODULBUS
						space */
	struct onboard
		__iomem	*onboard;	/* on-board registers */
};

#endif /* _MOD_PCI_H_ */
