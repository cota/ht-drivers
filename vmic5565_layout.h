/**
 * @file vmic5565_layout.h
 *
 * @brief Layout of the VMIPMC-5565 Reflective Memory RFM registers
 *
 * @author Julian Lewis AB/CO/HT CERN
 *
 * @date Created on 26/08/2004
 */

#ifndef VMIC_5565
#define VMIC_5565

#ifndef __KERNEL__
#include <stdint.h>
#endif

#define VMIC_VENDOR_ID 0x114A
#define VMIC_DEVICE_ID 0x5565


/*! @name VMIC RFM Register LCSR1 bits
 */
//@{
typedef enum {
   VmicLcsrLED_ON            = 0x80000000, //!< Red LED
   VmicLcsrTR_OFF            = 0x40000000, //!< Transmitter
   VmicLcsrDARK_ON           = 0x20000000, //!< Dark on Dark
   VmicLcsrLOOP_ON           = 0x10000000, //!< Test mode loop back
   VmicLcsrPARITY_ON         = 0x08000000, //!< Parity checking
   VmicLcsrREDUNDANT_ON      = 0x04000000, //!< Redundant transmission mode (RO)
   VmicLcsrROGUE_MASTER_1_ON = 0x02000000, //!< Rogue Master (RO)
   VmicLcsrROGUE_MASTER_0_ON = 0x01000000, //!< Rogue Master (RO)
   VmicLcsr128MB             = 0x00100000, //!< 128Mb else 64Mb (RO)
   VmicLcsrTX_EMPTY          = 0x00000080, //!< Transmit buffer empty (RO)
   VmicLcsrTX_ALMOST_FULL    = 0x00000040, //!< Transmit buffer almost full (RO)

   VmicLcsrRX_OVERFLOW       = 0x00000020,
   //!< Receive buffer has overflowed (RO)

   VmicLcsrRX_ALMOST_FULL    = 0x00000010,
   //!< Receive buffer is almost full (RO)

   VmicLcsrSYNC_LOST         = 0x00000008, //!< Lost sync clock (RO)

   VmicLcsrSIGNAL_DETECT     = 0x00000004,
   //!< Incomming signal (light) present (RO)

   VmicLcsrDATA_LOST         = 0x00000002,
   //!< Data is bad or has been lost (RO)

   VmicLcsrOWN_DATA          = 0x00000001  //!< Own data is present
 } VmicLcsr;
//@}


/*! @name VMIC RFM Registers LISR (Status/Control) bits
 */
//@{
#define VmicLisrMASK 0xFFCF
#define VmicLisrINT_MASK 0x7
#define VmicLisrSOURCES 14
#define VmicLisrSOURCE_MASK 0x3FCF

typedef enum {
   VmicLisrAC_FLAG           = 0x8000,
   //!< Auto clear Interrupt: This bit must always be On

   VmicLisrENABLE            = 0x4000, //!< Global Interrupt Enable
   VmicLisrPARITY_ERROR      = 0x2000, //!< Parity error

   VmicLisrWRITE_ERROR       = 0x1000,
   //!< Cant write a short or byte if parity on

   VmicLisrLOST_SYNC         = 0x0800,
   //!< PLL unlocked, data was lost, or signal lost

   VmicLisrRX_OVERFLOW       = 0x0400, //!< Receiver buffer overflow
   VmicLisrRX_ALMOST_FULL    = 0x0200, //!< Receive buffer almost full
   VmicLisrDATA_ERROR        = 0x0100, //!< Bad data received, error
   VmicLisrPENDING_INIT      = 0x0080, //!< Another node needs initializing

   VmicLisrROGUE_CLOBBER     = 0x0040,
   //!< This rogue master has clobbered a rogue packet

   VmicLisrBIT5              = 0x0020, //!< Reserved
   VmicLisrBIT4              = 0x0010, //!< Reserved

   VmicLisrRESET_RQ          = 0x0008,
   //!< Reset me request from some other node

   VmicLisrINT3              = 0x0004, //!< Pending Interrupt 3
   VmicLisrINT2              = 0x0002, //!< Pending Interrupt 2
   VmicLisrINT1              = 0x0001  //!< Pending Interrupt 1
 } VmicLisr;
//@}


/*! @name VMIC RFM Registers LIER (Enable) bits
 *
 * Valid enable bits mask in LIER are 1011 1111 1100 1111
 */
//@{
#define VmicLierMASK 0xBFCF

typedef enum {
   VmicLierAC_FLAG           = 0x8000,
   //!< Auto clear Interrupt: This bit must always be On

   VmicLierBIT14             = 0x4000, //!< Reserved
   VmicLierPARITY_ERROR      = 0x2000, //!< Parity error

   VmicLierWRITE_ERROR       = 0x1000,
   //!< Cant write a short or byte if parity on

   VmicLierLOST_SYNC         = 0x0800,
   //!< PLL unlocked, data was lost, or signal lost

   VmicLierRX_OVERFLOW       = 0x0400, //!< Receiver buffer overflow
   VmicLierRX_ALMOST_FULL    = 0x0200, //!< Receive buffer almost full
   VmicLierDATA_ERROR        = 0x0100, //!< Bad data received, error
   VmicLierPENDING_INIT      = 0x0080, //!< Another node needs initializing

   VmicLierROGUE_CLOBBER     = 0x0040,
   //!< This rogue master has clobbered a rogue packet

   VmicLierBIT5              = 0x0020, //!< Reserved
   VmicLierBIT4              = 0x0010, //!< Reserved

   VmicLierRESET_RQ          = 0x0008,
   //!< Reset me request from some other node

   VmicLierINT3              = 0x0004, //!< Pending Interrupt 3
   VmicLierINT2              = 0x0002, //!< Pending Interrupt 2
   VmicLierINT1              = 0x0001  //!< Pending Interrupt 1
 } VmicLier;
//@}


/*! @name VMIC RFM Register NIC bits
 */
//@{
typedef enum {
   VmicNicREQUEST_RESET      = 0x00, //!< Val X000: Request target node(s) reset
   VmicNicINT_1              = 0x01, //!< Val X001: Type 1 Interrupt
   VmicNicINT_2              = 0x02, //!< Val X010: Type 2 Interrupt
   VmicNicINT_3              = 0x03, //!< Val X011: Type 3 Interrupt
   VmicNicINITIALIZED        = 0x07, //!< Val X111: I am Initialized interrupt
   VmicNicBROADCAST          = 0x08  //!< Bit 1XXX: Send to all nodes
 } VmicNic;
//@}


/*! @name VMIC 5565 RFM register memory map
 */
//@{
typedef struct {
   uint8_t  BRV;      //!< 0x00: Board Revision RO
   uint8_t  BID;      //!< 0x01: Board ID = 65  RO
   uint16_t SHORT02;  //!< 0x02: Reserved
   uint8_t  NID;      //!< 0x04: Node ID 1..256 RO
   uint8_t  BYTE05;   //!< 0x05: Reserved
   uint16_t SHORT06;  //!< 0x06: Reserved
   uint32_t LCSR1;    //!< 0x08: Local control and Status 1
   uint32_t LONG0C;   //!< 0x0C: Reserved
   uint32_t LISR;     //!< 0x10: Local Interrupt Status Register
   uint32_t LIER;     //!< 0x14: Local Interrupt Enable Register
   uint32_t NTD;      //!< 0x18: Network Target Data
   uint8_t  NTN;      //!< 0x1C: Network Target Node
   uint8_t  NIC;      //!< 0x1D: Network Interrupt Command
   uint16_t SHORT1E;  //!< 0x1E: Reserved
   uint32_t ISD1;     //!< 0x20: Int-1 Sender Data
   uint8_t  SID1;     //!< 0x24: Int-1 Sender ID
   uint8_t  BYTE25;   //!< 0x25: Reserved
   uint16_t SHORT26;  //!< 0x26: Reserved
   uint32_t ISD2;     //!< 0x28: Int-2 Sender Data
   uint8_t  SID2;     //!< 0x2C: Int-2 Sender ID
   uint8_t  BYTE2D;   //!< 0x2D: Reserved
   uint16_t SHORT2E;  //!< 0x2E: Reserved
   uint32_t ISD3;     //!< 0x30: Int-3 Sender Data
   uint8_t  SID3;     //!< 0x34: Int-3 Sender ID
   uint8_t  BYTE35;   //!< 0x35: Reserved
   uint16_t HORT36;   //!< 0x36:  Reserved
   uint32_t INITD;    //!< 0x38: Initialized Node Data
   uint8_t  INITN;    //!< 0x3C: Initialized Node ID
   uint8_t  BYTE3D;   //!< 0x3D: Reserved
   uint16_t SHORT3E;  //!< 0x3E: Reserved
} __attribute__ ((packed)) VmicRfmMap;
//@}


/*! @name VMIC 5565 RFM register offsets
 */
//@{
typedef enum {
   VmicRfmBRV      = 0x00, //!< Board Revision RO
   VmicRfmBID      = 0x01, //!< Board ID = 65  RO
   VmicRfmSHORT02  = 0x02, //!< Reserved
   VmicRfmNID      = 0x04, //!< Node ID 1..256 RO
   VmicRfmBYTE05   = 0x05, //!< Reserved
   VmicRfmSHORT06  = 0x06, //!< Reserved
   VmicRfmLCSR1    = 0x08, //!< Local control and Status 1
   VmicRfmLONG0C   = 0x0C, //!< Reserved
   VmicRfmLISR     = 0x10, //!< Local Interrupt Status Register
   VmicRfmLIER     = 0x14, //!< Local Interrupt Enable Register
   VmicRfmNTD      = 0x18, //!< Network Target Data
   VmicRfmNTN      = 0x1C, //!< Network Target Node
   VmicRfmNIC      = 0x1D, //!< Network Interrupt Command
   VmicRfmSHORT1E  = 0x1E, //!< Reserved
   VmicRfmISD1     = 0x20, //!< Int-1 Sender Data
   VmicRfmSID1     = 0x24, //!< Int-1 Sender ID
   VmicRfmBYTE25   = 0x25, //!< Reserved
   VmicRfmSHORT26  = 0x26, //!< Reserved
   VmicRfmISD2     = 0x28, //!< Int-2 Sender Data
   VmicRfmSID2     = 0x2C, //!< Int-2 Sender ID
   VmicRfmBYTE2D   = 0x2D, //!< Reserved
   VmicRfmSHORT2E  = 0x2E, //!< Reserved
   VmicRfmISD3     = 0x30, //!< Int-3 Sender Data
   VmicRfmSID3     = 0x34, //!< Int-3 Sender ID
   VmicRfmBYTE35   = 0x35, //!< Reserved
   VmicRfmSHORT36  = 0x36, //!< Reserved
   VmicRfmINITD    = 0x38, //!< Initialized Node Data
   VmicRfmINITN    = 0x3C, //!< Initialized Node ID
   VmicRfmBYTE3D   = 0x3D, //!< Reserved
   VmicRfmSHORT3E  = 0x3E  //!< Reserved
 } VmicRfm;
//@}

#endif
