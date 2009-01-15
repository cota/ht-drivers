/* $Id$ */
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

/* Maximum number of simultaneous clients for driver */

#define XmemDrvrCLIENT_CONTEXTS 16

/* Maximum number of XMEM modules on one host processor */

#define XmemDrvrMODULE_CONTEXTS 2

/* These next two definitions are set to 32 so that I can use one bit */
/* for each of them in an unsigned long. Increasing the value implies */
/* driver logic changes !! */

#define XmemDrvrSEGMENTS 32
#define XmemDrvrNODES 32

/* The first 32 nodes can use the SEGMENT driver interface. The rest */
/* can use direct memory access. The first node number '0' cannot be */
/* accessed by this driver, node zero is illegal. */

#define XmemDrvrMAX_NODE 255
#define XmemDrvrMIN_NODE 1
#define XmemDrvrMAX_SEGMENT_SIZE 64*MB

/* ***************************************************** */
/* Debug option bits accessed by IOCTL calls...          */
/*   XmemDrvrSET_SW_DEBUG                                */
/*   XmemDrvrGET_SW_DEBUG                                */

typedef enum {
   XmemDrvrDebugNONE   = 0x00,   /* No Debugging */
   XmemDrvrDebugDEBUG  = 0x01,   /* Basic Driver call debug */
   XmemDrvrDebugTRACE  = 0x02,   /* Extra tracing */
   XmemDrvrDebugMODULE = 0x04    /* Module debug (Linux) */
 } XmemDrvrDebug;

/* ***************************************************** */
/* Version descriptor                                    */
/*   XmemDrvrGET_VERSION,                                */

typedef struct {
   unsigned long DriverVersion; /* UTC: Unix Time of compilation */
   unsigned char BoardRevision; /* BRV: revision or model numbers */
   unsigned char BoardId;       /* BID: Should be 0x65 */
 } XmemDrvrVersion;

/* ***************************************************** */
/* Queue flag                                            */
/*   XmemDrvrSET_QUEUE_FLAG                              */
/*   XmemDrvrGET_QUEUE_FLAG                              */

typedef enum {
   XmemDrvrQueueFlagON,     /* Interrupt event queueing On */
   XmemDrvrQueueFlagOFF,    /* No interrupt event queueing */
 } XmemDrvrQueueFlag;

/* ***************************************************** */
/* Module descriptor                                     */
/*   XmemDrvrGET_MODULE_DESCRIPTOR                       */

typedef struct {
   unsigned long     Module;    /* 1..n where n is the last installed VMIC */
   unsigned long     PciSlot;   /* Geographic address */
   unsigned long     NodeId;    /* Module node Id 1..32 */
   unsigned char    *Map;       /* Pointer to the real hardware */
   unsigned char    *SDRam;     /* Direct access to VMIC SD Ram */
 } XmemDrvrModuleDescriptor;

/* ***************************************************** */
/* VMIC Status and Command registers (SCR)               */
/*   XmemDrvrSET_COMMAND                                 */
/*   XmemDrvrGET_STATUS                                  */

#define XmemDrvrScrCMD_MASK 0xF8000001
#define XmemDrvrScrSTATAE 17

typedef enum {
   XmemDrvrScrLED_ON            = 0x80000000, /* RW Red LED */
   XmemDrvrScrTR_OFF            = 0x40000000, /* RW Transmitter */
   XmemDrvrScrDARK_ON           = 0x20000000, /* RW Dark on Dark */
   XmemDrvrScrLOOP_ON           = 0x10000000, /* RW Test mode loop back */
   XmemDrvrScrPARITY_ON         = 0x08000000, /* RW Parity checking */
   XmemDrvrScrREDUNDANT_ON      = 0x04000000, /* RW Redundant transmission mode
						 (RO) */
   XmemDrvrScrROGUE_MASTER_1_ON = 0x02000000, /* RO Rogue Master (RO) */
   XmemDrvrScrROGUE_MASTER_0_ON = 0x01000000, /* RO Rogue Master (RO) */
   XmemDrvrScr128MB             = 0x00100000, /* RO 128Mb else 64Mb (RO) */
   XmemDrvrScrTX_EMPTY          = 0x00000080, /* RO Transmit buffer empty (RO)*/
   XmemDrvrScrTX_ALMOST_FULL    = 0x00000040, /* RO Transmit buffer almost full
						 (RO) */
   XmemDrvrScrRX_OVERFLOW       = 0x00000020, /* RO Receive buffer has 
						 overflowed (RO) */
   XmemDrvrScrRX_ALMOST_FULL    = 0x00000010, /* RO Receive buffer is almost 
						 full (RO) */
   XmemDrvrScrSYNC_LOST         = 0x00000008, /* RO Lost sync clock (RO) */
   XmemDrvrScrSIGNAL_DETECT     = 0x00000004, /* RO Incomming signal (light) 
						 present (RO) */
   XmemDrvrScrDATA_LOST         = 0x00000002, /* RO Data is bad or has been 
						 lost (RO) */
   XmemDrvrScrOWN_DATA          = 0x00000001  /* RO Own data is present */
 } XmemDrvrScr;

/* ***************************************************** */
/* Clients list                                          */
/*   XmemDrvrGET_CLIENT_LIST                             */

typedef struct {
   unsigned long Size;
   unsigned long Pid[XmemDrvrCLIENT_CONTEXTS];
 } XmemDrvrClientList;

/* ***************************************************** */
/* Interrupt sources and enables                         */
/*   XmemDrvrCONNECT                                     */
/*   XmemDrvrDISCONNECT                                  */
/*   XmemDrvrGET_CLIENT_CONNECTIONS                      */
/*   read (The drivers read entry)                       */

#define XmemDrvrIntrSOURCES 17

typedef enum {
   XmemDrvrIntrAC_FLAG        = 0x8000, /* Auto clear Interrupt: This bit 
					   must always be On */
   XmemDrvrIntrBIT14          = 0x4000, /* Reserved */
   XmemDrvrIntrPARITY_ERROR   = 0x2000, /* Parity error */
   XmemDrvrIntrWRITE_ERROR    = 0x1000, /* Cant write a short or byte if 
					   parity on */
   XmemDrvrIntrLOST_SYNC      = 0x0800, /* PLL unlocked, data was lost, or 
					   signal lost */
   XmemDrvrIntrRX_OVERFLOW    = 0x0400, /* Receiver buffer overflow */
   XmemDrvrIntrRX_ALMOST_FULL = 0x0200, /* Receive buffer almost full */
   XmemDrvrIntrDATA_ERROR     = 0x0100, /* Bad data received, error */
   XmemDrvrIntrPENDING_INIT   = 0x0080, /* Another node needs initializing */
   XmemDrvrIntrROGUE_CLOBBER  = 0x0040, /* This rogue master has clobbered a 
					   rogue packet */
   XmemDrvrIntrBIT5           = 0x0020, /* Reserved */
   XmemDrvrIntrBIT4           = 0x0010, /* Reserved */
   XmemDrvrIntrREQUEST_RESET  = 0x0008, /* Reset me request from some other 
					   node */
   XmemDrvrIntrSEGMENT_UPDATE = 0x0004, /* Xmem memory segment updated, one 
					   bit per segment in data */
   XmemDrvrIntrINT_2          = 0x0002, /* Pending Interrupt 2 */
   XmemDrvrIntrINT_1          = 0x0001  /* Pending Interrupt 1 */
 } XmemDrvrIntr;

/* ***************************************************** */
/* Connect to, Disconnect from, interrupts               */

typedef struct {
   unsigned long Module;    /* Module to connect to */
   XmemDrvrIntr  Mask;      /* Interrupts */
 } XmemDrvrConnection;

/* ***************************************************** */
/* Get a clients connections                             */
/*   XmemDrvrGET_CLIENT_CONNECTIONS                      */

typedef struct {
  unsigned long      Pid;  /* Pid of client whos connections you want */
  unsigned long      Size; /* Number of connection entries */
  /* The connections it has: */
  XmemDrvrConnection Connections[XmemDrvrMODULE_CONTEXTS];
} XmemDrvrClientConnections;

/* *****************************************************
 * Send interrupt data to other nodes                   
 *   XmemDrvrSEND_INTERRUPT                             
 * 
 * The Nic pattern XX011 (INT_3) Is reserved for SEGMENT_UPDATE and is also 
 * used by the driver.
 * When synchronizing direct SDRAM access you can broadcast a SEGMENT_UPDATE
 * interrupt.
 * The Nic pattern XX100, XX101, XX110 are forbidden by the driver, 
 * they can't be used.
 * The BROADCAST and MULTICAST Bits: can use 'OR' with the Val: values.
 * Hence XmemDrvrNicINT_1 OR XmemDrvrNicBROADCAST broadcasts INT_1 to
 * all nodes. 
 * And XmemDrvrNicINT_1 OR XmemDrvrNicMULTICAST sends to the nodes in the 
 * MulticastMask.
 */

#define XmemDrvrNicCAST 0x38
#define XmemDrvrNicTYPE 0x07

typedef enum {
  /* Val: XXX000: Request target node(s) reset */
  XmemDrvrNicREQUEST_RESET  = 0x00, 
  /* Val: XXX001: Type 1 Interrupt */
  XmemDrvrNicINT_1          = 0x01, 
  /* Val: XXX010: Type 2 Interrupt */
  XmemDrvrNicINT_2          = 0x02, 
  /* Val: XXX011: Type 3 Interrupt (Segment update) */
  XmemDrvrNicSEGMENT_UPDATE = 0x03, 
  /* Val: XXX111: I am Initialized interrupt */
  XmemDrvrNicINITIALIZED    = 0x07, 

  /* The CAST part is an exclusive bit pattern, only one bit can be set */

  XmemDrvrNicBROADCAST      = 0x08, /* Bit: 001XXX: Send to all nodes */
  XmemDrvrNicMULTICAST      = 0x10, /* Bit: 010XXX: Multicast */
  XmemDrvrNicUNICAST        = 0x20  /* Bit: 100XXX: Unicast */
} XmemDrvrNic;

typedef struct {
   unsigned long Module;        /* Module to send from */
   unsigned long UnicastNodeId; /* Target Node for unicasts, value 1..256 */
   unsigned long MulticastMask; /* For the first 32 nodes 1..32, one bit each, 
				   multicast */
   XmemDrvrNic   InterruptType; /* See above */
   unsigned long Data;          /* The data to be sent */
 } XmemDrvrSendBuf;

/* ***************************************************** */
/* Get SDRAM address for direct memory access            */
/*    XmemDrvrGET_XMEM_ADDRESS                           */

typedef struct {
   unsigned long  Module;       /* Module to send from */
   char          *Address;      /* Address of SDRam    */
 } XmemDrvrRamAddress;

/* ***************************************************** */
/* Get the set of all segment descriptors                */
/*   XmemDrvrGET_SEGMENT_TABLE                           */

#define XmemDrvrSEG_NAME_SIZE 16

typedef char XmemDrvrSegName[XmemDrvrSEG_NAME_SIZE];

typedef struct {
   XmemDrvrSegName Name;    /* Name of segment */
   unsigned long   Id;      /* The Id of this segment as a bit 1..32 */
   unsigned long   Size;    /* Size of segment in bytes */
   char           *Address; /* Location in VMIC SDRAM */
   unsigned long   Nodes;   /* Nodes with write access, one bit per node */
   unsigned long   User;    /* Not used by the driver, for user data */
 } XmemDrvrSegDesc;         /* Segment descriptor */

typedef struct {
   unsigned long   Used;                         /* Which segments are in use */
   XmemDrvrSegDesc Descriptors[XmemDrvrSEGMENTS];/* Segment descriptors */
 } XmemDrvrSegTable;                             /* Segment table */

/* ***************************************************** */
/* This is just a structure to keep node names in.       */
/* Its not used in the driver, but it is used by the     */
/* test program and the library.                         */

#define XmemDrvrNODE_NAME_SIZE 32

typedef char XmemDrvrNodeName[XmemDrvrNODE_NAME_SIZE];

typedef struct {
   XmemDrvrNodeName Name;    /* Name of node */
   unsigned long    Id;      /* The Id of this node as a bit 1..32 */
 } XmemDrvrNodeDesc;         /* Node descriptor */

typedef struct {
   unsigned long    Used;                         /* Which nodes are in use */
   XmemDrvrNodeDesc Descriptors[XmemDrvrNODES];   /* Node descriptors */
 } XmemDrvrNodeTable;                             /* Segment table */

/* ***************************************************** */
/* Read/Write segment to/from local address space        */
/*   XmemDrvrREAD_SEGMENT                                */
/*   XmemDrvrWRITE_SEGMENT                               */

typedef struct {
   unsigned long  Module;    /* Module to do the IO on   */
   unsigned long  Id;        /* Segment Id one Bit Only  */
   unsigned long  Offset;    /* Where to start reading from */
   unsigned long  Size;      /* Amount to be read */
   unsigned char *UserArray; /* Users data buffer        */
   unsigned long  UpdateFlg; /* Send SEGMENT_UPDATED flag */
 } XmemDrvrSegIoDesc;

/* ***************************************************** */
/* Raw IO to CONFIG LOCAL and RFM registers              */
/*   XmemDrvrCONFIG_READ,                                */
/*   XmemDrvrLOCAL_READ,                                 */
/*   XmemDrvrRFM_READ,                                   */
/*   XmemDrvrRAW_READ,                                   */
/*   XmemDrvrCONFIG_WRITE,                               */
/*   XmemDrvrLOCAL_WRITE,                                */
/*   XmemDrvrRFM_WRITE,                                  */
/*   XmemDrvrRAW_WRITE,                                  */

typedef enum { XmemDrvrBYTE = 1, /* One 8-Bit Byte */
	       XmemDrvrWORD = 2, /* One 16-Bit Short Word */
	       XmemDrvrLONG = 4  /* One 32-Bit Long Double Word */
	     } XmemDrvrSize;

typedef struct {
  unsigned long  Items;     /* Number of items */
  XmemDrvrSize   Size;      /* Size of item */
  unsigned long  Offset;    /* Target offset (Bytes) */
  unsigned long *UserArray; /* Callers data area for IO, starts at zero */
} XmemDrvrRawIoBlock;;

/* ***************************************************** */
/* The read buffer                                       */
/*    read                                               */

typedef enum {
   XmemDrvrIntIdxINT_1          = 0, /* General purpose Interrupt 1 */
   XmemDrvrIntIdxINT_2          = 1, /* General purpose Interrupt 2 */
   XmemDrvrIntIdxSEGMENT_UPDATE = 2, /* Memory segment updated interrupt */
   XmemDrvrIntIdxPENDING_INIT   = 3, /* Pending init request */
   XmemDrvrIntIdxFIFOS               /* Number of FIFOs */
 } XmemDrvrIntIdx;

typedef struct {
   unsigned long Module;
   unsigned long NdData[XmemDrvrIntIdxFIFOS]; /* Interrupting nodes data */
   unsigned long NodeId[XmemDrvrIntIdxFIFOS]; /* Interrupting node number */
   XmemDrvrIntr  Mask;                        /* Interrupt source mask */
 } XmemDrvrReadBuf;





#if 0


/* ***************************************************** */
/* Define the IOCTLs                                     */

typedef enum {

   XmemDrvrILLEGAL_IOCTL,          /* LynxOs calls me with this */

   /* Standard IOCTL Commands */

   XmemDrvrSET_SW_DEBUG,           /* Set driver debug mode */
   XmemDrvrGET_SW_DEBUG,           /* Get the driver debug mode */

   XmemDrvrGET_VERSION,            /* Get version date */

   XmemDrvrSET_TIMEOUT,            /* Set the read timeout value in 
				      micro seconds */
   XmemDrvrGET_TIMEOUT,            /* Get the read timeout value in 
				      micro seconds */

   XmemDrvrSET_QUEUE_FLAG,         /* Set queuing capabiulities on off */
   XmemDrvrGET_QUEUE_FLAG,         /* 1=Q_off 0=Q_on */
   XmemDrvrGET_QUEUE_SIZE,         /* Number of events on queue */
   XmemDrvrGET_QUEUE_OVERFLOW,     /* Number of missed events */

   XmemDrvrGET_MODULE_DESCRIPTOR,  /* Get the current Module descriptor */
   XmemDrvrSET_MODULE,             /* Select the module to work with */
   XmemDrvrGET_MODULE,             /* Which module am I working with */
   XmemDrvrGET_MODULE_COUNT,       /* The number of installed VMIC modules */
   XmemDrvrSET_MODULE_BY_SLOT,     /* Select the module to work with by 
				      slot ID */
   XmemDrvrGET_MODULE_SLOT,        /* Get the slot ID of the selected module */

   XmemDrvrRESET,                  /* Reset the module and perform full 
				      initialize */
   XmemDrvrSET_COMMAND,            /* Send a command to the VMIC module */
   XmemDrvrGET_STATUS,             /* Read module status */
   XmemDrvrGET_NODES,              /* Get the detected nodes */

   XmemDrvrGET_CLIENT_LIST,        /* Get the list of driver clients */

   XmemDrvrCONNECT,                /* Connect to an object interrupt */
   XmemDrvrDISCONNECT,             /* Disconnect from an object interrupt */
   XmemDrvrGET_CLIENT_CONNECTIONS, /* Get the list of a client connections 
				      on module */

   XmemDrvrSEND_INTERRUPT,         /* Send an interrupt to other nodes */

   XmemDrvrGET_XMEM_ADDRESS,       /* Get physical address of reflective 
				      memory SDRAM */
   XmemDrvrSET_SEGMENT_TABLE,      /* Set the list of all defined xmem 
				      memory segments */
   XmemDrvrGET_SEGMENT_TABLE,      /* List of all defined xmem memory 
				      segments */

   XmemDrvrREAD_SEGMENT,           /* Copy from xmem segment to local memory */
   XmemDrvrWRITE_SEGMENT,          /* Update the segment with new contents */
   XmemDrvrCHECK_SEGMENT,          /* Check to see if a segment has been 
				      updated since last read */
   XmemDrvrFLUSH_SEGMENTS,         /* Flush segments out to other nodes after 
				      PendingInit */

   /* Hardware specialists raw IO access to PLX and VMIC memory */

   XmemDrvrCONFIG_OPEN,            /* Open plx9656 configuration registers 
				      block */
   XmemDrvrLOCAL_OPEN,             /* Open plx9656 local registers block 
				      Local/Runtime/DMA */
   XmemDrvrRFM_OPEN,               /* Open VMIC 5565 FPGA Register block RFM */
   XmemDrvrRAW_OPEN,               /* Open VMIC 5565 SDRAM */

   XmemDrvrCONFIG_READ,            /* Read from plx9656 configuration 
				      registers block */
   XmemDrvrLOCAL_READ,             /* Read from plx9656 local registers 
				      block Local/Runtime/DMA */
   XmemDrvrRFM_READ,               /* Read from VMIC 5565 FPGA Register 
				      block RFM */
   XmemDrvrRAW_READ,               /* Read from VMIC 5565 SDRAM */

   XmemDrvrCONFIG_WRITE,           /* Write to plx9656 configuration registers 
				      block */
   XmemDrvrLOCAL_WRITE,            /* Write to plx9656 local registers block 
				      Local/Runtime/DMA */
   XmemDrvrRFM_WRITE,              /* Write to VMIC 5565 FPGA Register block 
				      RFM */
   XmemDrvrRAW_WRITE,              /* Write to VMIC 5565 SDRAM */

   XmemDrvrCONFIG_CLOSE,           /* Close plx9656 configuration registers 
				      block */
   XmemDrvrLOCAL_CLOSE,            /* Close plx9656 local registers block 
				      Local/Runtime/DMA */
   XmemDrvrRFM_CLOSE,              /* Close VMIC 5565 FPGA Register block RFM */
   XmemDrvrRAW_CLOSE,              /* Close VMIC 5565 SDRAM */

   XmemDrvrSET_DMA_THRESHOLD,      /* Set Drivers DMA threshold */
   XmemDrvrGET_DMA_THRESHOLD,      /* Get Drivers DMA threshold */

   XmemDrvrLAST_IOCTL

 } XmemDrvrControlFunction;


#endif





/* ioctl numbers */
#define XMEM_IOCTL_MAGIC 'X'

#define XMEM_IO(nr)      _IO  (XMEM_IOCTL_MAGIC, nr)
#define XMEM_IOR(nr,sz)  _IOR (XMEM_IOCTL_MAGIC, nr, sz)
#define XMEM_IOW(nr,sz)  _IOW (XMEM_IOCTL_MAGIC, nr, sz)
#define XMEM_IOWR(nr,sz) _IOWR(XMEM_IOCTL_MAGIC, nr, sz)

#define XmemDrvrILLEGAL_IOCTL           XMEM_IO (0) 


#define XmemDrvrSET_SW_DEBUG            XMEM_IOW(1, long)
#define XmemDrvrGET_SW_DEBUG            XMEM_IOR(2, long)

#define XmemDrvrGET_VERSION             XMEM_IOR(3, XmemDrvrVersion)

#define XmemDrvrSET_TIMEOUT             XMEM_IOW(4, long)
#define XmemDrvrGET_TIMEOUT             XMEM_IOR(5, long)

#define XmemDrvrSET_QUEUE_FLAG          XMEM_IOW(6, long)
#define XmemDrvrGET_QUEUE_FLAG          XMEM_IOR(7, long)
#define XmemDrvrGET_QUEUE_SIZE          XMEM_IOR(8, long)
#define XmemDrvrGET_QUEUE_OVERFLOW      XMEM_IOR(9, long)

#define XmemDrvrGET_MODULE_DESCRIPTOR   XMEM_IOR(10, XmemDrvrModuleDescriptor)
#define XmemDrvrSET_MODULE              XMEM_IOW(11, long)
#define XmemDrvrGET_MODULE              XMEM_IOR(12, long)
#define XmemDrvrGET_MODULE_COUNT        XMEM_IOR(13, long)
#define XmemDrvrSET_MODULE_BY_SLOT      XMEM_IOW(14, long)
#define XmemDrvrGET_MODULE_SLOT         XMEM_IOW(15, long)

#define XmemDrvrRESET                   XMEM_IO (16)
#define XmemDrvrSET_COMMAND             XMEM_IOW(17, long)
#define XmemDrvrGET_STATUS              XMEM_IOR(18, long)
#define XmemDrvrGET_NODES               XMEM_IOR(19, long)

#define XmemDrvrGET_CLIENT_LIST         XMEM_IOR(20, XmemDrvrClientList)

#define XmemDrvrCONNECT                 XMEM_IOW(21, XmemDrvrConnection)
#define XmemDrvrDISCONNECT              XMEM_IOW(22, XmemDrvrConnection)
#define XmemDrvrGET_CLIENT_CONNECTIONS  XMEM_IOR(23, XmemDrvrClientConnections)

#define XmemDrvrSEND_INTERRUPT          XMEM_IOW(24, XmemDrvrSendBuf)

#define XmemDrvrGET_XMEM_ADDRESS        XMEM_IOR(25, XmemDrvrRamAddress)
#define XmemDrvrSET_SEGMENT_TABLE       XMEM_IOW(26, XmemDrvrSegTable)

#define XmemDrvrGET_SEGMENT_TABLE       XMEM_IOR(27, XmemDrvrSegTable)

#define XmemDrvrREAD_SEGMENT            XMEM_IOWR(28, XmemDrvrSegIoDesc)
#define XmemDrvrWRITE_SEGMENT           XMEM_IOWR(29, XmemDrvrSegIoDesc)
#define XmemDrvrCHECK_SEGMENT           XMEM_IOR(30, long)

#define XmemDrvrFLUSH_SEGMENTS          XMEM_IOW(31, long)

#define XmemDrvrCONFIG_OPEN             XMEM_IO (32)
#define XmemDrvrLOCAL_OPEN              XMEM_IO (33)

#define XmemDrvrRFM_OPEN                XMEM_IO (34)
#define XmemDrvrRAW_OPEN                XMEM_IO (35)

#define XmemDrvrCONFIG_READ             XMEM_IOWR(36, XmemDrvrRawIoBlock)
#define XmemDrvrLOCAL_READ              XMEM_IOWR(37, XmemDrvrRawIoBlock)

#define XmemDrvrRFM_READ                XMEM_IOWR(38, XmemDrvrRawIoBlock)
#define XmemDrvrRAW_READ                XMEM_IOWR(39, XmemDrvrRawIoBlock)

#define XmemDrvrCONFIG_WRITE            XMEM_IOW(40, XmemDrvrRawIoBlock)
#define XmemDrvrLOCAL_WRITE             XMEM_IOW(41, XmemDrvrRawIoBlock)

#define XmemDrvrRFM_WRITE               XMEM_IOW(42, XmemDrvrRawIoBlock)
#define XmemDrvrRAW_WRITE               XMEM_IOWR(43, XmemDrvrRawIoBlock)

#define XmemDrvrCONFIG_CLOSE            XMEM_IO (44)
#define XmemDrvrLOCAL_CLOSE             XMEM_IO (45)

#define XmemDrvrRFM_CLOSE               XMEM_IO (46)
#define XmemDrvrRAW_CLOSE               XMEM_IO (47)

#define XmemDrvrSET_DMA_THRESHOLD       XMEM_IOW(48, long)
#define XmemDrvrGET_DMA_THRESHOLD       XMEM_IOR(49, long)


#define XmemDrvrLAST_IOCTL 50

/* ============================================================ */
/* Info Table                                                   */
/* ============================================================ */

typedef struct {
  int ModuleFlag[XmemDrvrMODULE_CONTEXTS];
 } XmemDrvrInfoTable;

#endif
