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

/* Maximum number of dmachain elements that dmachain can handle */
#define XmemDrvrMAX_DMA_CHAIN ((XmemDrvrMAX_SEGMENT_SIZE)/((PAGESIZE)+1))

/**
 * \brief Module context
 * @InUse: Module context in use
 * @Handle: Handle from DRM
 * @IVector: Resulting interrupt vector
 * @LocalOpen: Plx9656 local conig open
 * @ConfigOpen: Plx9656 configuration open
 * @RfmOpen: VMIC RFM register space
 * @RawOpen: VMIC SDRAM space
 * @PciSlot: Module geographic ID PCI Slot
 * @ModuleIndex: Which module we're working on
 * @NodeId: Node 1..256
 * @RdDmaSemaphore: DMA 0 engine sem, used for reading
 * @WrDmaSemaphore: DMA 1 engine sem, used for writing
 * @BusySemaphore: module mutex
 * @RdTimer: Read DMA timer
 * @WrTimer: Write DMA timer
 * @BusyTimer: Module busy timer
 * @Map: Pointer to the real hardware
 * @SDRam: Direct access to VMIC SD Ram
 * @Local: Local PLX register space
 * @Command: Command bits settings
 * @InterruptEnable: Enabled interrupts mask
 * @Clients: Clients' interrupts
 * @DmaOp: DMA mapping info
 * @Dma: for CDCM internal use only
 * @dmachain: stores the return value from mmchain. Protected by @BusySemaphore
 * @Tempbuf: temp buffer, allocated during the installation
 * @TempbufSemaphore: protect Tempbuf
 * @TempbufTimer: Tempbuf timer
 */
typedef struct {
	unsigned long		InUse;
	struct drm_node_s	*Handle;
	unsigned long		IVector;
	unsigned long		LocalOpen;
	unsigned long		ConfigOpen;
	unsigned long		RfmOpen;
	unsigned long		RawOpen;
	unsigned long		PciSlot;
	unsigned long		ModuleIndex;
	unsigned long		NodeId;
	int			RdDmaSemaphore;
	int			WrDmaSemaphore;
	int			BusySemaphore;
	int			RdTimer;
	int			WrTimer;
	int			BusyTimer;
	VmicRfmMap		*Map;
	unsigned char		*SDRam;
	PlxLocalMap		*Local;
	XmemDrvrScr		Command;
	VmicLier		InterruptEnable;
	XmemDrvrIntr		Clients[XmemDrvrCLIENT_CONTEXTS];
	XmemDmaOp		DmaOp;
	struct cdcm_dmabuf	Dma;
	struct dmachain		dmachain[XmemDrvrMAX_DMA_CHAIN];
	void			*Tempbuf;
	int			TempbufSemaphore;
	int			TempbufTimer;
} XmemDrvrModuleContext;

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
