/**
 * @file plx9656_layout.h
 *
 * @brief Layout of the PLX9656 PCI Controller Chip.
 *
 * @author Julian Lewis AB/CO/HT CERN
 *
 * @date Created on 26/11/2003
 */

#ifndef PLX_9656
#define PLX_9656


/*! @name structure of the PLX 9656 Configuration Registers.
 */
//@{
#define PLX9656_VENDOR_ID 0x10B5
#define PLX9656_DEVICE_ID 0x9656

typedef struct {
  uint16_t DeviceId;
  uint16_t VendorId;
} __attribute__ ((packed)) PlxRegPCIIDR;

typedef enum {
  //!< Allows I/O space access (I/O Accelerator)
  PlxPcicrIO_SPACE       = 0x0001,

  //!< Allows memory space access (I/O Accelerator)
  PlxPcicrMEMORY_SPACE   = 0x0002,

  PlxPcicrMASTER_ENABLE  = 0x0008, //!< Allows bus master behaviour
  PlxPcicrMEMORY_WRITE   = 0x0010, //!< Allows DMA
  PlxPcicrPARITY_ERROR   = 0x0040, //!< Forces parity error checking
  PlxPcicrSERR           = 0x0100, //!< Enable SERR driver
  PlxPcicrFAST_BACK      = 0x0200  //!< Type of master fast back to back
} PlxPcicr;

typedef enum {
  //!< New capabilities, must be zero for VMIC5565
  PlxPcisrNEW_CAP        = 0x0010,

  //!< 66MHz, on the CES power PC this must be zero
  PlxPcisr66MHZ          = 0x0020,
  PlxPcisrUSER_DEFINED   = 0x0040, //!< Enable user defined functions

  //!< Indicates fast back to back, its always a 1
  PlxPcisrFAST_BACK      = 0x0080,
  PlxPcisrMPARITY_ERROR  = 0x0100, //!< Master parity error detected
  PlxPcisrDEVSEL_0       = 0x0200, //!< DEVSEL Timing set to medium always 1
  PlxPcisrDEVSEL_1       = 0x0400, //!< DEVSEL Timing set to medium always 0

  //!< Indicates target abort, cleared by writing 1
  PlxPcisrTARGET_ABORT   = 0x0800,

  //!< Received a target abort, cleared by writing 1
  PlxPcisrRTARGET_ABORT  = 0x1000,

  //!< Received a master abort, cleared by writing 1
  PlxPcisrRMTARGET_ABORT = 0x2000,
  PlxPcisrSERR           = 0x4000, //!< System error, clearer by writing 1

  //!< Parity error detected , cleared by writing 1
  PlxPcisrPARITY_ERROR   = 0x8000
} PlxPcisr;

typedef struct {
  PlxRegPCIIDR PCIIDR;	//!< 0x00: PCI Configuration ID
  uint16_t PCICR;	//!< 0x04: PCI Command
  uint16_t PCISR;	//!< 0x06: PCI Status
  uint8_t  PCIREV;	//!< 0x08: PCI Revision ID
  uint8_t  PCICCR0;	//!< 0x09: PCI Class code Byte 1
  uint8_t  PCICCR1;	//!< 0x0A: PCI Class code Byte 2
  uint8_t  PCICCR2;	//!< 0x0B: PCI Class code Byte 3
  uint8_t  PCICLSR;	//!< 0x0C: PCI Cash Line Size
  uint8_t  PCILTR;	//!< 0x0D: PCI Bus Latency Timer
  uint8_t  PCIHTR;	//!< 0x0E: PCI Header Type
  uint8_t  PCIBISTR;	//!< 0x0F: PCI PCI Built in Self Test
  uint32_t PCIBAR0;	//!< 0x10: PCI BAR0 for Local configuration space mapped
  uint32_t PCIBAR1;	//!< 0x14: PCI BAR1 for Local configuration space IO
  uint32_t PCIBAR2;	//!< 0x18: PCI BAR2 Local address space 0
  uint32_t PCIBAR3;	//!< 0x1C: PCI BAR3 Local address space 1
  uint32_t PCIBAR4;	//!< 0x20: PCI BAR4 Local address space 2
  uint32_t PCIBAR5;	//!< 0x24: PCI BAR5 Local address space 3
  uint32_t PCICIS;	//!< 0x28: PCI Card bus info structure pointer
  uint16_t PCISVID;	//!< 0x2C: PCI Subsystem Vendor ID
  uint16_t PCISID;	//!< 0x2E: PCI Subsystem ID
  uint32_t PCIERBAR;	//!< 0x30: PCI Expansion ROM Base address
  uint32_t CAP_PTR;	//!< 0x34: New capability pointer
  uint32_t Addr38;	//!< 0x38: Reserved
  uint8_t  PCIILR;	//!< 0x3C: PCI Interrupt Line
  uint8_t  PCIIPR;	//!< 0x3D: PCI Interrupt Pin
  uint8_t  PCIMGR;	//!< 0x3E: PCI Minimum Grant
  uint8_t  PCIMLR;	//!< 0x3F: PCI Maximum Latency
  uint8_t  PMCCAPID;	//!< 0x40: Power Management capability ID
  uint8_t  PMNEXT;	//!< 0x41: Power Management capability Pointer
  uint16_t PMC;		//!< 0x42: Power Management capabilities
  uint16_t PMCSR;	//!< 0x44: Power Management Control/Status
  uint8_t  PMCSR_BSE;	//!< 0x46: PMC Bridge Support Extensisions
  uint8_t  PMDATA;	//!< 0x47: Power Management Data
  uint8_t  HS_CNTL;	//!< 0x48: Hot Swap Control
  uint8_t  HS_NEXT;	//!< 0x49: Hot Swap Next Capability Pointer
  uint16_t HS_CSR;	//!< 0x4A: Hot Swap Control Status
  uint8_t  PVPDCNTL;	//!< 0x4C: PCI Vital Product Data Control
  uint8_t  PVPD_NEXT;	//!< 0x4D: PCI VPD Next Capability pointer
  uint16_t PVPDAD;	//!< 0x4E: PCI VPD Data Address
  uint32_t PVPDATA;	//!< 0x50: PCI VPD Data
} __attribute__ ((packed)) PlxConfigMap;
//@}

/*! @name Configuration register offsets
 */
//@{
typedef enum {
  PlxConfigPCIIDR	= 0x00, //!< PCI Configuration ID
  PlxConfigPCICR        = 0x04, //!< PCI Command
  PlxConfigPCISR        = 0x06, //!< PCI Status
  PlxConfigPCIREV       = 0x08, //!< PCI Revision ID
  PlxConfigPCICCR0      = 0x09, //!< PCI Class code Byte 1
  PlxConfigPCICCR1      = 0x0A, //!< PCI Class code Byte 2
  PlxConfigPCICCR2      = 0x0B, //!< PCI Class code Byte 3
  PlxConfigPCICLSR      = 0x0C, //!< PCI Cache Line Size
  PlxConfigPCILTR       = 0x0D, //!< PCI Bus Latency Timer
  PlxConfigPCIHTR       = 0x0E, //!< PCI Header Type
  PlxConfigPCIBISTR     = 0x0F, //!< PCI PCI Built in Self Test
  PlxConfigPCIBAR0      = 0x10, //!< PCI BAR0 for Local config space mapped
  PlxConfigPCIBAR1      = 0x14, //!< PCI BAR1 for Local config space IO
  PlxConfigPCIBAR2      = 0x18, //!< PCI BAR2 Local address space 0
  PlxConfigPCIBAR3      = 0x1C, //!< PCI BAR3 Local address space 1
  PlxConfigPCIBAR4      = 0x20, //!< PCI BAR4 Local address space 2
  PlxConfigPCIBAR5      = 0x24, //!< PCI BAR5 Local address space 3
  PlxConfigPCICIS       = 0x28, //!< PCI Card bus info structure pointer
  PlxConfigPCISVID      = 0x2C, //!< PCI Subsystem Vendor ID
  PlxConfigPCISID       = 0x2E, //!< PCI Subsystem ID
  PlxConfigPCIERBAR     = 0x30, //!< PCI Expansion ROM Base address
  PlxConfigCAP_PTR      = 0x34, //!< New capability pointer
  PlxConfigAddr38       = 0x38, //!< Reserved
  PlxConfigPCIILR       = 0x3C, //!< PCI Interrupt Line
  PlxConfigPCIIPR       = 0x3D, //!< PCI Interrupt Pin
  PlxConfigPCIMGR       = 0x3E, //!< PCI Minimum Grant
  PlxConfigPCIMLR       = 0x3F, //!< PCI Maximum Latency
  PlxConfigPMCCAPID     = 0x40, //!< Power Management capability ID
  PlxConfigPMNEXT       = 0x41, //!< Power Management capability Pointer
  PlxConfigPMC          = 0x42, //!< Power Management capabilities
  PlxConfigPMCSR        = 0x44, //!< Power Management Control/Status
  PlxConfigPMCSR_BSE    = 0x46, //!< PMC Bridge Support Extensisions
  PlxConfigPMDATA       = 0x47, //!< Power Management Data
  PlxConfigHS_CNTL      = 0x48, //!< Hot Swap Control
  PlxConfigHS_NEXT      = 0x49, //!< Hot Swap Next Capability Pointer
  PlxConfigHS_CSR       = 0x4A, //!< Hot Swap Control Status
  PlxConfigPVPDCNTL     = 0x4C, //!< PCI Vital Product Data Control
  PlxConfigPVPD_NEXT    = 0x4D, //!< PCI VPD Next Capability pointer
  PlxConfigPVPDAD       = 0x4E, //!< PCI VPD Data Address
  PlxConfigPVPDATA      = 0x50  //!< PCI VPD Data
} PlxConfig;
//@}

/*! @name structure of the PLX 9656 Local Configuration Registers.
 */
//@{
typedef enum {
  PlxIntcsrENABLE_0		= 0x00000001, //!< Enable Interrupt sources 0
  PlxIntcsrENABLE_1		= 0x00000002, //!< Enable Interrupt sources 1
  PlxIntcsrGEN_SERR		= 0x00000004, //!< Generate a SERR interrupt
  PlxIntcsrENABLE_MBOX		= 0x00000008, //!< Enable Mailbox interrupts

  //!< Enable power manager interrupts
  PlxIntcsrENABLE_PMAN		= 0x00000010,

  //!< Status of power manager interrupt
  PlxIntcsrSTATUS_PMAN		= 0x00000020,

  //!< Enable parity error intrerrupts
  PlxIntcsrENABLE_PERR		= 0x00000040,

  PlxIntcsrSTATUS_PERR		= 0x00000080, //!< Status of parity error
  PlxIntcsrENABLE_PCI		= 0x00000100, //!< Enable PCI interrupts
  PlxIntcsrENABLE_PCI_DBELL	= 0x00000200, //!< Enable Doorbell PCI Interrupt
  PlxIntcsrENABLE_PCI_ABORT	= 0x00000400, //!< Enable PCI abort interrupts
  PlxIntcsrENABLE_LOCAL		= 0x00000800, //!< Enable local interrupt inputs
  PlxIntcsrRETRY_ABORT		= 0x00001000, //!< Retry upto 256 times
  PlxIntcsrSTATUS_DBELL		= 0x00002000, //!< Doorbell interrupt active
  PlxIntcsrSTATUS_PCI_ABORT	= 0x00004000, //!< PCI abort interrupt active
  PlxIntcsrSTATUS_LOCAL		= 0x00008000, //!< Local interrupt active
  PlxIntcsrENABLE_LOCAL_OUT	= 0x00010000, //!< Enable local interrupt out

  //!< Enable local doorbell interrupts
  PlxIntcsrENABLE_LOCAL_DBELL	= 0x00020000,

  //!< Enable DMA channel 0 interrupts
  PlxIntcsrENABLE_DMA_CHAN_0	= 0x00040000,

  //!< Enable DMA channel 1 interrupts
  PlxIntcsrENABLE_DMA_CHAN_1	= 0x00080000,

  //!< Status of local doorbell interrupt
  PlxIntcsrSTATUS_LOCAL_DBELL	= 0x00100000,

  //!< Status of DMA channel 0 interrupt
  PlxIntcsrSTATUS_DMA_CHAN_0	= 0x00200000,

  //!< Status of DMA channel 1 interrupt
  PlxIntcsrSTATUS_DMA_CHAN_1	= 0x00400000,

  //!< Status of bilt in self test interrupt
  PlxIntcsrSTATUS_BIST		= 0x00800000,

  PlxIntcsrSTATUS_NOT_BMAST	= 0x01000000, //!< Direct master not bus master
  PlxIntcsrSTATUS_NOT_BDMA_0	= 0x02000000, //!< DMA channel 0 not bus master
  PlxIntcsrSTATUS_NOT_BDMA_1	= 0x04000000, //!< DMA channel 1 not bus master

  //!< No target abort after 256 retries
  PlxIntcsrSTATUS_NOT_TARGET_ABORT = 0x08000000,

  //!< PCI wrote to Mail Box 0 and interrupt enabled
  PlxIntcsrSTATUS_MBOX_0	= 0x10000000,

  //!< PCI wrote to Mail Box 1 and interrupt enabled
  PlxIntcsrSTATUS_MBOX_1	= 0x20000000,

  //!< PCI wrote to Mail Box 2 and interrupt enabled
  PlxIntcsrSTATUS_MBOX_2	= 0x40000000,

  //!< PCI wrote to Mail Box 3 and interrupt enabled
  PlxIntcsrSTATUS_MBOX_3	= 0x80000000
} PlxIntcsr;

typedef enum {
  PlxDmaMode16BIT           = 0x00000001, //!< 16 Bit Mode
  PlxDmaMode32BIT           = 0x00000002, //!< 32 Bit mode
  PlxDmaModeWSP1            = 0x00000004, //!< Wait States bit 1
  PlxDmaModeWSP2            = 0x00000008, //!< Wait States bit 2
  PlxDmaModeWSP3            = 0x00000010, //!< Wait States bit 3
  PlxDmaModeWSP4            = 0x00000020, //!< Wait States bit 4
  PlxDmaModeINP_ENB         = 0x00000040, //!< Input Enable

  //!< Continuous Burst Enable or Burst-4 Mode
  PlxDmaModeCONT_BURST_ENB  = 0x00000080,

  PlxDmaModeLOCL_BURST_ENB  = 0x00000100, //!< Local Burst Enable
  PlxDmaModeSCAT_GATHER     = 0x00000200, //!< Enables Scatter Gather mode
  PlxDmaModeDONE_INT_ENB    = 0x00000400, //!< Enable interrupt on DMA done

  //!< Constant local address or Incremented local address
  PlxDmaModeCONST_LOCL_ADR  = 0x00000800,

  PlxDmaModeDEMAND          = 0x00001000, //!< Demand mode piloted by DBREQ#

  //!< Perform invalidate cycles on the PCI bus
  PlxDmaModeINVALIDATE      = 0x00002000,

  PlxDmaModeEOT_ENB         = 0x00004000, //!< Enables the EOT# input pin
  PlxDmaModeFAST_TERM       = 0x00008000, //!< Enables fast terminate on EOT#

  //!< Clears byte count in Scatter Gather mode
  PlxDmaModeCLR_SCAT_GATH   = 0x00010000,

  PlxDmaModeINT_PIN_SELECT  = 0x00020000, //!< When set its INTA# else its LINT#

  //!< When set PCI Dual Cycles else load DMADAC0
  PlxDmaModeDAC_CHAIN_LOAD  = 0x00040000,

  //!< When set EOT# goto the next descriptor, else stop on EOT#
  PlxDmaModeEOT_SCAT_GATH   = 0x00080000,

  PlxDmaModeRING_MANAGE_END = 0x00100000, //!< When set use DMASIZ[31]
  PlxDmaModeRING_MANAGE_STP = 0x00200000, //!< How RING management is stopped
} PlxDmaMode;

typedef enum {
  PlxDmaCsrENABLE          = 0x01,  //!< Enable the DMA channel
  PlxDmaCsrSTART           = 0x02,  //!< Start transfer of data
  PlxDmaCsrABORT           = 0x04,  //!< Abort the DMA transfer
  PlxDmaCsrCLEAR_INTERRUPT = 0x08,  //!< Clear the DMA interrupt
  PlxDmaCsrDONE            = 0x10   //!< DMA transfer completed
} PlxDmaCsr;

typedef enum {
  //!< When set its PCI, else Local address space
  PlxDmaDprSPACE           = 0x01,

  //!< Set when the chain ends in scatter gather mode
  PlxDmaDprEND_CHAIN       = 0x02,

  //!< When set interrupts on reaching terminal count
  PlxDmaDprTERM_INT_ENB    = 0x04,

  PlxDmaDprDIR_LOC_PCI     = 0x08   //!< Set: Local to PCI. Clear: PCI to Local
} PlxDmaDpr;

#define PlxBIG_ENDIAN 0xFF

typedef enum {
  PlxBigEndBIG_CONFIG  = 0x01, //!< Configuration registers
  PlxBigEndBIG_MASTER  = 0x02, //!< Direct master access
  PlxBigEndBIG_SLAVE_0 = 0x04, //!< Direct slave access, space 0
  PlxBigEndBIG_EEPROM  = 0x08, //!< EEPROM access

  //!< 1=>[Short 0:15 Char 0:7] 0=>[Short 16:31 Char 24:31]
  PlxBigEndBYTE_LANE   = 0x10,

  PlxBigEndBIG_SLAVE_1 = 0x20, //!< Direct slave access, space 1
  PlxBigEndBIG_DMA_1   = 0x40, //!< DMA Channel 1
  PlxBigEndBIG_DMA_0   = 0x80, //!< DMA Channel 0
} PlxBigEnd;
//@}

typedef struct {

  /*! @name Local Configuration Registers
   */
  //@{
  uint32_t LAS0RR;	//!< 0x000: Local Address Space 0 Range
  uint32_t LAS0BA;	//!< 0x004: Remap for LAS0RR
  uint32_t MARBR;	//!< 0x008: DMA arbitration register
  uint8_t  BIGEND;	//!< 0x00C: Big/Little endian descriptor
  uint8_t  LMISC1;	//!< 0x00D: Local miscellaneous control 1
  uint8_t  PROT_AREA;	//!< 0x00E: Write protect address boundary
  uint8_t  LMISC2;	//!< 0x00F: Local miscellaneous control 1
  uint32_t EROMRR;	//!< 0x010: Direct Slave Expansion ROM range
  uint32_t EROMBA;	//!< 0x014: Its Local Base addrress (Remap)
  uint32_t LBRD0;	//!< 0x018: Space 0 Expansion ROM region
  uint32_t DMRR;	//!< 0x01C: Local range for Direct Master to PCI
  uint32_t DMLBAM;	//!< 0x020: LBA for Direct Master to PCI memory
  uint32_t DMLBAI;	//!< 0x024: LBA for Direct Master to PCI I/O config
  uint32_t DMPBAM;	//!< 0x028: PBA (Remap) Direct Master to PCI memory

  //!< 0x02C: Config address Direct Master to PCI I/O config
  uint32_t DMCFGA;
  //@}

  /*! @name Messaging Queue (I2O) Registers
   */
  //@{
  uint32_t OPQIS;	//!< 0x030: Outbound Post Queue Interrupt Status
  uint32_t OPQIM;	//!< 0x034: Outbound Post Queue Interrupt Mask

  uint32_t Addr38;	//!< 0x038: Reserved
  uint32_t Addr3C;	//!< 0x03C: Reserved
  //@}

  /*! @name Runtime Registers
   */
  //@{
  uint32_t MBOX0_IQP;	//!< 0x040: Mailbox 0 / Inbound  Queue pointer
  uint32_t MBOX1_OQP;	//!< 0x044: Mailbox 1 / Outbound Queue pointer
  uint32_t MBOX2;	//!< 0x048: Mailbox 2
  uint32_t MBOX3;	//!< 0x04C: Mailbox 3
  uint32_t MBOX4;	//!< 0x050: Mailbox 4
  uint32_t MBOX5;	//!< 0x054: Mailbox 5
  uint32_t MBOX6;	//!< 0x058: Mailbox 6
  uint32_t MBOX7;	//!< 0x05C: Mailbox 7
  uint32_t P2LDBELL;	//!< 0x060: PCI to local Doorbell
  uint32_t L2PDBELL;	//!< 0x064: Local to PCI Doorbell
  uint32_t INTCSR;	//!< 0x068: Interrupt Control Status

  //!< 0x06C: Serial EEPROM Control, PCI Cmds, User I/O, Init Control
  uint32_t CNTRL;

  uint32_t PCIHDR;	//!< 0x070: PCI Hardwired Config ID
  uint32_t PCIHREV;	//!< 0x074: PCI Hardwired Revision ID
  uint32_t MBOX0C;	//!< 0x078: Mailbox 0 Copy
  uint32_t MBOX1C;	//!< 0x07C: Mailbox 1 Copy
  //@}

  /*! @name DMA Registers
   */
  //@{
  uint32_t DMAMODE0;	//!< 0x080: DMA Chan 0 Mode
  uint32_t DMAPADR0;	//!< 0x084: DMA Chan 0 PCI Address
  uint32_t DMALADR0;	//!< 0x088: DMA Chan 0 Local Address
  uint32_t DMASIZ0;	//!< 0x08C: DMA Chan 0 Size Bytes
  uint32_t DMADPR0;	//!< 0x090: DMA Chan 0 Descriptor Pointer
  uint32_t DMAMODE1;	//!< 0x094: DMA Chan 1 Mode
  uint32_t DMAPADR1;	//!< 0x098: DMA Chan 1 PCI Address
  uint32_t DMALADR1;	//!< 0x09C: DMA Chan 1 Local Address
  uint32_t DMASIZ1;	//!< 0x0A0: DMA Chan 1 Size Bytes
  uint32_t DMADPR1;	//!< 0x0A4: DMA Chan 1 Descriptor Pointer
  uint8_t  DMACSR0;	//!< 0x0A8: DMA Chan 0 Command/Status
  uint8_t  DMACSR1;	//!< 0x0A9: DMA Chan 1 Command/Status

  uint16_t AddrAB;	//!< 0x0AB: Reserved

  uint32_t DMAARB;	//!< 0x0AC: DMA Arbitration
  uint32_t DMATHR;	//!< 0x0B0: DMA Threshold
  uint32_t DMADAC0;	//!< 0x0B4: DMA Chan 0 PCI Dual Address Cycle Upper
  uint32_t DMADAC1;	//!< 0x0B8: DMA Chan 1 PCI Dual Address Cycle Upper

  uint32_t AddrBC;	//!< 0x0BC: Reserved
  //@}

  /*! @name Messaging Queue (I2O) Registers
   */
  //@{
  uint32_t MQCR;	//!< 0x0C0: Message Queue Config
  uint32_t QBAR;	//!< 0x0C4: Queue Base Address
  uint32_t IFHPR;	//!< 0x0C8: Inbound free head pointer
  uint32_t IFTPR;	//!< 0x0CC: Inbound free tail pointer
  uint32_t IPHPR;	//!< 0x0D0: Inbound post head pointer
  uint32_t IPTPR;	//!< 0x0D4: Inbound post tail pointer
  uint32_t OFHPR;	//!< 0x0D8: Outbound free head pointer
  uint32_t OFTPR;	//!< 0x0DC: Outbound free tail pointer
  uint32_t OPHPR;	//!< 0x0E0: Outbound post head pointer
  uint32_t OPTPR;	//!< 0x0E4: Outbound post tail pointer
  uint32_t QSR;		//!< 0x0E8: Queue Status/Control

  uint32_t AddrEC;	//!< 0x0EC: Reserved
  //@}

  /*! @name Local Configuration Registers
   */
  //@{
  uint32_t LAS1RR;	//!< 0x0F0: Local Address Space 1 Range

  //!< 0x0F4: Local Address Space 1 Local Base Address (Remap)
  uint32_t LAS1BA;

  uint32_t LBRD1;	//!< 0x0F8: Space 1 Bus region
  uint32_t DMDAC;	//!< 0x0FC: Direct Master PCI Dual Address Cycles Upper
  uint32_t PCIARB;	//!< 0x100: PCI Arbiter Control
  uint32_t PABTADR;	//!< 0x104: PCI Abort address
  //@}

} __attribute__ ((packed)) PlxLocalMap;

/*! @name Local register offsets
 */
//@{
typedef enum {

  /*! @name Local Localiguration Registers
   */
  //@{
  PlxLocalLAS0RR       = 0x000, //!< Local Address Space 0 Range
  PlxLocalLAS0BA       = 0x004, //!< Remap for LAS0RR
  PlxLocalMARBR        = 0x008, //!< DMA arbitration register
  PlxLocalBIGEND       = 0x00C, //!< Big/Little endian descriptor
  PlxLocalLMISC1       = 0x00D, //!< Local miscellaneous control 1
  PlxLocalPROT_AREA    = 0x00E, //!< Write protect address boundary
  PlxLocalLMISC2       = 0x00F, //!< Local miscellaneous control 1
  PlxLocalEROMRR       = 0x010, //!< Direct Slave Expansion ROM range
  PlxLocalEROMBA       = 0x014, //!< Its Local Base addrress (Remap)
  PlxLocalLBRD0        = 0x018, //!< Space 0 Expansion ROM region
  PlxLocalDMRR         = 0x01C, //!< Local range for Direct Master to PCI
  PlxLocalDMLBAM       = 0x020, //!< LBA for Direct Master to PCI memory
  PlxLocalDMLBAI       = 0x024, //!< LBA for Direct Master to PCI I/O config
  PlxLocalDMPBAM       = 0x028, //!< PBA (Remap) Direct Master to PCI memory

  //!< Localig address Direct Master to PCI I/O config
  PlxLocalDMCFGA       = 0x02C,
  //@}

  /*! @name Messaging Queue (I2O) Registers
   */
  //@{
  PlxLocalOPQIS        = 0x030, //!< Outbound Post Queue Interrupt Status
  PlxLocalOPQIM        = 0x034, //!< Outbound Post Queue Interrupt Mask

  PlxLocalAddr38       = 0x038, //!< Reserved
  PlxLocalAddr3C       = 0x03C, //!< Reserved
  //@}

  /*! @name Runtime Registers
   */
  //@{
  PlxLocalMBOX0_IQP    = 0x040, //!< Mailbox 0 / Inbound  Queue pointer
  PlxLocalMBOX1_OQP    = 0x044, //!< Mailbox 1 / Outbound Queue pointer
  PlxLocalMBOX2        = 0x048, //!< Mailbox 2
  PlxLocalMBOX3        = 0x04C, //!< Mailbox 3
  PlxLocalMBOX4        = 0x050, //!< Mailbox 4
  PlxLocalMBOX5        = 0x054, //!< Mailbox 5
  PlxLocalMBOX6        = 0x058, //!< Mailbox 6
  PlxLocalMBOX7        = 0x05C, //!< Mailbox 7
  PlxLocalP2LDBELL     = 0x060, //!< PCI to local Doorbell
  PlxLocalL2PDBELL     = 0x064, //!< Local to PCI Doorbell
  PlxLocalINTCSR       = 0x068, //!< Interrupt Control Status

  //!< Serial EEPROM Control, PCI Cmds, User I/O, Init Control
  PlxLocalCNTRL        = 0x06C,

  PlxLocalPCIHDR       = 0x070, //!< PCI Hardwired Localig ID
  PlxLocalPCIHREV      = 0x074, //!< PCI Hardwired Revision ID
  PlxLocalMBOX0C       = 0x078, //!< Mailbox 0 Copy
  PlxLocalMBOX1C       = 0x07C, //!< Mailbox 1 Copy
  //@}

  /*! @name DMA Registers
   */
  //@{
  PlxLocalDMAMODE0     = 0x080, //!< DMA Chan 0 Mode
  PlxLocalDMAPADR0     = 0x084, //!< DMA Chan 0 PCI Address
  PlxLocalDMALADR0     = 0x088, //!< DMA Chan 0 Local Address
  PlxLocalDMASIZ0      = 0x08C, //!< DMA Chan 0 Size Bytes
  PlxLocalDMADPR0      = 0x090, //!< DMA Chan 0 Descriptor Pointer
  PlxLocalDMAMODE1     = 0x094, //!< DMA Chan 1 Mode
  PlxLocalDMAPADR1     = 0x098, //!< DMA Chan 1 PCI Address
  PlxLocalDMALADR1     = 0x09C, //!< DMA Chan 1 Local Address
  PlxLocalDMASIZ1      = 0x0A0, //!< DMA Chan 1 Size Bytes
  PlxLocalDMADPR1      = 0x0A4, //!< DMA Chan 1 Descriptor Pointer
  PlxLocalDMACSR0      = 0x0A8, //!< DMA Chan 0 Command/Status
  PlxLocalDMACSR1      = 0x0A9, //!< DMA Chan 1 Command/Status

  PlxLocalAddrAB       = 0x0AB, //!< Reserved

  PlxLocalDMAARB       = 0x0AC, //!< DMA Arbitration
  PlxLocalDMATHR       = 0x0B0, //!< DMA Threshold
  PlxLocalDMADAC0      = 0x0B4, //!< DMA Chan 0 PCI Dual Address Cycle Upper
  PlxLocalDMADAC1      = 0x0B8, //!< DMA Chan 1 PCI Dual Address Cycle Upper

  PlxLocalAddrBC       = 0x0BC, //!< Reserved
  //@}

  /*! @name Messaging Queue (I2O) Registers
   */
  //@{
  PlxLocalMQCR         = 0x0C0, //!< Message Queue Localig
  PlxLocalQBAR         = 0x0C4, //!< Queue Base Address
  PlxLocalIFHPR        = 0x0C8, //!< Inbound free head pointer
  PlxLocalIFTPR        = 0x0CC, //!< Inbound free tail pointer
  PlxLocalIPHPR        = 0x0D0, //!< Inbound post head pointer
  PlxLocalIPTPR        = 0x0D4, //!< Inbound post tail pointer
  PlxLocalOFHPR        = 0x0D8, //!< Outbound free head pointer
  PlxLocalOFTPR        = 0x0DC, //!< Outbound free tail pointer
  PlxLocalOPHPR        = 0x0E0, //!< Outbound post head pointer
  PlxLocalOPTPR        = 0x0E4, //!< Outbound post tail pointer
  PlxLocalQSR          = 0x0E8, //!< Queue Status/Control

  PlxLocalAddrEC       = 0x0EC, //!< Reserved
  //@}

  /*! @name Local Localiguration Registers
   */
  //@{
  PlxLocalLAS1RR       = 0x0F0, //!< Local Address Space 1 Range

  //!< Local Address Space 1 Local Base Address (Remap)
  PlxLocalLAS1BA       = 0x0F4,

  PlxLocalLBRD1        = 0x0F8, //!< Space 1 Bus region
  PlxLocalDMDAC        = 0x0FC, //!< Direct Master PCI Dual Address Cycles Upper
  PlxLocalPCIARB       = 0x100, //!< PCI Arbiter Control
  PlxLocalPABTADR      = 0x104  //!< PCI Abort address
  //@}

} PlxLocal;
//@}

#endif
