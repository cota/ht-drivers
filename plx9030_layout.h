/**
 * @file plx9030_layout.h
 *
 * @brief General definitions of PLX 9030 chip and mem layout for 93LC56B EPROM
 *
 * @author Julian Lewis CERN AB/CO/HT Geneva. Julian.Lewis@cern.ch
 *
 * @date Created on Nov, 2003
 */
#ifndef _PLX_9030_LAYOUT_H_INCLUDE_
#define _PLX_9030_LAYOUT_H_INCLUDE_


/*! @name PLX9030 Vendor/Device IDs */
//@{
#define PLX9030_VENDOR_ID 0x10B5
#define PLX9030_DEVICE_ID 0x9030
//@}


/** @brief */
typedef struct {
  unsigned short DeviceId;
  unsigned short VendorId;
} PlxRegPCIIDR;


/** @brief */
typedef struct {
   PlxRegPCIIDR   PCIIDR;       //!< Address 0x00: PCI Configuration ID
   unsigned short PCICR;        //!< Address 0x04: PCI Command
   unsigned short PCISR;        //!< Address 0x06: PCI Status
   unsigned char  PCIREV;       //!< Address 0x08: PCI Revision ID
   unsigned char  PCICCR0;      //!< Address 0x09: PCI Class code Byte 1
   unsigned char  PCICCR1;      //!< Address 0x0A: PCI Class code Byte 2
   unsigned char  PCICCR2;      //!< Address 0x0B: PCI Class code Byte 3
   unsigned char  PCICLSR;      //!< Address 0x0C: PCI Cash Line Size
   unsigned char  PCILTR;       //!< Address 0x0D: PCI Bus Latency Timer
   unsigned char  PCIHTR;       //!< Address 0x0E: PCI Header Type
   unsigned char  PCIBISTR;     //!< Address 0x0F: PCI PCI Built in Self Test
   unsigned long  PCIBAR0;      //!< Address 0x10: PCI BAR0 for Local configuration space mapped
   unsigned long  PCIBAR1;      //!< Address 0x14: PCI BAR1 for Local configuration space IO
   unsigned long  PCIBAR2;      //!< Address 0x18: PCI BAR2 Local address space 0
   unsigned long  PCIBAR3;      //!< Address 0x1C: PCI BAR3 Local address space 1
   unsigned long  PCIBAR4;      //!< Address 0x20: PCI BAR4 Local address space 2
   unsigned long  PCIBAR5;      //!< Address 0x24: PCI BAR5 Local address space 3
   unsigned long  PCICIS;       //!< Address 0x28: PCI Card bus info structure pointer
   unsigned short PCISVID;      //!< Address 0x2C: PCI Subsystem Vendor ID
   unsigned short PCISID;       //!< Address 0x2E: PCI Subsystem ID
   unsigned long  PCIERBAR;     //!< Address 0x30: PCI Expansion ROM Base address
   unsigned long  CAP_PTR;      //!< Address 0x34: New capability pointer
   unsigned long  Reserved0;    //!< Address 0x38: Reserved
   unsigned char  PCIILR;       //!< Address 0x3C: PCI Interrupt Line
   unsigned char  PCIIPR;       //!< Address 0x3D: PCI Interrupt Pin
   unsigned char  PCIMGR;       //!< Address 0x3E: PCI Minimum Grant
   unsigned char  PCIMLR;       //!< Address 0x3F: PCI Maximum Latency
   unsigned char  PMCCAPID;     //!< Address 0x40: Power Management capability ID
   unsigned char  PMNEXT;       //!< Address 0x41: Power Management capability Pointer
   unsigned short PMC;          //!< Address 0x42: Power Management capabilities
   unsigned short PMCSR;        //!< Address 0x44: Power Management Control/Status
   unsigned char  PMCSR_BSE;    //!< Address 0x46: PMC Bridge Support Extensisions
   unsigned char  PMDATA;       //!< Address 0x47: Power Management Data
   unsigned char  HS_CNTL;      //!< Address 0x48: Hot Swap Control
   unsigned char  HS_NEXT;      //!< Address 0x49: Hot Swap Next Capability Pointer
   unsigned short HS_CSR;       //!< Address 0x4A: Hot Swap Control Status
   unsigned char  PVPDCNTL;     //!< Address 0x4C: PCI Vital Product Data Control
   unsigned char  PVPD_NEXT;    //!< Address 0x4D: PCI VPD Next Capability pointer
   unsigned short PVPDAD;       //!< Address 0x4E: PCI VPD Data Address
   unsigned long  PVPDATA;      //!< Address 0x50: PCI VPD Data
 } PlxPciConfRegMap;


/** @brief Define the structure of the PLX 9030 Registers
 *
 * See PCI 9030 Data Book Section 10-3 Local Configuration Address Mapping
 */
typedef struct {
  unsigned long  LAS0RR;      //!< Address 0x00: Local Address Space 0 Range
  unsigned long  LAS1RR;      //!< Address 0x04: Local Address Space 1 Range
  unsigned long  LAS2RR;      //!< Address 0x08: Local Address Space 2 Range
  unsigned long  LAS3RR;      //!< Address 0x0C: Local Address Space 3 Range
  unsigned long  EROMRR;      //!< Address 0x10: Expansion ROM Range
  unsigned long  LAS0BA;      //!< Address 0x14: Local Address Space 0 Local Base Address (Remap)
  unsigned long  LAS1BA;      //!< Address 0x18: Local Address Space 1 Local Base Address (Remap)
  unsigned long  LAS2BA;      //!< Address 0x1C: Local Address Space 2 Local Base Address (Remap)
  unsigned long  LAS3BA;      //!< Address 0x20: Local Address Space 3 Local Base Address (Remap)
  unsigned long  EROMBA;      //!< Address 0x24: Expansion ROM Local Base Address (Remap)
  unsigned long  LAS0BRD;     //!< Address 0x28: Local Address Space 0 Bus Region Descriptor
  unsigned long  LAS1BRD;     //!< Address 0x2C: Local Address Space 1 Bus Region Descriptor
  unsigned long  LAS2BRD;     //!< Address 0x30: Local Address Space 2 Bus Region Descriptor
  unsigned long  LAS3BRD;     //!< Address 0x34: Local Address Space 3 Bus Region Descriptor
  unsigned long  EROMBRD;     //!< Address 0x38: Expansion ROM Bus Region Descriptor
  unsigned long  CS0BASE;     //!< Address 0x3C: Chip Select 0 Base Address
  unsigned long  CS1BASE;     //!< Address 0x40: Chip Select 1 Base Address
  unsigned long  CS2BASE;     //!< Address 0x44: Chip Select 2 Base Address
  unsigned long  CS3BASE;     //!< Address 0x48: Chip Select 3 Base Address
  unsigned short INTCSR;      //!< Address 0x4C: Interrupt Control/Status
  unsigned short PROT_AREA;   //!< Address 0x4E: Serial EEPROM Write-Protected Address Boundary
  unsigned long  CNTRL;       //!< Address 0x50: PCI Target Response, Serial EEPROM and Initialization Control
  unsigned long  GPIOC;       //!< Address 0x54: General Purpose IO Control
  unsigned long  Reserved0;   //!< Address 0x58: Reserved 0
  unsigned long  Reserved1;   //!< Address 0x5C: Reserved 1
  unsigned long  Reserved2;   //!< Address 0x60: Reserved 2
  unsigned long  Reserved3;   //!< Address 0x64: Reserved 3
  unsigned long  Reserved4;   //!< Address 0x66: Reserved 4
  unsigned long  Reserved5;   //!< Address 0x68: Reserved 5
  unsigned long  Reserved6;   //!< Address 0x6C: Reserved 6
  unsigned long  PMDATASEL;   //!< Address 0x70: Hidden 1 Power Management Data Select
  unsigned long  PMDATASCALE; //!< Address 0x74: Hidden 2 Power Management Data Select
} PlxLocalConfRegMap;


/** @brief Our EPROM is the 93LC56B
 *
 * Layout the 9030 configuration registers.
 * Enumeration type giving names to the fields of the 9030 EPROM
 * See PCI 9030 Data Book Section 3-3 Serial EEPROM Register Load Sequence
 */
typedef enum {
   Plx9030DeviceId,                        //!< Address 0x00
   Plx9030VendorId,                        //!< Address 0x02
   Plx9030PciStatus,                       //!< Address 0x04
   Plx9030PciCommand,                      //!< Address 0x06
   Plx9030ClassCode,                       //!< Address 0x08
   Plx9030ClassCode_Revision,              //!< Address 0x0A
   Plx9030SubsystemId,                     //!< Address 0x0C
   Plx9030SubsystemVendorId,               //!< Address 0x0E
   Plx9030MsbNewCapPntr,                   //!< Address 0x10
   Plx9030LsbNewCapPntr,                   //!< Address 0x12
   Plx9030MaxMinLatGrnt,                   //!< Address 0x14
   Plx9030InterruptPin,                    //!< Address 0x16
   Plx9030MswPowManCap,                    //!< Address 0x18
   Plx9030LswPowManNxtCapPntr_PowManCapId, //!< Address 0x1A
   Plx9030MswPowManData_PmcrBridgeSupExt,  //!< Address 0x1C
   Plx9030LswPowManCntrl_Stat,             //!< Address 0x1E
   Plx9030MswHotSwapCntrl_Stat,            //!< Address 0x20
   Plx9030LswHotSwapNxtCapPntr_Cntrl,      //!< Address 0x22
   Plx9030PciVpdAddr,                      //!< Address 0x24
   Plx9030PciVpdNxtCapPntr_Cntrl,          //!< Address 0x26
   Plx9030MswLocAddrSpace0Range,           //!< Address 0x28
   Plx9030LswLocAddrSpace0Range,           //!< Address 0x2A
   Plx9030MswLocAddrSpace1Range,           //!< Address 0x2C
   Plx9030LswLocAddrSpace1Range,           //!< Address 0x2E
   Plx9030MswLocAddrSpace2Range,           //!< Address 0x30
   Plx9030LswLocAddrSpace2Range,           //!< Address 0x32
   Plx9030MswLocAddrSpace3Range,           //!< Address 0x34
   Plx9030LswLocAddrSpace3Range,           //!< Address 0x36
   Plx9030MswExpRomRange,                  //!< Address 0x38
   Plx9030LswExpRomRange,                  //!< Address 0x3A
   Plx9030MswLocAddrSpace0Remap,           //!< Address 0x3C
   Plx9030LswLocAddrSpace0Remap,           //!< Address 0x3E
   Plx9030MswLocAddrSpace1Remap,           //!< Address 0x40
   Plx9030LswLocAddrSpace1Remap,           //!< Address 0x42
   Plx9030MswLocAddrSpace2Remap,           //!< Address 0x44
   Plx9030LswLocAddrSpace2Remap,           //!< Address 0x46
   Plx9030MswLocAddrSpace3Remap,           //!< Address 0x48
   Plx9030LswLocAddrSpace3Remap,           //!< Address 0x4A
   Plx9030MswExpRomRemap,                  //!< Address 0x4C
   Plx9030LswExpRomRemap,                  //!< Address 0x4E
   Plx9030MswLocAddrSpace0BusRgnDesc,      //!< Address 0x50
   Plx9030LswLocAddrSpace0BusRgnDesc,      //!< Address 0x52
   Plx9030MswLocAddrSpace1BusRgnDesc,      //!< Address 0x54
   Plx9030LswLocAddrSpace1BusRgnDesc,      //!< Address 0x56
   Plx9030MswLocAddrSpace2BusRgnDesc,      //!< Address 0x58
   Plx9030LswLocAddrSpace2BusRgnDesc,      //!< Address 0x5A
   Plx9030MswLocAddrSpace3BusRgnDesc,      //!< Address 0x5C
   Plx9030LswLocAddrSpace3BusRgnDesc,      //!< Address 0x5E
   Plx9030MswExpRomBusRgnDesc,             //!< Address 0x60
   Plx9030LswExpRomBusRgnDesc,             //!< Address 0x62
   Plx9030MswChipSelectBaseAddr0,          //!< Address 0x64
   Plx9030LswChipSelectBaseAddr0,          //!< Address 0x66
   Plx9030MswChipSelectBaseAddr1,          //!< Address 0x68
   Plx9030LswChipSelectBaseAddr1,          //!< Address 0x6A
   Plx9030MswChipSelectBaseAddr2,          //!< Address 0x6C
   Plx9030LswChipSelectBaseAddr2,          //!< Address 0x6E
   Plx9030MswChipSelectBaseAddr3,          //!< Address 0x70
   Plx9030LswChipSelectBaseAddr3,          //!< Address 0x72
   Plx9030SerialEpromWrtProtectAddrBnd,    //!< Address 0x74
   Plx9030LswInterruptCntrl_Stat,          //!< Address 0x76
   Plx9030MswPciTrgtRsp,                   //!< Address 0x78
   Plx9030LswPciTrgtRsp,                   //!< Address 0x7A
   Plx9030MswGpIo,                         //!< Address 0x7C
   Plx9030LswGpIo,                         //!< Address 0x7E
   Plx9030MswHid1PowManDataSelct,          //!< Address 0x80
   Plx9030LswHid1PowManDataSelct,          //!< Address 0x82
   Plx9030MswHid2PowManDataScale,          //!< Address 0x84
   Plx9030LswHid2PowManDataScale,          //!< Address 0x86

   Plx9030EPROM_REGS                       //!< Final EPROM Size is 0x86 bytes
 } Plx9030EpromRegMap;


#define Plx9030NAME_SIZE 32
#define Plx9030COMMENT_SIZE 64

#define Plx9030CONFIG_REGS 40
#define Plx9030LOCAL_REGS 32


/** @brief plx register description */
typedef struct {
   unsigned long FieldSize;
   char          Name[Plx9030NAME_SIZE];
   unsigned long Offset;
   char          Comment[Plx9030COMMENT_SIZE];
 } Plx9030Field;

static Plx9030Field __attribute__((unused)) Configs[Plx9030CONFIG_REGS] = {
   { 4, "PCIIDR", 0x00, "PCI Device ID & Vendor ID" },
   { 2, "PCICR",     0x04, "PCI Command" },
   { 2, "PCISR",     0x06, "PCI Status" },
   { 1, "PCIREV",    0x08, "PCI Revision ID" },
   { 1, "PCICCR0",   0x09, "PCI Class code Byte 1" },
   { 1, "PCICCR1",   0x0A, "PCI Class code Byte 2" },
   { 1, "PCICCR2",   0x0B, "PCI Class code Byte 3" },
   { 1, "PCICLSR",   0x0C, "PCI Cash Line Size" },
   { 1, "PCILTR",    0x0D, "PCI Bus Latency Timer" },
   { 1, "PCIHTR",    0x0E, "PCI Header Type" },
   { 1, "PCIBISTR",  0x0F, "PCI PCI Built in Self Test" },
   { 4, "PCIBAR0",   0x10, "PCI BAR0 for Local configuration space mapped" },
   { 4, "PCIBAR1",   0x14, "PCI BAR1 for Local configuration space IO" },
   { 4, "PCIBAR2",   0x18, "PCI BAR2 Local address space 0" },
   { 4, "PCIBAR3",   0x1C, "PCI BAR3 Local address space 1" },
   { 4, "PCIBAR4",   0x20, "PCI BAR4 Local address space 2" },
   { 4, "PCIBAR5",   0x24, "PCI BAR5 Local address space 3" },
   { 4, "PCICIS",    0x28, "PCI Card bus info structure pointer" },
   { 2, "PCISVID",   0x2C, "PCI Subsystem Vendor ID" },
   { 2, "PCISID",    0x2E, "PCI Subsystem ID" },
   { 4, "PCIERBAR",  0x30, "PCI Expansion ROM Base address" },
   { 4, "CAP_PTR",   0x34, "New capability pointer" },
   { 4, "Reserved0", 0x38, "Reserved" },
   { 1, "PCIILR",    0x3C, "PCI Interrupt Line" },
   { 1, "PCIIPR",    0x3D, "PCI Interrupt Pin" },
   { 1, "PCIMGR",    0x3E, "PCI Minimum Grant" },
   { 1, "PCIMLR",    0x3F, "PCI Maximum Latency" },
   { 1, "PMCCAPID",  0x40, "Power Management capability ID" },
   { 1, "PMNEXT",    0x41, "Power Management capability Pointer" },
   { 2, "PMC",       0x42, "Power Management capabilities" },
   { 2, "PMCSR",     0x44, "Power Management Control/Status" },
   { 1, "PMCSR_BSE", 0x46, "PMC Bridge Support Extensisions" },
   { 1, "PMDATA",    0x47, "Power Management Data" },
   { 1, "HS_CNTL",   0x48, "Hot Swap Control" },
   { 1, "HS_NEXT",   0x49, "Hot Swap Next Capability Pointer" },
   { 2, "HS_CSR",    0x4A, "Hot Swap Control Status" },
   { 1, "PVPDCNTL",  0x4C, "PCI Vital Product Data Control" },
   { 1, "PVPD_NEXT", 0x4D, "PCI VPD Next Capability pointer" },
   { 2, "PVPDAD",    0x4E, "PCI VPD Data Address" },
   { 4, "PVPDATA",   0x50, "PCI VPD Data" }
 };

static Plx9030Field __attribute__((unused)) Locals[Plx9030LOCAL_REGS] = {
   { 4, "LAS0RR",    0x00, "Local Address Space 0 Range" },
   { 4, "LAS1RR",    0x04, "Local Address Space 1 Range" },
   { 4, "LAS2RR",    0x08, "Local Address Space 2 Range" },
   { 4, "LAS3RR",    0x0C, "Local Address Space 3 Range" },
   { 4, "EROMRR",    0x10, "Expansion ROM Range" },
   { 4, "LAS0BA",    0x14, "Local Address Space 0 Local Base Address (Remap)" },
   { 4, "LAS1BA",    0x18, "Local Address Space 1 Local Base Address (Remap)" },
   { 4, "LAS2BA",    0x1C, "Local Address Space 2 Local Base Address (Remap)" },
   { 4, "LAS3BA",    0x20, "Local Address Space 3 Local Base Address (Remap)" },
   { 4, "EROMBA",    0x24, "Expansion ROM Local Base Address (Remap)" },
   { 4, "LAS0BRD",   0x28, "Local Address Space 0 Bus Region Descriptor" },
   { 4, "LAS1BRD",   0x2C, "Local Address Space 1 Bus Region Descriptor" },
   { 4, "LAS2BRD",   0x30, "Local Address Space 2 Bus Region Descriptor" },
   { 4, "LAS3BRD",   0x34, "Local Address Space 3 Bus Region Descriptor" },
   { 4, "EROMBRD",   0x38, "Expansion ROM Bus Region Descriptor" },
   { 4, "CS0BASE",   0x3C, "Chip Select 0 Base Address" },
   { 4, "CS1BASE",   0x40, "Chip Select 1 Base Address" },
   { 4, "CS2BASE",   0x44, "Chip Select 2 Base Address" },
   { 4, "CS3BASE",   0x48, "Chip Select 3 Base Address" },
   { 2, "INTCSR",    0x4C, "Interrupt Control/Status" },
   { 2, "PROT_AREA", 0x4E, "Serial EEPROM Write-Protected Address Boundary" },
   { 4, "CNTRL",     0x50, "PCI Target Response, Serial EEPROM and "
                           "Initialization Control" },
   { 4, "GPIOC",     0x54, "General Purpose IO Control" },
   { 4, "Reserved0", 0x58, "Reserved 0" },
   { 4, "Reserved1", 0x5C, "Reserved 1" },
   { 4, "Reserved2", 0x60, "Reserved 2" },
   { 4, "Reserved3", 0x64, "Reserved 3" },
   { 4, "Reserved4", 0x66, "Reserved 4" },
   { 4, "Reserved5", 0x68, "Reserved 5" },
   { 4, "Reserved6", 0x6C, "Reserved 6" },
   { 4, "PMDATASEL", 0x70, "Hidden 1 Power Management Data Select" },
   { 4, "PMDATASCALE",0x74, "Hidden 2 Power Management Data Select" }
 };

static Plx9030Field __attribute__((unused)) Eprom[Plx9030EPROM_REGS] = {
   { 2, "PCIIDR[31:16]"               ,0x00, "PCI Device Identifier                       " },
   { 2, "PCIIDR[15:0]"                ,0x02, "PCI Vendor Identifrier                      " },
   { 2, "PCISR[15:0]"                 ,0x04, "PCI Status word                             " },
   { 2, "Reserved"                    ,0x06, "PCI Command word                            " },
   { 2, "PCICCR[15:0]"                ,0x08, "Class Code                                  " },
   { 2, "PCICCR[7:0]/PCIREV[7:0]"     ,0x0A, "Class Code & Revision                       " },
   { 2, "PCISID[15:0]"                ,0x0C, "Subsystem Identifier                        " },
   { 2, "PCISVID[15:0]"               ,0x0E, "Subsystem Vendor Identifier                 " },
   { 2, "Reserved"                    ,0x10, "MSB New Capabilities Pointer                " },
   { 2, "CAP_PTR[7:0]"                ,0x12, "LSB New Capabilities Pointer                " },
   { 2, "Reserved"                    ,0x14, "MAX MIN Latency Grant                       " },
   { 2, "PCIIPR[7:0]/PCIILR[7:0]"     ,0x16, "Interrupt Pin number & Routing              " },
   { 2, "PMC[15:11,5,3:0]"            ,0x18, "MSW Power Management Capabilities           " },
   { 2, "PMNEXT[7:0]/PMCAPID[7:0]"    ,0x1A, "LSW Power ManNxtCapPointer & Power ManCapId " },
   { 2, "Reserved"                    ,0x1C, "MSW Power ManData  & PmcrBridgeSupExt       " },
   { 2, "PMCSR[14:8]"                 ,0x1E, "LSW Power Management Control & Status       " },
   { 2, "Reserved"                    ,0x20, "MSW Hot Swap Control & Status               " },
   { 2, "HS_NEXT[7:0]/PMCAPID[7:0]"   ,0x22, "LSW Hot Swap Next CapPointer & Control      " },
   { 2, "Reserved"                    ,0x24, "PCI Vital Product Data Address              " },
   { 2, "PVD_NEXT[7:0]/PVDCNTL[7:0]"  ,0x26, "PCI Vpd NxtCapPointer & Control             " },
   { 2, "LAS0RR[31:16]"               ,0x28, "MSW Local Address Space 0 Range             " },
   { 2, "LAS0RR[15:0]"                ,0x2A, "LSW Local Address Space 0 Range             " },
   { 2, "LAS1RR[31:16]"               ,0x2C, "MSW Local Address Space 1 Range             " },
   { 2, "LAS1RR[15:0]"                ,0x2E, "LSW Local Address Space 1 Range             " },
   { 2, "LAS2RR[31:16]"               ,0x30, "MSW Local Address Space 2 Range             " },
   { 2, "LAS2RR[15:0]"                ,0x32, "LSW Local Address Space 2 Range             " },
   { 2, "LAS3RR[31:16]"               ,0x34, "MSW Local Address Space 3 Range             " },
   { 2, "LAS3RR[15:0]"                ,0x36, "LSW Local Address Space 3 Range             " },
   { 2, "EROMRR[31:16]"               ,0x38, "MSW Expantion Rom Range                     " },
   { 2, "EROMRR[15:0]"                ,0x3A, "LSW Expantion Rom Range                     " },
   { 2, "LAS0BA[31:16]"               ,0x3C, "MSW Local Address Space 0 Remap             " },
   { 2, "LAS0BA[15:0]"                ,0x3E, "LSW Local Address Space 0 Remap             " },
   { 2, "LAS1BA[31:16]"               ,0x40, "MSW Local Address Space 1 Remap             " },
   { 2, "LAS1BA[15:0]"                ,0x42, "LSW Local Address Space 1 Remap             " },
   { 2, "LAS2BA[31:16]"               ,0x44, "MSW Local Address Space 2 Remap             " },
   { 2, "LAS2BA[15:0]"                ,0x46, "LSW Local Address Space 2 Remap             " },
   { 2, "LAS3BA[31:16]"               ,0x48, "MSW Local Address Space 3 Remap             " },
   { 2, "LAS3BA[15:0]"                ,0x4A, "LSW Local Address Space 3 Remap             " },
   { 2, "EROMBA[31:16]"               ,0x4C, "MSW Expansion Rom Remap                     " },
   { 2, "EROMBA[15:0]"                ,0x4E, "LSW Expansion Rom Remap                     " },
   { 2, "LAS0BRD[31:16]"              ,0x50, "MSW Local Address Space 0 Bus Region Desc   " },
   { 2, "LAS0BRD[15:0]"               ,0x52, "LSW Local Address Space 0 Bus Region Desc   " },
   { 2, "LAS1BRD[31:16]"              ,0x54, "MSW Local Address Space 1 Bus Region Desc   " },
   { 2, "LAS1BRD[15:0]"               ,0x56, "LSW Local Address Space 1 Bus Region Desc   " },
   { 2, "LAS2BRD[31:16]"              ,0x58, "MSW Local Address Space 2 Bus Region Desc   " },
   { 2, "LAS2BRD[15:0]"               ,0x5A, "LSW Local Address Space 2 Bus Region Desc   " },
   { 2, "LAS3BRD[31:16]"              ,0x5C, "MSW Local Address Space 3 Bus Region Desc   " },
   { 2, "LAS3BRD[15:0]"               ,0x5E, "LSW Local Address Space 3 Bus Region Desc   " },
   { 2, "EROMBRD[31:16]"              ,0x60, "MSW Expansion Rom Bus Region Descriptor     " },
   { 2, "EROMBRD[15:0]"               ,0x62, "LSW Expansion Rom Bus Region Descriptor     " },
   { 2, "CS0BASE[31:16]"              ,0x64, "MSW Chip Select Base Address 0              " },
   { 2, "CS0BASE[15:0]"               ,0x66, "LSW Chip Select Base Address 0              " },
   { 2, "CS1BASE[31:16]"              ,0x68, "MSW Chip Select Base Address 1              " },
   { 2, "CS1BASE[15:0]"               ,0x6A, "LSW Chip Select Base Address 1              " },
   { 2, "CS2BASE[31:16]"              ,0x6C, "MSW Chip Select Base Address 2              " },
   { 2, "CS2BASE[15:0]"               ,0x6E, "LSW Chip Select Base Address 2              " },
   { 2, "CS3BASE[31:16]"              ,0x70, "MSW Chip Select Base Address 3              " },
   { 2, "CS3BASE[15:0]"               ,0x72, "LSW Chip Select Base Address 3              " },
   { 2, "PROT_AREA[7:0]"              ,0x74, "Serial Eprom Write Protect Address Boundary " },
   { 2, "INTCSR[15:0]"                ,0x76, "LSW Interrupt Control & Status              " },
   { 2, "CNTRL[31:16]"                ,0x78, "MSW PCI Target Response                     " },
   { 2, "CNTRL[15:0]"                 ,0x7A, "LSW PCI Target Response                     " },
   { 2, "GPIOC[31:16]"                ,0x7C, "MSW GpIo General purpose IO                 " },
   { 2, "GPIOC[15:0]"                 ,0x7E, "LSW GpIo General purpose IO                 " },
   { 2, "PMDATA[7:0] Disipated"       ,0x80, "MSW Hidden 1 Power Management Data Select   " },
   { 2, "PMDATA[7:0] Consumed"        ,0x82, "LSW Hidden 1 Power Management Data Select   " },
   { 2, "Reserved"                    ,0x84, "MSW Hidden 2 Power Management Data Scale    " },
   { 2, "PMCSR[14:13,7:0]"            ,0x86, "LSW Hidden 2 Power Management Data Scale    " }
 };

//!< Now declare the short array for the above EPROM fields
typedef unsigned short Plx9030Eprom_t[Plx9030EPROM_REGS];

#endif /* _PLX_9030_LAYOUT_H_INCLUDE_ */
