/* ********************************************************** */
/*                                                            */
/* CTR (Controls Timing Receiver) Driver code.                */
/* Linux version using MEN A20 vmebridge driver.              */
/*                                                            */
/* Julian Lewis Wed 8th April 2009                            */
/*                                                            */
/* ********************************************************** */

#include <EmulateLynxOs.h>
#include <DrvrSpec.h>

#include <libinstkernel.h>  /* Routines to work with XML tree */

/* These next defines are needed just here for the emulation */

#define sel LynxSel
#define enable restore

#include <ctrhard.h>   /* Hardware description               */
#include <ctrdrvr.h>   /* Public driver interface            */
#include <ctrdrvrP.h>  /* Private driver structures          */

#include <ports.h>     /* XILINX Jtag stuff                  */
#include <hptdc.h>     /* High prescision time to digital convertor */

/* ========================= */
/* Driver entry points       */
/* ========================= */

irqreturn_t IntrHandler();
int CtrDrvrOpen();
int CtrDrvrClose();
int CtrDrvrRead();
int CtrDrvrWrite();
int CtrDrvrSelect();
char * CtrDrvrInstall();
int CtrDrvrUninstall();
int CtrDrvrIoctl();

extern void disable_intr();
extern void enable_intr();
extern int modules;
extern unsigned long infoaddr;

#include <vmebus.h>
#include <linux/interrupt.h>

#ifndef VME_PG_SHARED
#define VME_PG_SHARED 0
#endif

CtrDrvrWorkingArea *Wa       = NULL;

/* ============================= */
/* Forward references prototypes */
/* ============================= */

static void CancelTimeout(int *t);

static int ReadTimeout(CtrDrvrClientContext *ccon);

static int ResetTimeout(CtrDrvrModuleContext *mcon);

static void DebugIoctl(CtrDrvrControlFunction cm, char *arg);

static int GetVersion(CtrDrvrModuleContext *mcon,
		      CtrDrvrVersion       *ver);

static int PingModule(CtrDrvrModuleContext *mcon);

static int EnableInterrupts(CtrDrvrModuleContext *mcon, int msk);

static int DisableInterrupts(unsigned int           tndx,
			     unsigned int           midx,
			     CtrDrvrConnectionClass clss,
			     CtrDrvrClientContext  *ccon);

static int Reset(CtrDrvrModuleContext *mcon);

static unsigned int  GetStatus(CtrDrvrModuleContext *mcon);

static unsigned int  HptdcBitTransfer(unsigned int  *jtg,
				      unsigned int   tdi,
				      unsigned int   tms);

static void HptdcStateReset(unsigned int  *jtg);

static int HptdcCommand(CtrDrvrModuleContext *mcon,
			 HptdcCmd              cmd,
			 HptdcRegisterVlaue   *wreg,
			 HptdcRegisterVlaue   *rreg,
			 int                   size,
			 int                   pflg,
			 int                   rflg);

static CtrDrvrCTime *GetTime(CtrDrvrModuleContext *mcon);

static int SetTime(CtrDrvrModuleContext *mcon, unsigned int  tod);

static int RawIo(CtrDrvrModuleContext *mcon,
		 CtrDrvrRawIoBlock    *riob,
		 unsigned int          flag,
		 unsigned int          debg);

static CtrDrvrHwTrigger *TriggerToHard(CtrDrvrTrigger *strg);

static CtrDrvrTrigger *HardToTrigger(CtrDrvrHwTrigger *htrg);

static CtrDrvrHwCounterConfiguration *ConfigToHard(CtrDrvrCounterConfiguration *scnf);

static CtrDrvrCounterConfiguration *HardToConfig(CtrDrvrHwCounterConfiguration *hcnf);

static unsigned int  AutoShiftLeft(unsigned int  mask, unsigned int  value);

static unsigned int  AutoShiftRight(unsigned int  mask, unsigned int  value);

static unsigned int  GetPtimModule(unsigned int  eqpnum);

static int Connect(CtrDrvrConnection    *conx,
		   CtrDrvrClientContext *ccon);

static int DisConnectOne(unsigned int           tndx,
			 unsigned int           midx,
			 CtrDrvrConnectionClass clss,
			 CtrDrvrClientContext  *ccon);

static int DisConnect(CtrDrvrConnection    *conx,
		      CtrDrvrClientContext *ccon);

static int DisConnectAll(CtrDrvrClientContext *ccon);

static int AddModule(CtrDrvrModuleContext *mcon,
		     int                   index,
		     int                   flag);


static int GetClrBusErrCnt();
static int HRd(void *x);
static void HWr(int v, void *x);

static void Io32Write(unsigned int  *dst, /* Hardware destination address */
		      unsigned int  *src,
		      unsigned int  size);

static void Io32Read(unsigned int  *dst,
		     unsigned int  *src, /* Hardware Source address */
		     unsigned int  size);

static void Int32Copy(unsigned int  *dst,
		      unsigned int  *src,
		      unsigned int  size);

static void SwapWords(unsigned int  *dst,
		      unsigned int  size);

/*========================================================================*/
/* Perform VME access and check bus errors, always                        */
/*========================================================================*/

#define CLEAR_BUS_ERROR ((int) 1)
#define BUS_ERR_PRINT_THRESHOLD 10

static int bus_error_count = 0; /* For all modules */
static int isr_bus_error   = 0; /* Bus error in an ISR */
static int last_bus_error  = 0; /* Last printed bus error */

/* ==================== */

static void BusErrorHandler(struct vme_bus_error *error) {

   bus_error_count++;
}

/* ==================== */

static int GetClrBusErrCnt() {
int res;

   res = bus_error_count;

   bus_error_count = 0;
   last_bus_error = 0;
   isr_bus_error = 0;

   return res;
}

/* ==================== */

static int IHRd(void *x) {
int res;

   isr_bus_error = 0;
   res = ioread32be(x);
   if (bus_error_count > last_bus_error) isr_bus_error = 1;
   return res;
}

/* ==================== */

static int HRd(void *x) {
int res;

// udelay(10);
   res = ioread32be(x);
   if (bus_error_count > last_bus_error) {

      if (bus_error_count <= BUS_ERR_PRINT_THRESHOLD) {
	 cprintf("CtrDrvr:BUS_ERROR:A24D32:READ-Address:0x%p\n",x);
	 if (isr_bus_error) cprintf("CtrDrvr:BUS_ERROR:In ISR occured\n");
	 if (bus_error_count == BUS_ERR_PRINT_THRESHOLD) cprintf("CtrDrvr:BUS_ERROR:PrintSuppressed\n");
	 isr_bus_error = 0;
	 last_bus_error = bus_error_count;
      }
   }
   return res;
}

/* ==================== */

static void HWr(int v, void *x) {

// udelay(10);
   iowrite32be(v,x);
   if (bus_error_count > last_bus_error) {
      if (bus_error_count <= BUS_ERR_PRINT_THRESHOLD) {
	 cprintf("CtrDrvr:BUS_ERROR:A24D32:WRITE-Address:0x%p Data:0x%x\n",x,v);
	 if (bus_error_count == BUS_ERR_PRINT_THRESHOLD) cprintf("CtrDrvr:BUS_ERROR:PrintSuppressed\n");
	 last_bus_error = bus_error_count;
      }
   }
   return;
}

/* ==================== */

static short HRdJtag(void *x) {
short res;
unsigned long ps;

   res = ioread16be(x);
   if (vme_bus_error_check(CLEAR_BUS_ERROR)) {
      if (bus_error_count++ <= BUS_ERR_PRINT_THRESHOLD) {
	 disable(ps); {
	    kkprintf("CtrDrvr:BUS_ERROR:A16D16:JTAG-READ-Address:0x%p\n",x);
	    if (bus_error_count == BUS_ERR_PRINT_THRESHOLD) kkprintf("CtrDrvr:BUS_ERROR:PrintSuppressed\n");
	 } restore(ps);
      }
   }
   return res;
}

/* ==================== */

static void HWrJtag(short v, void *x) {
unsigned long ps;

   iowrite16be(v,x);
   if (vme_bus_error_check(CLEAR_BUS_ERROR)) {
      if (bus_error_count++ <= BUS_ERR_PRINT_THRESHOLD) {
	 disable(ps); {
	    kkprintf("CtrDrvr:BUS_ERROR:A16D16:JTAG-WRITE-Address:0x%p Data:0x%x\n",x,(int) v);
	    if (bus_error_count == BUS_ERR_PRINT_THRESHOLD) kkprintf("CtrDrvr:BUS_ERROR:PrintSuppressed\n");
	 } restore(ps);
      }
   }
   return;
}

/*========================================================================*/
/* 32 Bit int access for CTR structure to write to hardware               */
/*========================================================================*/

static void Io32Write(unsigned int *dst, /* Hardware destination address */
		      unsigned int *src,
		      unsigned int size) {
int i, sb;

   sb = size/sizeof(unsigned int);
   for (i=0; i<sb; i++) HWr(src[i], (void *) &(dst[i]));
}

/*========================================================================*/
/* 32 Bit int access for CTR structure to read from hardware             */
/*========================================================================*/

static void Io32Read(unsigned int *dst,
		     unsigned int *src, /* Hardware Source address */
		     unsigned int size) {
int i, sb;

   sb = size/sizeof(unsigned int);
   for (i=0; i<sb; i++) dst[i] = HRd((void *) &(src[i]));
}

/*========================================================================*/
/* 32 Bit int access copy                                                 */
/*========================================================================*/

static void Int32Copy(unsigned int *dst,
		      unsigned int *src,
		      unsigned int size) {
int i, sb;

   sb = size/sizeof(unsigned int);
   for (i=0; i<sb; i++) dst[i] = src[i];
}

/*========================================================================*/
/* Swap words                                                             */
/*========================================================================*/

static void SwapWords(unsigned int *dst,
		      unsigned int size) {

int i, sb;
unsigned int lft, rgt;

   sb = size/sizeof(unsigned int);
   for (i=0; i<sb; i++) {
      rgt = lft = dst[i];
      lft = (lft & 0xFFFF0000) >> 16;
      rgt = (rgt & 0x0000FFFF);
      dst[i] = (rgt<<16) | lft;
   }
}

/*========================================================================*/
/* Cancel a timeout in a safe way                                         */
/*========================================================================*/

static void CancelTimeout(int *t) {

int v;

   if ((v = *t)) {
      *t = 0;
      cancel_timeout(v);
   }
}

/*========================================================================*/
/* Handel read timeouts                                                   */
/*========================================================================*/

static int ReadTimeout(CtrDrvrClientContext *ccon) {

   ccon->Timer = 0;
   sreset(&(ccon->Semaphore));
   return 0;
}

/*========================================================================*/
/* Handel reset timeouts                                                  */
/*========================================================================*/

static int ResetTimeout(CtrDrvrModuleContext *mcon) {

   mcon->Timer = 0;
   sreset(&(mcon->Semaphore));
   return 0;
}

/* =========================================================================================== */
/* Print Debug message                                                                         */
/* =========================================================================================== */

char *ioctl_names[CtrDrvrLAST_IOCTL] = {

"SET_SW_DEBUG", "GET_SW_DEBUG", "GET_VERSION", "SET_TIMEOUT", "GET_TIMEOUT",
"SET_QUEUE_FLAG", "GET_QUEUE_FLAG", "GET_QUEUE_SIZE", "GET_QUEUE_OVERFLOW",
"GET_MODULE_DESCRIPTOR", "SET_MODULE",
"GET_MODULE", "GET_MODULE_COUNT", "RESET", "ENABLE", "GET_STATUS", "GET_INPUT_DELAY",
"SET_INPUT_DELAY", "GET_CLIENT_LIST", "CONNECT", "DISCONNECT", "GET_CLIENT_CONNECTIONS",
"SET_UTC", "GET_UTC", "GET_CABLE_ID", "GET_ACTION", "SET_ACTION", "CREATE_CTIM_OBJECT",
"DESTROY_CTIM_OBJECT", "LIST_CTIM_OBJECTS", "CHANGE_CTIM_FRAME", "CREATE_PTIM_OBJECT",
"DESTROY_PTIM_OBJECT", "LIST_PTIM_OBJECTS", "GET_PTIM_BINDING",
"GET_OUT_MASK", "SET_OUT_MASK", "GET_COUNTER_HISTORY", "GET_REMOTE",
"SET_REMOTE", "REMOTE", "GET_CONFIG", "SET_CONFIG", "GET_PLL", "SET_PLL", "SET_PLL_ASYNC_PERIOD",
"GET_PLL_ASYNC_PERIOD", "READ_TELEGRAM", "READ_EVENT_HISTORY", "JTAG_OPEN", "JTAG_READ_BYTE",
"JTAG_WRITE_BYTE", "JTAG_CLOSE", "HPTDC_OPEN", "HPTDC_IO", "HPTDC_CLOSE", "RAW_READ", "RAW_WRITE",
"GET_RECEPTION_ERRORS", "GET_IO_STATUS", "GET_IDENTITY",
"SET_DEBUG_HISTORY","SET_BRUTAL_PLL","GET_MODULE_STATS","SET_CABLE_ID",
"IOCTL64","IOCTL65","IOCTL66","IOCTL67","IOCTL68","IOCTL69",

"GET_OUTPUT_BYTE",
"SET_OUTPUT_BYTE",

};

/* =========================================================================================== */

static void DebugIoctl(CtrDrvrControlFunction cm, char *arg) {
int i;
char *iocname;

   if (cm < CtrDrvrLAST_IOCTL) {
      iocname = ioctl_names[(int) cm];
      if (arg) {
	 i = (int) *arg;
	 cprintf("CtrDrvr: Debug: Called with IOCTL: %s Arg: %d\n",iocname, i);
      } else {
	 cprintf("CtrDrvr: Debug: Called with IOCTL: %s Arg: NULL\n",iocname);
      }
      return;
   }
   cprintf("CtrDrvr: Debug: ERROR: Illegal IOCTL number: %d\n",(int) cm);
}

/*========================================================================*/
/* Remove module                                                          */
/*========================================================================*/

static void RemoveModule(CtrDrvrModuleContext *mcon) {

CtrDrvrModuleAddress *moad;
CtrDrvrMemoryMap     *mmap;
int cc;

   moad = &(mcon->Address);
   mmap = mcon->Map;

   HWr(0,&(mmap->InterruptEnable));
   cc = HRd(&(mmap->InterruptSource));
   vme_intclr(moad->InterruptVector,NULL);

   vme_unregister_berr_handler(mcon->BerrHandler);

   return_controller((unsigned) moad->VMEAddress,0x10000);
   return_controller((unsigned) moad->JTGAddress,0x100);

}

/*========================================================================*/
/* Add a module to the driver, called per module from install             */
/*========================================================================*/

static int AddModule(mcon,index,flag)
CtrDrvrModuleContext *mcon;        /* Module Context */
int index;                         /* Module index (0-based) */
int flag; {                        /* Check again */

CtrDrvrModuleAddress *moad; /* Module address, vector, level, ssch */
unsigned long                  addr; /* VME base address */
unsigned int                   coco; /* Completion code */
struct vme_bus_error berr;
struct pdparam_master param;
void *vmeaddr;

   /* Compute Virtual memory address as seen from system memory mapping */

   moad = &(mcon->Address);

   /* Check module has re-appeared after JTAG modification */

   if (!flag) {

      /* CES: build an address window (64 kbyte) for VME A16-D16 accesses */

      addr = (unsigned long) moad->JTGAddress;
      addr &= 0x0000ffff;                      /* A16 */

      param.iack   = 1;                /* no iack */
      param.rdpref = 0;                /* no VME read prefetch option */
      param.wrpost = 0;                /* no VME write posting option */
      param.swap   = 1;                /* VME auto swap option */
      param.dum[0] = VME_PG_SHARED;    /* window is sharable */
      param.dum[1] = 0;                /* XPC ADP-type */
      param.dum[2] = 0;                /* window is sharable */

      vmeaddr = (void *) find_controller(addr,                    /* Vme base address */
					(unsigned long) 0x100,    /* Module address space */
					(unsigned long) 0x29,     /* Address modifier A16 */
					0,                        /* Offset */
					2,                        /* Size is D16 */
					&param);                  /* Parameter block */
      if (vmeaddr == (void *)(-1)) {
	 cprintf("CtrDrvr: find_controller: ERROR: Module:%d. JTAG Addr:%x\n",
		  index+1,
		  (unsigned int) moad->JTGAddress);
	 vmeaddr = 0;
      }
      moad->JTGAddress = (unsigned short *) vmeaddr;
   }

   /* CES: build an address window (64 kbyte) for VME A24-D32 accesses */

   addr = (unsigned long) moad->VMEAddress;
   if (addr == 0) addr = (unsigned long) moad->CopyAddress;
   addr &= 0x00ffffff;                      /* A24 */

   param.iack   = 1;                /* no iack */
   param.rdpref = 0;                /* no VME read prefetch option */
   param.wrpost = 0;                /* no VME write posting option */
   param.swap   = 1;                /* VME auto swap option */
   param.dum[0] = VME_PG_SHARED;    /* window is sharable */
   param.dum[1] = 0;                /* XPC ADP-type */
   param.dum[2] = 0;                /* window is sharable */
   vmeaddr = (void *) find_controller(addr,                    /* Vme base address */
				     (unsigned long) 0x10000,  /* Module address space */
				     (unsigned long) 0x39,     /* Address modifier A24 */
				     0,                        /* Offset */
				     4,                        /* Size is D32 */
				     &param);                  /* Parameter block */

   if (vmeaddr == (void *)(-1)) {
      cprintf("CtrDrvr: find_controller: ERROR: Module:%d. VME Addr:%x\n",
	       index+1,
	       (unsigned int) moad->VMEAddress);
      pseterr(ENXIO);        /* No such device or address */

      if (moad->JTGAddress == 0) return 1;
      else                       return 2;
   }
   moad->VMEAddress = (unsigned int *) vmeaddr;

   berr.address = addr;
   berr.am = 0x39;
   mcon->BerrHandler = vme_register_berr_handler(&berr,0x10000,BusErrorHandler);
   if (IS_ERR(mcon->BerrHandler)) {
      cprintf("CtrDrvr: vme_register_berr_handler: ERROR: Module:%d. 0x10000 0x39 VME Addr:%x\n",
	       index+1,
	       (unsigned int) addr);
   }

   /* Now set up the interrupt handler */

   coco = vme_intset(moad->InterruptVector,         /* Vector */
		     (int (*)(void *)) IntrHandler, /* Address of ISR */
		     (void *) mcon,                 /* Parameter to pass */
		     0);                            /* Don't save previous */
   if (coco < 0) {
      cprintf("CtrDrvr: vme_intset: ERROR %d, MODULE %d\n",coco,index+1);
      pseterr(EFAULT);                                 /* Bad address */
      return 3;
   }
   mcon->IVector = moad->InterruptVector;

   /* Set module map to the VME address */

   mcon->Map = (CtrDrvrMemoryMap *) moad->VMEAddress;

   Reset(mcon); /* Soft reset and initialize module */

   return 0;
}

/* ========================================================== */
/* GetVersion, also used to check for Bus errors. See Ping    */
/* ========================================================== */

#ifndef COMPILE_TIME
#define COMPILE_TIME 0
#endif

static int GetVersion(CtrDrvrModuleContext *mcon,
		      CtrDrvrVersion       *ver) {

CtrDrvrMemoryMap *mmap;    /* Module Memory map */
unsigned int      stat;    /* Module status */

   mmap = mcon->Map;
   ver->VhdlVersion  = HRd(&mmap->VhdlVersion);
   ver->DrvrVersion  = COMPILE_TIME;
   ver->HardwareType = CtrDrvrHardwareTypeNONE;

   stat  = HRd(&mmap->Status);
   stat &= CtrDrvrTypeStatusMask;

   if (stat != CtrDrvrTypeStatusMask) {
      if (stat & CtrDrvrStatusCTRI) ver->HardwareType = CtrDrvrHardwareTypeCTRI;
      if (stat & CtrDrvrStatusCTRP) ver->HardwareType = CtrDrvrHardwareTypeCTRP;
      if (stat & CtrDrvrStatusCTRV) ver->HardwareType = CtrDrvrHardwareTypeCTRV;
   }
   return OK;
}

/* ========================================================== */
/* Ping module using GetVersion to check for bus errors       */
/* ========================================================== */

static int PingModule(CtrDrvrModuleContext *mcon) {

CtrDrvrVersion ver;

   return GetVersion(mcon,&ver);
}

/* ========================================================== */
/* Enable Interrupt                                           */
/* ========================================================== */

static int EnableInterrupts(CtrDrvrModuleContext *mcon, int msk) {

CtrDrvrMemoryMap *mmap; /* Module Memory map */

   mmap = mcon->Map;

   mcon->InterruptEnable |= msk;
   HWr(mcon->InterruptEnable,&mmap->InterruptEnable);

/* After a JTAG upgrade interrupt must be re-enabled once */
/* and at least one time on startup. */

// if (mcon->IrqBalance == 0) {
//    enable_irq(mcon->IVector);
//    mcon->IrqBalance = 1;
// }

   return OK;
}

/* ========================================================== */
/* Disable Interrupt                                          */
/* ========================================================== */

static int DisableInterrupts(unsigned int           tndx,
			     unsigned int           midx,
			     CtrDrvrConnectionClass clss,
			     CtrDrvrClientContext  *ccon) {

CtrDrvrModuleContext       *mcon;
CtrDrvrMemoryMap  *mmap;
int i, cmsk, hmsk, cntr;

   cmsk = 1 << ccon->ClientIndex;
   mcon = &(Wa->ModuleContexts[midx]);

   if (clss != CtrDrvrConnectionClassHARD) cntr = mcon->Trigs[tndx].Counter;
   else if (tndx <= CtrDrvrCOUNTERS)       cntr = tndx;
   else                                    cntr = -1;

   if (cntr != -1) {
      if (mcon->HardClients[cntr] & ~cmsk) return OK;
      hmsk = 1 << cntr;
      for (i=0; i<CtrDrvrRamTableSIZE; i++) {
	 if ((mcon->Trigs[i].Counter == cntr)
	 &&  (mcon->Clients[i] & ~cmsk)) {
	    return OK;
	 }
      }
   } else hmsk = 1 << tndx;

   if (mcon->HardClients[tndx] & ~cmsk) return OK;

   mmap = mcon->Map;
   mcon->InterruptEnable &= ~hmsk;
   HWr(mcon->InterruptEnable,&mmap->InterruptEnable);

   return OK;
}

/* ========================================================== */
/* Reset a module                                             */
/* ========================================================== */

static int Reset(CtrDrvrModuleContext *mcon) {

CtrDrvrModuleAddress *moad; /* Module address, vector, level, ssch */

CtrDrvrMemoryMap *mmap;     /* Module Memory map */
unsigned int     *jtg;      /* Hptdc JTAG register */
int i;

int interval = 10;
int interval_jiffies = msecs_to_jiffies(interval * 10); /* 10ms granularity */

   mmap = mcon->Map;

   HWr(CtrDrvrCommandRESET,&mmap->Command);
   mcon->Status = CtrDrvrStatusNO_LOST_INTERRUPTS | CtrDrvrStatusNO_BUS_ERROR;
   mcon->BusErrorCount = 0;

   /* Wait at least 10 ms after a "reset", for the module to recover */

   sreset(&(mcon->Semaphore));

   mcon->Timer = timeout(ResetTimeout, (char *) mcon, interval_jiffies);
   if (mcon->Timer < 0) mcon->Timer = 0;
   if (mcon->Timer) swait(&(mcon->Semaphore), SEM_SIGABORT);
   if (mcon->Timer) CancelTimeout(&(mcon->Timer));

   mcon->Command &= ~CtrDrvrCommandSET_HPTDC; /* Hptdc needs to be re-initialized */
   HWr(mcon->Command,&mmap->Command);

   for (i=CtrDrvrCounter1; i<=CtrDrvrCounter8; i++) {
      if (HRd(&mmap->Counters[i].Control.RemOutMask) == 0) {
	 HWr(AutoShiftLeft(CtrDrvrCntrCntrlOUT_MASK,(1 << i)),&mmap->Counters[i].Control.RemOutMask);
      }
   }

   if (mcon->OutputByte) HWr(0x100 | (1 << (mcon->OutputByte -1)),&mmap->OutputByte);
   else                  HWr(0x100,&mmap->OutputByte);
   moad = &(mcon->Address);
   HWr((unsigned int) ((moad->InterruptLevel << 8) | (moad->InterruptVector & 0xFF)),&mmap->Setup);

   if (mcon->Pll.KP == 0) {
      mcon->Pll.KP = 337326;
      mcon->Pll.KI = 901;
      mcon->Pll.NumAverage = 100;
      mcon->Pll.Phase = 1950;
   }
   Io32Write((unsigned int *) &(mmap->Pll),
	     (unsigned int *) &(mcon->Pll),
	     (unsigned int  ) sizeof(CtrDrvrPll));

   EnableInterrupts(mcon,mcon->InterruptEnable);

   jtg = &mmap->HptdcJtag;
   HptdcStateReset(jtg);           /* GOTO: Run test/idle */

   return OK;
}

/* ========================================================== */
/* Get Status                                                 */
/* ========================================================== */

static unsigned int GetStatus(CtrDrvrModuleContext *mcon) {

CtrDrvrMemoryMap *mmap;    /* Module Memory map */
unsigned int              stat;    /* Status */

   if (PingModule(mcon) == OK) {
      mmap = mcon->Map;
      stat = HRd(&mmap->Status) & CtrDrvrHwStatusMASK;

      /* Check for new bus errors */

      mcon->BusErrorCount += GetClrBusErrCnt(); /* Any bus error on any module !! */

      if (mcon->BusErrorCount) mcon->Status &= ~CtrDrvrStatusNO_BUS_ERROR;
      else                     mcon->Status |=  CtrDrvrStatusNO_BUS_ERROR;

      mcon->BusErrorCount = 0; /* Clear it after reading */

      stat  = stat | (mcon->Status & ~CtrDrvrHwStatusMASK);
      return stat;
   }
   return SYSERR;
}

/* ========================================================== */
/* HPTDC JTAG Bit transfer                                    */
/* ========================================================== */

static unsigned int HptdcBitTransfer(unsigned int *jtg,
				     unsigned int  tdi,
				     unsigned int  tms) {
unsigned int jwd, tdo;

   jwd = HptdcJTAG_TRST | tdi | tms;
   HWr(jwd,jtg);
   jwd = HptdcJTAG_TRST | HptdcJTAG_TCK | tdi | tms;
   HWr(jwd,jtg);
   tdo = HRd(jtg);
   jwd = HptdcJTAG_TRST | tdi | tms;
   HWr(jwd,jtg);
   if (tdo & HptdcJTAG_TDO) return 1;
   else                     return 0;
}

/* ========================================================== */
/* HPTDC JTAG State machine reset                             */
/* ========================================================== */

static void HptdcStateReset(unsigned int *jtg) {
unsigned int jwd;

   jwd = HptdcJTAG_TRST; /* TRST is active on zero */
   HWr(jwd,jtg);
   jwd = 0;              /* TCK and TRST are down */
   HWr(jwd,jtg);
   jwd = HptdcJTAG_TCK;  /* Clock in reset on rising edge */
   HWr(jwd,jtg);
   jwd = HptdcJTAG_TRST | HptdcJTAG_TCK;
   HWr(jwd,jtg);
   jwd = HptdcJTAG_TRST; /* Remove TRST */
   HWr(jwd,jtg);
   HptdcBitTransfer(jtg,0,0); /* GOTO: Run test/idle */
   return;
}

/* ========================================================== */
/* Execute an HPTDC command                                   */
/* ========================================================== */

static int HptdcCommand(CtrDrvrModuleContext *mcon,
			 HptdcCmd              cmd,
			 HptdcRegisterVlaue   *wreg,
			 HptdcRegisterVlaue   *rreg,    /* No read back if NULL */
			 int                   size,    /* Number of registers */
			 int                   pflg,    /* Parity flag */
			 int                   rflg) {  /* Reset state flag */

CtrDrvrMemoryMap *mmap;    /* Module Memory map */
unsigned int     *jtg;

unsigned int tdi, msk, parity, bits, wval, rval, cval;
int i, j;

   if (PingModule(mcon) == OK) {
      mmap = mcon->Map;
      jtg = &mmap->HptdcJtag;

      /* Cycle the state machine to get to selected command register */

      if (rflg) HptdcStateReset(jtg);           /* GOTO: Run test/idle */
      HptdcBitTransfer(jtg,0,HptdcJTAG_TMS);    /* GOTO: Select DR scan */
      HptdcBitTransfer(jtg,0,HptdcJTAG_TMS);    /* GOTO: Select IR scan */
      HptdcBitTransfer(jtg,0,0);                /* GOTO: Capture IR */
      HptdcBitTransfer(jtg,0,0);                /* GOTO: Shift IR */

      /* Select the IR by shifting in the register ID in cmd */

      parity = 0; cval = 0;
      for (i=0; i<HptdcCmdBITS; i++) {
	 msk = 1 << i;
	 if (msk & cmd) { tdi = HptdcJTAG_TDI; parity++; }
	 else           { tdi = 0;                       }
	 if (HptdcBitTransfer(jtg,tdi,0)) cval |= msk;
      }
      if (parity & 1) HptdcBitTransfer(jtg,HptdcJTAG_TDI,HptdcJTAG_TMS); /* GOTO: Exit 1 IR */
      else            HptdcBitTransfer(jtg,0,HptdcJTAG_TMS);

      HptdcBitTransfer(jtg,0,HptdcJTAG_TMS);    /* GOTO: Update IR */
      HptdcBitTransfer(jtg,0,HptdcJTAG_TMS);    /* GOTO: Select DR scan */
      HptdcBitTransfer(jtg,0,0);                /* GOTO: Capture DR */
      HptdcBitTransfer(jtg,0,0);                /* GOTO: Shift DR */

      /* Write and read back register data */

      parity = 0;
      for (j=0; j<size; j++) {
	 bits = wreg[j].EndBit - wreg[j].StartBit + 1;
	 wval = wreg[j].Value;
	 rval = 0;
	 for (i=0; i<bits; i++) {
	    msk = 1 << i;
	    if (msk & wval) { tdi = HptdcJTAG_TDI; parity++; }
	    else            { tdi = 0;                       }

	    if (pflg) {
	       if (HptdcBitTransfer(jtg,tdi,0)) rval |= msk;
	    } else {
	       if (i==bits-1) {
		  if (HptdcBitTransfer(jtg,tdi,HptdcJTAG_TMS)) rval |= msk;
	       } else {
		  if (HptdcBitTransfer(jtg,tdi,0)) rval |= msk;
	       }
	    }
	 }
	 if (rreg) {
	    rreg[j].EndBit   = wreg[j].EndBit;
	    rreg[j].StartBit = wreg[j].StartBit;
	    rreg[j].Value    = rval;
	 }
      }
      if (pflg) {
	 if (parity & 1) HptdcBitTransfer(jtg,HptdcJTAG_TDI,HptdcJTAG_TMS); /* GOTO: Exit 1 DR */
	 else            HptdcBitTransfer(jtg,0,HptdcJTAG_TMS);
      }

      HptdcBitTransfer(jtg,0,HptdcJTAG_TMS);    /* GOTO: Update DR */
      HptdcBitTransfer(jtg,0,0);                /* GOTO: Run test/idle */

      return OK;
   }
   pseterr(ENXIO);                              /* No such device or address */
   return SYSERR;
}

/* ========================================================== */
/* Read the time from the CTR                                 */
/* ========================================================== */

static CtrDrvrCTime *GetTime(CtrDrvrModuleContext *mcon) {

static CtrDrvrCTime t;

CtrDrvrMemoryMap *mmap;    /* Module Memory map */

   mmap = mcon->Map;
   HWr(CtrDrvrCommandLATCH_UTC,&mmap->Command);

   t.CTrain          = HRd(&mmap->ReadTime.CTrain);
   t.Time.Second     = HRd(&mmap->ReadTime.Time.Second);
   t.Time.TicksHPTDC = HRd(&mmap->ReadTime.Time.TicksHPTDC);
   return &t;
}

/* ========================================================== */
/* Set the time on the CTR                                    */
/* ========================================================== */

static int SetTime(CtrDrvrModuleContext *mcon, unsigned int tod) {

CtrDrvrMemoryMap *mmap;    /* Module Memory map */

   if (PingModule(mcon) == OK) {
      mmap = mcon->Map;
      HWr(tod,&mmap->SetTime);
      HWr(CtrDrvrCommandSET_UTC,&mmap->Command);

      return OK;
   }
   return SYSERR;
}

/*=========================================================== */
/* Raw IO                                                     */
/* The correct PLX9030 endian settings must have been made    */
/*=========================================================== */

static int RawIo(CtrDrvrModuleContext *mcon,
		 CtrDrvrRawIoBlock    *riob,
		 unsigned int          flag,
		 unsigned int          debg) {

unsigned int              *mmap; /* Module Memory map */
int                        rval; /* Return value */
int                        i, j;
unsigned int              *uary;
char                      *iod;

   mmap = (unsigned int *) mcon->Map;
   uary = riob->UserArray;
   rval = OK;
   if (flag)  iod = "Write"; else iod = "Read";

   i = j = 0;
   for (i=0; i<riob->Size; i++) {
      j = riob->Offset+i;
      if (flag) {
	 HWr(uary[i],&mmap[j]);
	 if (GetClrBusErrCnt()) {
	    pseterr(ENXIO);
	    return SYSERR;
	 }
	 if (debg >= 2) cprintf("RawIo: %s: %d:0x%x\n",iod,j,(int) uary[i]);
      } else {
	 uary[i] = HRd(&mmap[j]);
	 if (GetClrBusErrCnt()) {
	    pseterr(ENXIO);
	    return SYSERR;
	 }
	 if (debg >= 2) cprintf("RawIo: %s: %d:0x%x\n",iod,j,(int) uary[i]);
      }
   }
   riob->Size = i+1;
   return rval;
}

/* ========================================================== */
/* Auto shift left  a value for a given mask                  */
/* ========================================================== */

static unsigned int AutoShiftLeft(unsigned int mask, unsigned int value) {
int i,m;

   m = mask;
   for (i=0; i<32; i++) {
      if (m & 1) break;
      m >>= 1;
   }
   return (value << i) & mask;
}

/* ========================================================== */
/* Auto shift right a value for a given mask                  */
/* ========================================================== */

static unsigned int AutoShiftRight(unsigned int mask, unsigned int value) {
int i,m;

   m = mask;
   for (i=0; i<32; i++) {
      if (m & 1) break;
      m >>= 1;
   }
   return (value >> i) & m;
}

/* ========================================================== */
/* Convert driver API trigger to compact hardware form.       */
/* The API form for a trigger is too big to fit in the FPGA.  */
/* We keep a propper structure for the driver API, and        */
/* declare a second version packed into bit fields for the    */
/* hardware representation.                                   */
/* ========================================================== */

static CtrDrvrHwTrigger *TriggerToHard(CtrDrvrTrigger *strg) {
static CtrDrvrHwTrigger htrg;
unsigned int trigger;

   trigger  = AutoShiftLeft(CtrDrvrTrigGROUP_VALUE_MASK ,strg->Group.GroupValue);
   trigger |= AutoShiftLeft(CtrDrvrTrigCONDITION_MASK   ,strg->TriggerCondition);
   trigger |= AutoShiftLeft(CtrDrvrTrigMACHINE_MASK     ,strg->Machine);
   trigger |= AutoShiftLeft(CtrDrvrTrigCOUNTER_MASK     ,strg->Counter);
   trigger |= AutoShiftLeft(CtrDrvrTrigGROUP_NUMBER_MASK,strg->Group.GroupNumber);

   htrg.Frame   = strg->Frame;
   htrg.Trigger = trigger;

   return &htrg;
}

/* ========================================================== */
/* Convert compact hardware trigger to driver API trigger     */
/* ========================================================== */

static CtrDrvrTrigger *HardToTrigger(CtrDrvrHwTrigger *htrg) {
static CtrDrvrTrigger strg;
unsigned int trigger;

   strg.Frame.Long = HRd(&htrg->Frame);
   trigger         = HRd(&htrg->Trigger);

   strg.Group.GroupValue  = AutoShiftRight(CtrDrvrTrigGROUP_VALUE_MASK ,trigger);
   strg.TriggerCondition  = AutoShiftRight(CtrDrvrTrigCONDITION_MASK   ,trigger);
   strg.Machine           = AutoShiftRight(CtrDrvrTrigMACHINE_MASK     ,trigger);
   strg.Counter           = AutoShiftRight(CtrDrvrTrigCOUNTER_MASK     ,trigger);
   strg.Group.GroupNumber = AutoShiftRight(CtrDrvrTrigGROUP_NUMBER_MASK,trigger);

   return &strg;
}

/* ================================================================================== */
/* Convert a soft counter configuration to compact hardware form.                     */
/* ================================================================================== */

static CtrDrvrHwCounterConfiguration *ConfigToHard(CtrDrvrCounterConfiguration *scnf) {
static CtrDrvrHwCounterConfiguration hcnf;
unsigned int config;

   config  = AutoShiftLeft(CtrDrvrCounterConfigPULSE_WIDTH_MASK,scnf->PulsWidth);
   config |= AutoShiftLeft(CtrDrvrCounterConfigCLOCK_MASK      ,scnf->Clock);
   config |= AutoShiftLeft(CtrDrvrCounterConfigMODE_MASK       ,scnf->Mode);
   config |= AutoShiftLeft(CtrDrvrCounterConfigSTART_MASK      ,scnf->Start);
   config |= AutoShiftLeft(CtrDrvrCounterConfigON_ZERO_MASK    ,scnf->OnZero);

   hcnf.Config = config;
   hcnf.Delay  = scnf->Delay;

   return &hcnf;
}

/* ================================================================================== */
/* Convert a compacr hardware counter configuration to API form.                      */
/* ================================================================================== */

static CtrDrvrCounterConfiguration *HardToConfig(CtrDrvrHwCounterConfiguration *hcnf) {
static CtrDrvrCounterConfiguration scnf;
unsigned int config;

   config     = HRd(&hcnf->Config);
   scnf.Delay = HRd(&hcnf->Delay);

   scnf.OnZero    = AutoShiftRight(CtrDrvrCounterConfigON_ZERO_MASK    ,config);
   scnf.Start     = AutoShiftRight(CtrDrvrCounterConfigSTART_MASK      ,config);
   scnf.Mode      = AutoShiftRight(CtrDrvrCounterConfigMODE_MASK       ,config);
   scnf.Clock     = AutoShiftRight(CtrDrvrCounterConfigCLOCK_MASK      ,config);
   scnf.PulsWidth = AutoShiftRight(CtrDrvrCounterConfigPULSE_WIDTH_MASK,config);

   return &scnf;
}

/* ========================================================== */
/* Get a PTIM module number                                   */
/* ========================================================== */

static unsigned int GetPtimModule(unsigned int eqpnum) {
int i;

   for (i=0; i<Wa->Ptim.Size; i++) {
      if (eqpnum == Wa->Ptim.Objects[i].EqpNum) {
	 return Wa->Ptim.Objects[i].ModuleIndex + 1;
      }
   }
   return 0;
}

/* ========================================================== */
/* Connect to a timing object                                 */
/* ========================================================== */

static int Connect(CtrDrvrConnection    *conx,
		   CtrDrvrClientContext *ccon) {

CtrDrvrModuleContext       *mcon;
CtrDrvrMemoryMap           *mmap;
CtrDrvrTrigger              trig;
CtrDrvrCounterConfiguration conf;
CtrDrvrEventFrame           frme;

int i, midx, cmsk, hmsk, count, valu;

   /* Get the module to make the connection on */

   midx = conx->Module -1;
   if ((midx < 0) || (midx >= CtrDrvrMODULE_CONTEXTS))
      midx = ccon->ModuleIndex;

   /* Set up connection mask and module map and context */

   cmsk  = 1 << ccon->ClientIndex;
   mcon  = &(Wa->ModuleContexts[midx]);
   mmap  =  mcon->Map;

   /* Connect to an existing PTIM instance */

   count = 0;   /* Number of connections made so far is zero */

   if (conx->EqpClass == CtrDrvrConnectionClassPTIM) {
      midx = GetPtimModule(conx->EqpNum) -1;
      if (midx >= 0) {
	 mcon = &(Wa->ModuleContexts[midx]);
	 mmap = mcon->Map;
	 for (i=0; i<CtrDrvrRamTableSIZE; i++) {
	    if ((conx->EqpNum   == mcon->EqpNum[i])
	    &&  (conx->EqpClass == mcon->EqpClass[i])) {
	       count++;
	       mcon->Clients[i]        |= cmsk;
	       mcon->InterruptEnable   |= (1 << mcon->Trigs[i].Counter);
	       valu = HRd(&mmap->Configs[i].Config);
	       valu |= AutoShiftLeft(CtrDrvrCounterConfigON_ZERO_MASK, CtrDrvrCounterOnZeroBUS);
	       HWr(valu,&mmap->Configs[i].Config);
	    }
	 }
      }

   /* Connect to a CTIM object, may need to create an instance */

   } else if (conx->EqpClass == CtrDrvrConnectionClassCTIM) {

      /* Check to see if there is already an instance, and connect to it */

      for (i=0; i<CtrDrvrRamTableSIZE; i++) {
	 if ((conx->EqpNum   == mcon->EqpNum[i])
	 &&  (conx->EqpClass == mcon->EqpClass[i])) {
	    mcon->Clients[i]        |= cmsk;
	    mcon->InterruptEnable   |= CtrDrvrInterruptMaskCOUNTER_0;
	    valu = HRd(&mmap->Configs[i].Config);
	    valu |= AutoShiftLeft(CtrDrvrCounterConfigON_ZERO_MASK, CtrDrvrCounterOnZeroBUS);
	    HWr(valu,&mmap->Configs[i].Config);
	    count = 1;
	    break;
	 }
      }

      /* Need to create a new CTIM instance, so first find the CTIM object */

      if (count == 0) {
	 frme.Struct = (CtrDrvrFrameStruct) {0,0,0};    /* No frame has been found yet */
	 for (i=0; i<Wa->Ctim.Size; i++) {
	    if (conx->EqpNum == Wa->Ctim.Objects[i].EqpNum) {
	       frme = Wa->Ctim.Objects[i].Frame;
	       break;
	    }
	 }

	 /* If we found the CTIM object, look for an empty slot and create the instance */

	 if (frme.Struct.Header != 0) {
	    for (i=0; i<CtrDrvrRamTableSIZE; i++) {
	       if (mcon->EqpNum[i] == 0) {      /* Is the slot empty ? */
		  mcon->EqpNum[i]   = conx->EqpNum;
		  mcon->EqpClass[i] = CtrDrvrConnectionClassCTIM;

		  trig.Ctim             = conx->EqpNum;
		  trig.Frame            = frme;
		  trig.TriggerCondition = CtrDrvrTriggerConditionNO_CHECK;
		  trig.Machine          = CtrDrvrMachineNONE;
		  trig.Counter          = CtrDrvrCounter0;
		  trig.Group            = (CtrDrvrTgmGroup) {0,0}; /* Number and Value */
		  mcon->Trigs[i]        = trig;

		  conf.OnZero           = CtrDrvrCounterOnZeroBUS;
		  conf.Start            = CtrDrvrCounterStartNORMAL;
		  conf.Mode             = CtrDrvrCounterModeNORMAL;
		  conf.Clock            = CtrDrvrCounterClock1KHZ;
		  conf.Delay            = 0;
		  conf.PulsWidth        = 0;
		  mcon->Configs[i]      = conf;

		  Io32Write((unsigned int *) &(mmap->Trigs[i]),
			    (unsigned int *) TriggerToHard(&trig),
			    (unsigned int  ) sizeof(CtrDrvrHwTrigger));

		  Io32Write((unsigned int *) &(mmap->Configs[i]),
			    (unsigned int *) ConfigToHard(&conf),
			    (unsigned int  ) sizeof(CtrDrvrHwCounterConfiguration));

		  mcon->Clients[i]      |= cmsk;
		  mcon->InterruptEnable |= CtrDrvrInterruptMaskCOUNTER_0;
		  count = 1;
		  break;
	       }
	    }

	    /* Give a NOMEM error if no empty slot available */

	    if (count == 0) {
	       pseterr(ENOMEM);        /* No memory/slot available */
	       return SYSERR;
	    }
	 }
      }

   /* Connect to a hardware interrupt directly */

   } else {
      for (i=0; i<CtrDrvrInterruptSOURCES; i++) {
	 hmsk = 1 << i;
	 if (conx->EqpNum & hmsk) {
	    count++;
	    mcon->InterruptEnable |= hmsk;
	    mcon->HardClients[i]  |= cmsk;
	 }
      }
   }

   /* If the connection count is non zero, go enable interrupt on hardware */

   if (count) return EnableInterrupts(mcon,mcon->InterruptEnable);

   /* If count is zero, no connection was made, so give an error */

   pseterr(ENXIO);        /* No such device or address */
   return SYSERR;
}

/* ========================================================== */
/* Disconnect from one specified trigger                      */
/* ========================================================== */

static int DisConnectOne(unsigned int           tndx,
			 unsigned int           midx,
			 CtrDrvrConnectionClass clss,
			 CtrDrvrClientContext  *ccon) {

CtrDrvrModuleContext       *mcon;
CtrDrvrMemoryMap           *mmap;
CtrDrvrTrigger              trig;
CtrDrvrCounterConfiguration conf;
unsigned int ncmsk, cmsk;
int valu;

   cmsk = 1 << ccon->ClientIndex; ncmsk = ~cmsk;
   mcon = &(Wa->ModuleContexts[midx]);
   mmap = mcon->Map;

   if ((clss == CtrDrvrConnectionClassCTIM) || (clss == CtrDrvrConnectionClassPTIM)) {
      if (cmsk & mcon->Clients[tndx]) {
	 mcon->Clients[tndx] &= ncmsk;
	 if (mcon->Clients[tndx] == 0) {
	    DisableInterrupts(tndx,midx,clss,ccon);
	    if (mcon->EqpClass[tndx] == CtrDrvrConnectionClassCTIM) {

	       bzero((void *) &trig,sizeof(CtrDrvrTrigger));
	       bzero((void *) &conf,sizeof(CtrDrvrCounterConfiguration));

	       Io32Write((unsigned int *) &(mmap->Trigs[tndx]),
			 (unsigned int *) TriggerToHard(&trig),
			 (unsigned int  ) sizeof(CtrDrvrHwTrigger));

	       Io32Write((unsigned int *) &(mmap->Configs[tndx]),
			 (unsigned int *) ConfigToHard(&conf),
			 (unsigned int  ) sizeof(CtrDrvrHwCounterConfiguration));

	       mcon->EqpNum[tndx]   = 0;
	       mcon->EqpClass[tndx] = 0;

	    } else {

	       valu = HRd(&mmap->Configs[tndx].Config);
	       valu &= ~(AutoShiftLeft(CtrDrvrCounterConfigON_ZERO_MASK, CtrDrvrCounterOnZeroBUS));
	       HWr(valu,&mmap->Configs[tndx].Config);
	    }
	 }
      }
   }

   if (clss == CtrDrvrConnectionClassHARD) {
      if (mcon->HardClients[tndx]) {
	 mcon->HardClients[tndx] &= ncmsk;
	 if (mcon->HardClients[tndx] == 0) {
	    DisableInterrupts(tndx,midx,clss,ccon);
	 }
      }
   }

   return OK;
}

/* ========================================================== */
/* Disconnect from a timing object                            */
/* ========================================================== */

static int DisConnect(CtrDrvrConnection    *conx,
		      CtrDrvrClientContext *ccon) {

CtrDrvrModuleContext *mcon;
int i, midx;

   midx = conx->Module -1;
   mcon = &(Wa->ModuleContexts[midx]);

   if (conx->EqpClass != CtrDrvrConnectionClassHARD) {
      for (i=0; i<CtrDrvrRamTableSIZE; i++)
	 if ((conx->EqpNum   == mcon->EqpNum[i])
	 &&  (conx->EqpClass == mcon->EqpClass[i])) DisConnectOne(i,midx,conx->EqpClass,ccon);

   } else for (i=0; i<CtrDrvrInterruptSOURCES; i++) DisConnectOne(i,midx,CtrDrvrConnectionClassHARD,ccon);

   return OK;
}

/* ========================================================== */
/* Disconnect from all timing objects                         */
/* ========================================================== */

static int DisConnectAll(CtrDrvrClientContext *ccon) {

CtrDrvrModuleContext *mcon;
int i, midx;

   for (midx=0; midx<Wa->Modules; midx++) {
      mcon = &(Wa->ModuleContexts[midx]);
      for (i=0; i<CtrDrvrRamTableSIZE;     i++) {
	 if (mcon->EqpNum[i]) {
	    DisConnectOne(i,midx,mcon->EqpClass[i],ccon);
	 }
      }
      for (i=0; i<CtrDrvrInterruptSOURCES; i++) DisConnectOne(i,midx,CtrDrvrConnectionClassHARD,ccon);
   }
   return OK;
}

/* ========================================================== */
/* The ISR                                                    */
/* ========================================================== */

int debug_isr = 0;

irqreturn_t IntrHandler(CtrDrvrModuleContext *mcon) {

unsigned int isrc, clients, msk, i, hlock, tndx = 0;
unsigned char inum;

unsigned long ps;

CtrDrvrMemoryMap      *mmap = NULL;
CtrDrvrFpgaCounter    *fpgc = NULL;
CtrDrvrCounterHistory *hist = NULL;

CtrDrvrQueue             *queue;
CtrDrvrClientContext     *ccon;
CtrDrvrReadBuf            rbf;

irqreturn_t ret;

   mmap = mcon->Map;
   isrc = IHRd(&mmap->InterruptSource);    /* Read and clear interrupt sources */
   if (isr_bus_error) {
      isrc = IHRd(&mmap->InterruptSource); /* Try again: Read and clear interrupt sources */
      if (isr_bus_error) isrc = 0;         /* Say its not handled */
   }

   if (isrc) ret = IRQ_HANDLED;                  /* Handle the interrupt */
   else      ret = IRQ_NONE;                     /* Pass it on to the next ISR */

   /* For each interrupt source/counter */

   for (inum=0; inum<CtrDrvrInterruptSOURCES; inum++) {
      msk = 1 << inum;                           /* Get counter source mask bit */
      if (msk & isrc) {
	 bzero((void *) &rbf, sizeof(CtrDrvrReadBuf));
	 clients = 0;                            /* No clients yet */

	 if (inum < CtrDrvrCOUNTERS) {           /* Counter Interrupt ? */
	    fpgc = &(mmap->Counters[inum]);      /* Get counter FPGA configuration */
	    hist = &(fpgc->History);             /* History of counter */

	    if (IHRd(&fpgc->Control.LockConfig) == 0) {    /* If counter not in remote */
	       tndx = IHRd(&hist->Index);                  /* Index into trigger table */
	       if (tndx < CtrDrvrRamTableSIZE) {

		  clients = mcon->Clients[tndx];    /* Clients connected to timing objects */
		  if (clients) {
		     rbf.TriggerNumber = tndx +1;            /* Trigger that loaded counter */
		     rbf.Frame.Long    = IHRd(&hist->Frame); /* Actual event that interrupted */

		     rbf.TriggerTime.Time.Second     = IHRd(&hist->TriggerTime.Time.Second);     /* Time counter was loaded */
		     rbf.TriggerTime.Time.TicksHPTDC = IHRd(&hist->TriggerTime.Time.TicksHPTDC); /* Time counter was loaded */
		     rbf.TriggerTime.CTrain          = IHRd(&hist->TriggerTime.CTrain);          /* Time counter was loaded */

		     rbf.StartTime.Time.Second       = IHRd(&hist->StartTime.Time.Second);       /* Time start arrived */
		     rbf.StartTime.Time.TicksHPTDC   = IHRd(&hist->StartTime.Time.TicksHPTDC);   /* Time start arrived */
		     rbf.StartTime.CTrain            = IHRd(&hist->StartTime.CTrain);            /* Time start arrived */

		     rbf.OnZeroTime.Time.Second      = IHRd(&hist->OnZeroTime.Time.Second);      /* Time of interrupt */
		     rbf.OnZeroTime.Time.TicksHPTDC  = IHRd(&hist->OnZeroTime.Time.TicksHPTDC);  /* Time of interrupt */
		     rbf.OnZeroTime.CTrain           = IHRd(&hist->OnZeroTime.CTrain);           /* Time of interrupt */

		     hlock = IHRd(&fpgc->Control.LockHistory);      /* Unlock history count Read/Clear */
		     if (hlock > 1)
			mcon->Status &= ~CtrDrvrStatusNO_LOST_INTERRUPTS;

		     /* Copy the rest of the information from the module context */

		     rbf.Ctim                = mcon->Trigs[tndx].Ctim;
		     rbf.Connection.Module   = mcon->ModuleIndex +1;
		     rbf.Connection.EqpClass = mcon->EqpClass[tndx];
		     rbf.Connection.EqpNum   = mcon->EqpNum[tndx];
		  }
	       }
	    }
	 }

	 if (clients == 0) {                     /* No object clients connections */
	    clients = mcon->HardClients[inum];   /* Hardware only connection */
	    if (clients) {
	       if (inum < CtrDrvrCOUNTERS) {
		  rbf.OnZeroTime.Time.Second      = IHRd(&hist->OnZeroTime.Time.Second);
		  rbf.OnZeroTime.Time.TicksHPTDC  = IHRd(&hist->OnZeroTime.Time.TicksHPTDC);
		  rbf.OnZeroTime.CTrain           = IHRd(&hist->OnZeroTime.CTrain);
	       } else {
		  rbf.OnZeroTime = *GetTime(mcon);
	       }
	       rbf.Connection.Module   = mcon->ModuleIndex +1;
	       rbf.Connection.EqpClass = CtrDrvrConnectionClassHARD;
	       rbf.Connection.EqpNum   = msk;
	    }
	 }

	 /* If client is connected to a hardware interrupt, and   */
	 /* other clients are connected to a timing object on the */
	 /* same counter; the client needs the interrupt number   */
	 /* to know from where the interrupt came. */

	 rbf.InterruptNumber = inum; /* Source of interrupt, Counter or other */

	 /* Place read buffer on connected clients queues */

	 clients |= mcon->HardClients[inum];
	 for (i=0; i<CtrDrvrCLIENT_CONTEXTS; i++) {
	    if (clients & (1 << i)) {
	       ccon = &(Wa->ClientContexts[i]);
	       queue = &(ccon->Queue);

	       disable(ps); {
		  queue->Entries[queue->WrPntr] = rbf;
		  queue->WrPntr = (queue->WrPntr + 1) % CtrDrvrQUEUE_SIZE;
		  if (queue->Size < CtrDrvrQUEUE_SIZE) {
		     queue->Size++;
		     ssignal(&(ccon->Semaphore));
		  } else {
		     queue->Missed++;
		     queue->RdPntr = (queue->RdPntr + 1) % CtrDrvrQUEUE_SIZE;
		  }
	       } restore(ps);

	    }
	 }

	 if ((clients == 0) && (debug_isr)) {
	    kkprintf("CtrDrvr: Spurious interrupt: Module:%d Source:0x%X Number:%d Tindex:%d\n",
		    (int) mcon->ModuleIndex +1,
		    (int) isrc, (int) inum, tndx);
	 }
      }
   }
   return ret;
}

/*========================================================================*/
/* OPEN                                                                   */
/*========================================================================*/

int CtrDrvrOpen(wa, dnm, flp)
CtrDrvrWorkingArea * wa;        /* Working area */
int dnm;                        /* Device number */
struct LynxFile * flp; {        /* File pointer */

int cnum;                       /* Client number */
CtrDrvrClientContext * ccon;    /* Client context */

   /* We allow one client per minor device, we use the minor device */
   /* number as an index into the client contexts array. */

   cnum = minor(flp->dev) -1;
   if ((cnum < 0) || (cnum >= CtrDrvrCLIENT_CONTEXTS)) {

      /* EFAULT = "Bad address" */

      pseterr(EFAULT);
      return SYSERR;
   }
   ccon = &(wa->ClientContexts[cnum]);

   /* If already open by someone else, give a permission denied error */

   if (ccon->InUse) {

      /* This next error is normal */

      /* EBUSY = "Resource busy" */

      pseterr(EBUSY);           /* File descriptor already open */
      return SYSERR;
   }

   /* Initialize a client context */

   bzero((void *) ccon, sizeof(CtrDrvrClientContext));
   ccon->ClientIndex = cnum;
   ccon->Timeout     = CtrDrvrDEFAULT_TIMEOUT;
   ccon->InUse       = 1;
   ccon->Pid         = getpid();
   sreset(&(ccon->Semaphore));
   return OK;
}

/*========================================================================*/
/* CLOSE                                                                  */
/*========================================================================*/

int CtrDrvrClose(wa, flp)
CtrDrvrWorkingArea * wa;    /* Working area */
struct LynxFile * flp; {        /* File pointer */

int cnum;                   /* Client number */
CtrDrvrClientContext *ccon; /* Client context */

   /* We allow one client per minor device, we use the minor device */
   /* number as an index into the client contexts array.            */

   cnum = minor(flp->dev) -1;
   if ((cnum >= 0) && (cnum < CtrDrvrCLIENT_CONTEXTS)) {

      ccon = &(Wa->ClientContexts[cnum]);

      if (ccon->DebugOn)
	 cprintf("CtrDrvrClose: Close client: %d for Pid: %d\n",
		 (int) ccon->ClientIndex,
		 (int) ccon->Pid);

      if (ccon->InUse == 0)
	 cprintf("CtrDrvrClose: Error bad client context: %d  Pid: %d Not in use\n",
		 (int) ccon->ClientIndex,
		 (int) ccon->Pid);

      ccon->InUse = 0; /* Free the context */

      /* Cancel any pending timeouts */

      CancelTimeout(&ccon->Timer);

      /* Disconnect this client from events */

      DisConnectAll(ccon);

      return(OK);

   } else {

      cprintf("CtrDrvrClose: Error bad client context: %d\n",cnum);

      /* EFAULT = "Bad address" */

      pseterr(EFAULT);
      return SYSERR;
   }
}

/*========================================================================*/
/* READ                                                                   */
/*========================================================================*/

int CtrDrvrRead(wa, flp, u_buf, cnt)
CtrDrvrWorkingArea * wa;       /* Working area */
struct LynxFile * flp;              /* File pointer */
char * u_buf;                   /* Users buffer */
int cnt; {                      /* Byte count in buffer */

CtrDrvrClientContext *ccon;    /* Client context */
CtrDrvrQueue         *queue;
CtrDrvrReadBuf       *rb;
int                    cnum;    /* Client number */
unsigned long          ps;

   cnum = minor(flp->dev) -1;
   ccon = &(wa->ClientContexts[cnum]);

   if (ccon->DebugOn) cprintf("CtrDrvrRead:Client Number:%d Count:%d\n",cnum,cnt);

   queue = &(ccon->Queue);
   if (queue->QueueOff) {
      disable(ps); {
	 queue->Size   = 0;
	 queue->Missed = 0;         /* ToDo: What to do with this info ? */
	 queue->WrPntr = 0;
	 queue->RdPntr = 0;
	 sreset(&(ccon->Semaphore));
      } restore(ps);
   }

   if (ccon->Timeout) {
      ccon->Timer = timeout(ReadTimeout, (char *) ccon, ccon->Timeout);
      if (ccon->Timer < 0) {

	 ccon->Timer = 0;

	 if (ccon->DebugOn) cprintf("CtrDrvrRead:Client Number:%d Count:%d No timers available\n",cnum,cnt);

	 /* EBUSY = "Device or resource busy" */

	 pseterr(EBUSY);    /* No available timers */
	 return 0;
      }
   }

   if (swait(&(ccon->Semaphore), SEM_SIGABORT)) {

      if (ccon->DebugOn) cprintf("CtrDrvrRead:Client Number:%d Count:%d Signaled in swait\n",cnum,cnt);

      /* EINTR = "Interrupted system call" */

      pseterr(EINTR);   /* We have been signaled */
      return 0;
   }

   if (ccon->Timeout) {
      if (ccon->Timer) {
	 CancelTimeout(&(ccon->Timer));
      } else {

	 if (ccon->DebugOn) cprintf("CtrDrvrRead:Client Number:%d Count:%d Timed out\n",cnum,cnt);

	 /* ETIME = "Timer expired */

	 pseterr(ETIME);
	 return 0;
      }
   }

   rb = (CtrDrvrReadBuf *) u_buf;
   if (queue->Size) {
      disable(ps); {
	 *rb = queue->Entries[queue->RdPntr];
	 queue->RdPntr = (queue->RdPntr + 1) % CtrDrvrQUEUE_SIZE;
	 queue->Size--;
      } restore(ps);
      return sizeof(CtrDrvrReadBuf);
   }

   if (ccon->DebugOn) cprintf("CtrDrvrRead:Client Number:%d Count:%d Queue empty\n",cnum,cnt);

   pseterr(EINTR);
   return 0;
}

/*========================================================================*/
/* WRITE                                                                  */
/* Write is a way of simulating incomming events, calling write with the  */
/* appropriate values in the buffer will cause waiting clients to wake up */
/* with the supplied CtrDrvrReadBuf.                                     */
/*========================================================================*/

int CtrDrvrWrite(wa, flp, u_buf, cnt)
CtrDrvrWorkingArea * wa;       /* Working area */
struct LynxFile * flp;             /* File pointer */
char * u_buf;                  /* Users buffer */
int cnt; {                     /* Byte count in buffer */

CtrDrvrClientContext *ccon;    /* Client context */
CtrDrvrModuleContext *mcon;
CtrDrvrQueue         *queue;
CtrDrvrConnection    *conx;
CtrDrvrReadBuf        rb;
CtrDrvrWriteBuf      *wb;
unsigned long         ps;
int                   i, tndx, midx, hmsk;
unsigned int          clients;

   wb = (CtrDrvrWriteBuf *) u_buf;
   conx = &(wb->Connection);

   midx = conx->Module -1;
   if ((midx<0) || (midx>=CtrDrvrMODULE_CONTEXTS)) {
      pseterr(EINVAL);  /* Caller error */
      return 0;
   }
   mcon = &(wa->ModuleContexts[midx]);

   bzero((void *) &rb,sizeof(CtrDrvrReadBuf));

   clients = 0;
   if (conx->EqpClass != CtrDrvrConnectionClassHARD) {

      if (wb->TriggerNumber == 0) {

	 /* This code provokes the first matching trigger */

	 for (tndx=0; tndx<CtrDrvrRamTableSIZE; tndx++) {
	    if ((mcon->EqpNum[tndx]   == conx->EqpNum)
	    &&  (mcon->EqpClass[tndx] == conx->EqpClass)) {
	       clients = mcon->Clients[tndx];
	       rb.TriggerNumber = tndx +1;
	       rb.Frame = mcon->Trigs[tndx].Frame;
	       rb.Frame.Struct.Value = wb->Payload;
	       rb.Ctim = mcon->Trigs[tndx].Ctim;
	       rb.InterruptNumber = mcon->Trigs[tndx].Counter;
	       break;
	    }
	 }
      } else {
	 tndx = wb->TriggerNumber -1;
	 clients = mcon->Clients[tndx];
	 rb.TriggerNumber = tndx +1;
	 rb.Frame = mcon->Trigs[tndx].Frame;
	 rb.Ctim = mcon->Trigs[tndx].Ctim;
	 rb.InterruptNumber = mcon->Trigs[tndx].Counter;
      }
   } else {
      for (i=0; i<CtrDrvrInterruptSOURCES; i++) {
	 hmsk = 1 << i;
	 if (conx->EqpNum & hmsk) {
	    clients |= mcon->HardClients[i];
	    rb.InterruptNumber = i + 1;
	 }
      }
   }

   /* Place read buffer on connected clients queues */

   if (clients) {

      rb.Connection = *conx;
      rb.TriggerTime = *GetTime(mcon);
      rb.StartTime = rb.TriggerTime;
      rb.OnZeroTime = rb.TriggerTime;

      for (i=0; i<CtrDrvrCLIENT_CONTEXTS; i++) {
	 if (clients & (1 << i)) {
	    ccon = &(Wa->ClientContexts[i]);
	    queue = &(ccon->Queue);

	    disable(ps); {
	       queue->Entries[queue->WrPntr] = rb;
	       queue->WrPntr = (queue->WrPntr + 1) % CtrDrvrQUEUE_SIZE;
	       if (queue->Size < CtrDrvrQUEUE_SIZE) {
		  queue->Size++;
		  ssignal(&(ccon->Semaphore));
	       } else {
		  queue->Missed++;
		  queue->RdPntr = (queue->RdPntr + 1) % CtrDrvrQUEUE_SIZE;
	       }
	    } restore(ps);
	 }
      }
   }
   return sizeof(CtrDrvrConnection);
}

/*========================================================================*/
/* SELECT                                                                 */
/*========================================================================*/

int CtrDrvrSelect(wa, flp, wch, ffs)
CtrDrvrWorkingArea * wa;        /* Working area */
struct LynxFile * flp;              /* File pointer */
int wch;                        /* Read/Write direction */
struct sel * ffs; {             /* Selection structurs */

CtrDrvrClientContext * ccon;
int cnum;

   cnum = minor(flp->dev) -1;
   ccon = &(wa->ClientContexts[cnum]);

   if (wch == SREAD) {
      ffs->iosem = (int *) &(ccon->Semaphore); /* Watch out here I hope   */
      return OK;                               /* the system dosn't swait */
   }                                           /* the read does it too !! */

   pseterr(EACCES);         /* Permission denied */
   return SYSERR;
}

/* ========================================================= */
/* Convert the standard XML tree into a module address array */

static CtrDrvrInfoTable tinfo;

void CtrConvertInfo(InsLibDrvrDesc   *drvrd,
		    CtrDrvrInfoTable *addresses) {

InsLibModlDesc         *modld = NULL;
InsLibVmeModuleAddress *vmema = NULL;
InsLibVmeAddressSpace  *vmeas = NULL;
InsLibIntrDesc         *intrd = NULL;

int index;

   bzero((void *) addresses,(size_t) sizeof(CtrDrvrModuleAddress));

   modld = drvrd->Modules;
   while (modld) {
      if (modld->BusType != InsLibBusTypeVME) return;

      index = addresses->Modules;
      addresses->Modules += 1;

      intrd = modld->Isr;

      vmema = modld->ModuleAddress;
      if (vmema) {
	 vmeas = vmema->VmeAddressSpace;
	 while (vmeas) {

	    if (vmeas->AddressModifier == 0x29)
	       addresses->Addresses[index].JTGAddress = (unsigned short *) vmeas->BaseAddress;

	    if (vmeas->AddressModifier == 0x39)
	       addresses->Addresses[index].VMEAddress = (unsigned int *)   vmeas->BaseAddress;

	    if (intrd) {
	       addresses->Addresses[index].InterruptVector = (unsigned int) intrd->Vector;
	       addresses->Addresses[index].InterruptLevel  = (unsigned int) intrd->Level;
	    }

	    vmeas = vmeas->Next;
	 }
      }
      modld = modld->Next;
   }
}

/* ================================================= */
/* Build a ctr info object from the pointer infoaddr */

int BuildInfoObject() {

InsLibDrvrDesc *drvrd = NULL;
int rc = 1;

   InsLibSetCopyRoutine((void(*)(void*, const void*, int n)) copy_from_user);

   drvrd = InsLibCloneOneDriver((InsLibDrvrDesc *) infoaddr);

   if (drvrd) {
      if ( (strcmp(drvrd->DrvrName,"CTR" ) == 0)
      ||   (strcmp(drvrd->DrvrName,"CTRV") == 0)
      ||   (strcmp(drvrd->DrvrName,"ctr" ) == 0)
      ||   (strcmp(drvrd->DrvrName,"ctrv") == 0) ) {

	 CtrConvertInfo(drvrd,&tinfo);
	 rc = 0;
      }
   }
   InsLibFreeDriver(drvrd);
   return rc;
}

/*========================================================================*/
/* INSTALL  LynxOS Version 4 VME version                                  */
/*========================================================================*/

char * CtrDrvrInstall(void *junk) {

CtrDrvrWorkingArea *wa;

CtrDrvrModuleContext          *mcon;
CtrDrvrModuleAddress          *moad;
int                            i, j, erlv;
CtrDrvrMemoryMap              *mmap;

   /* Allocate the driver working area. */

   wa = (CtrDrvrWorkingArea *) sysbrk(sizeof(CtrDrvrWorkingArea));
   if (!wa) {
      cprintf("CtrDrvrInstall: NOT ENOUGH MEMORY(WorkingArea)\n");
      pseterr(ENOMEM);          /* Not enough core */
      return((char *) SYSERR);
   }
   Wa = wa;                     /* Global working area pointer */

   /****************************************/
   /* Initialize the driver's working area */
   /* and add the modules ISR into LynxOS  */
   /****************************************/

   bzero((void *) wa,sizeof(CtrDrvrWorkingArea)); /* Clear working area */

   bzero((void *) &tinfo, sizeof(CtrDrvrInfoTable));

   if (BuildInfoObject() /* Try to use the XML object tree */ ) {

      /* Failed so do a default install */

      if (modules <= 0)
	      modules = 1;
      if (modules >  CtrDrvrMODULE_CONTEXTS)
	      modules = CtrDrvrMODULE_CONTEXTS;


      cprintf("CtrDrvrInstall: Will auto-install:%d CTRV modules\n",modules);
      tinfo.Modules = modules;

      cprintf("CtrDrvrInstall: Will auto-install:%d CTRV modules\n",tinfo.Modules);


      for (i=0; i<modules; i++) {
	 moad = &tinfo.Addresses[i];
	 moad->VMEAddress      = (unsigned int   *) 0xC00000 + (0x10000 * i);
	 moad->JTGAddress      = (unsigned short *) 0x100    + (0x100   * i);
	 moad->InterruptVector = (unsigned char   ) 0xB8     + (0x1     * i);
	 moad->InterruptLevel  = 2;
      }
   }

   for (i=0; i<tinfo.Modules; i++) {
      mcon = &(wa->ModuleContexts[i]);
      moad = &mcon->Address;
      *moad = tinfo.Addresses[i];
      mcon->ModuleIndex = i;
      erlv = AddModule(mcon,i,0);
      if (erlv == 0) {
	 cprintf("CtrDrvr: Module %d. VME Addr: %x JTAG Addr: %x Vect: %x Level: %x Installed OK\n",
		 i+1,
		 (unsigned int) moad->VMEAddress,
		 (unsigned int) moad->JTGAddress,
		 (unsigned int) moad->InterruptVector,
		 (unsigned int) moad->InterruptLevel);

	 /* Wipe out any old triggers left in Ram after a warm reboot */

	 mmap = mcon->Map;
	 for (j=0; j<CtrDrvrRamTableSIZE; j++) {
	    HWr(0,&(mmap->Trigs[j].Frame));
	    HWr(0,&(mmap->Trigs[j].Trigger));
	 }
	 mcon->InUse = 1;
      } else if (erlv == 2) {
	 moad->VMEAddress = 0;
	 cprintf("CtrDrvr: Module: %d WARNING: JTAG Only: JTAG Addr: %x\n",
		 i+1, (unsigned int) moad->JTGAddress);

	 mcon->InUse = 1;
      } else {
	 moad->VMEAddress = 0;
	 moad->JTGAddress = 0;
	 cprintf("CtrDrvr: Module: %d ERROR: Not Installed\n",i+1);
      }
   }
   wa->Modules = tinfo.Modules;

   cprintf("CtrDrvr: Installed: %d GMT Modules in Total\n",(int) wa->Modules);
   return((char*) wa);
}

/*========================================================================*/
/* Uninstall                                                              */
/*========================================================================*/

int CtrDrvrUninstall(wa)
CtrDrvrWorkingArea * wa; {     /* Drivers working area pointer */

CtrDrvrClientContext *ccon;
CtrDrvrModuleContext *mcon;
int i;

   for (i=0; i<CtrDrvrMODULE_CONTEXTS; i++) {
      mcon = &(wa->ModuleContexts[i]);
      if (mcon->InUse) RemoveModule(mcon);
   }

   for (i=0; i<CtrDrvrCLIENT_CONTEXTS; i++) {
      ccon = &(wa->ClientContexts[i]);
      if (ccon->InUse) {
	 if (ccon->Timer) {
	    CancelTimeout(&(ccon->Timer));
	  }
	  ssignal(&(ccon->Semaphore));         /* Wakeup client */
	  ccon->InUse = 0;
      }
   }

   sysfree((void *) wa,sizeof(CtrDrvrWorkingArea)); Wa = NULL;
   cprintf("CtrDrvr: Driver: UnInstalled\n");
   return OK;
}

/*========================================================================*/
/* IOCTL                                                                  */
/*========================================================================*/

int CtrDrvrIoctl(wa, flp, cm, arg)
CtrDrvrWorkingArea * wa;       /* Working area */
struct LynxFile * flp;             /* File pointer */
CtrDrvrControlFunction cm;     /* IOCTL command */
char * arg; {                  /* Data for the IOCTL */

CtrDrvrModuleContext           *mcon;   /* Module context */
CtrDrvrClientContext           *ccon;   /* Client context */
CtrDrvrConnection              *conx;
CtrDrvrClientList              *cls;
CtrDrvrClientConnections       *ccn;
CtrDrvrRawIoBlock              *riob;
CtrDrvrVersion                 *ver;
CtrDrvrModuleAddress           *moad;
CtrDrvrCTime                   *ctod;
CtrDrvrHwTrigger               *htrg;
CtrDrvrTrigger                 *strg;
CtrDrvrAction                  *act;
CtrDrvrCtimBinding             *ctim;
CtrDrvrPtimBinding             *ptim;
CtrDrvrCtimObjects             *ctimo;
CtrDrvrPtimObjects             *ptimo;
CtrDrvrHptdcIoBuf              *hpio;
CtrDrvrCounterConfigurationBuf *conf;
CtrDrvrCounterHistoryBuf       *hisb;
CtrdrvrRemoteCommandBuf        *remc;
CtrDrvrCounterMaskBuf          *cmsb;
CtrDrvrPll                     *pll;
CtrDrvrPllAsyncPeriodNs        *asyp;
CtrDrvrTgmBuf                  *tgmb;
CtrDrvrEventHistory            *evhs;
CtrDrvrReceptionErrors         *rcpe;
CtrDrvrBoardId                 *bird;
CtrDrvrModuleStats             *mods;

CtrDrvrMemoryMap   *mmap;

int i, j, k, n, msk, pid, hmsk, found, size, start;

int cnum;                 /* Client number */
int lav, *lap;            /* Long Value pointed to by Arg */
unsigned short sav;       /* Short argument and for Jtag IO */
int rcnt, wcnt;           /* Readable, Writable byte counts at arg address */

   /* Check argument contains a valid address for reading or writing. */
   /* We can not allow bus errors to occur inside the driver due to   */
   /* the caller providing a garbage address in "arg". So if arg is   */
   /* not null set "rcnt" and "wcnt" to contain the byte counts which */
   /* can be read or written to without error. */

   if (arg != NULL) {
      rcnt = rbounds((int)arg);       /* Number of readable bytes without error */
      wcnt = wbounds((int)arg);       /* Number of writable bytes without error */
      if (rcnt < sizeof(int)) {       /* We at least need to read one int */
	 pid = getpid();
	 cprintf("CtrDrvrIoctl:Illegal arg-pntr:0x%X ReadCnt:%d(%d) Pid:%d Cmd:%d\n",
		 (int) arg,rcnt,sizeof(int),pid,(int) cm);
	 pseterr(EINVAL);        /* Invalid argument */
	 return SYSERR;
      }
      lav = *((int *) arg);       /* Int32 argument value */
      lap =   (int *) arg ;       /* Int32 argument pointer */
   } else {
      rcnt = 0; wcnt = 0; lav = 0; lap = NULL; /* Null arg = zero read/write counts */
   }
   sav = lav;                      /* Short argument value */

   /* We allow one client per minor device, we use the minor device */
   /* number as an index into the client contexts array. */

   cnum = minor(flp->dev) -1;
   if ((cnum < 0) || (cnum >= CtrDrvrCLIENT_CONTEXTS)) {
      pseterr(ENODEV);          /* No such device */
      return SYSERR;
   }

   /* We can't control a file which is not open. */

   ccon = &(wa->ClientContexts[cnum]);
   if (ccon->InUse == 0) {
      cprintf("CtrDrvrIoctl: DEVICE %2d IS NOT OPEN\n",cnum+1);
      pseterr(EBADF);           /* Bad file number */
      return SYSERR;
   }

   /* Only the pid that opened the driver is allowed to call an ioctl */

#ifdef STRICT_PID
   pid = getpid();
   if (pid != ccon->Pid) {
      if (ccon->DebugOn)
	 cprintf("CtrDrvrIoctl:Warning:Spurious IOCTL call:%d by PID:%d for PID:%d on FD:%d\n",
		 (int) cm,
		 (int) pid,
		 (int) ccon->Pid,
		 (int) cnum);
	 pseterr(EBADF);           /* Bad file number */
	 return SYSERR;
      }
   }
#endif

   /* Set up some useful module pointers */

   mcon = &(wa->ModuleContexts[ccon->ModuleIndex]); /* Default module selected */
   mmap = (CtrDrvrMemoryMap *) mcon->Map;

   if (mcon->InUse == 0) {
      cprintf("CtrDrvrIoctl: No hardware installed\n");
      pseterr(ENODEV);          /* No such device */
      return SYSERR;
   }

   if (ccon->DebugOn) DebugIoctl(cm,arg);   /* Print debug message */

   mcon->BusErrorCount += GetClrBusErrCnt();

   /*************************************/
   /* Decode callers command and do it. */
   /*************************************/

   switch (cm) {

      case CtrDrvrSET_SW_DEBUG:           /* Set driver debug mode */
	 if (lap) {
	    if (lav) ccon->DebugOn = lav;
	    else     ccon->DebugOn = 0;

	    if (lav == CtrDrvrDEBUG_ISR) { debug_isr = 1; ccon->DebugOn = 0; }
	    else                           debug_isr = 0;

	    return OK;
	 }
      break;

      case CtrDrvrGET_SW_DEBUG:
	 if (lap) {
	    *lap = ccon->DebugOn;
	    return OK;
	 }
      break;

      case CtrDrvrGET_VERSION:            /* Get version date */
	 if (wcnt >= sizeof(CtrDrvrVersion)) {
	    ver = (CtrDrvrVersion *) arg;
	    i = GetVersion(mcon,ver);
	    return i;
	 }
      break;

      case CtrDrvrSET_TIMEOUT:            /* Set the read timeout value */
	 if (lap) {
	    ccon->Timeout = lav;
	    return OK;
	 }
      break;

      case CtrDrvrGET_TIMEOUT:            /* Get the read timeout value */
	 if (lap) {
	    *lap = ccon->Timeout;
	    return OK;
	 }
      break;

      case CtrDrvrSET_QUEUE_FLAG:         /* Set queuing capabiulities on off */
	 if (lap) {
	    if (lav) ccon->Queue.QueueOff = 1;
	    else     ccon->Queue.QueueOff = 0;
	    return OK;
	 }
      break;

      case CtrDrvrGET_QUEUE_FLAG:         /* 1=Q_off 0=Q_on */
	 if (lap) {
	    *lap = ccon->Queue.QueueOff;
	    return OK;
	 }
      break;

      case CtrDrvrGET_QUEUE_SIZE:         /* Number of events on queue */
	 if (lap) {
	    *lap = ccon->Queue.Size;
	    return OK;
	 }
      break;

      case CtrDrvrGET_QUEUE_OVERFLOW:     /* Number of missed events */
	 if (lap) {
	    *lap = ccon->Queue.Missed;
	    ccon->Queue.Missed = 0;
	    return OK;
	 }
      break;

      case CtrDrvrGET_MODULE_DESCRIPTOR:
	 if (wcnt >= sizeof(CtrDrvrModuleAddress)) {
	    moad = (CtrDrvrModuleAddress *) arg;

	    moad->VMEAddress      = mcon->Address.VMEAddress;
	    moad->JTGAddress      = mcon->Address.JTGAddress;
	    moad->InterruptVector = mcon->Address.InterruptVector;
	    moad->InterruptLevel  = mcon->Address.InterruptLevel;
	    moad->CopyAddress     = mcon->Address.CopyAddress;
	    return OK;
	 }
      break;

      case CtrDrvrSET_MODULE:             /* Select the module to work with */
	 if (lap) {
	    if ((lav >= 1)
	    &&  (lav <= Wa->Modules)) {
	       ccon->ModuleIndex = lav -1;
	       mcon = &(wa->ModuleContexts[ccon->ModuleIndex]);
	       mcon->ModuleIndex = ccon->ModuleIndex;
	       return OK;
	    }
	 }
      break;

      case CtrDrvrGET_MODULE:             /* Which module am I working with */
	 if (lap) {
	    *lap = ccon->ModuleIndex +1;
	    return OK;
	 }
      break;

      case CtrDrvrGET_MODULE_COUNT:       /* The number of installed modules */
	 if (lap) {
	    *lap = Wa->Modules;
	    return OK;
	 }
      break;

      case CtrDrvrRESET:                  /* Reset the module, re-establish connections */
	 return Reset(mcon);

      case CtrDrvrGET_STATUS:             /* Read module status */
	 if (lap) {
	    *lap = GetStatus(mcon);
	    if (*lap != SYSERR) return OK;
	 }
      break;

      case CtrDrvrGET_CLIENT_LIST:        /* Get the list of driver clients */
	 if (wcnt >= sizeof(CtrDrvrClientList)) {
	    cls = (CtrDrvrClientList *) arg;
	    bzero((void *) cls, sizeof(CtrDrvrClientList));
	    for (i=0; i<CtrDrvrCLIENT_CONTEXTS; i++) {
	       ccon = &(wa->ClientContexts[i]);
	       if (ccon->InUse)
		  cls->Pid[cls->Size++] = ccon->Pid;
	    }
	    return OK;
	 }
      break;

      case CtrDrvrCONNECT:                /* Connect to an object interrupt */
	 conx = (CtrDrvrConnection *) arg;
	 if (rcnt >= sizeof(CtrDrvrConnection)) return Connect(conx,ccon);
      break;

      case CtrDrvrDISCONNECT:             /* Disconnect from an object interrupt */
	 conx = (CtrDrvrConnection *) arg;
	 if (rcnt >= sizeof(CtrDrvrConnection)) {
	    if (conx->EqpNum) return DisConnect(conx,ccon);
	    else              return DisConnectAll(ccon);
	 }
      break;

      case CtrDrvrGET_CLIENT_CONNECTIONS: /* Get the list of a client connections on module */
	 if (wcnt >= sizeof(CtrDrvrClientConnections)) {
	    ccn = (CtrDrvrClientConnections *) arg;
	    ccn->Size = 0;
	    for (i=0; i<CtrDrvrCLIENT_CONTEXTS; i++) {
	       ccon = &(wa->ClientContexts[i]);
	       if ((ccon->InUse) && (ccon->Pid == ccn->Pid)) {
		  msk = 1 << ccon->ClientIndex;
		  for (j=0; j<CtrDrvrMODULE_CONTEXTS; j++) {
		     mcon = &(Wa->ModuleContexts[j]);
		     for (k=0; k<CtrDrvrRamTableSIZE; k++) {
			if (msk & mcon->Clients[k]) {
			   found = 0;
			   for (n=0; n<ccn->Size; n++) {
			      conx = &(ccn->Connections[n]);
			      if ((mcon->EqpNum[k]   == conx->EqpNum)
			      &&  (mcon->EqpClass[k] == conx->EqpClass)) {
				 found = 1;
				 break;
			      }
			   }
			   if (!found) {
			      ccn->Connections[ccn->Size].Module   = mcon->ModuleIndex +1;
			      ccn->Connections[ccn->Size].EqpNum   = mcon->EqpNum[k];
			      ccn->Connections[ccn->Size].EqpClass = mcon->EqpClass[k];
			      if (ccn->Size++ >= CtrDrvrCONNECTIONS) return OK;
			   }
			}
		     }

		     hmsk = 0;
		     for (k=0; k<CtrDrvrInterruptSOURCES; k++)
			if (msk & mcon->HardClients[k]) hmsk |= (1 << k);
		     if (hmsk) {
			ccn->Connections[ccn->Size].Module   = mcon->ModuleIndex +1;
			ccn->Connections[ccn->Size].EqpNum   = hmsk;
			ccn->Connections[ccn->Size].EqpClass = CtrDrvrConnectionClassHARD;
			if (ccn->Size++ >= CtrDrvrCONNECTIONS) return OK;
		     }
		  }
	       }
	    }
	    return OK;
	 }
      break;

      case CtrDrvrSET_UTC:                /* Set Universal Coordinated Time for next PPS tick */
	 if (lap) return SetTime(mcon,lav);
      break;

      case CtrDrvrGET_UTC:                /* Latch and read the current UTC time */
	 if (wcnt >= sizeof(CtrDrvrCTime)) {
	    ctod = (CtrDrvrCTime *) arg;
	    *ctod = *GetTime(mcon);
	    return OK;
	 }
      break;

      case CtrDrvrGET_CABLE_ID:           /* Cables telegram ID */
	 if (lap) {
	    if (mcon->CableId) *lap = mcon->CableId;
	    else               *lap = HRd(&mmap->CableId);
	    return OK;
	 }
      break;

      case CtrDrvrSET_CABLE_ID:           /* If not sent by the hardware */
	 if (lap) {
	    mcon->CableId = lav;
	    return OK;
	 }
      break;

      case CtrDrvrJTAG_OPEN:              /* Open JTAG interface */
	 if (mcon->IrqBalance == 1) {
	    disable_irq(mcon->IVector);
	    mcon->IrqBalance = 0;
	 }

	 if (mcon->Address.JTGAddress != NULL) {
	    mcon->FlashOpen = 1;
	    return OK;
	 }
	 mcon->FlashOpen = 0;
	 cprintf("gmtdrvr: Can not open Jtag port: Hardware not present\n");
      break;

      case CtrDrvrJTAG_READ_BYTE:         /* Read back uploaded VHDL bit stream */
	 if (mcon->FlashOpen) {
	    sav = HRdJtag(mcon->Address.JTGAddress);
	    if (ccon->DebugOn == 2)
	       cprintf("Jtag: Addr: %x Read %x\n",(int) mcon->Address.JTGAddress,(int) sav);
	    *lap = (0x000000FF & (unsigned int) sav);
	    return OK;
	 }
	 pseterr(EBUSY);                    /* Device busy, not opened */
	 return SYSERR;

      case CtrDrvrJTAG_WRITE_BYTE:        /* Upload a new compiled VHDL bit stream */
	 if (mcon->FlashOpen) {
	    sav = (short) lav;
	    HWrJtag(sav,mcon->Address.JTGAddress);
	    if (ccon->DebugOn == 2)
	       cprintf("Jtag: Addr: %x Write %x\n",(int) mcon->Address.JTGAddress,(int) sav);
	    return OK;
	 }
	 pseterr(EBUSY);                    /* Device busy, not opened */
	 return SYSERR;

      case CtrDrvrJTAG_CLOSE:             /* Close JTAG interface */

	 /* Wait 1S for the module to settle */

	 sreset(&(mcon->Semaphore));
	 mcon->Timer = timeout(ResetTimeout, (char *) mcon, 100);
	 if (mcon->Timer < 0) mcon->Timer = 0;
	 if (mcon->Timer) swait(&(mcon->Semaphore), SEM_SIGABORT);
	 if (mcon->Timer) CancelTimeout(&(mcon->Timer));

	 EnableInterrupts(mcon,0);
	 mcon->FlashOpen = 0;
	 if (mcon->IrqBalance == 0) {
	    enable_irq(mcon->IVector);
	    mcon->IrqBalance = 1;
	 }
	 return OK;

      case CtrDrvrHPTDC_OPEN:             /* Open HPTDC JTAG interface */
	 mcon->HptdcOpen = 1;
	 return OK;

      case CtrDrvrHPTDC_IO:               /* Do HPTDC IO */
	 if (rcnt >= sizeof(CtrDrvrHptdcIoBuf)) {
	    if (mcon->HptdcOpen) {
	       hpio = (CtrDrvrHptdcIoBuf *) arg;
	       return HptdcCommand(mcon,hpio->Cmd,hpio->Wreg,hpio->Rreg,hpio->Size,hpio->Pflg,hpio->Rflg);
	    }
	 }
      break;

      case CtrDrvrHPTDC_CLOSE:            /* Close HPTDC JTAG interface */
	 mcon->HptdcOpen = 0;
	 return OK;

      case CtrDrvrRAW_READ:               /* Raw read  access to card for debug */
	 if (wcnt >= sizeof(CtrDrvrRawIoBlock)) {
	    riob = (CtrDrvrRawIoBlock *) arg;
	    if ((riob->UserArray != NULL)
	    &&  (wcnt > riob->Size * sizeof(unsigned int))) {
	       return RawIo(mcon,riob,0,ccon->DebugOn);
	    }
	 }
      break;

      case CtrDrvrRAW_WRITE:              /* Raw write access to card for debug */
	 if (rcnt >= sizeof(CtrDrvrRawIoBlock)) {
	    riob = (CtrDrvrRawIoBlock *) arg;
	    if ((riob->UserArray != NULL)
	    &&  (rcnt > riob->Size * sizeof(unsigned int))) {
	       return RawIo(mcon,riob,1,ccon->DebugOn);
	    }
	 }
      break;

      case CtrDrvrGET_ACTION:             /* Low level direct access to CTR RAM tables */
	 if (wcnt >= sizeof(CtrDrvrAction)) {
	    act = (CtrDrvrAction *) arg;
	    if ((act->TriggerNumber > 0) && (act->TriggerNumber <= CtrDrvrRamTableSIZE)) {
	       i = act->TriggerNumber -1;
	       Int32Copy((unsigned int *) &(act->Trigger),
			 (unsigned int *) HardToTrigger(&(mmap->Trigs[i])),
			 (unsigned int  ) sizeof(CtrDrvrTrigger));
	       Int32Copy((unsigned int *) &(act->Config ),
			 (unsigned int *) HardToConfig(&(mmap->Configs[i])),
			 (unsigned int  ) sizeof(CtrDrvrCounterConfiguration));
	       act->Trigger.Ctim = mcon->Trigs[i].Ctim;
	       act->EqpNum       = mcon->EqpNum[i];
	       act->EqpClass     = mcon->EqpClass[i];
	       return OK;
	    }
	 }
      break;

      case CtrDrvrSET_ACTION:             /* Set should not modify the bus interrupt settings */
	 if (rcnt >= sizeof(CtrDrvrAction)) {
	    act = (CtrDrvrAction *) arg;
	    if ((act->TriggerNumber > 0) && (act->TriggerNumber <= CtrDrvrRamTableSIZE)) {
	       i = act->TriggerNumber -1;

	       if (mcon->EqpClass[i] == CtrDrvrConnectionClassHARD) break;

	       if (mcon->EqpNum[i] !=0) {
		  if (mcon->EqpNum[i] != act->EqpNum) {
		     cprintf("CtrDrvr: SetAction: Illegal EqpNum: %d->%d\n",
			     (int) mcon->EqpNum[i],
			     (int) act->EqpNum);
			     break;
		  }
		  if (mcon->EqpClass[i] != act->EqpClass) {
		     cprintf("CtrDrvr: SetAction: Illegal EqpClass: %d->%d\n",
			     (int) mcon->EqpClass[i],
			     (int) act->EqpClass);
			     break;
		  }
		  if (mcon->Trigs[i].Counter != act->Trigger.Counter) {
		     cprintf("CtrDrvr: SetAction: Illegal Counter: %d->%d\n",
			     (int) mcon->Trigs[i].Counter,
			     (int) act->Trigger.Counter);
			     break;
		  }

	       } else {
		  mcon->EqpNum[i] = act->EqpNum;
		  mcon->EqpClass[i] = act->EqpClass;
		  mcon->Trigs[i].Counter = act->Trigger.Counter;
	       }

	       Io32Write((unsigned int *) &(mmap->Trigs[i]),
			 (unsigned int *) TriggerToHard(&act->Trigger),
			 (unsigned int  ) sizeof(CtrDrvrHwTrigger));
	       mcon->Trigs[i] = act->Trigger;

	       /* Override bus interrupt settings for connected clients */

	       if (mcon->Clients[i])
		  act->Config.OnZero |= CtrDrvrCounterOnZeroBUS;
	       else
		  act->Config.OnZero &= ~CtrDrvrCounterOnZeroBUS;

	       Io32Write((unsigned int *) &(mmap->Configs[i]),
			 (unsigned int *) ConfigToHard(&(act->Config)),
			 (unsigned int  ) sizeof(CtrDrvrHwCounterConfiguration));
	       mcon->Configs[i] = act->Config;

	       return OK;
	    }
	 }
      break;

      case CtrDrvrCREATE_CTIM_OBJECT:     /* Create a new CTIM timing object */
	 if (rcnt >= sizeof(CtrDrvrCtimBinding)) {
	    ctim = (CtrDrvrCtimBinding *) arg;
	    for (i=0; i<Wa->Ctim.Size; i++) {
	       if (ctim->EqpNum == Wa->Ctim.Objects[i].EqpNum) {
		  pseterr(EBUSY);        /* Already defined */
		  return SYSERR;
	       }
	    }
	    i = Wa->Ctim.Size;
	    if (i<CtrDrvrCtimOBJECTS) {
	       Wa->Ctim.Objects[i] = *ctim;
	       Wa->Ctim.Size = i +1;
	       return OK;
	    }
	    pseterr(ENOMEM);
	    return SYSERR;
	 }
      break;

      case CtrDrvrDESTROY_CTIM_OBJECT:    /* Destroy a CTIM timing object */
	 if (rcnt >= sizeof(CtrDrvrCtimBinding)) {
	    ctim = (CtrDrvrCtimBinding *) arg;
	    for (i=0; i<Wa->Ctim.Size; i++) {
	       if (ctim->EqpNum == Wa->Ctim.Objects[i].EqpNum) {
		  for (j=0; j<CtrDrvrRamTableSIZE; j++) {
		     strg = &(mcon->Trigs[j]);
		     if (strg->Ctim == ctim->EqpNum) {
			pseterr(EBUSY);   /* In use by ptim object */
			return SYSERR;
		     }
		  }
		  Wa->Ctim.Objects[i] = Wa->Ctim.Objects[Wa->Ctim.Size -1];
		  Wa->Ctim.Size--;
		  return OK;
	       }
	    }
	 }
      break;

      case CtrDrvrCHANGE_CTIM_FRAME:      /* Change the frame of an existing CTIM object */
	 if (rcnt >= sizeof(CtrDrvrCtimBinding)) {
	    ctim = (CtrDrvrCtimBinding *) arg;
	    for (i=0; i<Wa->Ctim.Size; i++) {
	       if (ctim->EqpNum == Wa->Ctim.Objects[i].EqpNum) {
		  for (j=0; j<CtrDrvrRamTableSIZE; j++) {
		     strg = &(mcon->Trigs[j]);
		     if (strg->Ctim == ctim->EqpNum) {
			pseterr(EBUSY);   /* In use by ptim object */
			return SYSERR;
		     }
		  }
		  Wa->Ctim.Objects[i].Frame.Long = ctim->Frame.Long;
		  return OK;
	       }
	    }
	 }
      break;

      case CtrDrvrLIST_CTIM_OBJECTS:      /* Returns a list of created CTIM objects */
	 if (wcnt >= sizeof(CtrDrvrCtimObjects)) {
	    ctimo = (CtrDrvrCtimObjects *) arg;
	    bcopy((void *) &(Wa->Ctim), (void *) ctimo, sizeof(CtrDrvrCtimObjects));
	    return OK;
	 }
      break;

      case CtrDrvrCREATE_PTIM_OBJECT:     /* Create a new PTIM timing object */
	 if (rcnt >= sizeof(CtrDrvrPtimBinding)) {
	    ptim = (CtrDrvrPtimBinding *) arg;
	    for (i=0; i<Wa->Ptim.Size; i++) {
	       if (ptim->EqpNum == Wa->Ptim.Objects[i].EqpNum) {
		  pseterr(EBUSY);        /* Already defined */
		  return SYSERR;
	       }
	    }
	    if (Wa->Ptim.Size >= CtrDrvrPtimOBJECTS) {
	       pseterr(ENOMEM);          /* No more place in memory */
	       return SYSERR;
	    }

	    if (ptim->ModuleIndex < Wa->Modules) {
	       mcon = &(wa->ModuleContexts[ptim->ModuleIndex]);

	       /* Look for available space in trigger table for specified module */

	       found = size = start = 0;
	       for (i=0; i<CtrDrvrRamTableSIZE; i++) {
		  if (mcon->EqpNum[i] == 0) {
		     if (size == 0) start = i;
		     if (++size >= ptim->Size) {
			found = 1;
			break;
		     }
		  } else size = 0;
	       }

	       if (found) {
		  for (i=start; i<start+ptim->Size; i++) {
		     mcon->EqpNum[i]   = ptim->EqpNum;
		     mcon->EqpClass[i] = CtrDrvrConnectionClassPTIM;
		     mcon->Trigs[i].Counter = ptim->Counter;
		  }

		  ptim->StartIndex = start;
		  Wa->Ptim.Objects[Wa->Ptim.Size++] = *ptim;
		  return OK;
	       }
	    }
	 }
      break;

      case CtrDrvrGET_PTIM_BINDING:     /* Search for a PTIM object binding */
	 if (rcnt >= sizeof(CtrDrvrPtimBinding)) {
	    ptim = (CtrDrvrPtimBinding *) arg;
	    for (i=0; i<Wa->Ptim.Size; i++) {
	       if (ptim->EqpNum == Wa->Ptim.Objects[i].EqpNum) {
		  *ptim = Wa->Ptim.Objects[i];
		  return OK;
	       }
	    }
	    bzero((void *) ptim, sizeof(CtrDrvrPtimBinding));
	 }
      break;

      case CtrDrvrDESTROY_PTIM_OBJECT:    /* Destroy a PTIM timing object */
	 if (rcnt >= sizeof(CtrDrvrPtimBinding)) {
	    ptim = (CtrDrvrPtimBinding *) arg;

	    i = ptim->ModuleIndex;
	    if ((i < 0) || (i >= wa->Modules)) break;
	    mcon = &(wa->ModuleContexts[ptim->ModuleIndex]);
	    mmap = (CtrDrvrMemoryMap *) mcon->Map;

	    found = 0;
	    for (i=0; i<CtrDrvrRamTableSIZE; i++) {
	       if ((mcon->EqpNum[i]   == ptim->EqpNum)
	       &&  (mcon->EqpClass[i] == CtrDrvrConnectionClassPTIM)) {
		  if (mcon->Clients[i]) {
		     pseterr(EBUSY);      /* Object is in use by connected client */
		     return SYSERR;
		  }
		  found = 1;
	       }
	    }

	    if (found) {
	       for (i=0; i<CtrDrvrRamTableSIZE; i++) {
		  if ((mcon->EqpNum[i]   == ptim->EqpNum)
		  &&  (mcon->EqpClass[i] == CtrDrvrConnectionClassPTIM)) {

		     htrg = &((CtrDrvrHwTrigger) {{0},0});
		     Io32Write((unsigned int *) &(mmap->Trigs[i]),
			       (unsigned int *) htrg,
			       (unsigned int  ) sizeof(CtrDrvrHwTrigger));

		     mcon->EqpNum[i]   = 0;
		     mcon->EqpClass[i] = 0;
		     bzero((void *) &(mcon->Configs[i]),sizeof(CtrDrvrCounterConfiguration));
		     bzero((void *) &(mcon->Trigs[i]  ),sizeof(CtrDrvrTrigger));
		     Io32Write((unsigned int *) &(mmap->Configs[i]),
			       (unsigned int *) ConfigToHard(&(mcon->Configs[i])),
			       (unsigned int  ) sizeof(CtrDrvrHwCounterConfiguration));
		  }
	       }

	       for (i=0; i<Wa->Ptim.Size; i++) {
		  if (ptim->EqpNum == Wa->Ptim.Objects[i].EqpNum) {
		     Wa->Ptim.Objects[i] = Wa->Ptim.Objects[Wa->Ptim.Size -1];
		     Wa->Ptim.Size--;
		     break;
		  }
	       }
	       return OK;
	    }
	 }
      break;

      case CtrDrvrLIST_PTIM_OBJECTS:      /* Returns a list of created PTIM objects */
	 if (wcnt >= sizeof(CtrDrvrPtimObjects)) {
	    ptimo = (CtrDrvrPtimObjects *) arg;
	    bcopy((void *) &(Wa->Ptim), (void *) ptimo, sizeof(CtrDrvrPtimObjects));
	    return OK;
	 }
      break;

      case CtrDrvrGET_OUTPUT_BYTE:        /* VME P2 output byte number */
	 if (lap) {
	    lav = HRd(&mmap->OutputByte) & 0xFF;
	    *lap = lav;
	    return OK;
	 }
      break;

      case CtrDrvrSET_OUTPUT_BYTE:        /* VME P2 output byte number */
	 if (lap) {
	    if ((lav >= 0) && (lav <= 8)) { /* 64 Bits on the P2 connector */

	       mcon->OutputByte = lav;

	       /* It would be crazey to have a module output to more */
	       /* than one byte on the P2 at a time, so I restrict it here */

	       if (lav) lav = 1 << (lav - 1); /* Set one bit only */
	       HWr(lav | 0x100,&mmap->OutputByte);

	       return OK;
	    }
	 }
      break;

      case CtrDrvrENABLE:                               /* Enable CTR module event reception */
	 if        (lav   == CtrDrvrCommandSET_HPTDC) { /* Enable HPTDC chip */
	    mcon->Command |=  CtrDrvrCommandSET_HPTDC;
	    HWr(mcon->Command,&mmap->Command);
	 } else if (lav   == ~CtrDrvrCommandSET_HPTDC) { /* Disable HPTDC chip */
	    mcon->Command &= ~CtrDrvrCommandSET_HPTDC;
	    HWr(mcon->Command,&mmap->Command);
	 } else if (lav   != 0) {
	    mcon->Command |=  CtrDrvrCommandENABLE;
	    mcon->Command &= ~CtrDrvrCommandDISABLE;
	    HWr(mcon->Command,&mmap->Command);
	 } else {
	    mcon->Command &= ~CtrDrvrCommandENABLE;;
	    mcon->Command |=  CtrDrvrCommandDISABLE;
	    HWr(mcon->Command,&mmap->Command);
	 }
	 return OK;

      case CtrDrvrSET_DEBUG_HISTORY:
	 if (lav) {
	    mcon->Command |=  CtrDrvrCommandDebugHisOn;
	    mcon->Command &= ~CtrDrvrCommandDebugHisOff;
	 } else {
	    mcon->Command &= ~CtrDrvrCommandDebugHisOn;
	    mcon->Command |=  CtrDrvrCommandDebugHisOff;
	 }
	 HWr(mcon->Command,&mmap->Command);
	 return OK;

      case CtrDrvrSET_BRUTAL_PLL:
	 if (lav) {
	    mcon->Command |=  CtrDrvrCommandUtcPllOff;
	    mcon->Command &= ~CtrDrvrCommandUtcPllOn;
	 } else {
	    mcon->Command &= ~CtrDrvrCommandUtcPllOff;
	    mcon->Command |=  CtrDrvrCommandUtcPllOn;
	 }
	 HWr(mcon->Command,&mmap->Command);
	 return OK;

      case CtrDrvrGET_INPUT_DELAY:        /* Get input delay in 25ns ticks */
	 if (lap) {
	    *lap = HRd(&mmap->InputDelay);
	    return OK;
	 }
      break;

      case CtrDrvrSET_INPUT_DELAY:        /* Set input delay in 25ns ticks */
	 HWr(lav,&mmap->InputDelay);
	 mcon->InputDelay = lav;
	 return OK;

      case CtrDrvrGET_REMOTE:             /* Counter Remote/Local status */
	 if (wcnt >= sizeof(CtrdrvrRemoteCommandBuf)) {
	    remc = (CtrdrvrRemoteCommandBuf *) arg;
	    if ((remc->Counter >= CtrDrvrCounter1) && (remc->Counter <= CtrDrvrCounter8)) {
	       remc->Remote = HRd(&mmap->Counters[remc->Counter].Control.LockConfig);
	       return OK;
	    }
	 }
      break;

      case CtrDrvrSET_REMOTE:             /* Counter Remote/Local status */
	 if (rcnt >= sizeof(CtrdrvrRemoteCommandBuf)) {
	    remc = (CtrdrvrRemoteCommandBuf *) arg;
	    if ((remc->Counter >= CtrDrvrCounter1) && (remc->Counter <= CtrDrvrCounter8)) {
	       if (remc->Remote) HWr(1,&mmap->Counters[remc->Counter].Control.LockConfig);
	       else              HWr(0,&mmap->Counters[remc->Counter].Control.LockConfig);
	       return OK;
	    }
	 }
      break;

      case CtrDrvrREMOTE:                 /* Remote control counter */
	 if (rcnt >= sizeof(CtrdrvrRemoteCommandBuf)) {
	    remc = (CtrdrvrRemoteCommandBuf *) arg;
	    if ((remc->Counter >= CtrDrvrCounter1) && (remc->Counter <= CtrDrvrCounter8)) {
	       if (HRd(&mmap->Counters[remc->Counter].Control.LockConfig)) {
		  HWr(CtrDrvrRemoteEXEC | (CtrDrvrCntrCntrlREMOTE_MASK & remc->Remote),
		      &mmap->Counters[remc->Counter].Control.RemOutMask);
		  return OK;
	       }
	    }
	 }
      break;

      case CtrDrvrGET_MODULE_STATS:
	 if (wcnt >= sizeof(CtrDrvrModuleStats)) {
	    mods = (CtrDrvrModuleStats *) arg;
	    Io32Read((unsigned int *) mods,
		     (unsigned int *) &(mmap->ModStats),
		     (unsigned int  ) sizeof(CtrDrvrModuleStats));
	    return OK;
	 }
      break;

      case CtrDrvrGET_OUT_MASK:           /* Counter output routing mask */
	 if (wcnt >= sizeof(CtrDrvrCounterMaskBuf)) {
	    cmsb = (CtrDrvrCounterMaskBuf *) arg;
	    if ((cmsb->Counter >= CtrDrvrCounter1) && (cmsb->Counter <= CtrDrvrCounter8)) {
	       i = cmsb->Counter;
	       msk = HRd(&mmap->Counters[i].Control.RemOutMask);
	       if (CtrDrvrCntrCntrlTTL_BAR & msk) cmsb->Polarity = CtrDrvrPolarityTTL_BAR;
	       else                               cmsb->Polarity = CtrDrvrPolarityTTL;
	       msk &= ~CtrDrvrCntrCntrlTTL_BAR;
	       cmsb->Mask = AutoShiftRight(CtrDrvrCntrCntrlOUT_MASK,msk);
	       return OK;
	    }
	 }
      break;

      case CtrDrvrSET_OUT_MASK:           /* Counter output routing mask */
	 if (rcnt >= sizeof(CtrDrvrCounterMaskBuf)) {
	    cmsb = (CtrDrvrCounterMaskBuf *) arg;
	    if ((cmsb->Counter >= CtrDrvrCounter1) && (cmsb->Counter <= CtrDrvrCounter8)) {
	       i = cmsb->Counter;
	       if (cmsb->Mask == 0) cmsb->Mask = 1 << cmsb->Counter;
	       if (cmsb->Polarity == CtrDrvrPolarityTTL_BAR) msk = CtrDrvrCntrCntrlTTL_BAR;
	       else                                          msk = 0;
	       HWr(AutoShiftLeft(CtrDrvrCntrCntrlOUT_MASK,cmsb->Mask) | msk,
		   &mmap->Counters[i].Control.RemOutMask);
	       return OK;
	    }
	 }
      break;

      case CtrDrvrGET_CONFIG:             /* Get a counter configuration */
	 if (wcnt >= sizeof(CtrDrvrCounterConfigurationBuf)) {
	    conf = (CtrDrvrCounterConfigurationBuf *) arg;
	    if ((conf->Counter >= CtrDrvrCounter0) && (conf->Counter <= CtrDrvrCounter8)) {
	       Int32Copy((unsigned int *) &(conf->Config),
			 (unsigned int *) HardToConfig(&mmap->Counters[conf->Counter].Config),
			 (unsigned int  ) sizeof(CtrDrvrCounterConfiguration));
	       if ((conf->Config.OnZero > (CtrDrvrCounterOnZeroBUS | CtrDrvrCounterOnZeroOUT))
	       ||  (conf->Config.Start  > CtrDrvrCounterSTARTS)
	       ||  (conf->Config.Mode   > CtrDrvrCounterMODES)
	       ||  (conf->Config.Clock  > CtrDrvrCounterCLOCKS)) {
		  pseterr(EFAULT);
		  return SYSERR;
	       }
	       return OK;
	    }
	 }
      break;

      case CtrDrvrSET_CONFIG:             /* Set a counter configuration */
	 if (rcnt >= sizeof(CtrDrvrCounterConfigurationBuf)) {
	    conf = (CtrDrvrCounterConfigurationBuf *) arg;
	    if (mmap->Counters[lav].Control.LockConfig) {
	       if ((conf->Counter >= CtrDrvrCounter0) && (conf->Counter <= CtrDrvrCounter8)) {
		  Io32Write((unsigned int *) &(mmap->Counters[conf->Counter].Config),
			    (unsigned int *) ConfigToHard(&conf->Config),
			    (unsigned int  ) sizeof(CtrDrvrHwCounterConfiguration));
		  return OK;
	       }
	    }
	 }
      break;

      case CtrDrvrGET_COUNTER_HISTORY:    /* One deep history of counter */
	 if (wcnt >= sizeof(CtrDrvrCounterHistoryBuf)) {
	    hisb = (CtrDrvrCounterHistoryBuf *) arg;
	    if ((hisb->Counter >= CtrDrvrCounter1) && (hisb->Counter <= CtrDrvrCounter8)) {
	       Io32Read((unsigned int *) &(hisb->History),
			(unsigned int *) &(mmap->Counters[hisb->Counter].History),
			(unsigned int  ) sizeof(CtrDrvrCounterHistory));
	       return OK;
	    }
	 }
      break;

      case CtrDrvrGET_PLL:                /* Get phase locked loop parameters */
	 if (wcnt >= sizeof(CtrDrvrPll)) {
	    pll = (CtrDrvrPll *) arg;
	    Io32Read((unsigned int *) pll,
		     (unsigned int *) &(mmap->Pll),
		     (unsigned int  ) sizeof(CtrDrvrPll));
	    return OK;
	 }
      break;

      case CtrDrvrSET_PLL:                /* Set phase locked loop parameters */
	 if (rcnt >= sizeof(CtrDrvrPll)) {
	    pll = (CtrDrvrPll *) arg;
	    Io32Write((unsigned int *) &(mmap->Pll),
		      (unsigned int *) pll,
		      (unsigned int  ) sizeof(CtrDrvrPll));
	    Int32Copy((unsigned int *) &(mcon->Pll),
		      (unsigned int *) pll,
		      (unsigned int  ) sizeof(CtrDrvrPll));
	    return OK;
	 }
      break;

      case CtrDrvrGET_PLL_ASYNC_PERIOD:   /* Get PLL asynchronous period */
	 if (wcnt >= sizeof(CtrDrvrPllAsyncPeriodNs)) {
	    asyp = (CtrDrvrPllAsyncPeriodNs *) arg;
	    if (mcon->PllAsyncPeriodNs != 0.0)
	       *asyp = mcon->PllAsyncPeriodNs;
	    else
	       *asyp = mcon->PllAsyncPeriodNs = 1000.00/44.736;
	    return OK;
	 }
      break;

      case CtrDrvrSET_PLL_ASYNC_PERIOD:   /* Set PLL asynchronous period */
	 if (rcnt >= sizeof(CtrDrvrPllAsyncPeriodNs)) {
	    asyp = (CtrDrvrPllAsyncPeriodNs *) arg;
	    *asyp = mcon->PllAsyncPeriodNs;
	    return OK;
	 }
      break;

      case CtrDrvrREAD_TELEGRAM:          /* Read telegrams from CTR */
	 if (wcnt >= sizeof(CtrDrvrTgmBuf)) {
	    tgmb = (CtrDrvrTgmBuf *) arg;
	    if ((tgmb->Machine > CtrDrvrMachineNONE) && (tgmb->Machine <= CtrDrvrMachineMACHINES)) {
	       Io32Read((unsigned int *) tgmb->Telegram,
			 (unsigned int *) mmap->Telegrams[(int) (tgmb->Machine) -1],
			 (unsigned int  ) sizeof(CtrDrvrTgm));
	       SwapWords((unsigned int *) tgmb->Telegram, (unsigned int  ) sizeof(CtrDrvrTgm));
	       return OK;
	    }
	 }
      break;

      case CtrDrvrREAD_EVENT_HISTORY:     /* Read incomming event history */
	 if (wcnt >= sizeof(CtrDrvrEventHistory)) {
	    evhs = (CtrDrvrEventHistory *) arg;
	    Io32Read((unsigned int *) evhs,
		     (unsigned int *) &(mmap->EventHistory),
		     (unsigned int  ) sizeof(CtrDrvrEventHistory));
	    return OK;
	 }
      break;


      case CtrDrvrGET_RECEPTION_ERRORS:   /* Timing frame reception error status */
	 if (wcnt >= sizeof(CtrDrvrReceptionErrors)) {
	    rcpe = (CtrDrvrReceptionErrors *) arg;
	    rcpe->LastReset    = HRd(&mmap->LastReset);
	    rcpe->PartityErrs  = HRd(&mmap->PartityErrs);
	    rcpe->SyncErrs     = HRd(&mmap->SyncErrs);
	    rcpe->TotalErrs    = HRd(&mmap->TotalErrs);
	    rcpe->CodeViolErrs = HRd(&mmap->CodeViolErrs);
	    rcpe->QueueErrs    = HRd(&mmap->QueueErrs);
	    return OK;
	 }
      break;

      case CtrDrvrGET_IO_STATUS:          /* Status of module inputs */
	 if (lap) {
	    *lap = HRd(&mmap->IoStat);
	    return OK;
	 }
      break;

      case CtrDrvrGET_IDENTITY:           /* Identity of board from ID chip */
	 if (wcnt >= sizeof(CtrDrvrBoardId)) {
	    bird = (CtrDrvrBoardId *) arg;
	    bird->IdLSL = HRd(&mmap->IdLSL);
	    bird->IdMSL = HRd(&mmap->IdMSL);
	    return OK;
	 }
      break;

      default: break;
   }

   /* EINVAL = "Invalid argument" */

   pseterr(EINVAL);                           /* Caller error */
   return SYSERR;
}

/*************************************************************/
/* Dynamic loading information for driver install routine.   */
/*************************************************************/

struct dldd entry_points = {
   CtrDrvrOpen, CtrDrvrClose,
   CtrDrvrRead, CtrDrvrWrite,
   CtrDrvrSelect,
   CtrDrvrIoctl,
   CtrDrvrInstall, CtrDrvrUninstall
};

