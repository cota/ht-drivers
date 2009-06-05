/**
 * @file xmemDrvrP.h
 *
 * @brief Private definitions used by the Xmem driver.
 *
 * @author Julian Lewis AB/CO/HT CERN
 *
 * @date Created on 02/09/2004
 *
 * This header file is *not* meant to be exported to clients--it's just
 * for the driver itself.
 *
 * @version 2.0 Emilio G. Cota 15/10/2008, CDCM-compliant.
 *
 * @version 1.0 Julian Lewis 31/08/2005, Lynx/Linux compatible.
 */
#ifndef XMEM_DRVR_P
#define XMEM_DRVR_P

#include <cdcm/cdcm.h>
#include <cdcm/cdcmPciDma.h>

#include <plx9656_layout.h>
#include "vmic5565_layout.h"
#include "xmemDrvr.h"

/*! @name Global definitions
 *
 * TransferDirection (Read,Write) and endianness.
 */
//@{
typedef enum {
	XmemDrvrREAD  = 1, //!< Do a read operation
	XmemDrvrWRITE = 2  //!< Do a write operation
} XmemDrvrIoDir;

typedef enum {
	XmemDrvrEndianNOT_SET = 0,
	XmemDrvrEndianBIG,
	XmemDrvrEndianLITTLE
} XmemDrvrEndian;
//@}

/*! @name Client Context
 *
 * Structures and definitions for the client's context
 */
//@{
#define XmemDrvrMINIMUM_TIMEOUT 10 	//!< TIMEOUT = 1 --> delay of 10ms
#define XmemDrvrDEFAULT_TIMEOUT 1000
#define XmemDrvrBUSY_TIMEOUT 200

#define XmemDrvrQUEUE_SIZE 128 		//!< Maximum queue size

/*! Client's queue
 */
typedef struct {
	unsigned short  QueueOff;
	unsigned short  Missed;
	unsigned short  Size;
	XmemDrvrReadBuf Entries[XmemDrvrQUEUE_SIZE];
} XmemDrvrQueue;

/*! Client's context
 */
typedef struct {
	unsigned long InUse;
	//!< Non zero when context is being used, else zero

	XmemDrvrDebug Debug;           //!< Clients debug settings mask
	unsigned long Pid;             //!< Clients Process ID

	unsigned long ClientIndex;
	//!< Position of this client in array of clients

	unsigned long ModuleIndex;     //!< The VMIC module he is working with
	unsigned long UpdatedSegments; //!< Updated segments mask
	unsigned long Timeout;         //!< Timeout value in ms or zero
	int  Timer;                    //!< Timer being used by client
	int  Semaphore;                //!< Semaphore to block on read
	XmemDrvrQueue Queue;           //!< Interrupt queue
} XmemDrvrClientContext;
//@}

/*! @name Module context
 *
 * Structures and definitions for the module's context
 */
//@{
#define XmemDrvrDMA_TIMEOUT 10 //!< TIMEOUT = 1 --> delay of 10ms.

/*! DMA Operations Struct
 *
 * This is a shared structure used to keep track of the DMA mappings
 */
typedef struct {
	char *Buffer;
	cdcm_dma_t Dma;
	unsigned long Len;
	unsigned short Flag;
} XmemDmaOp;

/*! Module context
 */
typedef struct {
	unsigned long InUse;			//!< Module context in use
	struct drm_node_s *Handle;    	//!< Handle from DRM
	unsigned long IVector;		//!< Resulting interrupt vector
	unsigned long LocalOpen;		//!< Plx9656 local conig open
	unsigned long ConfigOpen;		//!< Plx9656 configuration open
	unsigned long RfmOpen;		//!< VMIC RFM register space
	unsigned long RawOpen;		//!< VMIC SDRAM space
	unsigned long PciSlot;		//!< Module geographic ID PCI Slot
	unsigned long ModuleIndex;		//!< Which module we are
	unsigned long NodeId;			//!< Node 1..256

	int		RdDmaSemaphore;		//!< DMA 0 engine sem, used for reading
	int		WrDmaSemaphore;		//!< DMA 1 engine sem, used for writing

	int		BusySemaphore;		//!< mutex: Module is busy
	int		TempbufSemaphore;	//!< mutex: Tempbuf is being used

	int     	RdTimer;		//!< Read DMA timer
	int     	WrTimer;		//!< Write DMA timer
	int    	BusyTimer;		//!< Module busy timer
	int     	TempbufTimer;		//!< Temp buffer timer
	VmicRfmMap  	*Map;			//!< Pointer to the real hardware
	unsigned char	*SDRam;			//!< Direct access to VMIC SD Ram
	PlxLocalMap	*Local;			//!< Local Plx register space
	XmemDrvrScr	Command;		//!< Command bits settings
	VmicLier	InterruptEnable;	//!< Enabled interrupts mask

	XmemDrvrIntr	Clients[XmemDrvrCLIENT_CONTEXTS];
	//!< Clients interrupts

	XmemDmaOp	DmaOp;			//!< DMA mapping info
	struct cdcm_dmabuf Dma;		//!< For CDCM internal use only

	void *Tempbuf;
	//!< temporary buffer, allocated during the installation of the driver
} XmemDrvrModuleContext;
//@}


/*! @name Driver's Working Area
 *
 * Structures and definitions for the driver's working area (WA).
 * All the module and client contexts are kept inside the WA; memory space
 * for the WA is allocated during the installation of the driver.
 */
//@{
#define XmemDrvrDEFAULT_DMA_THRESHOLD 1024

#if XmemDrvrDEFAULT_DMA_THRESHOLD > PAGESIZE
#define XmemDrvrDEFAULT_DMA_THRESHOLD PAGESIZE
#endif

/*! Driver's working area
 */
typedef struct {
	unsigned long		Modules;
	unsigned long		DmaThreshold;
	unsigned long		Nodes;		//!< One bit for each node
	XmemDrvrEndian	Endian;
	XmemDrvrSegTable	SegTable;
	XmemDrvrModuleContext	ModuleContexts[XmemDrvrMODULE_CONTEXTS];
	XmemDrvrClientContext	ClientContexts[XmemDrvrCLIENT_CONTEXTS];
} XmemDrvrWorkingArea;
//@}

#ifdef __linux__
#define XMEM_ERR KERN_ERR
#define XMEM_WRN KERN_WARNING
#define XMEM_NFO KERN_INFO
#define XMEM_DBG KERN_DEBUG
#else /* lynx */
#define XMEM_ERR
#define XMEM_WRN
#define XMEM_NFO
#define XMEM_DBG
#endif /* __linux__ */

/* error printout */
#define XMEM_ERROR(format...)				\
        do {						\
		kkprintf(XMEM_ERR "[xmem_ERROR] ");	\
		kkprintf(format);			\
		kkprintf("\n");				\
        } while (0)

/* warning printout */
#define XMEM_WARN(format...)				\
        do {						\
		kkprintf(XMEM_WRN "[xmem_WARN] ");	\
		kkprintf(format);			\
		kkprintf("\n");				\
        } while (0)

/* informational printout */
#define XMEM_INFO(format...)			\
        do {					\
		kkprintf(XMEM_NFO "[xmem] ");	\
		kkprintf(format);		\
		kkprintf("\n");			\
        } while (0)

/* debugging printout */
#define XMEM_DEBUG(format...)						\
        do {								\
		kkprintf(XMEM_DBG "[xmem_DEBUG] %s()@%d: ", __FUNCTION__, __LINE__); \
		kkprintf(format);					\
		kkprintf("\n");						\
        } while (0)

#endif
