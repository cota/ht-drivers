/**
 * @file plx9656_layout.h
 *
 * @brief Layout of the PLX9656 PCI Controller Chip.
 *
 * @author Julian Lewis AB/CO/HT CERN
 *
 * @date Created on 26/11/2003
 *
 * @version $Id$
 */

#ifndef PLX_9656
#define PLX_9656


/*! @name Define the structure of the PLX 9656 Configuration Registers. 
 */
//@{
#define PLX9656_VENDOR_ID 0x10B5
#define PLX9656_DEVICE_ID 0x9656

typedef struct {
   u_int16_t DeviceId;
   u_int16_t VendorId;
 } __attribute__ ((packed)) PlxRegPCIIDR;

typedef enum {
   PlxPcicrIO_SPACE       = 0x0001, //!< Allows I/O    space access (I/O Accelerator)
   PlxPcicrMEMORY_SPACE   = 0x0002, //!< Allows memory space access (I/O Accelerator)
   PlxPcicrMASTER_ENABLE  = 0x0008, //!< Allows bus master behaviour
   PlxPcicrMEMORY_WRITE   = 0x0010, //!< Allows DMA
   PlxPcicrPARITY_ERROR   = 0x0040, //!< Forces parity error checking
   PlxPcicrSERR           = 0x0100, //!< Enable SERR driver
   PlxPcicrFAST_BACK      = 0x0200  //!< Type of master fast back to back
 } PlxPcicr;

typedef enum {
   PlxPcisrNEW_CAP        = 0x0010, //!< New capabilities, must be zero for VMIC5565
   PlxPcisr66MHZ          = 0x0020, //!< 66MHz, on the CES power PC this must be zero
   PlxPcisrUSER_DEFINED   = 0x0040, //!< Enable user defined functions
   PlxPcisrFAST_BACK      = 0x0080, //!< Indicates fast back to back, its always a 1
   PlxPcisrMPARITY_ERROR  = 0x0100, //!< Master parity error detected
   PlxPcisrDEVSEL_0       = 0x0200, //!< DEVSEL Timing set to medium always 1
   PlxPcisrDEVSEL_1       = 0x0400, //!< DEVSEL Timing set to medium always 0
   PlxPcisrTARGET_ABORT   = 0x0800, //!< Indicates target abort, cleared by writing 1
   PlxPcisrRTARGET_ABORT  = 0x1000, //!< Received a target abort, cleared by writing 1
   PlxPcisrRMTARGET_ABORT = 0x2000, //!< Received a master abort, cleared by writing 1
   PlxPcisrSERR           = 0x4000, //!< System error, clearer by writing 1
   PlxPcisrPARITY_ERROR   = 0x8000  //!< Parity error detected , cleared by writing 1
 } PlxPcisr;

typedef struct {
   PlxRegPCIIDR   PCIIDR;  //!< 0x00: PCI Configuration ID
   u_int16_t PCICR;        //!< 0x04: PCI Command
   u_int16_t PCISR;        //!< 0x06: PCI Status
   u_int8_t  PCIREV;       //!< 0x08: PCI Revision ID
   u_int8_t  PCICCR0;      //!< 0x09: PCI Class code Byte 1
   u_int8_t  PCICCR1;      //!< 0x0A: PCI Class code Byte 2
   u_int8_t  PCICCR2;      //!< 0x0B: PCI Class code Byte 3
   u_int8_t  PCICLSR;      //!< 0x0C: PCI Cash Line Size
   u_int8_t  PCILTR;       //!< 0x0D: PCI Bus Latency Timer
   u_int8_t  PCIHTR;       //!< 0x0E: PCI Header Type
   u_int8_t  PCIBISTR;     //!< 0x0F: PCI PCI Built in Self Test
   u_int32_t PCIBAR0;      //!< 0x10: PCI BAR0 for Local configuration space mapped
   u_int32_t PCIBAR1;      //!< 0x14: PCI BAR1 for Local configuration space IO
   u_int32_t PCIBAR2;      //!< 0x18: PCI BAR2 Local address space 0
   u_int32_t PCIBAR3;      //!< 0x1C: PCI BAR3 Local address space 1
   u_int32_t PCIBAR4;      //!< 0x20: PCI BAR4 Local address space 2
   u_int32_t PCIBAR5;      //!< 0x24: PCI BAR5 Local address space 3
   u_int32_t PCICIS;       //!< 0x28: PCI Card bus info structure pointer
   u_int16_t PCISVID;      //!< 0x2C: PCI Subsystem Vendor ID
   u_int16_t PCISID;       //!< 0x2E: PCI Subsystem ID
   u_int32_t PCIERBAR;     //!< 0x30: PCI Expansion ROM Base address
   u_int32_t CAP_PTR;      //!< 0x34: New capability pointer
   u_int32_t Addr38;       //!< 0x38: Reserved
   u_int8_t  PCIILR;       //!< 0x3C: PCI Interrupt Line
   u_int8_t  PCIIPR;       //!< 0x3D: PCI Interrupt Pin
   u_int8_t  PCIMGR;       //!< 0x3E: PCI Minimum Grant
   u_int8_t  PCIMLR;       //!< 0x3F: PCI Maximum Latency
   u_int8_t  PMCCAPID;     //!< 0x40: Power Management capability ID
   u_int8_t  PMNEXT;       //!< 0x41: Power Management capability Pointer
   u_int16_t PMC;          //!< 0x42: Power Management capabilities
   u_int16_t PMCSR;        //!< 0x44: Power Management Control/Status
   u_int8_t  PMCSR_BSE;    //!< 0x46: PMC Bridge Support Extensisions
   u_int8_t  PMDATA;       //!< 0x47: Power Management Data
   u_int8_t  HS_CNTL;      //!< 0x48: Hot Swap Control
   u_int8_t  HS_NEXT;      //!< 0x49: Hot Swap Next Capability Pointer
   u_int16_t HS_CSR;       //!< 0x4A: Hot Swap Control Status
   u_int8_t  PVPDCNTL;     //!< 0x4C: PCI Vital Product Data Control
   u_int8_t  PVPD_NEXT;    //!< 0x4D: PCI VPD Next Capability pointer
   u_int16_t PVPDAD;       //!< 0x4E: PCI VPD Data Address
   u_int32_t PVPDATA;      //!< 0x50: PCI VPD Data
 } __attribute__ ((packed)) PlxConfigMap;
//@}

/*! @name Configuration register offsets
 */
//@{
typedef enum {
   PlxConfigPCIIDR       = 0x00, //!< PCI Configuration ID
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
   PlxConfigPCIBAR0      = 0x10, //!< PCI BAR0 for Local configuration space mapped
   PlxConfigPCIBAR1      = 0x14, //!< PCI BAR1 for Local configuration space IO
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

/*! @name Define the structure of the PLX 9656 Local Configuration Registers. 
 */
//@{
typedef enum {
   PlxIntcsrENABLE_0                = 0x00000001, //!< Enable Interrupt sources 0
   PlxIntcsrENABLE_1                = 0x00000002, //!< Enable Interrupt sources 1
   PlxIntcsrGEN_SERR                = 0x00000004, //!< Generate a SERR interrupt
   PlxIntcsrENABLE_MBOX             = 0x00000008, //!< Enable Mailbox interrupts
   PlxIntcsrENABLE_PMAN             = 0x00000010, //!< Enable power manager interrupts
   PlxIntcsrSTATUS_PMAN             = 0x00000020, //!< Status of power manager interrupt
   PlxIntcsrENABLE_PERR             = 0x00000040, //!< Enable parity error intrerrupts
   PlxIntcsrSTATUS_PERR             = 0x00000080, //!< Status of parity error
   PlxIntcsrENABLE_PCI              = 0x00000100, //!< Enable PCI interrupts
   PlxIntcsrENABLE_PCI_DBELL        = 0x00000200, //!< Enable Doorbell PCI Interrupt
   PlxIntcsrENABLE_PCI_ABORT        = 0x00000400, //!< Enable PCI abort interrupts
   PlxIntcsrENABLE_LOCAL            = 0x00000800, //!< Enable local interrupt inputs
   PlxIntcsrRETRY_ABORT             = 0x00001000, //!< Retry upto 256 times
   PlxIntcsrSTATUS_DBELL            = 0x00002000, //!< Doorbell interrupt active
   PlxIntcsrSTATUS_PCI_ABORT        = 0x00004000, //!< PCI abort interrupt active
   PlxIntcsrSTATUS_LOCAL            = 0x00008000, //!< Local interrupt active
   PlxIntcsrENABLE_LOCAL_OUT        = 0x00010000, //!< Enable local interrupt out
   PlxIntcsrENABLE_LOCAL_DBELL      = 0x00020000, //!< Enable local doorbell interrupts
   PlxIntcsrENABLE_DMA_CHAN_0       = 0x00040000, //!< Enable DMA channel 0 interrupts
   PlxIntcsrENABLE_DMA_CHAN_1       = 0x00080000, //!< Enable DMA channel 1 interrupts
   PlxIntcsrSTATUS_LOCAL_DBELL      = 0x00100000, //!< Status of local doorbell interrupt
   PlxIntcsrSTATUS_DMA_CHAN_0       = 0x00200000, //!< Status of DMA channel 0 interrupt
   PlxIntcsrSTATUS_DMA_CHAN_1       = 0x00400000, //!< Status of DMA channel 1 interrupt
   PlxIntcsrSTATUS_BIST             = 0x00800000, //!< Status of bilt in self test interrupt
   PlxIntcsrSTATUS_NOT_BMAST        = 0x01000000, //!< Direct master not bus master
   PlxIntcsrSTATUS_NOT_BDMA_0       = 0x02000000, //!< DMA channel 0 not bus master
   PlxIntcsrSTATUS_NOT_BDMA_1       = 0x04000000, //!< DMA channel 1 not bus master
   PlxIntcsrSTATUS_NOT_TARGET_ABORT = 0x08000000, //!< No target abort after 256 retries
   PlxIntcsrSTATUS_MBOX_0           = 0x10000000, //!< PCI wrote to Mail Box 0 and interrupt enabled
   PlxIntcsrSTATUS_MBOX_1           = 0x20000000, //!< PCI wrote to Mail Box 1 and interrupt enabled
   PlxIntcsrSTATUS_MBOX_2           = 0x40000000, //!< PCI wrote to Mail Box 2 and interrupt enabled
   PlxIntcsrSTATUS_MBOX_3           = 0x80000000  //!< PCI wrote to Mail Box 3 and interrupt enabled
 } PlxIntcsr;

typedef enum {
   PlxDmaMode16BIT           = 0x00000001, //!< 16 Bit Mode
   PlxDmaMode32BIT           = 0x00000002, //!< 32 Bit mode
   PlxDmaModeWSP1            = 0x00000004, //!< Wait States bit 1
   PlxDmaModeWSP2            = 0x00000008, //!< Wait States bit 2
   PlxDmaModeWSP3            = 0x00000010, //!< Wait States bit 3
   PlxDmaModeWSP4            = 0x00000020, //!< Wait States bit 4
   PlxDmaModeINP_ENB         = 0x00000040, //!< Input Enable
   PlxDmaModeCONT_BURST_ENB  = 0x00000080, //!< Continuous Burst Enable or Burst-4 Mode
   PlxDmaModeLOCL_BURST_ENB  = 0x00000100, //!< Local Burst Enable
   PlxDmaModeSCAT_GATHER     = 0x00000200, //!< Enables Scatter Gather mode
   PlxDmaModeDONE_INT_ENB    = 0x00000400, //!< Enable interrupt on DMA done
   PlxDmaModeCONST_LOCL_ADR  = 0x00000800, //!< Constant local address or Incremented local address
   PlxDmaModeDEMAND          = 0x00001000, //!< Demand mode piloted by DBREQ#
   PlxDmaModeINVALIDATE      = 0x00002000, //!< Perform invalidate cycles on the PCI bus
   PlxDmaModeEOT_ENB         = 0x00004000, //!< Enables the EOT# input pin
   PlxDmaModeFAST_TERM       = 0x00008000, //!< Enables fast terminate on EOT#
   PlxDmaModeCLR_SCAT_GATH   = 0x00010000, //!< Clears byte count in Scatter Gather mode
   PlxDmaModeINT_PIN_SELECT  = 0x00020000, //!< When set its INTA# else its LINT#
   PlxDmaModeDAC_CHAIN_LOAD  = 0x00040000, //!< When set PCI Dual Cycles else load DMADAC0
   PlxDmaModeEOT_SCAT_GATH   = 0x00080000, //!< When set EOT# goto the next descriptor, else stop on EOT#
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
   PlxDmaDprSPACE           = 0x01, //!< When set its PCI, else Local address space
   PlxDmaDprEND_CHAIN       = 0x02, //!< Set when the chain ends in scatter gather mode
   PlxDmaDprTERM_INT_ENB    = 0x04, //!< When set interrupts on reaching terminal count
   PlxDmaDprDIR_LOC_PCI     = 0x08  //!< Set: Local to PCI. Clear: PCI to Local
 } PlxDmaDpr;

#define PlxBIG_ENDIAN 0xFF

typedef enum {
   PlxBigEndBIG_CONFIG  = 0x01, //!< Configuration registers
   PlxBigEndBIG_MASTER  = 0x02, //!< Direct master access
   PlxBigEndBIG_SLAVE_0 = 0x04, //!< Direct slave access, space 0
   PlxBigEndBIG_EEPROM  = 0x08, //!< EEPROM access
   PlxBigEndBYTE_LANE   = 0x10, //!< 1=>[Short 0:15 Char 0:7] 0=>[Short 16:31 Char 24:31]
   PlxBigEndBIG_SLAVE_1 = 0x20, //!< Direct slave access, space 1
   PlxBigEndBIG_DMA_1   = 0x40, //!< DMA Channel 1
   PlxBigEndBIG_DMA_0   = 0x80, //!< DMA Channel 0
 } PlxBigEnd;
//@}

typedef struct {

/*! @name Local Configuration Registers
 */
//@{
   u_int32_t LAS0RR;       //!< 0x000: Local Address Space 0 Range
   u_int32_t LAS0BA;       //!< 0x004: Remap for LAS0RR
   u_int32_t MARBR;        //!< 0x008: DMA arbitration register
   u_int8_t  BIGEND;       //!< 0x00C: Big/Little endian descriptor
   u_int8_t  LMISC1;       //!< 0x00D: Local miscellaneous control 1
   u_int8_t  PROT_AREA;    //!< 0x00E: Write protect address boundary
   u_int8_t  LMISC2;       //!< 0x00F: Local miscellaneous control 1
   u_int32_t EROMRR;       //!< 0x010: Direct Slave Expansion ROM range
   u_int32_t EROMBA;       //!< 0x014: Its Local Base addrress (Remap)
   u_int32_t LBRD0;        //!< 0x018: Space 0 Expansion ROM region
   u_int32_t DMRR;         //!< 0x01C: Local range for Direct Master to PCI
   u_int32_t DMLBAM;       //!< 0x020: LBA for Direct Master to PCI memory
   u_int32_t DMLBAI;       //!< 0x024: LBA for Direct Master to PCI I/O config
   u_int32_t DMPBAM;       //!< 0x028: PBA (Remap) Direct Master to PCI memory
   u_int32_t DMCFGA;       //!< 0x02C: Config address Direct Master to PCI I/O config
//@}


/*! @name Messaging Queue (I2O) Registers
 */
//@{
   u_int32_t OPQIS;        //!< 0x030: Outbound Post Queue Interrupt Status
   u_int32_t OPQIM;        //!< 0x034: Outbound Post Queue Interrupt Mask

   u_int32_t Addr38;       //!< 0x038: Reserved
   u_int32_t Addr3C;       //!< 0x03C: Reserved
//@}


/*! @name Runtime Registers
 */
//@{
   u_int32_t MBOX0_IQP;    //!< 0x040: Mailbox 0 / Inbound  Queue pointer
   u_int32_t MBOX1_OQP;    //!< 0x044: Mailbox 1 / Outbound Queue pointer
   u_int32_t MBOX2;        //!< 0x048: Mailbox 2
   u_int32_t MBOX3;        //!< 0x04C: Mailbox 3
   u_int32_t MBOX4;        //!< 0x050: Mailbox 4
   u_int32_t MBOX5;        //!< 0x054: Mailbox 5
   u_int32_t MBOX6;        //!< 0x058: Mailbox 6
   u_int32_t MBOX7;        //!< 0x05C: Mailbox 7
   u_int32_t P2LDBELL;     //!< 0x060: PCI to local Doorbell
   u_int32_t L2PDBELL;     //!< 0x064: Local to PCI Doorbell
   u_int32_t INTCSR;       //!< 0x068: Interrupt Control Status
   u_int32_t CNTRL;        //!< 0x06C: Serial EEPROM Control, PCI Cmds, User I/O, Init Control
   u_int32_t PCIHDR;       //!< 0x070: PCI Hardwired Config ID
   u_int32_t PCIHREV;      //!< 0x074: PCI Hardwired Revision ID
   u_int32_t MBOX0C;       //!< 0x078: Mailbox 0 Copy
   u_int32_t MBOX1C;       //!< 0x07C: Mailbox 1 Copy
//@}

/*! @name DMA Registers
 */
//@{
   u_int32_t DMAMODE0;     //!< 0x080: DMA Chan 0 Mode
   u_int32_t DMAPADR0;     //!< 0x084: DMA Chan 0 PCI Address
   u_int32_t DMALADR0;     //!< 0x088: DMA Chan 0 Local Address
   u_int32_t DMASIZ0;      //!< 0x08C: DMA Chan 0 Size Bytes
   u_int32_t DMADPR0;      //!< 0x090: DMA Chan 0 Descriptor Pointer
   u_int32_t DMAMODE1;     //!< 0x094: DMA Chan 1 Mode
   u_int32_t DMAPADR1;     //!< 0x098: DMA Chan 1 PCI Address
   u_int32_t DMALADR1;     //!< 0x09C: DMA Chan 1 Local Address
   u_int32_t DMASIZ1;      //!< 0x0A0: DMA Chan 1 Size Bytes
   u_int32_t DMADPR1;      //!< 0x0A4: DMA Chan 1 Descriptor Pointer
   u_int8_t  DMACSR0;      //!< 0x0A8: DMA Chan 0 Command/Status
   u_int8_t  DMACSR1;      //!< 0x0A9: DMA Chan 1 Command/Status

   u_int16_t AddrAB;       //!< 0x0AB: Reserved

   u_int32_t DMAARB;       //!< 0x0AC: DMA Arbitration
   u_int32_t DMATHR;       //!< 0x0B0: DMA Threshold
   u_int32_t DMADAC0;      //!< 0x0B4: DMA Chan 0 PCI Dual Address Cycle Upper
   u_int32_t DMADAC1;      //!< 0x0B8: DMA Chan 1 PCI Dual Address Cycle Upper

   u_int32_t AddrBC;       //!< 0x0BC: Reserved
//@}

/*! @name Messaging Queue (I2O) Registers
 */
//@{
   u_int32_t MQCR;         //!< 0x0C0: Message Queue Config
   u_int32_t QBAR;         //!< 0x0C4: Queue Base Address
   u_int32_t IFHPR;        //!< 0x0C8: Inbound free head pointer
   u_int32_t IFTPR;        //!< 0x0CC: Inbound free tail pointer
   u_int32_t IPHPR;        //!< 0x0D0: Inbound post head pointer
   u_int32_t IPTPR;        //!< 0x0D4: Inbound post tail pointer
   u_int32_t OFHPR;        //!< 0x0D8: Outbound free head pointer
   u_int32_t OFTPR;        //!< 0x0DC: Outbound free tail pointer
   u_int32_t OPHPR;        //!< 0x0E0: Outbound post head pointer
   u_int32_t OPTPR;        //!< 0x0E4: Outbound post tail pointer
   u_int32_t QSR;          //!< 0x0E8: Queue Status/Control

   u_int32_t AddrEC;       //!< 0x0EC: Reserved
//@}

/*! @name Local Configuration Registers
 */
//@{
   u_int32_t LAS1RR;       //!< 0x0F0: Local Address Space 1 Range
   u_int32_t LAS1BA;       //!< 0x0F4: Local Address Space 1 Local Base Address (Remap)
   u_int32_t LBRD1;        //!< 0x0F8: Space 1 Bus region
   u_int32_t DMDAC;        //!< 0x0FC: Direct Master PCI Dual Address Cycles Upper
   u_int32_t PCIARB;       //!< 0x100: PCI Arbiter Control
   u_int32_t PABTADR;      //!< 0x104: PCI Abort address
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
   PlxLocalDMCFGA       = 0x02C, //!< Localig address Direct Master to PCI I/O config
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
   PlxLocalCNTRL        = 0x06C, //!< Serial EEPROM Control, PCI Cmds, User I/O, Init Control
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
   PlxLocalLAS1BA       = 0x0F4, //!< Local Address Space 1 Local Base Address (Remap)
   PlxLocalLBRD1        = 0x0F8, //!< Space 1 Bus region
   PlxLocalDMDAC        = 0x0FC, //!< Direct Master PCI Dual Address Cycles Upper
   PlxLocalPCIARB       = 0x100, //!< PCI Arbiter Control
   PlxLocalPABTADR      = 0x104  //!< PCI Abort address
//@}

 } PlxLocal;
//@}

#endif
