
/**
 * @file mod-pci.h
 *
 * @brief MOD-PCI carrier board hardware register definitions
 *
 * The MOD-PCI carrier board is a PCI card that may host until two VMOD
 * mezannine cards in slots numbered 0 and 1. Definitions of hardware
 * addresses, registersn masks and convenience macros concerning this
 * module are provided in this header file.
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
#define MOD_PCI_LOCAL_CONFIG_REGS_MMIO_BAR	0
#define MOD_PCI_LOCAL_CONFIG_REGS_IO_BAR	1
#define MOD_PCI_MODULBUS_MEMSPACE_LITTLE_BAR	2
#define MOD_PCI_MODULBUS_MEMSPACE_BIG_BAR	3
#define MOD_PCI_ONBOARD_REGS_BAR		4
#define MOD_PCI_UNUSED_BAR			5

#define MOD_PCI_LOCAL_CONFIG_REGS_MMIO_BAR_WIDTH	128
#define MOD_PCI_LOCAL_CONFIG_REGS_IO_BAR_WIDTH		128
#define MOD_PCI_MODULBUS_MEMSPACE_LITTLE_BAR_WIDTH	4096
#define MOD_PCI_MODULBUS_MEMSPACE_BIG_BAR_WIDTH		4096
#define MOD_PCI_ONBOARD_REGS_BAR_WIDTH			4096
#define MOD_PCI_UNUSED_BAR_WIDTH			0

/* MODULbus memory space offsets */
#define MOD_PCI_SLOTS			2
#define MOD_PCI_MODULBUS_SLOT0_OFFSET	0x000
#define MOD_PCI_MODULBUS_SLOT1_OFFSET	0x200

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

#endif /* _MOD_PCI_H_ */
