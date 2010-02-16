/* ========================================================================== */
/* Parameters for the PLX9030 PCI Controller chip and some definitions for    */
/* 93LC56B EPROM                                                              */
/* Julian Lewis 26th Nov 2003. CERN AB/CO/HT Geneva                           */
/* Julian.Lewis@cern.ch                                                       */
/* ========================================================================== */

#ifndef PLX9030
#define PLX9030

/* ======================================================================= */
/* Define the structure of the PLX 9030 Registers                          */
/* See PCI 9030 Data Book Section 10-2 Register Address Mapping            */

#define PLX9030_VENDOR_ID 0x10B5
#define PLX9030_DEVICE_ID 0x9030

#define PLX9030_LAS0BA  0x14
#define PLX9030_LAS0BRD 0x28
#define PLX9030_INTCSR  0x4C
#define PLX9030_CNTRL   0x50
#define PLX9030_GPIOC   0x54
#define PLX9030_PCIILR  0x3C

#define Plx9030CntrlCHIP_UNSELECT 0x00780000
#define Plx9030CntrlCHIP_SELECT   0x02780000
#define Plx9030CntrlDATA_CLOCK    0x01000000
#define Plx9030CntrlDATA_OUT_ONE  0x06780000
#define Plx9030CntrlDATA_OUT_ZERO 0x02780000
#define Plx9030CntrlDONE_FLAG     0x08000000
#define Plx9030CntrlDATA_IN_ONE   0x08000000
#define Plx9030CntrlRECONFIGURE   0x20780000
#define Plx9030CntrlPCI_SOFTWARE_RESET 0x40000000

#define Plx9030Las0brdBIG_ENDIAN    0x03800002
#define Plx9030Las0brdLITTLE_ENDIAN 0x00800002

#define Plx9030IntcsrLINT_ENABLE   0x0009
#define Plx9030IntcsrLINT_HIGH     0x0012
#define Plx9030IntcsrPCI_INT_ENABLE 0x0040

typedef struct {
   unsigned short DeviceId;
   unsigned short VendorId;
 } PlxRegPCIIDR;

typedef struct {
   PlxRegPCIIDR   PCIIDR;       /* Address 0x00: PCI Configuration ID */
   unsigned short PCICR;        /* Address 0x04: PCI Command */
   unsigned short PCISR;        /* Address 0x06: PCI Status */
   unsigned char  PCIREV;       /* Address 0x08: PCI Revision ID */
   unsigned char  PCICCR0;      /* Address 0x09: PCI Class code Byte 1 */
   unsigned char  PCICCR1;      /* Address 0x0A: PCI Class code Byte 2 */
   unsigned char  PCICCR2;      /* Address 0x0B: PCI Class code Byte 3 */
   unsigned char  PCICLSR;      /* Address 0x0C: PCI Cash Line Size */
   unsigned char  PCILTR;       /* Address 0x0D: PCI Bus Latency Timer */
   unsigned char  PCIHTR;       /* Address 0x0E: PCI Header Type */
   unsigned char  PCIBISTR;     /* Address 0x0F: PCI PCI Built in Self Test */
   unsigned int   PCIBAR0;      /* Address 0x10: PCI BAR0 for Local configuration space mapped */
   unsigned int   PCIBAR1;      /* Address 0x14: PCI BAR1 for Local configuration space IO */
   unsigned int   PCIBAR2;      /* Address 0x18: PCI BAR2 Local address space 0 */
   unsigned int   PCIBAR3;      /* Address 0x1C: PCI BAR3 Local address space 1 */
   unsigned int   PCIBAR4;      /* Address 0x20: PCI BAR4 Local address space 2 */
   unsigned int   PCIBAR5;      /* Address 0x24: PCI BAR5 Local address space 3 */
   unsigned int   PCICIS;       /* Address 0x28: PCI Card bus info structure pointer */
   unsigned short PCISVID;      /* Address 0x2C: PCI Subsystem Vendor ID */
   unsigned short PCISID;       /* Address 0x2E: PCI Subsystem ID */
   unsigned int   PCIERBAR;     /* Address 0x30: PCI Expansion ROM Base address */
   unsigned int   CAP_PTR;      /* Address 0x34: New capability pointer */
   unsigned int   Reserved0;    /* Address 0x38: Reserved */
   unsigned char  PCIILR;       /* Address 0x3C: PCI Interrupt Line */
   unsigned char  PCIIPR;       /* Address 0x3D: PCI Interrupt Pin */
   unsigned char  PCIMGR;       /* Address 0x3E: PCI Minimum Grant */
   unsigned char  PCIMLR;       /* Address 0x3F: PCI Maximum Latency */
   unsigned char  PMCCAPID;     /* Address 0x40: Power Management capability ID */
   unsigned char  PMNEXT;       /* Address 0x41: Power Management capability Pointer */
   unsigned short PMC;          /* Address 0x42: Power Management capabilities */
   unsigned short PMCSR;        /* Address 0x44: Power Management Control/Status */
   unsigned char  PMCSR_BSE;    /* Address 0x46: PMC Bridge Support Extensisions */
   unsigned char  PMDATA;       /* Address 0x47: Power Management Data */
   unsigned char  HS_CNTL;      /* Address 0x48: Hot Swap Control */
   unsigned char  HS_NEXT;      /* Address 0x49: Hot Swap Next Capability Pointer */
   unsigned short HS_CSR;       /* Address 0x4A: Hot Swap Control Status */
   unsigned char  PVPDCNTL;     /* Address 0x4C: PCI Vital Product Data Control */
   unsigned char  PVPD_NEXT;    /* Address 0x4D: PCI VPD Next Capability pointer */
   unsigned short PVPDAD;       /* Address 0x4E: PCI VPD Data Address */
   unsigned int   PVPDATA;      /* Address 0x50: PCI VPD Data */
 } PlxPciConfRegMap;

/* ======================================================================= */
/* Define the structure of the PLX 9030 Registers                          */
/* See PCI 9030 Data Book Section 10-3 Local Configuration Address Mapping */

typedef struct {
   unsigned int  LAS0RR;        /* Address 0x00: Local Address Space 0 Range */
   unsigned int  LAS1RR;        /* Address 0x04: Local Address Space 1 Range */
   unsigned int  LAS2RR;        /* Address 0x08: Local Address Space 2 Range */
   unsigned int  LAS3RR;        /* Address 0x0C: Local Address Space 3 Range */
   unsigned int  EROMRR;        /* Address 0x10: Expansion ROM Range */
   unsigned int  LAS0BA;        /* Address 0x14: Local Address Space 0 Local Base Address (Remap) */
   unsigned int  LAS1BA;        /* Address 0x18: Local Address Space 1 Local Base Address (Remap) */
   unsigned int  LAS2BA;        /* Address 0x1C: Local Address Space 2 Local Base Address (Remap) */
   unsigned int  LAS3BA;        /* Address 0x20: Local Address Space 3 Local Base Address (Remap) */
   unsigned int  EROMBA;        /* Address 0x24: Expansion ROM Local Base Address (Remap) */
   unsigned int  LAS0BRD;       /* Address 0x28: Local Address Space 0 Bus Region Descriptor */
   unsigned int  LAS1BRD;       /* Address 0x2C: Local Address Space 1 Bus Region Descriptor */
   unsigned int  LAS2BRD;       /* Address 0x30: Local Address Space 2 Bus Region Descriptor */
   unsigned int  LAS3BRD;       /* Address 0x34: Local Address Space 3 Bus Region Descriptor */
   unsigned int  EROMBRD;       /* Address 0x38: Expansion ROM Bus Region Descriptor */
   unsigned int  CS0BASE;       /* Address 0x3C: Chip Select 0 Base Address */
   unsigned int  CS1BASE;       /* Address 0x40: Chip Select 1 Base Address */
   unsigned int  CS2BASE;       /* Address 0x44: Chip Select 2 Base Address */
   unsigned int  CS3BASE;       /* Address 0x48: Chip Select 3 Base Address */
   unsigned short INTCSR;       /* Address 0x4C: Interrupt Control/Status */
   unsigned short PROT_AREA;    /* Address 0x4E: Serial EEPROM Write-Protected Address Boundary */
   unsigned int  CNTRL;         /* Address 0x50: PCI Target Response, Serial EEPROM and Initialization Control */
   unsigned int  GPIOC;         /* Address 0x54: General Purpose IO Control */
   unsigned int  Reserved0;     /* Address 0x58: Reserved 0 */
   unsigned int  Reserved1;     /* Address 0x5C: Reserved 1 */
   unsigned int  Reserved2;     /* Address 0x60: Reserved 2 */
   unsigned int  Reserved3;     /* Address 0x64: Reserved 3 */
   unsigned int  Reserved4;     /* Address 0x66: Reserved 4 */
   unsigned int  Reserved5;     /* Address 0x68: Reserved 5 */
   unsigned int  Reserved6;     /* Address 0x6C: Reserved 6 */
   unsigned int  PMDATASEL;     /* Address 0x70: Hidden 1 Power Management Data Select */
   unsigned int  PMDATASCALE;   /* Address 0x74: Hidden 2 Power Management Data Select */
 } PlxLocalConfRegMap;

/* ======================================================================= */
/* Our EPROM is the 93LC56B: Layout the 9030 configuration registers       */
/* Enumeration type giving names to the fields of the 9030 EPROM           */
/* See PCI 9030 Data Book Section 3-3 Serial EEPROM Register Load Sequence */

typedef enum {
   Plx9030DeviceId,                        /* Address 0x00 */
   Plx9030VendorId,                        /* Address 0x02 */
   Plx9030PciStatus,                       /* Address 0x04 */
   Plx9030PciCommand,                      /* Address 0x06 */
   Plx9030ClassCode,                       /* Address 0x08 */
   Plx9030ClassCode_Revision,              /* Address 0x0A */
   Plx9030SubsystemId,                     /* Address 0x0C */
   Plx9030SubsystemVendorId,               /* Address 0x0E */
   Plx9030MsbNewCapPntr,                   /* Address 0x10 */
   Plx9030LsbNewCapPntr,                   /* Address 0x12 */
   Plx9030MaxMinLatGrnt,                   /* Address 0x14 */
   Plx9030InterruptPin,                    /* Address 0x16 */
   Plx9030MswPowManCap,                    /* Address 0x18 */
   Plx9030LswPowManNxtCapPntr_PowManCapId, /* Address 0x1A */
   Plx9030MswPowManData_PmcrBridgeSupExt,  /* Address 0x1C */
   Plx9030LswPowManCntrl_Stat,             /* Address 0x1E */
   Plx9030MswHotSwapCntrl_Stat,            /* Address 0x20 */
   Plx9030LswHotSwapNxtCapPntr_Cntrl,      /* Address 0x22 */
   Plx9030PciVpdAddr,                      /* Address 0x24 */
   Plx9030PciVpdNxtCapPntr_Cntrl,          /* Address 0x26 */
   Plx9030MswLocAddrSpace0Range,           /* Address 0x28 */
   Plx9030LswLocAddrSpace0Range,           /* Address 0x2A */
   Plx9030MswLocAddrSpace1Range,           /* Address 0x2C */
   Plx9030LswLocAddrSpace1Range,           /* Address 0x2E */
   Plx9030MswLocAddrSpace2Range,           /* Address 0x30 */
   Plx9030LswLocAddrSpace2Range,           /* Address 0x32 */
   Plx9030MswLocAddrSpace3Range,           /* Address 0x34 */
   Plx9030LswLocAddrSpace3Range,           /* Address 0x36 */
   Plx9030MswExpRomRange,                  /* Address 0x38 */
   Plx9030LswExpRomRange,                  /* Address 0x3A */
   Plx9030MswLocAddrSpace0Remap,           /* Address 0x3C */
   Plx9030LswLocAddrSpace0Remap,           /* Address 0x3E */
   Plx9030MswLocAddrSpace1Remap,           /* Address 0x40 */
   Plx9030LswLocAddrSpace1Remap,           /* Address 0x42 */
   Plx9030MswLocAddrSpace2Remap,           /* Address 0x44 */
   Plx9030LswLocAddrSpace2Remap,           /* Address 0x46 */
   Plx9030MswLocAddrSpace3Remap,           /* Address 0x48 */
   Plx9030LswLocAddrSpace3Remap,           /* Address 0x4A */
   Plx9030MswExpRomRemap,                  /* Address 0x4C */
   Plx9030LswExpRomRemap,                  /* Address 0x4E */
   Plx9030MswLocAddrSpace0BusRgnDesc,      /* Address 0x50 */
   Plx9030LswLocAddrSpace0BusRgnDesc,      /* Address 0x52 */
   Plx9030MswLocAddrSpace1BusRgnDesc,      /* Address 0x54 */
   Plx9030LswLocAddrSpace1BusRgnDesc,      /* Address 0x56 */
   Plx9030MswLocAddrSpace2BusRgnDesc,      /* Address 0x58 */
   Plx9030LswLocAddrSpace2BusRgnDesc,      /* Address 0x5A */
   Plx9030MswLocAddrSpace3BusRgnDesc,      /* Address 0x5C */
   Plx9030LswLocAddrSpace3BusRgnDesc,      /* Address 0x5E */
   Plx9030MswExpRomBusRgnDesc,             /* Address 0x60 */
   Plx9030LswExpRomBusRgnDesc,             /* Address 0x62 */
   Plx9030MswChipSelectBaseAddr0,          /* Address 0x64 */
   Plx9030LswChipSelectBaseAddr0,          /* Address 0x66 */
   Plx9030MswChipSelectBaseAddr1,          /* Address 0x68 */
   Plx9030LswChipSelectBaseAddr1,          /* Address 0x6A */
   Plx9030MswChipSelectBaseAddr2,          /* Address 0x6C */
   Plx9030LswChipSelectBaseAddr2,          /* Address 0x6E */
   Plx9030MswChipSelectBaseAddr3,          /* Address 0x70 */
   Plx9030LswChipSelectBaseAddr3,          /* Address 0x72 */
   Plx9030SerialEpromWrtProtectAddrBnd,    /* Address 0x74 */
   Plx9030LswInterruptCntrl_Stat,          /* Address 0x76 */
   Plx9030MswPciTrgtRsp,                   /* Address 0x78 */
   Plx9030LswPciTrgtRsp,                   /* Address 0x7A */
   Plx9030MswGpIo,                         /* Address 0x7C */
   Plx9030LswGpIo,                         /* Address 0x7E */
   Plx9030MswHid1PowManDataSelct,          /* Address 0x80 */
   Plx9030LswHid1PowManDataSelct,          /* Address 0x82 */
   Plx9030MswHid2PowManDataScale,          /* Address 0x84 */
   Plx9030LswHid2PowManDataScale,          /* Address 0x86 */

   Plx9030EPROM_REGS                       /* Final EPROM Size is 0x86 bytes */
 } Plx9030EpromRegMap;

/* ======================================================================= */
/* Now declare the short array for the above EPROM fields                  */

typedef unsigned short Plx9030Eprom[Plx9030EPROM_REGS];

/* ======================================================================= */
/* Instruction set for the 93LC56B EEPROM from Microchip Technology Inc.   */
/* We write the above 9030 configuration fields to this EEPROM using the   */
/* 9030 Serial Control Register (CNTRL) See: 10.34-10.35 Bits 24..29       */
/* N.B VPD will not work. See errata #1 and chapter 9.                     */

typedef enum {
   EpCmdERASE = 0xE000, /* Sets all data bits at the specified address to 1 */
   EpCmdERAL  = 0x9000, /* Set all bits in the entire memory to 1           */
   EpCmdEWDS  = 0x8000, /* Close the device, disallow all ERASE and WRITES  */
   EpCmdEWEN  = 0x9800, /* Open the device, allow ERASE and WRITES          */
   EpCmdREAD  = 0xC000, /* Read data bits at specified address to D0 pin    */
   EpCmdWRITE = 0xA000, /* Write data bits from D1 pin to specified address */
   EpCmdWRAL  = 0x8800, /* Write all memory with specified data             */

   EpCmdCOMMANDS = 7
 } EpCmd;

#define EpAddMsk 0x0FC0  /* Address mask */
#define EpAddPos    12   /* Address position */
#define EpClkCycles 11   /* Number of clock cycles for a command */

#endif
