/* ========================================================================== */
/* Private definitions used by the Xmem driver, not to be exported to clients */
/* Julian Lewis AB/CO/HT CERN 31st Aug 2005                                   */
/* Julian.Lewis@cern.ch                                                       */
/* ========================================================================== */


#ifndef XMEM_DRVR_P
#define XMEM_DRVR_P

#include "plx9656_layout.h"
#include "vmic5565_layout.h"
#include "xmemDrvr.h"



typedef enum {
   XmemDrvrREAD  = 1, /* Do a read operation */
   XmemDrvrWRITE = 2  /* Do a write operation */
 } XmemDrvrIoDir;     /* IO direction */

typedef enum {
   XmemDrvrEndianNOT_SET = 0,
   XmemDrvrEndianBIG,
   XmemDrvrEndianLITTLE
 } XmemDrvrEndian;

/* ============================================================ */
/* The client context                                           */
/* ============================================================ */

/* TIMEOUT = 1 --> delay of 10ms. */
#define XmemDrvrMINIMUM_TIMEOUT 10
#define XmemDrvrDEFAULT_TIMEOUT 1000
#define XmemDrvrBUSY_TIMEOUT 200
/* Maximum queue size: */
#define XmemDrvrQUEUE_SIZE 128

typedef struct {
   unsigned short  QueueOff;
   unsigned short  Missed;
   unsigned short  Size;
   XmemDrvrReadBuf Entries[XmemDrvrQUEUE_SIZE];
 } XmemDrvrQueue;

typedef struct {
   unsigned long InUse;             /* Non zero when context is being used, else zero */
   XmemDrvrDebug Debug;             /* Clients debug settings mask */
   unsigned long Pid;               /* Clients Process ID */
   unsigned long ClientIndex;       /* Position of this client in array of clients */
   unsigned long ModuleIndex;       /* The VMIC module he is working with */
   unsigned long UpdatedSegments;   /* Updated segments mask */
   unsigned long Timeout;           /* Timeout value in ms or zero */
	    int  Timer;             /* Timer being used by client */
	    int  Semaphore;         /* Semaphore to block on read */
   XmemDrvrQueue Queue;             /* Interrupt queue */
 } XmemDrvrClientContext;

/* ============================================================ */
/* The Module context                                           */
/* ============================================================ */

/* TIMEOUT = 1 --> delay of 10ms. */
#define XmemDrvrDMA_TIMEOUT 10

typedef struct {
  char *Buffer;
  cdcm_dma_t Dma;
  unsigned long Len;
  unsigned short Flag;
} XmemDmaOp;


typedef struct {
  unsigned long    InUse;             /* Module context in use */
  struct drm_node_s *Handle;          /* Handle from DRM */
  unsigned long    IVector;           /* Resulting interrupt vector */
  unsigned long    LocalOpen;         /* Plx9656 local conig open */
  unsigned long    ConfigOpen;        /* Plx9656 configuration open */
  unsigned long    RfmOpen;           /* VMIC RFM register space */
  unsigned long    RawOpen;           /* VMIC SDRAM space */
  unsigned long    PciSlot;           /* Module geographic ID PCI Slot */
  unsigned long    ModuleIndex;       /* Which module we are */
  unsigned long    NodeId;            /* Node 1..256 */
  /* {Rd,Wr}DmaSemaphores are used for tracking the DMA transfer time */
  int     RdDmaSemaphore;    /* DMA 0 engine sem, used for reading */
  int     WrDmaSemaphore;    /* DMA 1 engine sem, used for writing */
  /* the two semaphores below are mutexes */
  int     BusySemaphore;     /* Module is busy */
  int     TempbufSemaphore;  /* Tempbuf is being used */
  int     RdTimer;           /* Read DMA timer */
  int     WrTimer;           /* Write DMA timer */
  int     BusyTimer;         /* Module busy timer */
  int     TempbufTimer;      /* Temp buffer timer */
  VmicRfmMap      *Map;               /* Pointer to the real hardware */
  unsigned char   *SDRam;             /* Direct access to VMIC SD Ram */
  PlxLocalMap     *Local;             /* Local Plx register space */
  XmemDrvrScr      Command;           /* Command bits settings */
  VmicLier         InterruptEnable;   /* Enabled interrupts mask */
  XmemDrvrIntr     Clients[XmemDrvrCLIENT_CONTEXTS]; /* Clients interrupts */
  XmemDmaOp        DmaOp;             /* DMA mapping info */
  struct cdcm_dmabuf Dma;             /* For CDCM internal use only */
  void *Tempbuf; /* temporary buffer, allocated in the installation */
 } XmemDrvrModuleContext;

/* ============================================================ */
/* Driver Working Area                                          */
/* ============================================================ */

#define XmemDrvrDEFAULT_DMA_THRESHOLD 1024

#if XmemDrvrDEFAULT_DMA_THRESHOLD > PAGESIZE
#define XmemDrvrDEFAULT_DMA_THRESHOLD PAGESIZE
#endif

typedef struct {
   unsigned long         Modules;
   unsigned long         DmaThreshold;
   unsigned long         Nodes;        /* One bit for every node */
   XmemDrvrEndian        Endian;
   XmemDrvrSegTable      SegTable;
   XmemDrvrModuleContext ModuleContexts[XmemDrvrMODULE_CONTEXTS];
   XmemDrvrClientContext ClientContexts[XmemDrvrCLIENT_CONTEXTS];
 } XmemDrvrWorkingArea;


#endif
