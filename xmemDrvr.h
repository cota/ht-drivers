/**
 * @file xmemDrvr.h
 *
 * @brief Device driver for the PCI (PMC/PCI) VMIC Reflective memory
 *
 * @author Julian Lewis AB/CO/HT CERN
 *
 * @date Created on 02/09/2004
 *
 * @version 2.0 Emilio G. Cota 15/10/2008, CDCM-compliant.
 *
 * @version 1.0 Julian Lewis 26/08/2004, Lynx/Linux compatible.
 */

#ifndef XMEM_DRVR
#define XMEM_DRVR

#include <general_both.h>

#define XmemDrvrCLIENT_CONTEXTS 16
//!< Maximum number of simultaneous clients for driver


#define XmemDrvrMODULE_CONTEXTS 2
//!< Maximum number of XMEM modules on one host processor



/*! @name number of segments and nodes
 *
 * These are set to 32 so that we can use one bit for each of them
 * with 4 bytes. Increasing the value implies driver logic changes!!
 */
//@{
#define XmemDrvrSEGMENTS 32
#define XmemDrvrNODES 32
//@}


/*! @name Node numbers
 *
 * The first 32 nodes can use the SEGMENT driver interface. The rest
 * can use direct memory access. The first node number '0' cannot be
 * accessed by this driver, node zero is illegal. */
//@{
#define XmemDrvrMAX_NODE 255
#define XmemDrvrMIN_NODE 1
#define XmemDrvrMAX_SEGMENT_SIZE (64*1024*1024)
//@}


/*! @name Debug option bits accessed by IOCTL calls
 *
 * These are accessed by XmemDrvr{GET,SET}_SW_DEBUG
 */
//@{
typedef enum {
	XmemDrvrDebugNONE   = 0x00, //!< No Debugging
	XmemDrvrDebugDEBUG  = 0x01, //!< Basic Driver call debug
	XmemDrvrDebugTRACE  = 0x02, //!< Extra tracing
	XmemDrvrDebugMODULE = 0x04  //!< Module debug (Linux)
} XmemDrvrDebug;
//@}

/*! version descriptor
 *
 * Accessed by XmemDrvrGET_VERSION
 */
typedef struct {
	unsigned long DriverVersion; //!< UTC: Unix Time of compilation
	unsigned char BoardRevision; //!< BRV: revision or model numbers
	unsigned char BoardId;       //!< BID: Should be 0x65
} XmemDrvrVersion;


/*! @name Queue flag
 *
 * Used by XmemDrvr{GET,SET}_QUEUE_FLAG
 */
//@{
typedef enum {
	XmemDrvrQueueFlagON,  //!< Interrupt event queueing On
	XmemDrvrQueueFlagOFF, //!< No interrupt event queueing
} XmemDrvrQueueFlag;
//@}


/*! Module descriptor
 *
 * Used by XmemDrvrGET_MODULE_DESCRIPTOR
 */
typedef struct {
	unsigned long     Module;  //!< 1..n where n is the last installed VMIC
	unsigned long     PciSlot; //!< Geographic address
	unsigned long     NodeId;  //!< Module node Id 1..32
	unsigned char    *Map;     //!< Pointer to the real hardware
	unsigned char    *SDRam;   //!< Direct access to VMIC SD Ram
} XmemDrvrModuleDescriptor;


/*! @name VMIC Status and Command registers (SCR)
 *
 * Used by XmemDrvrSET_COMMAND, XmemDrvrGET_STATUS
 */
//@{
#define XmemDrvrScrCMD_MASK 0xF8000001
#define XmemDrvrScrSTATAE 17

typedef enum {
	XmemDrvrScrLED_ON            = 0x80000000, //!< RW Red LED
	XmemDrvrScrTR_OFF            = 0x40000000, //!< RW Transmitter
	XmemDrvrScrDARK_ON           = 0x20000000, //!< RW Dark on Dark
	XmemDrvrScrLOOP_ON           = 0x10000000, //!< RW Test mode loop back
	XmemDrvrScrPARITY_ON         = 0x08000000, //!< RW Parity checking

	XmemDrvrScrREDUNDANT_ON      = 0x04000000,
	//!< RW Redundant transmission mode (RO)

	XmemDrvrScrROGUE_MASTER_1_ON = 0x02000000, //!< RO Rogue Master (RO)
	XmemDrvrScrROGUE_MASTER_0_ON = 0x01000000, //!< RO Rogue Master (RO)
	XmemDrvrScr128MB             = 0x00100000, //!< RO 128Mb else 64Mb (RO)
	XmemDrvrScrTX_EMPTY          = 0x00000080, //!< RO Transmit buffer empty (RO)

	XmemDrvrScrTX_ALMOST_FULL    = 0x00000040,
	//!< RO Transmit buffer almost full (RO)

	XmemDrvrScrRX_OVERFLOW       = 0x00000020,
	//!< RO Receive buffer has overflowed (RO)

	XmemDrvrScrRX_ALMOST_FULL    = 0x00000010,
	//!< RO Receive buffer is almost full (RO)

	XmemDrvrScrSYNC_LOST         = 0x00000008, //!< RO Lost sync clock (RO)

	XmemDrvrScrSIGNAL_DETECT     = 0x00000004,
	//!< RO Incomming signal (light) present (RO)

	XmemDrvrScrDATA_LOST         = 0x00000002,
	//!< RO Data is bad or has been lost (RO)

	XmemDrvrScrOWN_DATA          = 0x00000001  //!< RO Own data is present
} XmemDrvrScr;
//@}


/*! Clients list
 *
 * Used by XmemDrvrGET_CLIENT_LIST
 */
typedef struct {
	unsigned long Size;
	unsigned long Pid[XmemDrvrCLIENT_CONTEXTS];
} XmemDrvrClientList;


/*! @name Interrupt sources and enables
 *
 * Used by XmemDrvrCONNECT, XmemDrvrDISCONNECT, XmemDrvrGET_CLIENT_CONNECTIONS,
 * read() and write() entry points.
 */
//@{
#define XmemDrvrIntrSOURCES 17

typedef enum {
	XmemDrvrIntrDAEMON         = 0x10000, //!< software sync int. from daemon

	XmemDrvrIntrAC_FLAG        = 0x8000,
	//!< Auto clear Interrupt: This bit must always be On

	XmemDrvrIntrBIT14          = 0x4000, //!< Reserved
	XmemDrvrIntrPARITY_ERROR   = 0x2000, //!< Parity error

	XmemDrvrIntrWRITE_ERROR    = 0x1000,
	//!< Cant write a short or byte if parity on

	XmemDrvrIntrLOST_SYNC      = 0x0800,
	//!< PLL unlocked, data was lost, or signal lost

	XmemDrvrIntrRX_OVERFLOW    = 0x0400, //!< Receiver buffer overflow
	XmemDrvrIntrRX_ALMOST_FULL = 0x0200, //!< Receive buffer almost full
	XmemDrvrIntrDATA_ERROR     = 0x0100, //!< Bad data received, error
	XmemDrvrIntrPENDING_INIT   = 0x0080, //!< Another node needs initializing

	XmemDrvrIntrROGUE_CLOBBER  = 0x0040,
	//!< This rogue master has clobbered a rogue packet

	XmemDrvrIntrBIT5           = 0x0020, //!< Reserved
	XmemDrvrIntrBIT4           = 0x0010, //!< Reserved

	XmemDrvrIntrREQUEST_RESET  = 0x0008,
	//!< Reset me request from some other node

	XmemDrvrIntrSEGMENT_UPDATE = 0x0004,
	//!< Xmem memory segment updated, one bit per segment in data

	XmemDrvrIntrINT_2          = 0x0002, //!< Pending Interrupt 2
	XmemDrvrIntrINT_1          = 0x0001  //!< Pending Interrupt 1
} XmemDrvrIntr;
//@}

/*! Connect/Disconnect to/from interrupts
 *
 * This structured is used by the IOCTL calls to connect/disconnect to/from
 * interrupts described in the field 'Mask'.
 */
typedef struct {
	unsigned long Module; //!< Module to connect to
	XmemDrvrIntr  Mask;   //!< Interrupts
} XmemDrvrConnection;


/*! Get a client's connections
 *
 * Used by XmemDrvrGET_CLIENT_CONNECTIONS
 */
typedef struct {
	unsigned long Pid;  //!< Pid of client whos connections you want
	unsigned long Size; //!< Number of connection entries

	XmemDrvrConnection Connections[XmemDrvrMODULE_CONTEXTS];
	//!< Connections this client has:
} XmemDrvrClientConnections;


/*! @name Send interrupt data to other nodes
 */
//@{
#define XmemDrvrNicCAST 0x38
#define XmemDrvrNicTYPE 0x07

typedef enum {
	XmemDrvrNicREQUEST_RESET  = 0x00,
	//!< Val: XXX000: Request target node(s) reset

	XmemDrvrNicINT_1          = 0x01,
	//!< Val: XXX001: Type 1 Interrupt

	XmemDrvrNicINT_2          = 0x02,
	//!< Val: XXX010: Type 2 Interrupt

	XmemDrvrNicSEGMENT_UPDATE = 0x03,
	//!< Val: XXX011: Type 3 Interrupt (Segment update)

	XmemDrvrNicINITIALIZED    = 0x07,
	//!< Val: XXX111: I am Initialized interrupt

	XmemDrvrNicBROADCAST      = 0x08, //!< Bit: 001XXX: Send to all nodes
	//!< The CAST part is an exclusive bit pattern, only one bit can be set
	XmemDrvrNicMULTICAST      = 0x10, //!< Bit: 010XXX: Multicast
	//!< The CAST part is an exclusive bit pattern, only one bit can be set
	XmemDrvrNicUNICAST        = 0x20  //!< Bit: 100XXX: Unicast
	//!< The CAST part is an exclusive bit pattern, only one bit can be set
} XmemDrvrNic;


/*! Used by XmemDrvrSEND_INTERRUPT.
 * Nic pattern XX011 (INT_3) is reserved for SEGMENT_UPDATE and is also
 * used by the driver.
 *
 * Nic patterns XX100, XX101, XX110 are forbidden by the driver.
 * BROADCAST and MULTICAST Bits: they can be ORed with the value to transfer.
 *
 * Example: (XmemDrvrNicINT_1 | XmemDrvrNicBROADCAST) broadcasts INT_1 to
 * all nodes.
 */
typedef struct {
	unsigned long Module;        //!< Module to send from
	unsigned long UnicastNodeId; //!< Target Node for unicasts, value 1..256

	unsigned long MulticastMask;
	//!< For the first 32 nodes 1..32, one bit each, multicast

	XmemDrvrNic   InterruptType; //!< See above
	unsigned long Data;          //!< The data to be sent
} XmemDrvrSendBuf;
//@}


/*! Get Xmem memory address for direct I/O
 *
 * Used by XmemDrvrGET_XMEM_ADDRESS
 */
typedef struct {
	unsigned long  Module;  //!< Module to send from
	char          *Address; //!< Address of SDRam
} XmemDrvrRamAddress;


/*! @name Segment descriptors
 *
 * Used by XmemDrvrGET_SEGMENT_TABLE
 */
//@{
#define XmemDrvrSEG_NAME_SIZE 16

typedef char XmemDrvrSegName[XmemDrvrSEG_NAME_SIZE];

/*! Segment descriptor
 *
 * Used by XmemDrvrGET_SEGMENT_TABLE
 */
typedef struct {
	XmemDrvrSegName Name;    //!< Name of segment
	unsigned long   Id;      //!< The Id of this segment as a bit 1..32
	unsigned long   Size;    //!< Size of segment in bytes
	char           *Address; //!< Location in VMIC SDRAM
	unsigned long   Nodes;   //!< Nodes with write access, one bit per node
	unsigned long   User;    //!< Not used by the driver, for user data
} XmemDrvrSegDesc;         //!< Segment descriptor

/*! Segment table
 *
 * Used by XmemDrvrGET_SEGMENT_TABLE
 */
typedef struct {
	unsigned long   Used;                          //!< Which segments are in use
	XmemDrvrSegDesc Descriptors[XmemDrvrSEGMENTS]; //!< Segment descriptors
} XmemDrvrSegTable;                              //!< Segment table
//@}


/*! @name Node names
 */
//@{
#define XmemDrvrNODE_NAME_SIZE 32

typedef char XmemDrvrNodeName[XmemDrvrNODE_NAME_SIZE];


/*! Node descriptors
 * Describes nodes in the driver.
 */
typedef struct {
	XmemDrvrNodeName Name; //!< Name of node
	unsigned long    Id;   //!< The Id of this node as a bit 1..32
} XmemDrvrNodeDesc;      //!< Node descriptor

/*! Node tables
 * This is just a structure to keep node names in.
 *
 * It's not used in the driver, but it is used by the
 * test program and the library.
 */
typedef struct {
	unsigned long    Used;                       //!< Which nodes are in use
	XmemDrvrNodeDesc Descriptors[XmemDrvrNODES]; //!< Node descriptors
} XmemDrvrNodeTable;                           //!< Segment table
//@}

/*! Read/Write segment to/from local address space */
typedef struct {
	unsigned long  Module;    //!< Module to do the IO on
	unsigned long  Id;        //!< Segment Id, "one bit only"
	unsigned long  Offset;    //!< Where to start reading from
	unsigned long  Size;      //!< Amount to be read
	unsigned char *UserArray; //!< Users data buffer
	unsigned long  UpdateFlg; //!< Send SEGMENT_UPDATE flag
} XmemDrvrSegIoDesc;

/*! @name Raw IO to CONFIG LOCAL and RFM registers
 *
 * Used to I/O on CONFIG and RFM regs, through the IOCTLS:
 * XmemDrvr{CONFIG,LOCAL,RFM,RAW}_{READ,WRITE}
 */
//@{
typedef enum {
	XmemDrvrBYTE = 1, //!< 8-Bit (Byte)
	XmemDrvrWORD = 2, //!< 16-Bit (Word)
	XmemDrvrLONG = 4  //!< 32-Bit (Double Word)
} XmemDrvrSize;

/*! Raw IO operations to CONFIG LOCAL and RFM registers
 */
typedef struct {
	unsigned long  Items;     //!< Number of items
	XmemDrvrSize   Size;      //!< Size of item
	unsigned long  Offset;    //!< Target offset (Bytes)
	unsigned long *UserArray; //!< Callers data area for IO, starts at zero
} XmemDrvrRawIoBlock;
//@}

/*! @name user's buffer for the read() entry point
 *
 * The read() entry point is used by the clients to read from their queue.
 * For each of the events they suscribe to, we keep a FIFO.
 */
//@{
typedef enum {
	XmemDrvrIntIdxINT_1          = 0, //!< General purpose Interrupt 1
	XmemDrvrIntIdxINT_2          = 1, //!< General purpose Interrupt 2
	XmemDrvrIntIdxSEGMENT_UPDATE = 2, //!< Memory segment updated interrupt
	XmemDrvrIntIdxPENDING_INIT   = 3, //!< Pending init request
	XmemDrvrIntIdxDAEMON         = 4, //!< DAEMON interrupt to clients
	XmemDrvrIntIdxFIFOS               //!< Number of FIFOs
} XmemDrvrIntIdx;

/*! read() entry point structure
 *
 * The read() entry point is used by the clients to read from their queue.
 * For each of the events they suscribe to, we keep a FIFO.
 */
typedef struct {
	unsigned long Module;
	unsigned long NdData[XmemDrvrIntIdxFIFOS]; //!< Interrupting nodes data
	unsigned long NodeId[XmemDrvrIntIdxFIFOS]; //!< Interrupting node number
	XmemDrvrIntr  Mask;                        //!< Interrupt source mask
} XmemDrvrReadBuf;
//@}

/*! write() entry point structure
 *
 * This structure is the one passed from user space to the driver via write().
 * It allows more flexibility from the client's point of view: let's say
 * we have clients of type A and B. For certain events (e.g. SEGMENT_UPDATE)
 * clients A need to perform some operations before B are notified of the
 * event. Therefore B connect to a 'software interrupt from A', called
 * IntrDAEMON, while A are connected to the hardware interrupt for that event.
 * Then when it happens, A issues IntrDAEMON through a write(), waking up
 * every B client waiting on IntrDAEMON, which will obtain a proper
 * XmemDrvrReadBuf.
 *
 * Note that this doesn't impose any policy; it is up to the clients (A and B)
 * to decide what to connect to and when to issue a write().
 */
typedef struct {
	unsigned long Module;
	unsigned long NdData;
	unsigned long NodeId;
} XmemDrvrWriteBuf;


/*! @name IOCTLS
 *
 * IOCTL numbers and structures used. We use the appropriate macros here.
 */
//@{
#define XMEM_IOCTL_MAGIC 'X' //!< XMEM magic number (chosen arbitratily)

#define XMEM_IO(nr)      _IO  (XMEM_IOCTL_MAGIC, nr)
#define XMEM_IOR(nr,sz)  _IOR (XMEM_IOCTL_MAGIC, nr, sz)
#define XMEM_IOW(nr,sz)  _IOW (XMEM_IOCTL_MAGIC, nr, sz)
#define XMEM_IOWR(nr,sz) _IOWR(XMEM_IOCTL_MAGIC, nr, sz)

#define XmemDrvrILLEGAL_IOCTL           XMEM_IO (0)


#define XmemDrvrSET_SW_DEBUG            XMEM_IOW(1, long)
//!< Set driver debug mode

#define XmemDrvrGET_SW_DEBUG            XMEM_IOR(2, long)
//!< Get the driver debug mode

#define XmemDrvrGET_VERSION             XMEM_IOR(3, XmemDrvrVersion)
//!< Get version date

#define XmemDrvrSET_TIMEOUT             XMEM_IOW(4, long)
//!< Set the read() timeout value in intervals of 10ms

#define XmemDrvrGET_TIMEOUT             XMEM_IOR(5, long)
//!< Get the read() timeout value

#define XmemDrvrSET_QUEUE_FLAG          XMEM_IOW(6, long)
//!< Set queuing capabilities on/off

#define XmemDrvrGET_QUEUE_FLAG          XMEM_IOR(7, long)
//!< 1==Q_off, 0==Q_on

#define XmemDrvrGET_QUEUE_SIZE          XMEM_IOR(8, long)
//!< Number of events on queue

#define XmemDrvrGET_QUEUE_OVERFLOW      XMEM_IOR(9, long)
//!< Number of missed events

#define XmemDrvrGET_MODULE_DESCRIPTOR   XMEM_IOR(10, XmemDrvrModuleDescriptor)
//!< Get the current Module descriptor

#define XmemDrvrSET_MODULE              XMEM_IOW(11, long)
//!< Select the module to work with

#define XmemDrvrGET_MODULE              XMEM_IOR(12, long)
//!< Which module am I working with

#define XmemDrvrGET_MODULE_COUNT        XMEM_IOR(13, long)
//!< The number of installed VMIC modules

#define XmemDrvrSET_MODULE_BY_SLOT      XMEM_IOW(14, long)
//!< Select the module to work with by slot ID

#define XmemDrvrGET_MODULE_SLOT         XMEM_IOW(15, long)
//!< Get the slot ID of the selected module

#define XmemDrvrRESET                   XMEM_IO (16)
//!< Reset the module and perform full initialise

#define XmemDrvrSET_COMMAND             XMEM_IOW(17, long)
//!< Send a command to the VMIC module

#define XmemDrvrGET_STATUS              XMEM_IOR(18, long)
//!< Read module status

#define XmemDrvrGET_NODES               XMEM_IOR(19, long)
//!< Get the detected nodes

#define XmemDrvrGET_CLIENT_LIST         XMEM_IOR(20, XmemDrvrClientList)
//!< Get the list of driver's clients

#define XmemDrvrCONNECT                 XMEM_IOW(21, XmemDrvrConnection)
//!< Connect to an object interrupt

#define XmemDrvrDISCONNECT              XMEM_IOW(22, XmemDrvrConnection)
//!< Disconnect from an object interrupt

#define XmemDrvrGET_CLIENT_CONNECTIONS  XMEM_IOR(23, XmemDrvrClientConnections)
//!< Get the list of a client connections on module

#define XmemDrvrSEND_INTERRUPT          XMEM_IOW(24, XmemDrvrSendBuf)
//!< Send an interrupt to other nodes

#define XmemDrvrGET_XMEM_ADDRESS        XMEM_IOR(25, XmemDrvrRamAddress)
//!< Get mapped address of reflective memory SDRAM

#define XmemDrvrSET_SEGMENT_TABLE       XMEM_IOW(26, XmemDrvrSegTable)
//!< Set the list of all defined xmem memory segments

#define XmemDrvrGET_SEGMENT_TABLE       XMEM_IOR(27, XmemDrvrSegTable)
//!< List of all defined xmem memory segments

#define XmemDrvrREAD_SEGMENT            XMEM_IOWR(28, XmemDrvrSegIoDesc)
//!< Copy from xmem segment to local memory

#define XmemDrvrWRITE_SEGMENT           XMEM_IOWR(29, XmemDrvrSegIoDesc)
//!< Update the segment with new contents

#define XmemDrvrCHECK_SEGMENT           XMEM_IOR(30, long)
//!< Check to see if a segment has been updated since last read

#define XmemDrvrFLUSH_SEGMENTS          XMEM_IOW(31, long)
//!< Flush segments out to other nodes after PendingInit

#define XmemDrvrCONFIG_OPEN             XMEM_IO (32)
//!< Open plx9656 configuration registers block

#define XmemDrvrLOCAL_OPEN              XMEM_IO (33)
//!< Open plx9656 local registers block Local/Runtime/DMA

#define XmemDrvrRFM_OPEN                XMEM_IO (34)
//!< Open VMIC 5565 FPGA Register block RFM

#define XmemDrvrRAW_OPEN                XMEM_IO (35)
//!< Open VMIC 5565 SDRAM

#define XmemDrvrCONFIG_READ             XMEM_IOWR(36, XmemDrvrRawIoBlock)
//!< Read from plx9656 configuration registers block

#define XmemDrvrLOCAL_READ              XMEM_IOWR(37, XmemDrvrRawIoBlock)
//!< Read from plx9656 local registers block Local/Runtime/DMA

#define XmemDrvrRFM_READ                XMEM_IOWR(38, XmemDrvrRawIoBlock)
//!< Read from VMIC 5565 FPGA Register block RFM

#define XmemDrvrRAW_READ                XMEM_IOWR(39, XmemDrvrRawIoBlock)
//!< Read from VMIC 5565 SDRAM

#define XmemDrvrCONFIG_WRITE            XMEM_IOW(40, XmemDrvrRawIoBlock)
//!< Write to plx9656 configuration registers block

#define XmemDrvrLOCAL_WRITE             XMEM_IOW(41, XmemDrvrRawIoBlock)
//!< Write to plx9656 local registers block Local/Runtime/DMA

#define XmemDrvrRFM_WRITE               XMEM_IOW(42, XmemDrvrRawIoBlock)
//!< Write to VMIC 5565 FPGA Register block RFM

#define XmemDrvrRAW_WRITE               XMEM_IOWR(43, XmemDrvrRawIoBlock)
//!< Write to VMIC 5565 SDRAM

#define XmemDrvrCONFIG_CLOSE            XMEM_IO (44)
//!< Close plx9656 configuration registers block

#define XmemDrvrLOCAL_CLOSE             XMEM_IO (45)
//!< Close plx9656 local registers block Local/Runtime/DMA

#define XmemDrvrRFM_CLOSE               XMEM_IO (46)
//!< Close VMIC 5565 FPGA Register block RFM

#define XmemDrvrRAW_CLOSE               XMEM_IO (47)
//!< Close VMIC 5565 SDRAM

#define XmemDrvrSET_DMA_THRESHOLD       XMEM_IOW(48, long)
//!< Set Drivers DMA threshold

#define XmemDrvrGET_DMA_THRESHOLD       XMEM_IOR(49, long)
//!< Get Drivers DMA threshold

#define XmemDrvrLAST_IOCTL 50
//@}

/*! Info Table
 *
 * In our case this is a dummy one -- our install procedure requires it.
 */
typedef struct {
	int ModuleFlag[XmemDrvrMODULE_CONTEXTS];
} XmemDrvrInfoTable;

#endif /* !defined(XMEM_DRVR) */
