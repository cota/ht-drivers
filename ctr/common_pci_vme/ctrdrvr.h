/* ********************************************************************************************* */
/*                                                                                               */
/* CTR (Controls Timing Receiver PMC) Public include file containing driver structures, that     */
/* are used by client tasks.                                                                     */
/*                                                                                               */
/* Julian Lewis 19th May 2003                                                                    */
/*                                                                                               */
/* ********************************************************************************************* */

#ifndef CTRDRVR
#define CTRDRVR

#include <ctrhard.h>   /* Hardware interface for expert and diagnostic clients */
#include <hptdc.h>     /* HPTDC (Time to Digital Convertor) definitions */

/* Maximum number of simultaneous clients for driver */

#define CtrDrvrCLIENT_CONTEXTS 16

/* Maximum number of CTR modules on one host processor */

#ifdef CTR_PMC
#define CtrDrvrMODULE_CONTEXTS 16
#else
#define CtrDrvrMODULE_CONTEXTS 16
#endif

/* *********************************************************************************** */
/* In some rare cases, it is necessary to distinguish between CTR device types.        */
/* This can control the open and install routines so that more than one device type    */
/* can be used at the sam time on a single host.                                       */

typedef enum {
   CtrDrvrDeviceANY,    /* Any CTR device, don't care which */
   CtrDrvrDeviceCTRI,   /* CTR PCI type */
   CtrDrvrDeviceCTRP,   /* CTR PMC type */
   CtrDrvrDeviceCTRV,   /* CTR VME type */
   CtrDrvrDeviceCTRE,   /* CTR VME modified for Beam Energy Safe Beam Parameters */
   CtrDrvrDEVICES
 } CtrDrvrDevice;

/* *********************************************************************************** */
/* All client access to the ctrp timing receivers is through the concept of the timing */
/* object, as per the equipment module classes PTIM, and CTIM. Event frame layouts are */
/* usually hidden from normal clients. They are available in the ctrphard.h interface. */

#define CtrDrvrCONNECTIONS 512

typedef enum {
   CtrDrvrConnectionClassHARD, /* Used when connecting to CTR hardware interrupt  */
   CtrDrvrConnectionClassCTIM, /* Used when connecting to incomming timing on GMT */
   CtrDrvrConnectionClassPTIM  /* Used when connecting to a local PTIM timing     */
 } CtrDrvrConnectionClass;

typedef struct {
   unsigned int            Module;       /* The module 1..n */
   CtrDrvrConnectionClass  EqpClass;     /* Incomming CTIM or local PTIM timing */
   unsigned int            EqpNum;       /* Either a MASK or PTIM or CTIM object  */
 } CtrDrvrConnection;

typedef struct {
   unsigned int Size;
   unsigned int Pid[CtrDrvrCLIENT_CONTEXTS];
 } CtrDrvrClientList;

typedef struct {
   unsigned int      Pid;
   unsigned int      Size;
   CtrDrvrConnection  Connections[CtrDrvrCONNECTIONS];
 } CtrDrvrClientConnections;

typedef struct {
   CtrDrvrConnection Connection;       /* PTIM, CTIM or HARD object */
   unsigned int      TriggerNumber;    /* Trigger number 1..n */
   unsigned int      InterruptNumber;  /* 0,1..8 (Counter) 9..n Hardware */
   unsigned int      Ctim;             /* Ctim trigger */
   CtrDrvrEventFrame Frame;            /* Triggering event frame */
   CtrDrvrCTime      TriggerTime;      /* Time counter loaded */
   CtrDrvrCTime      StartTime;        /* Time counter was started */
   CtrDrvrCTime      OnZeroTime;       /* Time of interrupt */
 } CtrDrvrReadBuf;

typedef struct {
   unsigned int      TriggerNumber;    /* Trigger number 0..n 0=>First in connection */
   CtrDrvrConnection Connection;       /* Connection for trigger */
   unsigned int      Payload;          /* Used when simulating CTIMs */
 } CtrDrvrWriteBuf;

/* ******************************************************* */
/* Direct access to triggers and counter configurations    */
/* stored in the CTR ram.                                  */

typedef struct {
   unsigned int                TriggerNumber;   /* 1..n    */
   unsigned int                EqpNum;          /* Object  */
   CtrDrvrConnectionClass      EqpClass;        /* Class   */
   CtrDrvrTrigger              Trigger;         /* Trigger */
   CtrDrvrCounterConfiguration Config;          /* Counter */
 } CtrDrvrAction;

/* ******************************************************* */
/* CTIM object bindings                                    */

#define CtrDrvrCtimOBJECTS 2048

typedef struct {
   unsigned int      EqpNum;
   CtrDrvrEventFrame Frame;
 } CtrDrvrCtimBinding;

typedef struct {
   unsigned short     Size;
   CtrDrvrCtimBinding Objects[CtrDrvrCtimOBJECTS];
 } CtrDrvrCtimObjects;

/* ******************************************************* */
/* PTIM object bindings                                    */

#define CtrDrvrPtimOBJECTS 2048

typedef struct {
   unsigned int  EqpNum;
   unsigned char  ModuleIndex;
   unsigned char  Counter;
   unsigned short Size;
   unsigned short StartIndex;
 } CtrDrvrPtimBinding;

typedef struct {
   unsigned short     Size;
   CtrDrvrPtimBinding Objects[CtrDrvrPtimOBJECTS];
 } CtrDrvrPtimObjects;

/* ***************************************************** */
/* Module descriptor                                     */

#ifdef CTR_PCI
typedef enum {
   CtrDrvrModuleTypeCTR,    /* Controls Timing Receiver */
   CtrDrvrModuleTypePLX     /* Uninitialized Plx9030 PCI chip */
 } CtrDrvrModuleType;

typedef struct {
   CtrDrvrModuleType ModuleType;
   unsigned int      PciSlot;
   unsigned int      ModuleNumber;
   unsigned int      DeviceId;
   unsigned int      VendorId;
   unsigned long     MemoryMap;
   unsigned long     LocalMap;
 } CtrDrvrModuleAddress;
#endif

#ifdef CTR_VME
typedef struct {
   unsigned int   *VMEAddress;         /* Base address for main logig A24,D32 */
   unsigned short *JTGAddress;         /* Base address for       JTAG A16,D16 */
   unsigned int    InterruptVector;    /* Interrupt vector number */
   unsigned int    InterruptLevel;     /* Interrupt level (2 usually) */
   unsigned int   *CopyAddress;        /* Copy of VME address */
 } CtrDrvrModuleAddress;
#endif

/* ***************************************************** */
/* The compilation dates in UTC time for components.     */

typedef enum {
   CtrDrvrHardwareTypeNONE,
   CtrDrvrHardwareTypeCTRP,
   CtrDrvrHardwareTypeCTRI,
   CtrDrvrHardwareTypeCTRV
 } CtrDrvrHardwareType;

#define CtrDrvrHardwareTYPES 4

typedef struct {
   unsigned int        VhdlVersion;  /* VHDL Compile date */
   unsigned int        DrvrVersion;  /* Drvr Compile date */
   CtrDrvrHardwareType HardwareType; /* Hardware type of module */
 } CtrDrvrVersion;

/* ***************************************************** */
/* HPTDC IO buffer                                       */

typedef struct {
   unsigned int        Size;   /* Number of HPTDC regs   */
   unsigned int        Pflg;   /* Parity flag            */
   unsigned int        Rflg;   /* Reset state flag       */
   HptdcCmd            Cmd;    /* Command                */
   HptdcRegisterVlaue  *Wreg;  /* Write  to HPTDC buffer */
   HptdcRegisterVlaue  *Rreg;  /* Read from HPTDC buffer */
 } CtrDrvrHptdcIoBuf;

/* ***************************************************** */
/* Counter configuration IO buffer                       */

typedef struct {
   CtrDrvrCounter Counter;
   CtrDrvrCounterConfiguration Config;
 } CtrDrvrCounterConfigurationBuf;

/* ***************************************************** */
/* Counter history IO buffer                             */

typedef struct {
   CtrDrvrCounter Counter;
   CtrDrvrCounterHistory History;
 } CtrDrvrCounterHistoryBuf;

/* ***************************************************** */
/* Counter remote command IO buffer                      */

typedef struct {
   CtrDrvrCounter Counter;
   CtrDrvrRemote Remote;
 } CtrdrvrRemoteCommandBuf;

/* ***************************************************** */
/* Counter output mask IO buffer                         */

typedef enum {
   CtrDrvrPolarityTTL,
   CtrDrvrPolarityTTL_BAR
 } CtrDrvrPolarity;

typedef struct {
   CtrDrvrCounter Counter;
   CtrDrvrCounterMask Mask;
   CtrDrvrPolarity Polarity;
 } CtrDrvrCounterMaskBuf;

/* ***************************************************** */
/* Telegram IO buffer                                    */

typedef struct {
   CtrDrvrMachine Machine;
   CtrDrvrTgm Telegram;
 } CtrDrvrTgmBuf;

/* ***************************************************** */
/* Raw IO                                                */

typedef struct {
   unsigned int Size;       /* Number int to read/write */
   unsigned int Offset;     /* Long offset address space */
   unsigned int *UserArray; /* Callers data area for  IO */
 } CtrDrvrRawIoBlock;

/* ***************************************************** */
/* Reception errors                                      */

typedef struct {
   unsigned int LastReset;     /* UTC Second of last reset */
   unsigned int PartityErrs;   /* Number of parity errors since last reset */
   unsigned int SyncErrs;      /* Number of frame synchronization errors since last reset */
   unsigned int TotalErrs;     /* Total number of IO errors since last reset */
   unsigned int CodeViolErrs;  /* Number of code violations since last reset */
   unsigned int QueueErrs;     /* Number of input Queue overflows since last reset */
 } CtrDrvrReceptionErrors;

/* ***************************************************** */
/* Board chip identity                                   */

typedef struct {
   unsigned int IdLSL;         /* ID Chip value Least Sig 32-bits */
   unsigned int IdMSL;         /* ID Chip value Most  Sig 32-bits */
 } CtrDrvrBoardId;

/* ***************************************************** */
/* Very special ISR debug code                           */

#define CtrDrvrDEBUG_ISR 0xFFFF

/* ***************************************************** */
/* Define the IOCTLs                                     */

typedef enum {

   /* Standard IOCTL Commands for timing users and developers */

   CtrDrvrSET_SW_DEBUG,           /* 00 Set driver debug mode */
   CtrDrvrGET_SW_DEBUG,

   CtrDrvrGET_VERSION,            /* 01 Get version date */

   CtrDrvrSET_TIMEOUT,            /* 02 Set the read timeout value */
   CtrDrvrGET_TIMEOUT,            /* 03 Get the read timeout value */

   CtrDrvrSET_QUEUE_FLAG,         /* 04 Set queuing capabiulities on off */
   CtrDrvrGET_QUEUE_FLAG,         /* 05 1=Q_off 0=Q_on */
   CtrDrvrGET_QUEUE_SIZE,         /* 06 Number of events on queue */
   CtrDrvrGET_QUEUE_OVERFLOW,     /* 07 Number of missed events */

   CtrDrvrGET_MODULE_DESCRIPTOR,  /* 08 Get the current Module descriptor */
   CtrDrvrSET_MODULE,             /* 09 Select the module to work with */
   CtrDrvrGET_MODULE,             /* 10 Which module am I working with */
   CtrDrvrGET_MODULE_COUNT,       /* 11 The number of installed modules */

   CtrDrvrRESET,                  /* 12 Reset the module, re-establish connections */
   CtrDrvrENABLE,                 /* 13 Enable CTR module event reception */
   CtrDrvrGET_STATUS,             /* 14 Read module status */

   CtrDrvrGET_INPUT_DELAY,        /* 15 Get input delay in 25ns ticks */
   CtrDrvrSET_INPUT_DELAY,        /* 16 Set input delay in 25ns ticks */

   CtrDrvrGET_CLIENT_LIST,        /* 17 Get the list of driver clients */

   CtrDrvrCONNECT,                /* 18 Connect to an object interrupt */
   CtrDrvrDISCONNECT,             /* 19 Disconnect from an object interrupt */
   CtrDrvrGET_CLIENT_CONNECTIONS, /* 20 Get the list of a client connections on module */

   CtrDrvrSET_UTC,                /* 21 Set Universal Coordinated Time for next PPS tick */
   CtrDrvrGET_UTC,                /* 22 Latch and read the current UTC time */

   CtrDrvrGET_CABLE_ID,           /* 23 Cables telegram ID */

   CtrDrvrGET_ACTION,             /* 24 Low level direct access to CTR RAM tables */
   CtrDrvrSET_ACTION,             /* 25 Set may not modify the bus interrupt settings */

   CtrDrvrCREATE_CTIM_OBJECT,     /* 26 Create a new CTIM timing object */
   CtrDrvrDESTROY_CTIM_OBJECT,    /* 27 Destroy a CTIM timing object */
   CtrDrvrLIST_CTIM_OBJECTS,      /* 28 Returns a list of created CTIM objects */
   CtrDrvrCHANGE_CTIM_FRAME,      /* 29 Change the frame of an existing CTIM object */

   CtrDrvrCREATE_PTIM_OBJECT,     /* 30 Create a new PTIM timing object */
   CtrDrvrDESTROY_PTIM_OBJECT,    /* 31 Destroy a PTIM timing object */
   CtrDrvrLIST_PTIM_OBJECTS,      /* 32 List PTIM timing objects */
   CtrDrvrGET_PTIM_BINDING,       /* 33 Search for a PTIM object binding */

   CtrDrvrGET_OUT_MASK,           /* 34 Counter output routing mask */
   CtrDrvrSET_OUT_MASK,           /* 35 Counter output routing mask */
   CtrDrvrGET_COUNTER_HISTORY,    /* 36 One deep history of counter */

   CtrDrvrGET_REMOTE,             /* 37 Counter Remote/Local status */
   CtrDrvrSET_REMOTE,             /* 38 Counter Remote/Local status */
   CtrDrvrREMOTE,                 /* 39 Remote control counter */

   CtrDrvrGET_CONFIG,             /* 40 Get a counter configuration */
   CtrDrvrSET_CONFIG,             /* 41 Set a counter configuration */

   CtrDrvrGET_PLL,                /* 42 Get phase locked loop parameters */
   CtrDrvrSET_PLL,                /* 43 Set phase locked loop parameters */
   CtrDrvrSET_PLL_ASYNC_PERIOD,   /* 44 Set PLL asynchronous period */
   CtrDrvrGET_PLL_ASYNC_PERIOD,   /* 45 Get PLL asynchronous period */

   CtrDrvrREAD_TELEGRAM,          /* 46 Read telegrams from CTR */
   CtrDrvrREAD_EVENT_HISTORY,     /* 47 Read incomming event history */

   /* ============================================================ */
   /* Hardware specialists IOCTL Commands to maintain and diagnose */
   /* the chips on the CTR board. Not for normal timing users.     */

   CtrDrvrJTAG_OPEN,              /* 48 Open JTAG interface to the Xilinx FPGA */
   CtrDrvrJTAG_READ_BYTE,         /* 49 Read back uploaded VHDL bit stream byte */
   CtrDrvrJTAG_WRITE_BYTE,        /* 50 Write compiled VHDL bit stream byte */
   CtrDrvrJTAG_CLOSE,             /* 51 Close JTAG interface */

   CtrDrvrHPTDC_OPEN,             /* 52 Open HPTDC JTAG interface */
   CtrDrvrHPTDC_IO,               /* 53 Perform HPTDC IO operation */
   CtrDrvrHPTDC_CLOSE,            /* 54 Close HPTDC JTAG interface */

   CtrDrvrRAW_READ,               /* 55 Raw read  access to mapped card for debug */
   CtrDrvrRAW_WRITE,              /* 56 Raw write access to mapped card for debug */

   CtrDrvrGET_RECEPTION_ERRORS,   /* 57 Timing fram reception error status */
   CtrDrvrGET_IO_STATUS,          /* 58 Status of module inputs */
   CtrDrvrGET_IDENTITY,           /* 59 Identity of board from ID chip */

   CtrDrvrSET_DEBUG_HISTORY,      /* 60 All events get logged in event history */
   CtrDrvrSET_BRUTAL_PLL,         /* 61 Control how UTC PLL relocks */
   CtrDrvrGET_MODULE_STATS,       /* 62 Get module statistics */

   CtrDrvrSET_CABLE_ID,           /* 63 Needed when no ID events sent */

   CtrDrvrIOCTL_64,               /* 64 Spare */
   CtrDrvrIOCTL_65,               /* 65 Spare */
   CtrDrvrIOCTL_66,               /* 66 Spare */
   CtrDrvrIOCTL_67,               /* 67 Spare */
   CtrDrvrIOCTL_68,               /* 68 Spare */
   CtrDrvrIOCTL_69,               /* 69 Spare */

   /* ============================================================ */
   /* Module specific IOCTL commands, Can't be used in a library!! */

#ifdef CTR_VME
   CtrDrvrGET_OUTPUT_BYTE,        /* 57 VME P2 output byte number */
   CtrDrvrSET_OUTPUT_BYTE,        /* 58 VME P2 output byte number */
#endif

#ifdef CTR_PCI
   CtrDrvrSET_MODULE_BY_SLOT,     /* 57 Select the module to work with by slot ID */
   CtrDrvrGET_MODULE_SLOT,        /* 58 Get the slot ID of the selected module */

   CtrDrvrREMAP,                  /* 59 Remap BAR2 after a config change */

   CtrDrvr93LC56B_EEPROM_OPEN,    /* 60 Open the PLX9030 configuration EEPROM 93LC56B for write */
   CtrDrvr93LC56B_EEPROM_READ,    /* 61 Read from the EEPROM 93LC56B the PLX9030 configuration */
   CtrDrvr93LC56B_EEPROM_WRITE,   /* 62 Write to the EEPROM 93LC56B a new PLX9030 configuration */
   CtrDrvr93LC56B_EEPROM_ERASE,   /* 63 Erase the EEPROM 93LC56B, deletes PLX9030 configuration */
   CtrDrvr93LC56B_EEPROM_CLOSE,   /* 64 Close EEPROM 93LC56B and load new PLX9030 configuration */

   CtrDrvrPLX9030_RECONFIGURE,    /* 65 Load EEPROM configuration into the PLX9030 */

   CtrDrvrPLX9030_CONFIG_OPEN,    /* 66 Open the PLX9030 configuration */
   CtrDrvrPLX9030_CONFIG_READ,    /* 67 Read the PLX9030 configuration registers */
   CtrDrvrPLX9030_CONFIG_WRITE,   /* 68 Write to PLX9030 configuration registers (Experts only) */
   CtrDrvrPLX9030_CONFIG_CLOSE,   /* 69 Close the PLX9030 configuration */

   CtrDrvrPLX9030_LOCAL_OPEN,     /* 70 Open the PLX9030 local configuration */
   CtrDrvrPLX9030_LOCAL_READ,     /* 71 Read the PLX9030 local configuration registers */
   CtrDrvrPLX9030_LOCAL_WRITE,    /* 72 Write the PLX9030 local configuration registers (Experts only) */
   CtrDrvrPLX9030_LOCAL_CLOSE,    /* 73 Close the PLX9030 local configuration */
#endif

   CtrDrvrLAST_IOCTL

 } CtrDrvrControlFunction;
#endif
