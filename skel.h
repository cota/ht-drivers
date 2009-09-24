/* ===================================================================== */
/* Standard definitions for all drivers. Basic driver skeleton           */
/* Julian Lewis 25th/Nov/2008                                            */
/* ===================================================================== */
#ifndef SKELINCLUDE
#define SKELINCLUDE

#include <general_both.h>
#if !defined(__KERNEL__)
# include <stdint.h> /* fixed width types */
#else
# include <linux/types.h>
#endif

/* ========================================== */
/* Start of standard SKEL driver declarations */

/* ================================ */
/* Debug tracing options per client */

#define SkelDrvrDEBUG_NAMES 6

#define SkelDrvrMAX_MAPS 10 /* max. entries in SkelDrvrMaps */

typedef enum {
   SkelDrvrDebugFlagASSERTION   = 0x01,  /* Assertion violations */
   SkelDrvrDebugFlagTRACE       = 0x02,  /* Trace all IOCTL calls */
   SkelDrvrDebugFlagWARNING     = 0x04,  /* Warning messages */
   SkelDrvrDebugFlagMODULE      = 0x08,  /* Module specific debug warnings */
   SkelDrvrDebugFlagINFORMATION = 0x10,  /* All debug information is printed */
   SkelDrvrDebugFlagEMULATION   = 0x100  /* Driver emulation ON no hardware */
 } SkelDrvrDebugFlag;

typedef struct {
   int32_t		ClientPid; /* Client PID (0=me) to set get debug flags */
   SkelDrvrDebugFlag 	DebugFlag; /* Flags to get set */
 } SkelDrvrDebug;

/* ============================================= */
/* The compilation dates in UTC time for driver  */
/* and whatever from the hardware module.        */

typedef struct {
   uint32_t   	DriverVersion; 	/* Drvr Compile date */
   char 	ModuleVersion[64]; /* Whatever we can get */
 } SkelDrvrVersion;

/* ============================================ */
/* Raw IO                                       */

typedef struct {
   uint32_t	SpaceNumber;/* Address space to read/write */
   uint32_t	Offset;     /* Hardware address byte offset */
   unsigned int	DataWidth;  /* 0 Default, 8, 16, 32 */
   uint32_t	Data;       /* Callers IO data, max 32 bits */
 } SkelDrvrRawIoBlock;

/* ====================================== */
/* The driver Connect and Read structures */

typedef struct {
   uint32_t Module;   /* The module number 1..n     */
   uint32_t ConMask;  /* Connection mask bits or index */
 } SkelDrvrConnection;

typedef struct {
   uint32_t Size;
   uint32_t Pid[SkelDrvrCLIENT_CONTEXTS];
 } SkelDrvrClientList;

typedef struct {
   uint32_t                Pid;
   uint32_t                Size;
   SkelDrvrConnection Connections[SkelDrvrCONNECTIONS];
 } SkelDrvrClientConnections;

typedef struct {
   uint32_t Second;      /* UTC time second */
   uint32_t NanoSecond;  /* Nano second in second */
 } SkelDrvrTime;

typedef struct {
   SkelDrvrConnection Connection;   /* Interrupting connection */
   SkelDrvrTime       Time;         /* No available when Second is zero */
 } SkelDrvrReadBuf;                 /* Returned from read() */

struct mapping_info {
   unsigned int	 SpaceNumber;
   unsigned long Mapped;
};

typedef struct {
   unsigned int		Mappings;
   struct mapping_info 	Maps[SkelDrvrMAX_MAPS];
} SkelDrvrMaps;

/* ============================================================= */
/* Standard status definitions: A value of ZERO is the BAD state */

typedef enum {
   SkelDrvrStandardStatusNO_ISR        = 0x001,   /* No ISR installed */
   SkelDrvrStandardStatusBUS_ERROR     = 0x002,   /* Bus error detected */
   SkelDrvrStandardStatusDISABLED      = 0x004,   /* Hardware is not enabled */
   SkelDrvrStandardStatusHARDWARE_FAIL = 0x008,   /* Hardware has failed */
   SkelDrvrStandardStatusWATCH_DOG     = 0x010,   /* Hardware watch dog time out */
   SkelDrvrStandardStatusBUS_FAULT     = 0x020,   /* Bus error(s) detected */
   SkelDrvrStandardStatusFLASH_OPEN    = 0x040,   /* Flash memory open */
   SkelDrvrStandardStatusEMULATION     = 0x080,   /* Hardware emulation ON */
   SkelDrvrStandardStatusNO_HARDWARE   = 0x100,   /* Hardware has not been installed */
   SkelDrvrStandardStatusIDLE          = 0x200,   /* Idle state */
   SkelDrvrStandardStatusBUSY          = 0x400,   /* Busy state */
   SkelDrvrStandardStatusREADY         = 0x800,   /* Data ready */
   SkelDrvrStandardStatusHARDWARE_DBUG = 0x1000   /* Hardware debug mode */
 } SkelDrvrStandardStatus;

typedef struct {
   SkelDrvrStandardStatus StandardStatus;
   uint32_t               HardwareStatus;
 } SkelDrvrStatus;

/* ==================== */
/* Standard IOCTL calls */

#define SKEL_IOCTL_MAGIC '#' //!< SKEL magic number (chosen arbitrarily)

#define SKEL_IO(nr)	   _IO(SKEL_IOCTL_MAGIC, nr)
#define SKEL_IOR(nr,sz)   _IOR(SKEL_IOCTL_MAGIC, nr, sz)
#define SKEL_IOW(nr,sz)   _IOW(SKEL_IOCTL_MAGIC, nr, sz)
#define SKEL_IOWR(nr,sz) _IOWR(SKEL_IOCTL_MAGIC, nr, sz)

/* Standard SKEL driver IOCTL call definitions and arguments. */

#define SkelDrvrIoctlSET_DEBUG		SKEL_IOW( 0, SkelDrvrDebug)
//!< Set driver debug mode (SkelDrvrDebug)
#define SkelDrvrIoctlGET_DEBUG		SKEL_IOR( 1, SkelDrvrDebug)
//!< Get driver debug mode (SkelDrvrDebug)

#define SkelDrvrIoctlGET_VERSION	SKEL_IOR( 2, SkelDrvrVersion)
//!< Get version date

#define SkelDrvrIoctlSET_TIMEOUT	SKEL_IOW( 3, uint32_t)
//!< Set the read timeout value
#define SkelDrvrIoctlGET_TIMEOUT	SKEL_IOR( 4, uint32_t)
//!< Get the read timeout value

#define SkelDrvrIoctlSET_QUEUE_FLAG	SKEL_IOW( 5, uint32_t)
//!< Set queuing capabilities on/off
#define SkelDrvrIoctlGET_QUEUE_FLAG	SKEL_IOR( 6, uint32_t)
//!< TRUE=Q_off FALSE=Q_on
#define SkelDrvrIoctlGET_QUEUE_SIZE	SKEL_IOR( 7, uint32_t)
//!< Number of events on queue
#define SkelDrvrIoctlGET_QUEUE_OVERFLOW	SKEL_IOR( 8, uint32_t)
//!< Number of missed events

#define SkelDrvrIoctlSET_MODULE		SKEL_IOW( 9, uint32_t)
//!< Select the module to work with
#define SkelDrvrIoctlGET_MODULE		SKEL_IOR(10, uint32_t)
//!< Which module am I working with
#define SkelDrvrIoctlGET_MODULE_COUNT	SKEL_IOR(11, uint32_t)
//!< The number of installed modules
#define SkelDrvrIoctlGET_MODULE_MAPS	SKEL_IOWR(12, SkelDrvrMaps)
//!< Get the VME module base address

#define SkelDrvrIoctlCONNECT		SKEL_IOW(13, SkelDrvrConnection)
//!< Connect to an object interrupt
#define SkelDrvrIoctlGET_CLIENT_LIST	SKEL_IOR(14, SkelDrvrClientList)
//!< Get list of driver client PIDs
#define SkelDrvrIoctlGET_CLIENT_CONNECTIONS SKEL_IOWR(15, SkelDrvrClientConnections)
//!< Get list of client connections

#define SkelDrvrIoctlENABLE		SKEL_IOW(16, uint32_t)
//!< Enable=TRUE Disable=FALSE module */
#define SkelDrvrIoctlRESET		SKEL_IO (17)
//!< Reset the module, initialise, re-establish all connections */

#define SkelDrvrIoctlGET_STATUS		SKEL_IOR(18, SkelDrvrStatus)
//!< Read standard module status
#define SkelDrvrIoctlGET_CLEAR_STATUS	SKEL_IOR(19, SkelDrvrStatus)
//!< Read and clear standard module status

#define SkelDrvrIoctlRAW_READ		SKEL_IOWR(20, SkelDrvrRawIoBlock)
//!< Raw read  access to card for debug
#define SkelDrvrIoctlRAW_WRITE		SKEL_IOWR(21, SkelDrvrRawIoBlock)
//!< Raw write access to card for debug

#define SkelDrvrIoctlJTAG_OPEN		SKEL_IO (22)
//!< Open JTAG interface to the Xilinx FPGA
#define SkelDrvrIoctlJTAG_READ_BYTE	SKEL_IOR(23, uint32_t)
//!< Read back uploaded VHDL bit stream byte
#define SkelDrvrIoctlJTAG_WRITE_BYTE	SKEL_IOW(24, uint32_t)
//!< Write compiled VHDL bit stream byte
#define SkelDrvrIoctlJTAG_CLOSE		SKEL_IO (25)
//!< Close JTAG interface

#define SkelDrvrIoctlLAST_STANDARD	SKEL_IO (26)

#define SkelDrvrSPECIFIC_IOCTL_OFFSET (_IOC_NR(SkelDrvrIoctlLAST_STANDARD) + 1)

/* compatibility with utils/fpga_loader/ports.c */
#define JTAG_READ_BYTE	SkelDrvrIoctlJTAG_READ_BYTE
#define JTAG_WRITE_BYTE	SkelDrvrIoctlJTAG_WRITE_BYTE


#endif
