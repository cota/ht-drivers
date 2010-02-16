/* ********************************************************************************************* */
/*                                                                                               */
/* CTR (Controls Timing Receiver) Private include file containing driver structures.             */
/*                                                                                               */
/* Julian Lewis 26th Aug 2003                                                                    */
/*                                                                                               */
/* ********************************************************************************************* */

#include <ctrhard.h>
#include <ctrdrvr.h>

#ifndef CTR_PRIVATE
#define CTR_PRIVATE

#ifdef CTR_PCI
#include <plx9030.h>

#define CERN_VENDOR_ID 0x10DC
#define CTRP_DEVICE_ID 0x0300

/* ============================================================ */
/* Mapping of JTAG pins onto plx9030 GPIOC                      */
/* ============================================================ */

#define CtrDrvrJTAG_PIN_TMS_ONE  0x4
#define CtrDrvrJTAG_PIN_TMS_OUT  0x2
#define CtrDrvrJTAG_PIN_TCK_ONE  0x20
#define CtrDrvrJTAG_PIN_TCK_OUT  0x10
#define CtrDrvrJTAG_PIN_TDI_ONE  0x100
#define CtrDrvrJTAG_PIN_TDI_OUT  0x80
#define CtrDrvrJTAG_PIN_TDO_ONE  0x800  /* These are inputs to the PLX */
#define CtrDrvrJTAG_PIN_TDO_OUT  0x400  /* Never set this bit */

/* ============================================================ */
/* Endian is needed to set up the Plx9030 mapping               */
/* ============================================================ */

typedef enum {
   CtrDrvrEndianUNKNOWN,
   CtrDrvrEndianBIG,
   CtrDrvrEndianLITTLE
 } CtrDrvrEndian;
#endif

/* ============================================================ */
/* The client context                                           */
/* ============================================================ */

#define CtrDrvrDEFAULT_TIMEOUT 2000 /* In 10 ms ticks */
#define CtrDrvrQUEUE_SIZE 64        /* Maximum queue size */

typedef struct {
   unsigned short QueueOff;
   unsigned short Missed;
   unsigned short Size;
   unsigned int   RdPntr;
   unsigned int   WrPntr;
   CtrDrvrReadBuf Entries[CtrDrvrQUEUE_SIZE];
 } CtrDrvrQueue;

typedef struct {
   unsigned int InUse;
   unsigned int DebugOn;
   unsigned int Pid;
   unsigned int ClientIndex;
   unsigned int ModuleIndex;
   unsigned int Timeout;
	    int Timer;
	    int Semaphore;
   CtrDrvrQueue Queue;
 } CtrDrvrClientContext;

/* ============================================================ */
/* The Module context                                           */
/* ============================================================ */

#define CtrDrvrMAX_BUS_ERROR_COUNT 100

typedef struct {

#ifdef CTR_PCI
   drm_node_handle             Handle;            /* Handle from DRM */
   int                         LinkId;            /* IointSet Link ID */
   unsigned int                DeviceId;          /* CTRP or PLX9030 */
   unsigned int               *Local;             /* Plx9030 local config space */
   unsigned int                LocalOpen;         /* Plx9030 local conig open */
   unsigned int                ConfigOpen;        /* Plx9030 configuration open */
   unsigned int                PciSlot;           /* Module geographic ID PCI Slot */
#endif

#ifdef CTR_VME
   CtrDrvrModuleAddress        Address;           /* Vme address */
   unsigned int                OutputByte;        /* Output byte number 1..64 */
   struct vme_berr_handler     *BerrHandler;       /* Address of VME bus error handler */
#endif

   unsigned int                IrqBalance;        /* Need to keep track of this */
   unsigned int                IVector;           /* Resulting interrupt vector */

   CtrDrvrMemoryMap            *Map;               /* Pointer to the real hardware */
   unsigned int                FlashOpen;         /* Flash for CTR VHDL or 93LC56B open */
   unsigned int                HptdcOpen;         /* Hptdc chip configuration open */
   unsigned int                ModuleIndex;       /* Which module we are */
   unsigned int                Command;           /* Command word */
   unsigned int                InterruptEnable;   /* Enabled interrupts mask */
   CtrDrvrPllAsyncPeriodNs     PllAsyncPeriodNs;  /* Pll async period */
   unsigned int                InputDelay;        /* Specified in 40MHz ticks */
   CtrDrvrPll                  Pll;               /* PLL parameters */
   unsigned int                Status;            /* Module status */
   unsigned int                InUse;             /* Module context in use */
   unsigned int                BusErrorCount;     /* Abort fatal loops with this */
   unsigned int                CableId;           /* Frig a cable ID for module */

   CtrDrvrCounterConfiguration Configs [CtrDrvrRamTableSIZE];
   unsigned int                Clients [CtrDrvrRamTableSIZE];      /* Clients interrupts PTIM/CTIM */
   CtrDrvrTrigger              Trigs   [CtrDrvrRamTableSIZE];
   unsigned int                EqpNum  [CtrDrvrRamTableSIZE];
   CtrDrvrConnectionClass      EqpClass[CtrDrvrRamTableSIZE];

   unsigned int                HardClients[CtrDrvrInterruptSOURCES];   /* Clients interrupts HARD */

   int                         Timer;             /* Handel module time delay during reset */
   int                         Semaphore;

 } CtrDrvrModuleContext;

/* ============================================================ */
/* Driver Working Area                                          */
/* ============================================================ */

typedef struct {

#ifdef CTR_PCI
   CtrDrvrEndian             Endian;
#endif

   CtrDrvrCtimObjects        Ctim;
   CtrDrvrPtimObjects        Ptim;
   unsigned int              Modules;
   CtrDrvrModuleContext      ModuleContexts[CtrDrvrMODULE_CONTEXTS];
   CtrDrvrClientContext      ClientContexts[CtrDrvrCLIENT_CONTEXTS];
 } CtrDrvrWorkingArea;

/* ============================================================ */
/* Info Table                                                   */
/* ============================================================ */

typedef struct {

#ifdef CTR_VME
   unsigned int         Modules;
   CtrDrvrModuleAddress Addresses[CtrDrvrMODULE_CONTEXTS];
#endif

   int RecoverMode; /* Needed to recover "lost" modules after FPGA crash */

 } CtrDrvrInfoTable;
#endif
