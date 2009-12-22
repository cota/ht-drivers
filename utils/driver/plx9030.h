/**
 * @file plx9030.h
 *
 * @brief Parameters for the PLX9030 PCI Controller chip and some
 *        definitions for 93LC56B EPROM.
 *
 * @author Julian Lewis CERN AB/CO/HT Geneva. Julian.Lewis@cern.ch
 *
 * @date Nov, 2003
 *
 * @version 1.0  lewis  26/11/2003  Initial version.
 */
#ifndef _PLX_9030_H_INCLUDE_
#define _PLX_9030_H_INCLUDE_

#include <linux/pci.h>
#include "plx9030_layout.h"
#include "drvr_vid_did.h"

#define PLX9030_DEVICE PCI_DEVICE(PLX9030_VENDOR_ID, PLX9030_DEVICE_ID)


/*! @name Define the structure of the PLX 9030 Registers
 *
 * See PCI 9030 Data Book Section 10-2 Register Address Mapping.
 */
//@{
#define PLX9030_LAS0BA  0x14 //!< Local Address Space 0 Local Base Address
#define PLX9030_LAS0BRD 0x28 //!< Local Address Space 0 Bus Region Descriptor
#define PLX9030_INTCSR  0x4C //!<
#define PLX9030_CNTRL   0x50 //!<
#define PLX9030_GPIOC   0x54 //!<
#define PLX9030_PCIILR  0x3C //!<

#define Plx9030CntrlCHIP_UNSELECT      0x00780000
#define Plx9030CntrlCHIP_SELECT        0x02780000
#define Plx9030CntrlDATA_CLOCK         0x01000000
#define Plx9030CntrlDATA_OUT_ONE       0x06780000
#define Plx9030CntrlDATA_OUT_ZERO      0x02780000
#define Plx9030CntrlDONE_FLAG          0x08000000
#define Plx9030CntrlDATA_IN_ONE        0x08000000
#define Plx9030CntrlRECONFIGURE        0x20780000
#define Plx9030CntrlPCI_SOFTWARE_RESET 0x40000000

#define Plx9030Las0brdBIG_ENDIAN    0x03800002
#define Plx9030Las0brdLITTLE_ENDIAN 0x00800002

#define Plx9030IntcsrLINT_ENABLE    0x0009
#define Plx9030IntcsrLINT_HIGH      0x0012
#define Plx9030IntcsrPCI_INT_ENABLE 0x0040
//@}


/** @brief Instruction set for the 93LC56B EEPROM from Microchip Technology Inc.
 *
 * We write the above 9030 configuration fields to this EEPROM using the
 * 9030 Serial Control Register (CNTRL) See: 10.34-10.35 Bits 24..29
 * N.B VPD will not work. See errata #1 and chapter 9.
 */
typedef enum {
  EpCmdERASE = 0xE000, //!< Sets all data bits at the specified address to 1
  EpCmdERAL  = 0x9000, //!< Set all bits in the entire memory to 1
  EpCmdEWDS  = 0x8000, //!< Close the device, disallow all ERASE and WRITES
  EpCmdEWEN  = 0x9800, //!< Open the device, allow ERASE and WRITES
  EpCmdREAD  = 0xC000, //!< Read data bits at specified address to D0 pin
  EpCmdWRITE = 0xA000, //!< Write data bits from D1 pin to specified address
  EpCmdWRAL  = 0x8800, //!< Write all memory with specified data

  EpCmdCOMMANDS = 7
} EpCmd;

#define EpAddMsk 0x0FC0  //!< Address mask
#define EpAddPos    12   //!< Address position
#define EpClkCycles 11   //!< Number of clock cycles for a command

#define PLX_PCI_CMD 0x4	//!< Register 10-2 (PCICR; PCI:04g) PCI Command


/** @brief PLX9030 data */
typedef struct _plx9030Info {
  struct list_head plx_bar_list; //!< Mapped BARs list.
  void __iomem *plx_remap;       //!< Plx9030 local config space address (BAR0)
  unsigned long plx_flash_open;  //!< if flash for 93LC56B is open
} plx_info_t;


/*! @name PLX return status codes */
//@{
#define PLX_OK              0
#define PLX_FAILED          1
#define PLX_NOT_INITIALIZED 2
#define PLX_NOT_SUPPORTED   4
#define PLX_TIMEOUT         8
//@}


/** @brief Local Configuration Registers read/write */
typedef enum {
  PLX_LCR_RD = 0, //!< read LCR
  PLX_LCR_WR = 1  //!< write LCR
} plxrw_t;


/** @defgroup plx9030api plx9030 API function declaration
 *@{
 */
int  plx9030_init(struct pci_dev*, plx_info_t*);
int  plx9030_read_eeprom(plx_info_t *card, int reg, Plx9030Eprom_t* e_data);
int  plx9030_write_eeprom(plx_info_t *card, int reg, Plx9030Eprom_t* e_data);
void plx9030_erase_eeprom(plx_info_t *card);
void plx9030_software_reset(plx_info_t *card);
//@} end of group


#endif /* _PLX_9030_H_INCLUDE_ */
