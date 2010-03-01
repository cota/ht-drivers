/* ********************************************************** */
/*                                                            */
/* CTR (Controls Timing Receiver) Driver code.                */
/*                                                            */
/* Julian Lewis December 2003                                 */
/* Linux PCI only. version July 2009 Julian Major revision    */
/*                                                            */
/* ********************************************************** */

#include <EmulateLynxOs.h>
#include <DrvrSpec.h>
#include <plx9030.h>   /* PLX9030 Registers and definition   */

#include <linux/interrupt.h>	/* enable_irq, disable_irq */

/* These next defines are needed just here for the emulation */

#define sel LynxSel
#define enable restore

#include <ctrhard.h>   /* Hardware description               */
#include <ctrdrvr.h>   /* Public driver interface            */
#include <ctrdrvrP.h>  /* Private driver structures          */

#include <ports.h>     /* XILINX Jtag stuff                  */
#include <hptdc.h>     /* High prescision time to digital convertor */

/*========================================================================*/
/* Define prototypes and forward references for local driver routines.    */
/*========================================================================*/

static void Int32Copy(volatile unsigned int *dst, volatile unsigned int *src, unsigned int size);

static void CancelTimeout(int *t);

static int ReadTimeout(CtrDrvrClientContext *ccon);

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

static unsigned int GetStatus(CtrDrvrModuleContext *mcon);

static unsigned int HptdcBitTransfer(volatile unsigned int  *jtg,
					      unsigned int   tdi,
					      unsigned int   tms);

static void HptdcStateReset(volatile unsigned int *jtg);

static int HptdcCommand(CtrDrvrModuleContext  *mcon,
			 HptdcCmd              cmd,
			 HptdcRegisterVlaue   *wreg,
			 HptdcRegisterVlaue   *rreg,
			 int                   size,
			 int                   pflg,
			 int                   rflg);

static CtrDrvrCTime *GetTime(CtrDrvrModuleContext *mcon);

static int SetTime(CtrDrvrModuleContext *mcon, unsigned int tod);

static int RawIo(CtrDrvrModuleContext *mcon,
		 CtrDrvrRawIoBlock    *riob,
		 unsigned int          flag,
		 unsigned int          debg);

static CtrDrvrHwTrigger *TriggerToHard(CtrDrvrTrigger *strg);

static CtrDrvrTrigger *HardToTrigger(volatile CtrDrvrHwTrigger *htrg);

static CtrDrvrHwCounterConfiguration *ConfigToHard(CtrDrvrCounterConfiguration *scnf);

static CtrDrvrCounterConfiguration *HardToConfig(volatile CtrDrvrHwCounterConfiguration *hcnf);

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

/* ==================================================== */

static void SetEndian();

static void ToLittleEndian(char *from, char *to, int size);

static void SwapWords(unsigned int  *dst, unsigned int  size);

static int DrmLocalReadWrite(CtrDrvrModuleContext *mcon,
			     unsigned int   addr,
			     unsigned int  *value,
			     unsigned int   size,
			     unsigned int   flag);

static int DrmConfigReadWrite(CtrDrvrModuleContext *mcon,
			      unsigned int   addr,
			      unsigned int  *value,
			      unsigned int   size,
			      unsigned int   flag);

static int Remap(CtrDrvrModuleContext *mcon);

static void SendEpromCommand(CtrDrvrModuleContext *mcon,
			     EpCmd cmd, unsigned int  adr);

static int WriteEpromWord(CtrDrvrModuleContext *mcon, unsigned int  word);

static void ReadEpromWord(CtrDrvrModuleContext *mcon, unsigned int  *word);

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

/* Flash the i/o pipe line */

#define EIEIO
#define SYNC

extern void disable_intr();
extern void enable_intr();

CtrDrvrWorkingArea *Wa       = NULL;

/*========================================================================*/
/* 32 Bit int access for CTR structure to/from hardware copy              */
/*========================================================================*/

static void Int32Copy(volatile unsigned int *dst,
		      volatile unsigned int *src,
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

"SET_MODULE_BY_SLOT", "GET_MODULE_SLOT",
"REMAP", "93LC56B_EEPROM_OPEN", "93LC56B_EEPROM_READ", "93LC56B_EEPROM_WRITE",
"93LC56B_EEPROM_ERASE", "93LC56B_EEPROM_CLOSE", "PLX9030_RECONFIGURE", "PLX9030_CONFIG_OPEN",
"PLX9030_CONFIG_READ", "PLX9030_CONFIG_WRITE", "PLX9030_CONFIG_CLOSE", "PLX9030_LOCAL_OPEN",
"PLX9030_LOCAL_READ", "PLX9030_LOCAL_WRITE", "PLX9030_LOCAL_CLOSE",

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

/* ========================================================== */
/* This driver works on platforms that have different endians */
/* so this routine finds out which one we are.                */
/* ========================================================== */

static void SetEndian() {

static unsigned int str[2] = { 0x41424344, 0x0 };

   if (strcmp ((char *) &str[0], "ABCD") == 0)
      Wa->Endian = CtrDrvrEndianBIG;
   else
      Wa->Endian = CtrDrvrEndianLITTLE;
}

/* ========================================================== */
/* Arrange bytes if we are on a Big Endian platform           */
/* The Plx9030 local space is always little endian            */
/* ========================================================== */

static void ToLittleEndian(char *from, char *to, int size) {

int i;
char buff[4];    /* Buffer because "to" and "from" can be the same address */

   if (Wa->Endian == CtrDrvrEndianBIG) {
      for (i=0; i<size; i++) buff[size-i-1] = from[i];
      for (i=0; i<size; i++) to[i] = buff[i];
   }
}

/* ========================================================== */
/* Read/Write PLX9030 local config registers.                 */
/* The local config space is always Little Endian.            */
/* This routine always uses Long access, for smaller sizes    */
/* the Long is first read, modified, and written back.        */
/* ========================================================== */

static int DrmLocalReadWrite(CtrDrvrModuleContext *mcon,
			     unsigned int   addr,
			     unsigned int  *value,
			     unsigned int   size,
			     unsigned int   flag) {

int i;
unsigned int  regnum, regval;
unsigned char *cp;
unsigned short *sp;

   if (mcon->Local) {

      cp = (char  *) &regval;                   /* For size 1, byte */
      sp = (short *) &regval;                   /* For size 2, short */
      regnum = addr>>2;                         /* Long address is the regnum */

      regval = (mcon->Local)[regnum];           /* First Read the Long, Little Endian */
      ToLittleEndian((char *) &regval,
		     (char *) &regval,
			      4);               /* Endian convert the Long to local Endian */

      if (flag) {                               /* Write flag set ? */
	 switch (size) {
	    case 1:                             /* Is it a Byte ? */
	       i = 3 - (addr & 3);              /* Address of Byte in regval */
	       cp[i] = (unsigned char) *value;  /* Overwrite the Byte */
	    break;

	    case 2:                             /* Is it a Short ? */
	       i = 1 - ((addr>>1) & 1);         /* Address of Short in regval */
	       sp[i] = (unsigned short) *value; /* Overwrute the short */
	    break;

	    default:                            /* It must be a Long */
	       regval = *value;                 /* Overwrite the Long */
	 }

	 ToLittleEndian((char *) &regval,
			(char *) &regval,
				 4);            /* Endian convert the Long back to Little */
	 (mcon->Local)[regnum] = regval;        /* Write back the Long */

      } else {                                  /* Write flag is down so it's a Read */
	 switch (size) {
	    case 1:                             /* Is it a Byte ? */
	       i = 3 - (addr & 3);              /* Address of Byte in regval */
	       *value = (unsigned int) cp[i];  /* Return Byte value */
	    break;

	    case 2:                             /* Is it a Short ? */
	       i = 1 - ((addr>>1) & 1);         /* Address of Short in regval */
	       *value = (unsigned int) sp[i];  /* Return Short value */
	    break;

	    default:                            /* It must be a Long */
	       *value = regval;
	 }
      }
      return OK;
   }
   return SYSERR;
}

/* ========================================================== */
/* Read/Write PLX9030 config registers.                       */
/* This is needed because of the brain dead definition of the */
/* offset field in drm_read/write as int addresses.           */
/* ========================================================== */

static int DrmConfigReadWrite(CtrDrvrModuleContext *mcon,
			      unsigned int   addr,
			      unsigned int  *value,
			      unsigned int   size,
			      unsigned int   flag) {

int i, cc;
unsigned int regnum, regval;
unsigned char *cp;
unsigned short *sp;

   cp = (char  *) &regval;  /* For size 1, byte */
   sp = (short *) &regval;  /* For size 2, short */
   regnum = addr>>2;        /* The regnum turns out to be the int address */

   /* First read the int register value (Forced because of drm lib) */

   cc = drm_device_read(mcon->Handle,PCI_RESID_REGS,regnum,0,&regval);
   if (cc) {
      cprintf("CtrDrvr: DrmConfigReadWrite: Error: %d From drm_device_read\n",cc);
      return cc;
   }

   /* For read/write access the byte or short or int in regval */

   if (flag) {                                  /* Write */

      /* Modify regval according to address and size */

      switch (size) {
	 case 1:                                /* Byte */
	    i = 3 - (addr & 3);                 /* Byte in regval */
	    cp[i] = (unsigned char) *value;
	 break;

	 case 2:                                /* Short */
	   i = 1 - ((addr>>1) & 1);             /* Shorts in regval */
	   sp[i] = (unsigned short) *value;
	 break;

	 default: regval = *value;              /* Long */
      }

      /* Write back modified configuration register */

      cc = drm_device_write(mcon->Handle,PCI_RESID_REGS,regnum,0,&regval);
      if (cc) {
	 cprintf("CtrDrvr: DrmConfigReadWrite: Error: %d From drm_device_write\n",cc);
	 return cc;
      }

   } else {                                     /* Read */

      /* Extract the data from regval according to address and size */

      switch (size) {
	 case 1:
	    i = 3 - (addr & 3);
	    *value = (unsigned int) cp[i];
	 break;

	 case 2:
	   i = 1 - ((addr>>1) & 1);
	   *value = (unsigned int) sp[i];
	 break;

	 default: *value = regval;
      }
   }
   return OK;
}

/* ========================================================== */
/* Send a command to the 93lc56b EEPROM                       */
/* See plx9030.h                                              */
/* ========================================================== */

static void SendEpromCommand(CtrDrvrModuleContext *mcon,
			     EpCmd cmd, unsigned int adr) {

unsigned int cntrl;
unsigned short cmnd, msk;
int i;

   /* Reset flash memory state machine */

   cntrl = Plx9030CntrlCHIP_UNSELECT;
   DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1); /* Chip select off */
   cntrl |= Plx9030CntrlDATA_CLOCK;
   DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1); /* Strobe Clk up */
   cntrl = Plx9030CntrlCHIP_UNSELECT;
   DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1); /* Strobe Clk down */

   cmnd = ((0x7F & (unsigned short) adr) << 5) | (unsigned short) cmd;
   msk = 0x8000;
   for (i=0; i<EpClkCycles; i++) {
      if (msk & cmnd) cntrl = Plx9030CntrlDATA_OUT_ONE;
      else            cntrl = Plx9030CntrlDATA_OUT_ZERO;
      DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1); /* Write 1 or 0 */
      cntrl |= Plx9030CntrlDATA_CLOCK;
      DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1); /* Clk Up */
      msk = msk >> 1;
   }
}

/* ========================================================== */
/* Write a data word to the 93lc56b EEPROM                    */
/* See plx9030.h                                              */
/* ========================================================== */

static int WriteEpromWord(CtrDrvrModuleContext *mcon, unsigned int word) {

unsigned int cntrl;
unsigned short bits, msk;
int i, plxtmo;

   bits = (unsigned short) word;
   msk = 0x8000;
   for (i=0; i<16; i++) {
      if (msk & bits) cntrl = Plx9030CntrlDATA_OUT_ONE;
      else            cntrl = Plx9030CntrlDATA_OUT_ZERO;
      DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1); /* Write 1 or 0 */
      cntrl |= 0x01000000;
      DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1); /* Strobe Clk */
      msk = msk >> 1;
   }
   cntrl = Plx9030CntrlCHIP_UNSELECT;
   DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1);    /* Chip select off */
   cntrl = Plx9030CntrlCHIP_SELECT;
   DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1);    /* Chip select on */

   plxtmo = 6000;                                       /* At 1us per cycle = 6ms */
   while (((Plx9030CntrlDONE_FLAG & cntrl) == 0) && (plxtmo-- >0)) {
      DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,0); /* Read Busy Status */
   }
   cntrl = Plx9030CntrlCHIP_UNSELECT;
   DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1);    /* Chip select off */

   if (plxtmo <= 0) {
      cprintf("WriteEpromWord: Timout from 93lc56B\n");
      return SYSERR;
   }
   return OK;
}

/* ========================================================== */
/* Read a data word from the 93lc56b EEPROM                   */
/* See plx9030.h                                              */
/* ========================================================== */

static void ReadEpromWord(CtrDrvrModuleContext *mcon, unsigned int *word) {

unsigned int cntrl;
unsigned short bits, msk;
int i;

   bits = 0;
   msk = 0x8000;
   for (i=0; i<16; i++) {
      cntrl = Plx9030CntrlCHIP_SELECT;
      DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1);
      cntrl |= Plx9030CntrlDATA_CLOCK;
      DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1);
      DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,0);
      if (cntrl & Plx9030CntrlDATA_IN_ONE) bits |= msk;
      msk = msk >> 1;
   }
   *word = (unsigned int) bits;
}

/* ========================================================== */
/* Rebuild PCI device tree, uninstall devices and reinstall.  */
/* Needed if device descriptions (ie IDs)  are modified.      */
/* This should be done after the Plx9030 EEPROM has been      */
/* reloaded, or simply do a reboot of the DSC.                */
/* ========================================================== */

static int RemapFlag = 0;

static int Remap(CtrDrvrModuleContext *mcon) {

   drm_locate(mcon->Handle);    /* Rebuild PCI tree */

   RemapFlag = 1;               /* Don't release memory */
   CtrDrvrUninstall(Wa);        /* Uninstall all */
   CtrDrvrInstall(Wa);          /* Re-install all */
   RemapFlag = 0;               /* Normal uninstall */

   return OK;
}

/* ========================================================== */
/* GetVersion, also used to check for Bus errors. See Ping    */
/* ========================================================== */

#ifndef COMPILE_TIME
#define COMPILE_TIME 0
#endif

static int GetVersion(CtrDrvrModuleContext *mcon,
		      CtrDrvrVersion       *ver) {

volatile CtrDrvrMemoryMap *mmap;    /* Module Memory map */
unsigned long             ps;      /* Processor status  */
unsigned int              stat;    /* Module status */

   ps = 0;
   mmap = mcon->Map;

   ver->VhdlVersion  = mmap->VhdlVersion;
   ver->DrvrVersion  = COMPILE_TIME;
   ver->HardwareType = CtrDrvrHardwareTypeNONE;

   EIEIO;
   SYNC;

   stat  = mmap->Status;

   EIEIO;
   SYNC;

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

volatile CtrDrvrMemoryMap *mmap; /* Module Memory map */

unsigned int intcsr;           /* Plx control word  */

   mmap = mcon->Map;

   /* We want level triggered interrupts for Plx INT1  */
   /* for an active high. Level type interrupts do not */
   /* require clearing on the Plx chip it self.        */

   intcsr = Plx9030IntcsrLINT_ENABLE
	  | Plx9030IntcsrLINT_HIGH
	  | Plx9030IntcsrPCI_INT_ENABLE;

   DrmLocalReadWrite(mcon,PLX9030_INTCSR,&intcsr,2,1);
   mcon->InterruptEnable |= msk;
   mmap->InterruptEnable = mcon->InterruptEnable;

   EIEIO;
   SYNC;

/* After a JTAG upgrade interrupt must be re-enabled once */
/* and at least one time on startup. */

   if (mcon->IrqBalance == 0) {
      enable_irq(mcon->IVector);
      mcon->IrqBalance = 1;
   }
   return OK;
}

/* ========================================================== */
/* Disable Interrupt                                          */
/* ========================================================== */

static int DisableInterrupts(unsigned int          tndx,
			     unsigned int          midx,
			     CtrDrvrConnectionClass clss,
			     CtrDrvrClientContext  *ccon) {

CtrDrvrModuleContext       *mcon;
volatile CtrDrvrMemoryMap  *mmap;
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
   mmap->InterruptEnable = mcon->InterruptEnable;

   EIEIO;
   SYNC;

   return OK;
}

/* ========================================================== */
/* Reset a module                                             */
/* ========================================================== */

static unsigned char RecoverMode = 0;

static int Reset(CtrDrvrModuleContext *mcon) {

volatile CtrDrvrMemoryMap *mmap;     /* Module Memory map */
unsigned long              ps;       /* Processor status  */
volatile unsigned int     *jtg;      /* Hptdc JTAG register */
int i = 0;

   ps = 0;
   if (RecoverMode) {
      RecoverMode--;
      return OK;  /* Do nothing in recover mode */
   }
   mmap = mcon->Map;

   i = 0;                 /* Used to say more about bus error */

   mmap->Command = CtrDrvrCommandRESET;
   mcon->Status = CtrDrvrStatusNO_LOST_INTERRUPTS | CtrDrvrStatusNO_BUS_ERROR;
   mcon->BusErrorCount = 0;

   EIEIO;
   SYNC;

   /* Wait at least 10 ms after a "reset", for the module to recover */

   sreset(&(mcon->Semaphore));
   mcon->Timer = timeout(ResetTimeout, (char *) mcon, 2);
   if (mcon->Timer < 0) mcon->Timer = 0;
   if (mcon->Timer) swait(&(mcon->Semaphore), SEM_SIGABORT);
   if (mcon->Timer) CancelTimeout(&(mcon->Timer));

   mcon->Command &= ~CtrDrvrCommandSET_HPTDC; /* Hptdc needs to be re-initialized */
   mmap->Command = mcon->Command;

   for (i=CtrDrvrCounter1; i<=CtrDrvrCounter8; i++) {
      if (mmap->Counters[i].Control.RemOutMask == 0) {
	 mmap->Counters[i].Control.RemOutMask = AutoShiftLeft(CtrDrvrCntrCntrlOUT_MASK,(1 << i));
      }
   }
   i = 0;                /* Not a counter bus error */

   mmap->OutputByte = 0x100;  /* Enable front pannel output */

   if (mcon->Pll.KP == 0) {
      mcon->Pll.KP = 337326;
      mcon->Pll.KI = 901;
      mcon->Pll.NumAverage = 100;
      mcon->Pll.Phase = 1950;
   }
   Int32Copy((unsigned int *) &(mmap->Pll),
	    (unsigned int *) &(mcon->Pll),
	    (unsigned int  ) sizeof(CtrDrvrPll));

   EnableInterrupts(mcon,mcon->InterruptEnable);

   EIEIO;
   SYNC;

   jtg = &mmap->HptdcJtag;
   HptdcStateReset(jtg);           /* GOTO: Run test/idle */

   return OK;
}

/* ========================================================== */
/* Get Status                                                 */
/* ========================================================== */

static unsigned int GetStatus(CtrDrvrModuleContext *mcon) {

volatile CtrDrvrMemoryMap *mmap;    /* Module Memory map */
unsigned int              stat;    /* Status */

   if (PingModule(mcon) == OK) {
      mmap  = mcon->Map;
      stat  = mmap->Status & CtrDrvrHwStatusMASK;
      if (mcon->BusErrorCount >  0) --mcon->BusErrorCount;
      if (mcon->BusErrorCount == 0) mcon->Status |= CtrDrvrStatusNO_BUS_ERROR;
      stat  = stat | (mcon->Status & ~CtrDrvrHwStatusMASK);
      return stat;
   }
   return SYSERR;
}

/* ========================================================== */
/* HPTDC JTAG Bit transfer                                    */
/* ========================================================== */

static unsigned int HptdcBitTransfer(volatile unsigned int *jtg,
					       unsigned int  tdi,
					       unsigned int  tms) {
unsigned int jwd, tdo;

   jwd = HptdcJTAG_TRST | tdi | tms;
   *jtg = jwd;
   EIEIO;
   jwd = HptdcJTAG_TRST | HptdcJTAG_TCK | tdi | tms;
   *jtg = jwd;
   EIEIO;
   tdo = *jtg;
   EIEIO;
   jwd = HptdcJTAG_TRST | tdi | tms;
   *jtg = jwd;
   EIEIO;
   if (tdo & HptdcJTAG_TDO) return 1;
   else                     return 0;
}

/* ========================================================== */
/* HPTDC JTAG State machine reset                             */
/* ========================================================== */

static void HptdcStateReset(volatile unsigned int *jtg) {
unsigned int jwd;

   jwd = HptdcJTAG_TRST; /* TRST is active on zero */
   *jtg = jwd;
   EIEIO;
   jwd = 0;              /* TCK and TRST are down */
   *jtg = jwd;
   EIEIO;
   jwd = HptdcJTAG_TCK;  /* Clock in reset on rising edge */
   *jtg = jwd;           /* GOTO: Test logic reset */
   EIEIO;
   jwd = HptdcJTAG_TRST | HptdcJTAG_TCK;
   *jtg = jwd;
   EIEIO;
   jwd = HptdcJTAG_TRST; /* Remove TRST */
   *jtg = jwd;
   EIEIO;
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

volatile CtrDrvrMemoryMap *mmap;    /* Module Memory map */
volatile unsigned int    *jtg;

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

volatile CtrDrvrMemoryMap *mmap;    /* Module Memory map */

   mmap = mcon->Map;
   mmap->Command = CtrDrvrCommandLATCH_UTC;

   EIEIO;
   SYNC;

   t.CTrain          = mmap->ReadTime.CTrain;
   t.Time.Second     = mmap->ReadTime.Time.Second;
   t.Time.TicksHPTDC = mmap->ReadTime.Time.TicksHPTDC;
   return &t;
}

/* ========================================================== */
/* Set the time on the CTR                                    */
/* ========================================================== */

static int SetTime(CtrDrvrModuleContext *mcon, unsigned int tod) {

volatile CtrDrvrMemoryMap *mmap;    /* Module Memory map */

   if (PingModule(mcon) == OK) {
      mmap = mcon->Map;
      mmap->SetTime = tod;
      mmap->Command = CtrDrvrCommandSET_UTC;

      EIEIO;
      SYNC;

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
		 unsigned int         flag,
		 unsigned int         debg) {

volatile unsigned int    *mmap; /* Module Memory map */
int                       rval; /* Return value */
unsigned long             ps;   /* Processor status word */
int                       i, j;
unsigned int             *uary;
char                     *iod;

   ps = 0;
   mmap = (unsigned int *) mcon->Map;
   uary = riob->UserArray;
   rval = OK;

   if (flag)  iod = "Write"; else iod = "Read";

   i = j = 0;

   for (i=0; i<riob->Size; i++) {
      j = riob->Offset+i;
      if (flag) {
	 mmap[j] = uary[i];
	 if (debg >= 2) cprintf("RawIo: %s: %d:0x%x\n",iod,j,(int) uary[i]);
      } else {
	 uary[i] = mmap[j];
	 if (debg >= 2) cprintf("RawIo: %s: %d:0x%x\n",iod,j,(int) uary[i]);
      }
      EIEIO;
      SYNC;
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

static CtrDrvrTrigger *HardToTrigger(volatile CtrDrvrHwTrigger *htrg) {
static CtrDrvrTrigger strg;
unsigned int trigger;

   strg.Frame = htrg->Frame;
   trigger    = htrg->Trigger;

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

static CtrDrvrCounterConfiguration *HardToConfig(volatile CtrDrvrHwCounterConfiguration *hcnf) {
static CtrDrvrCounterConfiguration scnf;
unsigned int config;

   config     = hcnf->Config;
   scnf.Delay = hcnf->Delay;

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
volatile CtrDrvrMemoryMap  *mmap;
CtrDrvrTrigger              trig;
CtrDrvrCounterConfiguration conf;
CtrDrvrEventFrame           frme;

int i, midx, cmsk, hmsk, count;

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
	       mmap->Configs[i].Config |= AutoShiftLeft(CtrDrvrCounterConfigON_ZERO_MASK,
						     CtrDrvrCounterOnZeroBUS);
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
	    mmap->Configs[i].Config |= AutoShiftLeft(CtrDrvrCounterConfigON_ZERO_MASK,
						     CtrDrvrCounterOnZeroBUS);
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

		  Int32Copy((unsigned int *) &(mmap->Trigs[i]),
			   (unsigned int *) TriggerToHard(&trig),
			   (unsigned int  ) sizeof(CtrDrvrHwTrigger));

		  Int32Copy((unsigned int *) &(mmap->Configs[i]),
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

static int DisConnectOne(unsigned int          tndx,
			 unsigned int          midx,
			 CtrDrvrConnectionClass clss,
			 CtrDrvrClientContext  *ccon) {

CtrDrvrModuleContext       *mcon;
volatile CtrDrvrMemoryMap  *mmap;
CtrDrvrTrigger              trig;
CtrDrvrCounterConfiguration conf;
unsigned int ncmsk, cmsk;

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

	       Int32Copy((unsigned int *) &(mmap->Trigs[tndx]),
			(unsigned int *) TriggerToHard(&trig),
			(unsigned int  ) sizeof(CtrDrvrHwTrigger));

	       Int32Copy((unsigned int *) &(mmap->Configs[tndx]),
			(unsigned int *) ConfigToHard(&conf),
			(unsigned int  ) sizeof(CtrDrvrHwCounterConfiguration));

	       mcon->EqpNum[tndx]   = 0;
	       mcon->EqpClass[tndx] = 0;

	    } else {

	       mmap->Configs[tndx].Config &= ~(AutoShiftLeft(CtrDrvrCounterConfigON_ZERO_MASK,
							     CtrDrvrCounterOnZeroBUS));
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

unsigned int  isrc, clients, msk, i, hlock, tndx = 0;
unsigned char inum;
unsigned long ps;

CtrDrvrMemoryMap      *mmap = NULL;
CtrDrvrFpgaCounter    *fpgc = NULL;
CtrDrvrCounterHistory *hist = NULL;

CtrDrvrQueue          *queue;
CtrDrvrClientContext  *ccon;
CtrDrvrReadBuf         rbf;

   mmap = mcon->Map;
   isrc = mmap->InterruptSource;                 /* Read and clear interrupt sources */
   if (isrc == 0) return IRQ_NONE;               /* Pass it on to the next ISR */

   /* For each interrupt source/counter */

   for (inum=0; inum<CtrDrvrInterruptSOURCES; inum++) {
      msk = 1 << inum;                           /* Get counter source mask bit */
      if (msk & isrc) {
	 bzero((void *) &rbf, sizeof(CtrDrvrReadBuf));
	 clients = 0;                            /* No clients yet */

	 if (inum < CtrDrvrCOUNTERS) {           /* Counter Interrupt ? */
	    fpgc = &(mmap->Counters[inum]);      /* Get counter FPGA configuration */
	    hist = &(fpgc->History);             /* History of counter */

	    if (fpgc->Control.LockConfig == 0) {    /* If counter not in remote */
	       tndx = hist->Index;                  /* Index into trigger table */
	       if (tndx < CtrDrvrRamTableSIZE) {

		  clients = mcon->Clients[tndx];    /* Clients connected to timing objects */
		  if (clients) {
		     rbf.TriggerNumber = tndx +1;            /* Trigger that loaded counter */
		     rbf.Frame         = hist->Frame;        /* Actual event that interrupted */
		     rbf.TriggerTime   = hist->TriggerTime;  /* Time counter was loaded */
		     rbf.StartTime     = hist->StartTime;    /* Time start arrived */
		     rbf.OnZeroTime    = hist->OnZeroTime;   /* Time of interrupt */

		     hlock = fpgc->Control.LockHistory;      /* Unlock history count Read/Clear */
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
	       if (inum < CtrDrvrCOUNTERS) rbf.OnZeroTime = hist->OnZeroTime;
	       else                        rbf.OnZeroTime = *GetTime(mcon);
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

	       disable(ps);
	       queue = &(ccon->Queue);
	       queue->Entries[queue->WrPntr] = rbf;
	       queue->WrPntr = (queue->WrPntr + 1) % CtrDrvrQUEUE_SIZE;
	       if (queue->Size < CtrDrvrQUEUE_SIZE) {
		  queue->Size++;
		  ssignal(&(ccon->Semaphore));
	       } else {
		  queue->Missed++;
		  queue->RdPntr = (queue->RdPntr + 1) % CtrDrvrQUEUE_SIZE;
	       }
	       restore(ps);
	    }
	 }

	 if ((clients == 0) && (debug_isr)) {
	    kkprintf("CtrDrvr: Spurious interrupt: Module:%d Source:0x%X Number:%d Tindex:%d\n",
		    (int) mcon->ModuleIndex +1,
		    (int) isrc, (int) inum, tndx);
	 }
      }
   }
   return IRQ_HANDLED;
}

/*========================================================================*/
/* OPEN                                                                   */
/*========================================================================*/

int CtrDrvrOpen(wa, dnm, flp)
CtrDrvrWorkingArea * wa;        /* Working area */
int dnm;                        /* Device number */
struct LynxFile * flp; {            /* File pointer */

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
struct LynxFile * flp; {    /* File pointer */

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
struct LynxFile * flp;         /* File pointer */
char * u_buf;                  /* Users buffer */
int cnt; {                     /* Byte count in buffer */

CtrDrvrClientContext *ccon;    /* Client context */
CtrDrvrQueue         *queue;
CtrDrvrReadBuf       *rb;
int                   cnum;    /* Client number */
unsigned long         ps;

   ps = 0;

   cnum = minor(flp->dev) -1;
   ccon = &(wa->ClientContexts[cnum]);

   queue = &(ccon->Queue);
   if (queue->QueueOff) {
      disable(ps);
      {
	 queue->Size   = 0;
	 queue->Missed = 0;         /* ToDo: What to do with this info ? */
	 queue->WrPntr = 0;
	 queue->RdPntr = 0;
	 sreset(&(ccon->Semaphore));
      }
      restore(ps);
   }

   if (ccon->Timeout) {
      ccon->Timer = timeout(ReadTimeout, (char *) ccon, ccon->Timeout);
      if (ccon->Timer < 0) {

	 ccon->Timer = 0;

	 /* EBUSY = "Device or resource busy" */

	 pseterr(EBUSY);    /* No available timers */
	 return 0;
      }
   }

   if (swait(&(ccon->Semaphore), SEM_SIGABORT)) {

      /* EINTR = "Interrupted system call" */

      pseterr(EINTR);   /* We have been signaled */
      return 0;
   }

   if (ccon->Timeout) {
      if (ccon->Timer) {
	 CancelTimeout(&(ccon->Timer));
      } else {

	 /* ETIME = "Timer expired */

	 pseterr(ETIME);
	 return 0;
      }
   }

   rb = (CtrDrvrReadBuf *) u_buf;
   if (queue->Size) {
      disable(ps);
      {
	 *rb = queue->Entries[queue->RdPntr];
	 queue->RdPntr = (queue->RdPntr + 1) % CtrDrvrQUEUE_SIZE;
	 queue->Size--;
      }
      restore(ps);
      return sizeof(CtrDrvrReadBuf);
   }

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

   ps = 0;
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

	    disable(ps);
	    queue->Entries[queue->WrPntr] = rb;
	    queue->WrPntr = (queue->WrPntr + 1) % CtrDrvrQUEUE_SIZE;
	    if (queue->Size < CtrDrvrQUEUE_SIZE) {
	       queue->Size++;
	       ssignal(&(ccon->Semaphore));
	    } else {
	       queue->Missed++;
	       queue->RdPntr = (queue->RdPntr + 1) % CtrDrvrQUEUE_SIZE;
	    }
	    restore(ps);
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

/*========================================================================*/
/* INSTALL  LynxOS Version 4 with Full DRM support                        */
/* Assumes that all Ctr modules are brothers                              */
/*========================================================================*/

char * CtrDrvrInstall(info)
CtrDrvrInfoTable * info; {      /* Driver info table */

CtrDrvrWorkingArea *wa;
drm_node_handle handle;
CtrDrvrModuleContext *mcon;
unsigned long vadr;
unsigned int las0brd, ivec;
int modix, mid, cc, j;
CtrDrvrMemoryMap *mmap;
int cmd;

   // RecoverMode = (unsigned char) info->RecoverMode;

   /* Allocate the driver working area. */

   if (!RemapFlag) {
      wa = (CtrDrvrWorkingArea *) sysbrk(sizeof(CtrDrvrWorkingArea));
      if (!wa) {
	 cprintf("CtrDrvrInstall: NOT ENOUGH MEMORY(WorkingArea)\n");
	 pseterr(ENOMEM);
	 return (char *) SYSERR;
      }
      bzero((void *) wa,sizeof(CtrDrvrWorkingArea));  /* Clear working area */
      Wa = wa;                                        /* Global working area pointer */
   } else wa = Wa;

   SetEndian(); /* Set Big/Little endian flag in driver working area */

   for (modix=0; modix<CtrDrvrMODULE_CONTEXTS; modix++) {
      cc = drm_get_handle(PCI_BUSLAYER,CERN_VENDOR_ID,CTRP_DEVICE_ID,&handle);
      if (cc) {
	 if (modix == 0)
	    cc = drm_get_handle(PCI_BUSLAYER,PLX9030_VENDOR_ID,PLX9030_DEVICE_ID,&handle);
	 if (cc) {
	    if (modix) break;
	    sysfree((void *) wa,sizeof(CtrDrvrWorkingArea)); Wa = NULL;
	    cprintf("CtrDrvrInstall: No PLX9030 found: No hardware: Fatal\n");
	    pseterr(ENODEV);
	    return (char *) SYSERR;
	 } else {
	    mcon = &(wa->ModuleContexts[modix]);
	    drm_device_read(handle,PCI_RESID_DEVNO,0,0,&mid);
	    mcon->PciSlot     = mid;
	    mcon->ModuleIndex = modix;
	    mcon->Handle      = handle;
	    mcon->InUse       = 1;
	    mcon->DeviceId    = PLX9030_DEVICE_ID;
	 }
      }

      mcon = &(wa->ModuleContexts[modix]);
      if (mcon->InUse == 0) {
	 drm_device_read(handle,PCI_RESID_DEVNO,0,0,&mid);
	 mcon->PciSlot     = mid;
	 mcon->ModuleIndex = modix;
	 mcon->Handle      = handle;
	 mcon->InUse       = 1;
	 mcon->DeviceId    = CTRP_DEVICE_ID;
      }

      /* Ensure using memory mapped I/O */
      drm_device_read(handle,  PCI_RESID_REGS, 1, 0, &cmd);
      cmd |= 2;
      drm_device_write(handle, PCI_RESID_REGS, 1, 0, &cmd);

      cc = drm_map_resource(handle,PCI_RESID_BAR2,&vadr);
      if ((cc) || (!vadr)) {
	 cprintf("CtrDrvrInstall: Can't map memory (BAR2) for timing module: %d\n",modix+1);
	 return((char *) SYSERR);
      }
      mcon->Map = (CtrDrvrMemoryMap *) vadr;
      cprintf("CtrDrvrInstall: BAR2 is mapped to address: 0x%08X\n",(int) vadr);

      vadr = (int) NULL;
      cc = drm_map_resource(handle,PCI_RESID_BAR0,&vadr);
      if ((cc) || (!vadr)) {
	 cprintf("CtrDrvrInstall: Can't map plx9030 (BAR0) local configuration: %d\n",modix+1);
	 return((char *) SYSERR);
      }
      mcon->Local = (unsigned int *) vadr;
      cprintf("CtrDrvrInstall: BAR0 is mapped to address: 0x%08X\n",(int) vadr);


      /* Set up the LAS0BRD local configuration register to do appropriate */
      /* endian mapping, and enable the address space for CTR hardware.    */
      /* See PLX PCI 9030 Data Book Chapter 10-2 & 10-21 for more details. */

      if (Wa->Endian == CtrDrvrEndianBIG)
	 las0brd = Plx9030Las0brdBIG_ENDIAN;    /* Big endian mapping, enable space */
      else
	 las0brd = Plx9030Las0brdLITTLE_ENDIAN; /* Little endian mapping, enable space */

      DrmLocalReadWrite(mcon,PLX9030_LAS0BRD,&las0brd,4,1);

      /* register the ISR */
      cc = drm_register_isr(handle,IntrHandler, (void *)mcon);
      if (cc == SYSERR) {
        cprintf("CtrDrvrInstall: Can't register ISR for timing module: %d\n",
          modix + 1);
        return (char *)SYSERR;
      } else {
        mcon->LinkId = cc;

	ivec = (unsigned int)handle->irq;
        mcon->IVector = 0xFF & ivec;

        cprintf("CtrDrvrInstall: Isr installed OK: LinkId: %d Vector: 0x%2X\n",
          cc, (int)mcon->IVector);
      }

      wa->Modules = modix+1;

      if (mcon->DeviceId == PLX9030_DEVICE_ID)
	 cprintf("CtrDrvrInstall: Installed: PLX9030 ModuleNumber: %d In Slot: %d OK\n",
		 (int) modix+1, (int) mid);
      else {
	 cprintf("CtrDrvrInstall: Installed: CTR ModuleNumber: %d In Slot: %d OK\n",
		 (int) modix+1, (int) mid);

	 mcon->IrqBalance = 1; /* don't enable IRQ twice */
	 if (Reset(mcon) == SYSERR) {   /* Soft reset and initialize module */
	    cprintf("CtrDrvrInstall: Error returned from Reset on module: %d Recovering\n",(int) modix+1);
	    if (Reset(mcon) == OK) cprintf("CtrDrvrInstall: Recovered from error OK\n");
	 } else {

	    /* Wipe out any old triggers left in Ram after a warm reboot */

	    mmap = mcon->Map;
	    for (j=0; j<CtrDrvrRamTableSIZE; j++) {
	       mmap->Trigs[j].Frame.Long = 0;
	       mmap->Trigs[j].Trigger    = 0;
	    }
	 }
      }
   }
   if (!RemapFlag) cprintf("CtrDrvrInstall: %d Modules installed: OK\n",(int) wa->Modules);
   return (char *) wa;
}

/*========================================================================*/
/* Uninstall                                                              */
/*========================================================================*/

int CtrDrvrUninstall(wa)
CtrDrvrWorkingArea * wa; {     /* Drivers working area pointer */

CtrDrvrMemoryMap *mmap;
CtrDrvrClientContext *ccon;
int i;

CtrDrvrModuleContext *mcon;

   for (i=0; i<CtrDrvrMODULE_CONTEXTS; i++) {
      mcon = &(wa->ModuleContexts[i]);
      if (mcon->InUse) {

	 mmap = mcon->Map;
	 if (mmap) mmap->InterruptEnable = 0;

	 drm_unregister_isr(mcon->Handle);
	 drm_free_handle(mcon->Handle);

	 if (mcon->Local) {
	    drm_unmap_resource(mcon->Handle,PCI_RESID_BAR0);
	    mcon->LocalOpen = 0;
	    mcon->Local = NULL;
	 }

	 if (mcon->Map) {
	    drm_unmap_resource(mcon->Handle,PCI_RESID_BAR2);
	    mcon->Map = NULL;
	 }

	 bzero((void *) mcon, sizeof(CtrDrvrModuleContext));
      }
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

   if (RemapFlag) return OK;

   sysfree((void *) wa,sizeof(CtrDrvrWorkingArea)); Wa = NULL;
   cprintf("CtrDrvr: Driver: UnInstalled\n");
   return OK;
}

/*========================================================================*/
/* IOCTL                                                                  */
/*========================================================================*/

int CtrDrvrIoctl(wa, flp, cm, arg)
CtrDrvrWorkingArea * wa;       /* Working area */
struct LynxFile * flp;         /* File pointer */
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

volatile CtrDrvrMemoryMap   *mmap;

int i, j, k, n, msk, pid, hmsk, found, size, start;

int cnum;                 /* Client number */
int lav, *lap;           /* Long Value pointed to by Arg */
unsigned short sav;       /* Short argument and for Jtag IO */
int rcnt, wcnt;           /* Readable, Writable byte counts at arg address */

unsigned int  lval;      /* For general IO stuff */
unsigned int cntrl;      /* PLX9030 serial EEPROM control register */

   /* Check argument contains a valid address for reading or writing. */
   /* We can not allow bus errors to occur inside the driver due to   */
   /* the caller providing a garbage address in "arg". So if arg is   */
   /* not null set "rcnt" and "wcnt" to contain the byte counts which */
   /* can be read or written to without error. */

   if (arg != NULL) {
      rcnt = rbounds((long) arg);       /* Number of readable bytes without error */
      wcnt = wbounds((long) arg);       /* Number of writable bytes without error */
      if (rcnt < sizeof(int)) {      /* We at least need to read one int */
	 pid = getpid();
	 cprintf("CtrDrvrIoctl:Illegal arg-pntr:0x%lX ReadCnt:%d(%d) Pid:%d Cmd:%d\n",
		 (long) arg,rcnt,(int) sizeof(int),pid,(int) cm);
	 pseterr(EINVAL);        /* Invalid argument */
	 return SYSERR;
      }
      lav = *((int *) arg);       /* Long argument value */
      lap =   (int *) arg ;       /* Long argument pointer */
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

#if 0
   pid = getpid();
   if (pid != ccon->Pid) {
      cprintf("CtrDrvrIoctl: Spurious IOCTL call:%d by PID:%d for PID:%d on FD:%d\n",
	      (int) cm,
	      (int) pid,
	      (int) ccon->Pid,
	      (int) cnum);
      pseterr(EBADF);           /* Bad file number */
      return SYSERR;
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
	    return GetVersion(mcon,ver);
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

      case CtrDrvrSET_MODULE_BY_SLOT:     /* Select the module to work with by ID */
	 if (lap) {
	    for (i=0; i<Wa->Modules; i++) {
	       mcon = &(wa->ModuleContexts[i]);
	       if (mcon->PciSlot == lav) {
		  ccon->ModuleIndex = i;
		  return OK;
	       }
	    }
	 }
      break;

      case CtrDrvrGET_MODULE_DESCRIPTOR:
	 if (wcnt >= sizeof(CtrDrvrModuleAddress)) {
	    moad = (CtrDrvrModuleAddress *) arg;

	    if (mcon->DeviceId == CTRP_DEVICE_ID) {
	       moad->ModuleType = CtrDrvrModuleTypeCTR;
	       moad->DeviceId   = CTRP_DEVICE_ID;
	       moad->VendorId   = CERN_VENDOR_ID;
	       moad->MemoryMap  = (unsigned long) mcon->Map;
	       moad->LocalMap   = (unsigned long) mcon->Local;
	    } else {
	       moad->ModuleType = CtrDrvrModuleTypePLX;
	       moad->DeviceId   = PLX9030_DEVICE_ID;
	       moad->VendorId   = PLX9030_VENDOR_ID;
	       moad->MemoryMap  = 0;
	       moad->LocalMap   = (unsigned long) mcon->Local;
	    }
	    moad->ModuleNumber  = mcon->ModuleIndex +1;
	    moad->PciSlot       = mcon->PciSlot;
	    return OK;
	 }
      break;

      case CtrDrvrGET_MODULE_SLOT:        /* Get the ID of the selected module */
	 if (lap) {
	    *lap = mcon->PciSlot;
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
	    else               *lap = mmap->CableId;
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

	 mmap->InterruptEnable = 0; /* Kill module interupts */
	 lval = 0;
	 DrmLocalReadWrite(mcon,PLX9030_INTCSR,&lval,2,1); /* Kill 9030 interrupts */
	 lval = 0;
	 DrmLocalReadWrite(mcon,PLX9030_LAS0BA,&lval,4,1); /* Disable address space */

	 lval = CtrDrvrJTAG_PIN_TMS_OUT
	      | CtrDrvrJTAG_PIN_TCK_OUT
	      | CtrDrvrJTAG_PIN_TDI_OUT; /* I/O Direction out */

	 DrmLocalReadWrite(mcon,PLX9030_GPIOC,&lval,4,1);
	 mcon->FlashOpen = 1;
	 return OK;

      case CtrDrvrJTAG_READ_BYTE:         /* Read back uploaded VHDL bit stream */

	 if (lap) {
	    if (mcon->FlashOpen) {
	       DrmLocalReadWrite(mcon,PLX9030_GPIOC,&lval,4,0);
	       if (lval & CtrDrvrJTAG_PIN_TDO_ONE) *lap = CtrDrvrJTAG_TDO;
	       else                                *lap = 0;
	       return OK;
	    }
	    pseterr(EBUSY);                  /* Device busy, not opened */
	    return SYSERR;
	 }
      break;

      case CtrDrvrJTAG_WRITE_BYTE:        /* Upload a new compiled VHDL bit stream */

	 if (lap) {
	    if (mcon->FlashOpen) {

	       lval = CtrDrvrJTAG_PIN_TMS_OUT
		    | CtrDrvrJTAG_PIN_TCK_OUT
		    | CtrDrvrJTAG_PIN_TDI_OUT; /* I/O Direction out */

	       if (lav & CtrDrvrJTAG_TMS) lval |= CtrDrvrJTAG_PIN_TMS_ONE;
	       if (lav & CtrDrvrJTAG_TCK) lval |= CtrDrvrJTAG_PIN_TCK_ONE;
	       if (lav & CtrDrvrJTAG_TDI) lval |= CtrDrvrJTAG_PIN_TDI_ONE;

	       DrmLocalReadWrite(mcon,PLX9030_GPIOC,&lval,4,1);
	       return OK;
	    }
	    pseterr(EBUSY);                  /* Device busy, not opened */
	    return SYSERR;
	 }
      break;

      case CtrDrvrJTAG_CLOSE:             /* Close JTAG interface */

	 /* Wait 1S for the module to settle */

	 sreset(&(mcon->Semaphore));
	 mcon->Timer = timeout(ResetTimeout, (char *) mcon, 100);
	 if (mcon->Timer < 0) mcon->Timer = 0;
	 if (mcon->Timer) swait(&(mcon->Semaphore), SEM_SIGABORT);
	 if (mcon->Timer) CancelTimeout(&(mcon->Timer));

	 lval = 1;
	 DrmLocalReadWrite(mcon,PLX9030_LAS0BA,&lval,4,1); /* Enable address space */
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

      case CtrDrvrREMAP:                  /* Remap BAR2 after a config change */
	 return Remap(mcon);

      case CtrDrvr93LC56B_EEPROM_OPEN:    /* Open the PLX9030 configuration EEPROM 93LC56B for Write */
	 if (mcon->FlashOpen == 0) {

	    /* Assert the chip select for the EEPROM and the target retry delay clocks */
	    /* See PLX PCI 9030 Data Book 10-35 */

	    cntrl  = Plx9030CntrlCHIP_UNSELECT; /* Chip un-select/reset 93lc56b EEPROM */
	    DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1);
	    cntrl |= Plx9030CntrlCHIP_SELECT;   /* Chip select 93lc56b EEPROM */
	    DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1);
	    SendEpromCommand(mcon,EpCmdEWEN,0);
	    mcon->FlashOpen = 1;
	 }
	 return OK;

      case CtrDrvr93LC56B_EEPROM_READ:    /* Read from the EEPROM 93LC56B the PLX9030 configuration */
	 if (mcon->FlashOpen) {
	    if (wcnt >= sizeof(CtrDrvrRawIoBlock)) {
	       riob = (CtrDrvrRawIoBlock *) arg;
	       if (riob->UserArray != NULL) {
		  SendEpromCommand(mcon,EpCmdREAD,(riob->Offset)>>1);
		  ReadEpromWord(mcon,&(riob->UserArray[0]));
		  return OK;
	       }
	    }
	 }
      break;

      case CtrDrvr93LC56B_EEPROM_WRITE:   /* Write to the EEPROM 93LC56B a new PLX9030 configuration */
	 if (mcon->FlashOpen) {
	    if (rcnt >= sizeof(CtrDrvrRawIoBlock)) {
	       riob = (CtrDrvrRawIoBlock *) arg;
	       if (riob->UserArray != NULL) {
		  SendEpromCommand(mcon,EpCmdWRITE,(riob->Offset)>>1);
		  WriteEpromWord(mcon,riob->UserArray[0]);
		  return OK;
	       }
	    }
	 }
      break;

      case CtrDrvr93LC56B_EEPROM_ERASE:   /* Erase the EEPROM 93LC56B, deletes PLX9030 configuration */
	 if (mcon->FlashOpen) {
	    SendEpromCommand(mcon,EpCmdERAL,0);
	    return OK;
	 }
      break;

      case CtrDrvr93LC56B_EEPROM_CLOSE:   /* Close EEPROM 93LC56B and load new PLX9030 configuration */
	 SendEpromCommand(mcon,EpCmdEWDS,0);
	 cntrl = Plx9030CntrlCHIP_UNSELECT; /* Chip un-select 93lc56b EEPROM */
	 DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1);
	 mcon->FlashOpen = 0;
	 return OK;

      case CtrDrvrPLX9030_RECONFIGURE:    /* Load EEPROM configuration into the PLX9030 */
	 if (mcon->FlashOpen == 0) {
	    cntrl  = Plx9030CntrlRECONFIGURE;   /* Reload PLX9030 from 93lc56b EEPROM */
	    DrmLocalReadWrite(mcon,PLX9030_CNTRL,&cntrl,4,1);
	    return OK;
	 }
      break;                              /* EEPROM must be closed */

      case CtrDrvrPLX9030_CONFIG_OPEN:    /* Open the PLX9030 configuration for read */
	 mcon->ConfigOpen = 1;
	 return OK;

      case CtrDrvrPLX9030_CONFIG_READ:    /* Read the PLX9030 configuration registers */
	 if (mcon->ConfigOpen) {
	    if (wcnt >= sizeof(CtrDrvrRawIoBlock)) {
	       riob = (CtrDrvrRawIoBlock *) arg;
	       if ((riob->UserArray != NULL) &&  (wcnt > riob->Size)) {
		  return DrmConfigReadWrite(mcon,riob->Offset,riob->UserArray,riob->Size,0);
	       }
	    }
	 }
      break;

      case CtrDrvrPLX9030_CONFIG_WRITE:   /* Write to PLX9030 configuration registers (Experts only) */
	 if (mcon->ConfigOpen) {
	    if (rcnt >= sizeof(CtrDrvrRawIoBlock)) {
	       riob = (CtrDrvrRawIoBlock *) arg;
	       if ((riob->UserArray != NULL) &&  (rcnt >= riob->Size)) {
		  return DrmConfigReadWrite(mcon,riob->Offset,riob->UserArray,riob->Size,1);
	       }
	    }
	 }
      break;

      case CtrDrvrPLX9030_CONFIG_CLOSE:   /* Close the PLX9030 configuration */
	 mcon->ConfigOpen = 0;
	 return OK;

      case CtrDrvrPLX9030_LOCAL_OPEN:     /* Open the PLX9030 local configuration for read */
	 mcon->LocalOpen = 1;
	 return OK;

      case CtrDrvrPLX9030_LOCAL_READ:     /* Read the PLX9030 local configuration registers */
	 if (mcon->LocalOpen) {
	    if (wcnt >= sizeof(CtrDrvrRawIoBlock)) {
	       riob = (CtrDrvrRawIoBlock *) arg;
	       if ((riob->UserArray != NULL) &&  (wcnt >= riob->Size)) {
		  return DrmLocalReadWrite(mcon,riob->Offset,riob->UserArray,riob->Size,0);
	       }
	    }
	 }
      break;

      case CtrDrvrPLX9030_LOCAL_WRITE:    /* Write the PLX9030 local configuration registers (Experts only) */
	 if (mcon->LocalOpen) {
	   if (rcnt >= sizeof(CtrDrvrRawIoBlock)) {
	       riob = (CtrDrvrRawIoBlock *) arg;
	       if ((riob->UserArray != NULL) &&  (rcnt > riob->Size)) {
		  return DrmLocalReadWrite(mcon,riob->Offset,riob->UserArray,riob->Size,1);
	       }
	    }
	 }
      break;

      case CtrDrvrPLX9030_LOCAL_CLOSE:    /* Close the PLX9030 local configuration */
	 mcon->LocalOpen = 0;
	 return OK;

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

	       Int32Copy((unsigned int *) &(mmap->Trigs[i]),
			(unsigned int *) TriggerToHard(&act->Trigger),
			(unsigned int  ) sizeof(CtrDrvrHwTrigger));
	       mcon->Trigs[i] = act->Trigger;

	       /* Override bus interrupt settings for connected clients */

	       if (mcon->Clients[i])
		  act->Config.OnZero |= CtrDrvrCounterOnZeroBUS;
	       else
		  act->Config.OnZero &= ~CtrDrvrCounterOnZeroBUS;

	       Int32Copy((unsigned int *) &(mmap->Configs[i]),
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
		     Int32Copy((unsigned int *) &(mmap->Trigs[i]),
			      (unsigned int *) htrg,
			      (unsigned int  ) sizeof(CtrDrvrHwTrigger));

		     mcon->EqpNum[i]   = 0;
		     mcon->EqpClass[i] = 0;
		     bzero((void *) &(mcon->Configs[i]),sizeof(CtrDrvrCounterConfiguration));
		     bzero((void *) &(mcon->Trigs[i]  ),sizeof(CtrDrvrTrigger));
		     Int32Copy((unsigned int *) &(mmap->Configs[i]),
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

      case CtrDrvrENABLE:                               /* Enable CTR module event reception */
	 if        (lav   == CtrDrvrCommandSET_HPTDC) { /* Enable HPTDC chip */
	    mcon->Command |=  CtrDrvrCommandSET_HPTDC;
	    mmap->Command  =  mcon->Command;
	 } else if (lav   == ~CtrDrvrCommandSET_HPTDC) { /* Disable HPTDC chip */
	    mcon->Command &= ~CtrDrvrCommandSET_HPTDC;
	    mmap->Command  =  mcon->Command;
	 } else if (lav   != 0) {
	    mcon->Command |=  CtrDrvrCommandENABLE;
	    mcon->Command &= ~CtrDrvrCommandDISABLE;
	    mmap->Command  =  mcon->Command;
	 } else {
	    mcon->Command &= ~CtrDrvrCommandENABLE;;
	    mcon->Command |=  CtrDrvrCommandDISABLE;
	    mmap->Command  =  mcon->Command;
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
	 mmap->Command  =  mcon->Command;
	 return OK;

      case CtrDrvrSET_BRUTAL_PLL:
	 if (lav) {
	    mcon->Command |=  CtrDrvrCommandUtcPllOff;
	    mcon->Command &= ~CtrDrvrCommandUtcPllOn;
	 } else {
	    mcon->Command &= ~CtrDrvrCommandUtcPllOff;
	    mcon->Command |=  CtrDrvrCommandUtcPllOn;
	 }
	 mmap->Command  =  mcon->Command;
	 return OK;

      case CtrDrvrGET_INPUT_DELAY:        /* Get input delay in 25ns ticks */
	 if (lap) {
	    *lap = mmap->InputDelay;
	    return OK;
	 }
      break;

      case CtrDrvrSET_INPUT_DELAY:        /* Set input delay in 25ns ticks */
	 mmap->InputDelay = lav;
	 mcon->InputDelay = lav;
	 return OK;

      case CtrDrvrGET_REMOTE:             /* Counter Remote/Local status */
	 if (wcnt >= sizeof(CtrdrvrRemoteCommandBuf)) {
	    remc = (CtrdrvrRemoteCommandBuf *) arg;
	    if ((remc->Counter >= CtrDrvrCounter1) && (remc->Counter <= CtrDrvrCounter8)) {
	       remc->Remote = mmap->Counters[remc->Counter].Control.LockConfig;
	       return OK;
	    }
	 }
      break;

      case CtrDrvrSET_REMOTE:             /* Counter Remote/Local status */
	 if (rcnt >= sizeof(CtrdrvrRemoteCommandBuf)) {
	    remc = (CtrdrvrRemoteCommandBuf *) arg;
	    if ((remc->Counter >= CtrDrvrCounter1) && (remc->Counter <= CtrDrvrCounter8)) {
	       if (remc->Remote) mmap->Counters[remc->Counter].Control.LockConfig = 1;
	       else              mmap->Counters[remc->Counter].Control.LockConfig = 0;
	       return OK;
	    }
	 }
      break;

      case CtrDrvrREMOTE:                 /* Remote control counter */
	 if (rcnt >= sizeof(CtrdrvrRemoteCommandBuf)) {
	    remc = (CtrdrvrRemoteCommandBuf *) arg;
	    if ((remc->Counter >= CtrDrvrCounter1) && (remc->Counter <= CtrDrvrCounter8)) {
	       if (mmap->Counters[remc->Counter].Control.LockConfig) {
		  mmap->Counters[remc->Counter].Control.RemOutMask =
		     CtrDrvrRemoteEXEC | (CtrDrvrCntrCntrlREMOTE_MASK & remc->Remote);
		  return OK;
	       }
	    }
	 }
      break;

      case CtrDrvrGET_MODULE_STATS:
	 if (wcnt >= sizeof(CtrDrvrModuleStats)) {
	    mods = (CtrDrvrModuleStats *) arg;
	    Int32Copy((unsigned int *) mods,
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
	       msk = mmap->Counters[i].Control.RemOutMask;
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
	       mmap->Counters[i].Control.RemOutMask =
		  (AutoShiftLeft(CtrDrvrCntrCntrlOUT_MASK,cmsb->Mask) | msk);
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
		  Int32Copy((unsigned int *) &(mmap->Counters[conf->Counter].Config),
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
	       Int32Copy((unsigned int *) &(hisb->History),
			(unsigned int *) &(mmap->Counters[hisb->Counter].History),
			(unsigned int  ) sizeof(CtrDrvrCounterHistory));
	       return OK;
	    }
	 }
      break;

      case CtrDrvrGET_PLL:                /* Get phase locked loop parameters */
	 if (wcnt >= sizeof(CtrDrvrPll)) {
	    pll = (CtrDrvrPll *) arg;
	    Int32Copy((unsigned int *) pll,
		     (unsigned int *) &(mmap->Pll),
		     (unsigned int  ) sizeof(CtrDrvrPll));
	    return OK;
	 }
      break;

      case CtrDrvrSET_PLL:                /* Set phase locked loop parameters */
	 if (rcnt >= sizeof(CtrDrvrPll)) {
	    pll = (CtrDrvrPll *) arg;
	    Int32Copy((unsigned int *) &(mmap->Pll),
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
	       Int32Copy((unsigned int *) tgmb->Telegram,
			(unsigned int *) mmap->Telegrams[(int) (tgmb->Machine) -1],
			(unsigned int  ) sizeof(CtrDrvrTgm));
	       if (Wa->Endian == CtrDrvrEndianLITTLE) SwapWords((unsigned int *) tgmb->Telegram,
								(unsigned int  ) sizeof(CtrDrvrTgm));
	       return OK;
	    }
	 }
      break;

      case CtrDrvrREAD_EVENT_HISTORY:     /* Read incomming event history */
	 if (wcnt >= sizeof(CtrDrvrEventHistory)) {
	    evhs = (CtrDrvrEventHistory *) arg;
	    Int32Copy((unsigned int *) evhs,
		     (unsigned int *) &(mmap->EventHistory),
		     (unsigned int  ) sizeof(CtrDrvrEventHistory));
	    return OK;
	 }
      break;


      case CtrDrvrGET_RECEPTION_ERRORS:   /* Timing frame reception error status */
	 if (wcnt >= sizeof(CtrDrvrReceptionErrors)) {
	    rcpe = (CtrDrvrReceptionErrors *) arg;
	    rcpe->LastReset    = mmap->LastReset;
	    rcpe->PartityErrs  = mmap->PartityErrs;
	    rcpe->SyncErrs     = mmap->SyncErrs;
	    rcpe->TotalErrs    = mmap->TotalErrs;
	    rcpe->CodeViolErrs = mmap->CodeViolErrs;
	    rcpe->QueueErrs    = mmap->QueueErrs;
	    return OK;
	 }
      break;

      case CtrDrvrGET_IO_STATUS:          /* Status of module inputs */
	 if (lap) {
	    *lap = mmap->IoStat;
	    return OK;
	 }
      break;

      case CtrDrvrGET_IDENTITY:           /* Identity of board from ID chip */
	 if (wcnt >= sizeof(CtrDrvrBoardId)) {
	    bird = (CtrDrvrBoardId *) arg;
	    bird->IdLSL = mmap->IdLSL;
	    bird->IdMSL = mmap->IdMSL;
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

