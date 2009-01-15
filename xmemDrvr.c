/* $Id$ */
/**
 * @file xmemDrvr.c
 *
 * @brief Device driver for the PCI (PMC/PCI) VMIC Reflective memory
 *
 * @author Julian Lewis AB/CO/HT CERN
 *
 * @date Created on 02/09/2004
 *
 * The driver can handle multiple VMIC modules.
 * This is useful for making "bridges" between distributed memory networks.
 * Bridged networks are connected together by the PCI DMA engines 
 * on the Plx9656 chips.
 *
 * The driver can handle multiple clients on one platform accessing
 * the reflective memory.
 * Each Client has a unique client context where current working
 * settings are kept.
 *
 * Support functions needed to implement a distributed shared memory
 * have been provided.
 * See my paper "Distributed Memory in a Heterogeneous Network" ICALEPCS
 * http://www-bd.fnal.gov/icalepcs/abstracts/PDF/fpo30.pdf
 *
 * The driver partitions the memory in the VMIC SDRAM into
 * areas called segments.
 * The segment table is initialized from file at run time.
 * Each segment has an Id 1..32, encoded as a single bit in a 32-Bit long.
 * Each segment has a size and is located in SDRAM at a fixed location.
 * The segment descriptor contains a 32-Bit field which indicates
 * which of the first 32 Nodes are allowed write access.
 *
 * Node ID ZERO is illegal and must never be used, the first valid Node ID is 1.
 * The first 1..32 VMIC node Ids are encoded in the same way as a single
 * bit in a 32-bit long.
 * Nodes with higher than 32 Id numbers cannot use the segment interface,
 * they use direct access.
 * Read/Write segment access is via the READ/WRITE_SEGMENT IOCTLS.
 * (This uses the DMA engines)
 *
 * The general purpose interrupt 3 is used by the driver to know when segments
 * have been updated.
 * General purpose interrupt number 3 is not available for client use, 
 * they can use 1 and 2.
 * The 32-bit data word of Int-3 indicates which of the 32 segments
 * have been written.
 * Connecting to the SEGMENT_UPDATE interrupt (INT-3) and reading
 * the 32-Bit value permits a task to synchronise on segment updates.
 *
 * Each time a segment is updated, the fact is latched for each client 
 * in a bit for each segment.
 * The CHECK_SEGMENT IOCTL returns the value of the updated Id's
 * in a 32-Bit long. This is for polling.
 * Each READ_SEGMENT IOCTL Clears the segments Id bit in the updated bits mask.
 *
 * Direct access to the VMIC SDRAM is permitted, the layout of the 
 * segment table is available by calling the GET_SEGMENT_TABLE IOCTL.
 * This is the only interface allowed for node Id > 32
 *
 * The driver is transparent to the host machine's endianness; this is achieved
 * by using the CDCM functions to convert data from/to the host computer 
 * to/from the device.
 * Note that the PLX does not swap any data (i.e. BIGEND set to little endian),
 * except for the case of a DMA transfer (where the CPU is not involved).
 * Therefore we can only transfer to the VMIC's SDRAM chunks of 4 bytes -- to
 * be consistent since otherwise the PLX we would be performing data-invariance,
 * and what we want to is address invariance, which is simpler.
 * See the paper below for data and address invariance:
 * "Data Movement Between Big-Endian and Little-Endian Devices",
 * by Kyle Aubrey and Ashan Kabir.
 * http://www.freescale.com/files/32bit/doc/app_note/AN2285.pdf
 *
 * Note that the driver will not work on 64-bit platforms, since it assumes that
 * a type 'long' is 4 bytes long (this comes from the initial version).
 *
 * @version 2.0 Emilio G. Cota 15/10/2008, CDCM-compliant.
 * @version 1.0 Julian Lewis 02/09/2004, Lynx driver.
 */

#include <cdcm/cdcmBoth.h>
#include <cdcm/cdcmIo.h>

#ifdef __linux__
#include <cdcm/cdcmDrvr.h>
#include <cdcm/cdcmLynxAPI.h>
#include <cdcm/cdcmLynxDefs.h>
#define sel  cdcm_sel  /* see Lynx <sys/file.h> for more details */
#define file cdcm_file /* see Lynx <sys/file.h> for more details */
#define enable restore
#else  /* __Lynx__ */
#include <conf.h>
#include <dldd.h>
#include "drm.h"
#include <errno.h>
#include <kernel.h>
#include <io.h>
#include <mem.h>
#include <param.h>
#include <pci_resource.h>
#include <signal.h>
#include <string.h>
#include <sys/drm_errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/timeout.h>
#include <sys/types.h>
#include <time.h>
extern int nanotime(unsigned long *);

/* we include the contents of cdcmBoth and cdcmIo here, because the
 * makefiles are not ready yet :( 
 */
#include <cdcm/cdcmBoth.c>
#endif /* !__linux__ */

#include "plx9656_layout.h"
#include "vmic5565_layout.h"
#include "xmemDrvr.h"
#include "xmemDrvrP.h"

#ifndef KB
#define KB 1024
#endif
#ifndef MB
#define MB 1024*KB
#endif

#ifdef __powerpc__
extern void iointunmask(); /* needed to register an interrupt */
#endif

//#define __DEBUG_TRACK_ALL__ /* comment this out for debug tracking */
#ifdef __DEBUG_TRACK_ALL__
#define IOCTL_TRACK(name)					\
do {								\
  cprintf("XmemDrvr: [IOCTL]--------------------> '%s' called\n", name);	\
} while (0)
#define FUNCT_TRACK							    \
do {									    \
  cprintf("XmemDrvr:[FUNCTION]--------------------> '%s()' called\n", __FUNCTION__); \
} while (0)
#else
#define IOCTL_TRACK(name) 
#define FUNCT_TRACK
#endif /* !__DEBUG_TRACK_ALL__ */

//#define __TEST_TT__ /* comment this out to measure data transfer times */
#ifdef __TEST_TT__
#ifdef __linux__
struct timespec t_a, t_b, t_a1, t_a2, t_a3, t_a4;
#else /* Lynx */
unsigned long tt_a, tt_na, tt_b, tt_nb;
unsigned long tt_a1, tt_na1, tt_a2, tt_na2, tt_a3, tt_na3, tt_a4, tt_na4;
#endif /* !__linux__ */
unsigned long tt_diff, tt_diff01, tt_diff12, tt_diff23, tt_diff34;
#endif /* __TEST_TT__ */


XmemDrvrWorkingArea *Wa = NULL;

static unsigned long GetRfmReg(XmemDrvrModuleContext *mcon, VmicRfm reg, 
			       int size);
static void SetRfmReg(XmemDrvrModuleContext *mcon, VmicRfm reg, int size,
		      unsigned long data);
static int SendInterrupt(XmemDrvrSendBuf *sbuf);

/**
 * LongCopy - copies size bytes in chunks of longs from src to dst.
 *
 * @param dst: copy to
 * @param src: copy from
 * @param size: size of the transfer
 *
 * Note that if the size to be copied is smaller than sizeof(long),
 * no copy will be performed.
 * 
 */
static void LongCopy(unsigned long *dst, unsigned long *src, unsigned long size)
{
  int sb;
  int i;

  FUNCT_TRACK;
  sb = size/sizeof(unsigned long);
  for (i = 0; i < sb; i++) dst[i] = src[i];
}

/**
 * LongCopyToXmem - copies size bytes in chunks of longs to the Xmem.
 *
 * @param vmic_to: copy to
 * @param v_from: copy from
 * @param size: size of the transfer
 *
 * Note that if the size to be copied is smaller than sizeof(long),
 * no copy will be performed.
 *
 */
static void LongCopyToXmem(void *vmic_to, void *v_from, unsigned long size)
{
  int count;
  u_int32_t *tbuf = (u_int32_t *)v_from;
  u_int32_t *ioaddr = (u_int32_t *)vmic_to;

  FUNCT_TRACK;
  count = size / sizeof(u_int32_t);
  if (count <= 0) return;
  do {
    cdcm_iowrite32(cdcm_cpu_to_le32(*tbuf), ioaddr);
    tbuf++;
    ioaddr++;
  } while (--count != 0);
}

/**
 * LongCopyFromXmem - copies size bytes in chunks of longs from the Xmem.
 *
 * @param vmic_from: copy from
 * @param v_to: copy to
 * @param size: size of the transfer
 *
 * Note that if the size to be copied is smaller than sizeof(long),
 * no copy will be performed.
 *
 */
static void LongCopyFromXmem(void *vmic_from, void *v_to, unsigned long size)
{
  int count;
  u_int32_t *tbuf = (u_int32_t *)v_to;
  u_int32_t *ioaddr = (u_int32_t *)vmic_from;

  FUNCT_TRACK;
  count = size / sizeof(u_int32_t);
  if (count <= 0) return;
  do {
    *tbuf++ = cdcm_le32_to_cpu(cdcm_ioread32(ioaddr));
    ioaddr++;
  } while (--count != 0);
}

/**
 * CancelTimeout - cancels a given timeout 
 *
 * @param t: pointer to the ID of the timeout to be cancelled
 *
 * Care has been taken to avoid cancelling an already cancelled (or expired)
 * timeout by protecting the critical region properly.
 *
 */
static void CancelTimeout(int *t)
{
  unsigned long ps;
  int v;

  FUNCT_TRACK;
  disable(ps);
  {
    if ((v = *t)) {
      *t = 0;
      cancel_timeout(v);
    }
  }
  restore(ps);
}

/**
 * ReadTimeout - handler function for ccon->Timer.
 *
 * @param c: pointer to the ID of the timeout
 *
 * @return 0 on success
 */
static int ReadTimeout(void *c)
{
  XmemDrvrClientContext *ccon = (XmemDrvrClientContext*)c;
  ccon->Timer = 0;
  ccon->Queue.Size   = 0;
  ccon->Queue.Missed = 0;
  sreset(&ccon->Semaphore);
  return 0;
}

/**
 * WrDmaTimeout - handler function for mcon->WrDmaSemaphore
 *
 * @param m: pointer to the ID of the timeout
 *
 * WrDmaSemaphore is signalled when the timeout expires.
 *
 * @return 0 on success
 */
static int WrDmaTimeout(void *m)
{
  XmemDrvrModuleContext *mcon = (XmemDrvrModuleContext*)m;
  mcon->WrTimer = 0;
  ssignal(&mcon->WrDmaSemaphore);
  return 0;
}

/**
 * RdDmaTimeout - handler function for mcon->RdDmaSemaphore
 *
 * @param m: pointer to the ID of the timeout
 *
 * RdDmaSemaphore is signalled when the timeout expires.
 *
 * @return 0 on success
 */
static int RdDmaTimeout(void *m)
{
  XmemDrvrModuleContext *mcon = (XmemDrvrModuleContext*)m;
  mcon->RdTimer = 0;
  ssignal(&mcon->RdDmaSemaphore);
  return 0;
}

/*! @name ioctl_names
 * keeps the names of all the IOCTL's, to be printed when debug is set.
 */
//@{
char *ioctl_names[XmemDrvrLAST_IOCTL] = {

  "ILLEGAL_IOCTL",          /* LynxOs calls me with this */

  /* Standard IOCTL Commands */

  "SET_SW_DEBUG",           /* Set driver debug mode */
  "GET_SW_DEBUG",           /* Get the driver debug mode */

  "GET_VERSION",            /* Get version date */

  "SET_TIMEOUT",            /* Set the read timeout value in micro seconds */
  "GET_TIMEOUT",            /* Get the read timeout value in micro seconds */

  "SET_QUEUE_FLAG",         /* Set queuing capabiulities on off */
  "GET_QUEUE_FLAG",         /* 1=Q_off 0=Q_on */
  "GET_QUEUE_SIZE",         /* Number of events on queue */
  "GET_QUEUE_OVERFLOW",     /* Number of missed events */

  "GET_MODULE_DESCRIPTOR",  /* Get the current Module descriptor */
  "SET_MODULE",             /* Select the module to work with */
  "GET_MODULE",             /* Which module am I working with */
  "GET_MODULE_COUNT",       /* The number of installed VMIC modules */
  "SET_MODULE_BY_SLOT",     /* Select the module to work with by slot ID */
  "GET_MODULE_SLOT",        /* Get the slot ID of the selected module */

  "RESET",                  /* Reset the module and perform full initialize */
  "SET_COMMAND",            /* Send a command to the VMIC module */
  "GET_STATUS",             /* Read module status */
  "GET_NODES",              /* Get the node map of all connected to
			       pending-init */

  "GET_CLIENT_LIST",        /* Get the list of driver clients */

  "CONNECT",                /* Connect to an object interrupt */
  "DISCONNECT",             /* Disconnect from an object interrupt */
  "GET_CLIENT_CONNECTIONS", /* Get the list of a client connections on module */

  "SEND_INTERRUPT",         /* Send an interrupt to other nodes */

  "GET_XMEM_ADDRESS",       /* Get phsical address of reflective memory SDRAM */
  "SET_SEGMENT_TABLE",      /* Set the list of all defined xmem
			       memory segments */
  "GET_SEGMENT_TABLE",      /* List of all defined xmem memory segments */

  "READ_SEGMENT",           /* Copy from xmem segment to local memory */
  "WRITE_SEGMENT",          /* Update the segment with new contents */
  "CHECK_SEGMENT",          /* Check to see if a segment has been updated since
			       last read */
  "FLUSH-SEGMENTS",         /* Flush segments to xmem nodes */

  /* Hardware specialists raw IO access to PLX and VMIC memory */

  "CONFIG_OPEN",            /* Open plx9656 configuration registers block */
  "LOCAL_OPEN",             /* Open plx9656 local registers block 
			       Local/Runtime/DMA */
  "RFM_OPEN",               /* Open VMIC 5565 FPGA Register block RFM */
  "RAW_OPEN",               /* Open VMIC 5565 SDRAM  */

  "CONFIG_READ",            /* Read from plx9656 configuration 
			       registers block */
  "LOCAL_READ",             /* Read from plx9656 local registers 
			       block Local/Runtime/DMA */
  "RFM_READ",               /* Read from VMIC 5565 FPGA Register block RFM */
  "RAW_READ",               /* Read from VMIC 5565 SDRAM */

  "CONFIG_WRITE",           /* Write to plx9656 configuration registers block */
  "LOCAL_WRITE",            /* Write to plx9656 local registers block
			       Local/Runtime/DMA */
  "RFM_WRITE",              /* Write to VMIC 5565 FPGA Register block RFM */
  "RAW_WRITE",              /* Write to VMIC 5565 SDRAM */

  "CONFIG_CLOSE",           /* Close plx9656 configuration registers block */
  "LOCAL_CLOSE",            /* Close plx9656 local registers block
			       Local/Runtime/DMA */
  "RFM_CLOSE",              /* Close VMIC 5565 FPGA Register block RFM */
  "RAW_CLOSE",              /* Close VMIC 5565 SDRAM */

  "SET_DMA_THRESHOLD",      /* Set Drivers DMA threshold */
  "GET_DMA_THRESHOLD"       /* Get Drivers DMA threshold */
};
//@}

/**
 * TraceIoctl - tracks the IOCTL requests by printing on the kernel's log.
 *
 * @param cm: IOCTL command number
 * @param arg: argument of the IOCTL call
 * @param ccon: pointer to client context
 *
 * This function will work only if XmemDrvrDebugTRACE and ccon->Debug are set.
 *
 */
static void TraceIoctl(int cm, char *arg, XmemDrvrClientContext *ccon)
{ 
  int lav;
  char *iocname;

  if (XmemDrvrDebugTRACE & ccon->Debug) {
    if ((cm >= 0) && (cm < XmemDrvrLAST_IOCTL)) {
      iocname = ioctl_names[(int) cm];
      if (arg) {
	lav = *((long *) arg);
	cprintf("xmemDrvr:Trace:Called IOCTL: %s Arg: %d\n", iocname, (int)lav);
      } else {
	cprintf("xmemDrvr:Trace:Called IOCTL: %s (Null argument)\n", iocname);
      }
      return ;
    }
    cprintf("xmemDrvr: Debug: Trace: Illegal IOCTL number: %d\n",(int) cm);
  }
}

/**
 * SetEndian - sets the endianness of the machine in the working area.
 *
 * This function uses the CDCM_{BIG,LITTLE}_ENDIAN macros, which
 * are platform-independent.
 *
 */
static void SetEndian() 
{
#ifdef CDCM_LITTLE_ENDIAN
  Wa->Endian = XmemDrvrEndianLITTLE;
#else
  Wa->Endian = XmemDrvrEndianBIG;
#endif
}

/**
 * DrmLocalReadWrite - for PLX Local Configuration
 *
 * @param mcon: module context
 * @param addr: offset within the PLX Local Registers
 * @param value: pointer to where the data is read/written
 * @param size: size of the data to be transferred
 * @param flag: XmemDrvr READ/WRITE flag
 *
 * This function reads/writes from/to the PLX9656 local config registers.
 *
 * @return OK on success
 */
static int DrmLocalReadWrite(XmemDrvrModuleContext *mcon, unsigned long addr,
			     unsigned long *value, unsigned long size,
			     XmemDrvrIoDir flag)
{
  void *lmap;
  void *ioaddr;
  unsigned long ps;

  FUNCT_TRACK;
  lmap = (void *)mcon->Local;
  if (!lmap) return SYSERR;
  ioaddr = lmap + addr;
  if (!recoset()) {
    if (flag == XmemDrvrWRITE) {
      switch (size) {
      case XmemDrvrBYTE:
	cdcm_iowrite8((unsigned char)*value, ioaddr);
	break;
      case XmemDrvrWORD:
	cdcm_iowrite16(cdcm_cpu_to_le16((unsigned short)*value), ioaddr);
	break;
      case XmemDrvrLONG:
	cdcm_iowrite32(cdcm_cpu_to_le32((unsigned int)*value), ioaddr);
	break;
      }
    } else {
      switch (size) {
      case XmemDrvrBYTE:
	*value = (unsigned long)cdcm_ioread8(ioaddr);
	break;
      case XmemDrvrWORD:
	*value = (unsigned long)cdcm_le16_to_cpu(cdcm_ioread16(ioaddr));
	break;
      case XmemDrvrLONG:
	*value = (unsigned long)cdcm_le32_to_cpu(cdcm_ioread32(ioaddr));
	break;
      }
    }
    return OK;
  }
  else {
    disable(ps);
    kkprintf("xmemDrvr: BUS-ERROR: Module:%d PciSlot:%d \n",
	     (int) mcon->ModuleIndex+1,
	     (int) mcon->PciSlot);
    noreco();
    enable(ps);
    pseterr(ENXIO); /* No such device or address */
    return SYSERR;
  }
  noreco();
  return OK;  
}

/**
 * DrmConfigReadWrite - Read/Write PLX' config registers
 *
 * @param mcon: module context
 * @param addr: offset within the config registers
 * @param value: pointer to where the data is read/written
 * @param size_l: size of the data to be transferred
 * @param flag: XmemDrvr READ/WRITE flag
 *
 * Reads/writes from the PLX9656 configuration registers.
 * Since drm_device_{read,write} can only access 4-byte aligned addresses,
 * we only read/write in chunks of 4 bytes.
 *
 * @return OK on success
 */
static int DrmConfigReadWrite(XmemDrvrModuleContext *mcon, unsigned long addr,
			      unsigned long *value, unsigned long size,
			      XmemDrvrIoDir flag) 
{
  int cc;
  unsigned long regval;
  unsigned long regnum;
  unsigned char *cptr;
  unsigned short *sptr;
  int i=0;

  FUNCT_TRACK;
  regnum = addr >> 2;
  cptr = (char *)&regval;
  sptr = (short *)&regval;
   /* First read the long register value (forced because of drm lib) */
   cc = drm_device_read(mcon->Handle, PCI_RESID_REGS, regnum, 0, &regval);
   if (cc) {
      cprintf("xmemDrvr: DrmConfigReadWrite:errno %d in drm_device_read\n", cc);
      return cc;
   }
   switch (size) {
   case 1: /* byte: i = {0,1,2,3} */
#if defined(CDCM_BIG_ENDIAN)
     i = (~addr) & 0x3; 
#else 
     i = addr & 0x3;
#endif
     break;
   case 2: /* word: i = {0,1} */
#if defined(CDCM_BIG_ENDIAN)
     i = (~addr >> 1) & 0x1;
#else
     i = (addr >> 1) & 0x1;
#endif
     break;
   default: /* double word: no swapping needed here */
     break;
   }
   if (flag == XmemDrvrWRITE) {
     switch(size) {
     case 1:
       cptr[i] = (unsigned char)*value;
       break;
     case 2:
       sptr[i] = (unsigned short)*value;
       break;
     default:
       regval = (unsigned long)*value;
     }
      /* Write back modified configuration register */
      cc = drm_device_write(mcon->Handle, PCI_RESID_REGS, regnum, 0, &regval);
      if (cc) {
	 cprintf("xmemDrvr:DrmConfigReadWrite:errno %d@drm_device_write\n", cc);
	 return cc;
      }
   }
   else { /* XmemDrvrREAD */
     switch(size) {
     case 1:
       *value = (unsigned long)cptr[i];
       break;
     case 2:
       *value = (unsigned long)sptr[i];
       break;
     default:
       *value = (unsigned long)regval;
     }
   }
   return OK;
}

#ifndef COMPILE_TIME /* the makefile should set it */
#define COMPILE_TIME 0
#endif
/**
 * GetVersion - gets the version (compile time) and checks for bus errors.
 *
 * @param mcon: module context
 * @param ver: pointer to struct XmemDrvrVersion, which will be written to.
 *
 * Apart from getting the version, this function is also used by PingModule() to
 * as a way of checking if the device is alive -- hence the bus error recovery.
 *
 * @return OK on success
 */
static int GetVersion(XmemDrvrModuleContext *mcon, XmemDrvrVersion *ver)
{
  unsigned long ps;

  FUNCT_TRACK;
  if (!recoset()) { /* Catch bus errors  */
    ver->DriverVersion = COMPILE_TIME;                   /* Driver version */
    ver->BoardRevision = GetRfmReg(mcon,VmicRfmBRV,1);   /* Board Revision RO */
    ver->BoardId       = GetRfmReg(mcon,VmicRfmBID,1);   /* Board ID = 65  RO */
  } else {
    disable(ps);
    kkprintf("xmemDrvr: BUS-ERROR: Module:%d PciSlot:%d \n",
	     (int) mcon->ModuleIndex+1, (int) mcon->PciSlot);
    noreco();
    enable(ps);
    pseterr(ENXIO); /* No such device or address */
    return SYSERR;
  }
  noreco();
  return OK;
}

/**
 * PingModule - ping module using GetVersion()
 *
 * @param mcon: module context
 *
 * GetVersion() is called by this function; if the device is alive then
 * PingModule() will return OK.
 *
 * @return OK if the device pointed by mcon is alive.
 */
static int PingModule(XmemDrvrModuleContext *mcon)
{
  XmemDrvrVersion ver;
  FUNCT_TRACK;
  return GetVersion(mcon,&ver);
}

/**
 * EnableInterrupts - Enables interrupts given by msk on module mcon.
 *
 * @param mcon: module context
 * @param msk: bitmask that contains the interrupts to be enabled
 *
 * This function operates on the Rfm register area of the VMIC5565.
 * Registers are set to trigger the interrupts given through the bitmask msk.
 *
 * @return OK on success
 */
static int EnableInterrupts(XmemDrvrModuleContext *mcon, VmicLier msk)
{
  void *vmap;
  unsigned int lisr_temp;
  unsigned long intcsr;

  FUNCT_TRACK;
  vmap = mcon->Map;
  msk &= VmicLierMASK; /* Clear out any crap bits (e.g. IntDAEMON) */
  if (msk & VmicLierINT1) {
    if ((mcon->InterruptEnable & VmicLierINT1) == 0) {
      SetRfmReg(mcon, VmicRfmSID1, 1, 0);
      cdcm_iowrite32(0, vmap + VmicRfmISD1);
    }
  }
  if (msk & VmicLierINT2) {
    if ((mcon->InterruptEnable & VmicLierINT2) == 0) {
      SetRfmReg(mcon, VmicRfmSID2, 1, 0);
      cdcm_iowrite32(0, vmap + VmicRfmISD2);
    }
  }
  if (msk & VmicLierINT3) {
    if ((mcon->InterruptEnable & VmicLierINT3) == 0) {
      SetRfmReg(mcon, VmicRfmSID3, 1, 0);
      cdcm_iowrite32(0, vmap + VmicRfmISD3);
    }
  }
  if (msk & VmicLierPENDING_INIT) {
    if ((mcon->InterruptEnable & VmicLierPENDING_INIT) == 0) {
      SetRfmReg(mcon, VmicRfmINITN, 1, 0);
      cdcm_iowrite32(0, vmap + VmicRfmINITD);
    }
  }
  if (msk & VmicLierPARITY_ERROR) {
    if ((mcon->InterruptEnable & VmicLierPARITY_ERROR) == 0) {
      mcon->Command |= VmicLcsrPARITY_ON;
      cdcm_iowrite32(cdcm_cpu_to_le32(mcon->Command), vmap + VmicRfmLCSR1);
    }
  }
  if (msk) {
    mcon->InterruptEnable |= msk;
    cdcm_iowrite32(cdcm_cpu_to_le32(mcon->InterruptEnable), vmap + VmicRfmLIER);

    lisr_temp = cdcm_le32_to_cpu(cdcm_ioread32(vmap + VmicRfmLISR));

    cdcm_iowrite32(cdcm_cpu_to_le32(lisr_temp | VmicLisrENABLE),
		   vmap + VmicRfmLISR);
    lisr_temp = cdcm_le32_to_cpu(cdcm_ioread32(vmap + VmicRfmLISR));

    DrmLocalReadWrite(mcon, PlxLocalINTCSR, &intcsr, 4, XmemDrvrREAD);
    intcsr |= PlxIntcsrENABLE_PCI 
      | PlxIntcsrENABLE_LOCAL
      | PlxIntcsrENABLE_DMA_CHAN_0
      | PlxIntcsrENABLE_DMA_CHAN_1;
    DrmLocalReadWrite(mcon, PlxLocalINTCSR, &intcsr, 4, XmemDrvrWRITE);
#ifdef __powerpc__
    iointunmask(mcon->IVector);
#endif
  }
  return OK;
}

/**
 * DisableInterrupts - disables triggering the interrupts given through msk
 *
 * @param mcon: module context
 * @param msk: bitmask that contains the interrupts to be disabled
 *
 * @return OK on success
 */
static int DisableInterrupts(XmemDrvrModuleContext *mcon, VmicLier msk)
{
  void *vmap;

  FUNCT_TRACK

  vmap = mcon->Map;
  msk &= VmicLierMASK; /* Clear out any crap bits */

  if (msk) {
    mcon->InterruptEnable &= ~msk;
    cdcm_iowrite32(cdcm_cpu_to_le32(mcon->InterruptEnable), vmap + VmicRfmLIER);
  }
  return OK;
}

/**
 * GetSetStatus - Get or Set the the value of LCSR1: Local Control and Status 1.
 *
 * @param mcon: module context
 * @param cmd: command (32bits) to be set
 * @param iod: XmemDrvrREAD or XmemDrvrWRITE
 *
 * Commands are sent through cmd, which is a 32 bits long bitmask.
 *
 * @return resulting value of register LCSR1
 */
static XmemDrvrScr GetSetStatus(XmemDrvrModuleContext *mcon, XmemDrvrScr cmd,
				XmemDrvrIoDir iod)
{
  void *vmap;
  XmemDrvrScr stat;

  FUNCT_TRACK;
  vmap = mcon->Map;

  if (iod == XmemDrvrWRITE) {
    cmd &= XmemDrvrScrCMD_MASK;
    mcon->Command = cmd;
    cdcm_iowrite32(cdcm_cpu_to_le32(mcon->Command), vmap + VmicRfmLCSR1);
  }

  stat = (XmemDrvrScr)cdcm_le32_to_cpu(cdcm_ioread32(vmap + VmicRfmLCSR1));

  return stat;
}

/**
 * Reset - Resets a module to a default state.
 *
 * @param mcon: module context
 *
 * If the device is not alive, SYSERR is returned.
 *
 * @return OK on success
 */
static int Reset(XmemDrvrModuleContext *mcon)
{
  XmemDrvrSendBuf sbuf;

  FUNCT_TRACK;
  if (PingModule(mcon) == OK) {
    EnableInterrupts(mcon, mcon->InterruptEnable);
    GetSetStatus(mcon, mcon->Command | XmemDrvrScrDARK_ON, XmemDrvrWRITE);
    mcon->NodeId = GetRfmReg(mcon, VmicRfmNID, 1);
    Wa->Nodes = 1 << (mcon->NodeId - 1);
    /* Tell the network that I need to be initialised in case I'm connected 
       to PENDING_INIT. */
    if (mcon->InterruptEnable & XmemDrvrIntrPENDING_INIT) {
      sbuf.Module        = mcon->ModuleIndex + 1;
      sbuf.UnicastNodeId = 0;
      sbuf.MulticastMask = 0;
      sbuf.Data          = 1 << (mcon->NodeId -1);
      sbuf.InterruptType = XmemDrvrNicINITIALIZED | XmemDrvrNicBROADCAST;
      return SendInterrupt(&sbuf);
    } else return OK;
  }
  pseterr(ENXIO);
  return SYSERR;
}

/**
 * InterruptSelf - 'Simulate' an interrupt to the module itself.
 *
 * @param sbuf: pointer to struct XmemDrvrSendBuf
 * @param mcon: module context
 *
 * This is what we mean by 'simulating' an interrupt:
 * For the client, the result of an interrupt is that ccon->Semaphore
 * is signalled, and then the client reads from ccon->Queue. So for each of
 * the clients connected to module mcon, a queue entry is filled according
 * to sbuf, and then ccon->Semaphore is signalled.
 *
 */
static void InterruptSelf(XmemDrvrSendBuf *sbuf, XmemDrvrModuleContext *mcon) {

  XmemDrvrNic            ntype;
  XmemDrvrReadBuf        rbf;
  XmemDrvrClientContext *ccon;
  XmemDrvrQueue         *queue;
  unsigned long usegs, msk;
  int i;

  FUNCT_TRACK;
  bzero((void *) &rbf, sizeof(XmemDrvrReadBuf));
  usegs = 0;
  rbf.Module = sbuf->Module;
  ntype = sbuf->InterruptType & XmemDrvrNicTYPE;
  switch (ntype) {
  case XmemDrvrNicREQUEST_RESET:
    rbf.Mask = XmemDrvrIntrREQUEST_RESET;
    break;
  case XmemDrvrNicINT_1:
    rbf.Mask = XmemDrvrIntrINT_1;
    rbf.NodeId[XmemDrvrIntIdxINT_1] = mcon->NodeId;
    rbf.NdData[XmemDrvrIntIdxINT_1] = sbuf->Data;
    break;
  case XmemDrvrNicINT_2:
    rbf.Mask = XmemDrvrIntrINT_2;
    rbf.NodeId[XmemDrvrIntIdxINT_2] = mcon->NodeId;
    rbf.NdData[XmemDrvrIntIdxINT_2] = sbuf->Data;
    break;
  case XmemDrvrNicSEGMENT_UPDATE:
    rbf.Mask = XmemDrvrIntrSEGMENT_UPDATE;
    rbf.NodeId[XmemDrvrIntIdxSEGMENT_UPDATE] = mcon->NodeId;
    rbf.NdData[XmemDrvrIntIdxSEGMENT_UPDATE] = sbuf->Data;
    usegs = sbuf->Data;
    break;
  case XmemDrvrNicINITIALIZED:
    rbf.Mask = XmemDrvrIntrPENDING_INIT;
    rbf.NodeId[XmemDrvrIntIdxPENDING_INIT] = mcon->NodeId;
    rbf.NdData[XmemDrvrIntIdxPENDING_INIT] = sbuf->Data;
    break;
  default:
    break;
  }

  for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {
    ccon = &Wa->ClientContexts[i];
    if (usegs) 
      ccon->UpdatedSegments |= usegs; 
    msk = mcon->Clients[i];

    if (msk & rbf.Mask) {
      queue = &ccon->Queue;
      if (queue->Size < XmemDrvrQUEUE_SIZE) {
	queue->Entries[queue->Size++] = rbf;
	ssignal(&ccon->Semaphore); /* Wakeup client */
      }
      else {
	queue->Size = XmemDrvrQUEUE_SIZE;
	queue->Missed++;
      }
    }

  }
}

/**
 * SendInterrupt - Send an interrupt to other node(s)
 *
 * @param sbuf: pointer to the XmemDrvrSendBuf struct
 *
 * @return OK on success
 */
static int SendInterrupt(XmemDrvrSendBuf *sbuf)
{
  XmemDrvrModuleContext *mcon;
  XmemDrvrNic ntype, ncast;
  int i, msk, midx;
  void *vmap;

  FUNCT_TRACK;
  midx = sbuf->Module - 1;
  if (midx < 0 || midx >= Wa->Modules) {
    pseterr(ENXIO);
    return SYSERR;
  }

  mcon = &Wa->ModuleContexts[midx];
  vmap = mcon->Map;
  ntype = sbuf->InterruptType & XmemDrvrNicTYPE;
  ncast = sbuf->InterruptType & XmemDrvrNicCAST;

  cdcm_iowrite32(cdcm_cpu_to_le32(sbuf->Data), vmap + VmicRfmNTD);

  switch (ncast) {  

  case XmemDrvrNicBROADCAST:

    cdcm_iowrite8(XmemDrvrNicBROADCAST | ntype, vmap + VmicRfmNIC);
    InterruptSelf(sbuf, mcon);
    return OK;

  case XmemDrvrNicMULTICAST:

    for (i = 0; i < XmemDrvrNODES; i++) {
      if (i == mcon->NodeId - 1) 
	InterruptSelf(sbuf, mcon);
      else {
	msk = 1 << i;
	if (msk & sbuf->MulticastMask) {
	  cdcm_iowrite8(i + 1, vmap + VmicRfmNTN);
	  cdcm_iowrite8(ntype, vmap + VmicRfmNIC);
	}
      }
    }
    return OK;

  case XmemDrvrNicUNICAST:

    if (sbuf->UnicastNodeId == mcon->NodeId)
      InterruptSelf(sbuf, mcon);
    else {
      /* cast needed: UnicastNodeId is a LONG, although its values range 
       * from 0 to 255.
       */
      cdcm_iowrite8((unsigned char)sbuf->UnicastNodeId, vmap + VmicRfmNTN);
      cdcm_iowrite8(ntype, vmap + VmicRfmNIC);
    }
    return OK;

  default:

    pseterr(EINVAL);
    return SYSERR;

  }
}

/**
 * GetSeg - returns 1 if the segment is valid.
 *
 * @param segid: segment id (hex)
 * @param sgx: index number of the segment [0... n-1].
 *
 * Checks that the segment is valid, and also returns by reference the segment
 * index number (i.e. [0 ... n-1], where n is the number of nodes). With sgx
 * the called can access the segment matrix directly.
 *
 * @return 1 if the segment is valid; 0 otherwise.
 */
static int GetSeg(unsigned long segid, unsigned int *sgx)
{
  int i;
  int nodemsk;
  for (i = 0; i < XmemDrvrSEGMENTS; i++) {
    nodemsk = 0x1 << i;
    if (nodemsk & segid) {
      *sgx = i;
      break;
    }
  }
  if (! (Wa->SegTable.Used & nodemsk)) return 0;
  return 1;
}

/**
 * WrPermSeg - checks if the node has write access to the segment.
 *
 * @param segid: segment id
 * @param node: node [1...n]
 * @param nodeid: node id (bitmask)
 *
 * First it checks that the segment is valid; then checks if the node has
 * write access to the segment given by segid. It also returns by reference
 * the node id, which is a bitmask of the given node.
 *
 * @return 1 if node has been granted write permission, 0 otherwise.
 */
static int WrPermSeg(unsigned long segid, unsigned long node, int *nodeid)
{
  int sgx;
  XmemDrvrSegDesc *segment;

  if (! GetSeg(segid, &sgx)) return 0;
  segment = &Wa->SegTable.Descriptors[sgx];
  *nodeid = 0x1 << (node - 1);
  if (! (*nodeid & segment->Nodes)) return 0;
  return 1;
}


/**
 * PageCopy - Copies a page to/from VMIC's SDRAM using DMA.
 *
 * @param siod: contains the size and addresses of the buffers involved
 * @param iod: direction of the transfer (XmemDrvr{READ,WRITE})
 * @param mapped: 1 if the buffer has already been DMA-mapped. 0 otherwise.
 *
 * This function has to be called from a region protected by 
 * mcon->BusySemaphore, since it accesses the struct mcon->DmaOp. Therefore
 * BusySemaphore will be held at least by those regions willing to perform
 * DMA to/from the VMIC.
 * if(!mapped), the host machine will set up a DMA mapping that will return
 * the physical address of the page given in siod.
 * if(mapped), that means DmaOp has been set by the caller and therefore 
 * mcon->DmaOp contains all the necessary information to perform the DMA.
 *
 * @return OK on success
 */
static int PageCopy(XmemDrvrSegIoDesc *siod, XmemDrvrIoDir iod,
		    const int mapped)
{
  XmemDrvrModuleContext *mcon;
  XmemDrvrSegDesc *sgds;
  unsigned int midx, sgx, myid;
  char *sram;
  unsigned long dmamode, dptr, cmdma, intcsr;

  FUNCT_TRACK;
  sgx = 0;
  midx = siod->Module - 1;
  if (siod->UserArray == NULL && !mapped) goto address_err;
  if (midx < 0 || midx >= Wa->Modules) goto device_err;
  if (! siod->Id) goto device_err;
  if (! GetSeg(siod->Id, &sgx)) goto nosegment;

  mcon = &Wa->ModuleContexts[midx];
  DrmLocalReadWrite(mcon, PlxLocalINTCSR, &intcsr, 4, XmemDrvrREAD);
  intcsr |= PlxIntcsrENABLE_PCI 
    | PlxIntcsrENABLE_LOCAL
    | PlxIntcsrENABLE_DMA_CHAN_0
    | PlxIntcsrENABLE_DMA_CHAN_1;
  DrmLocalReadWrite(mcon, PlxLocalINTCSR, &intcsr, 4, XmemDrvrWRITE);

  sgds = &Wa->SegTable.Descriptors[sgx];
  sram = sgds->Address + siod->Offset;
  if (! mapped) {
    mcon->DmaOp.Flag = iod == XmemDrvrWRITE;
    mcon->DmaOp.Len = siod->Size;
    mcon->DmaOp.Buffer = siod->UserArray;
    mcon->DmaOp.Dma = cdcm_pci_map(mcon, siod->UserArray, 
				   mcon->DmaOp.Len, mcon->DmaOp.Flag);
  }

#ifdef __TEST_TT__
#ifdef __linux__
      getnstimeofday(&t_a2);
#else
      tt_na2 = nanotime(&tt_a2);
#endif /* !__linux__ */
#endif /* __TEST_TT__ */

      /* FIXME: code is repeated below. Do it properly! */
  switch (iod) {
  case XmemDrvrWRITE:  
    /* Write to the VMIC's SDRAM from host machine
     * We use the PLX' DMACHANNEL1 for writing
     */
    if (! WrPermSeg(siod->Id, mcon->NodeId, &myid)) goto access_err;
    dmamode = PlxDmaMode16BIT
      | PlxDmaMode32BIT
      | PlxDmaModeINP_ENB
      | PlxDmaModeCONT_BURST_ENB
      | PlxDmaModeLOCL_BURST_ENB
      | PlxDmaModeINT_PIN_SELECT
      | PlxDmaModeDONE_INT_ENB;
    DrmLocalReadWrite(mcon, PlxLocalDMAMODE1, &dmamode, 4, XmemDrvrWRITE);
    DrmLocalReadWrite(mcon, PlxLocalDMAPADR1, (unsigned long *)&mcon->DmaOp.Dma,
		      4, XmemDrvrWRITE);
    DrmLocalReadWrite(mcon, PlxLocalDMALADR1, (unsigned long *)&sram, 4, 
		      XmemDrvrWRITE);
    DrmLocalReadWrite(mcon, PlxLocalDMASIZ1, (unsigned long *)&mcon->DmaOp.Len,
		      4, XmemDrvrWRITE);
    /* Set transfer direction and termination interrupt */
    dptr = PlxDmaDprTERM_INT_ENB & ~PlxDmaDprDIR_LOC_PCI; 
    DrmLocalReadWrite(mcon, PlxLocalDMADPR1, &dptr, 4, XmemDrvrWRITE);

#ifdef __TEST_TT__
#ifdef __linux__
      getnstimeofday(&t_a3);
#else
      tt_na3 = nanotime(&tt_a3);
#endif /* !__linux__ */
#endif /* __TEST_TT__ */

    mcon->WrTimer = timeout(WrDmaTimeout, (char *) mcon, XmemDrvrDMA_TIMEOUT);
    if (mcon->WrTimer < 0) {
      mcon->WrTimer = 0;
      pseterr(EBUSY);
      return SYSERR;
    }

    sreset(&mcon->WrDmaSemaphore);

    cmdma = PlxDmaCsrENABLE | PlxDmaCsrSTART; /* Command: Start transfer */
    DrmLocalReadWrite(mcon, PlxLocalDMACSR1, &cmdma, 1, XmemDrvrWRITE);

    if (swait(&mcon->WrDmaSemaphore, SEM_SIGABORT)) {
      sreset(&mcon->WrDmaSemaphore);
      pseterr(EINTR);
      return SYSERR;
    }
    if (mcon->WrTimer) {
      CancelTimeout(&mcon->WrTimer);
    }
    else {
      cmdma = 0;
      DrmLocalReadWrite(mcon, PlxLocalDMACSR1, &cmdma, 1, XmemDrvrREAD);
      if ((cmdma & PlxDmaCsrDONE) == 0) {
	pseterr(EBUSY);
	return SYSERR;
      }
    }
    if (! mapped)  /* mapping has to be cleared */
      cdcm_pci_unmap(mcon, mcon->DmaOp.Dma, mcon->DmaOp.Len, mcon->DmaOp.Flag);
    
    return OK;
    break;

  case XmemDrvrREAD:
    /* Read from the VMIC's SDRAM to host machine
     * We use the PLX' DMACHANNEL0 for writing
     */
    dmamode = PlxDmaMode16BIT
      | PlxDmaMode32BIT
      | PlxDmaModeINP_ENB
      | PlxDmaModeCONT_BURST_ENB
      | PlxDmaModeLOCL_BURST_ENB
      | PlxDmaModeINT_PIN_SELECT
      | PlxDmaModeDONE_INT_ENB;

    DrmLocalReadWrite(mcon, PlxLocalDMAMODE0, &dmamode, 4, XmemDrvrWRITE);
    DrmLocalReadWrite(mcon, PlxLocalDMAPADR0, (unsigned long *)&mcon->DmaOp.Dma,
		      4, XmemDrvrWRITE);
    DrmLocalReadWrite(mcon, PlxLocalDMALADR0, (unsigned long *)&sram, 4,
		      XmemDrvrWRITE);
    DrmLocalReadWrite(mcon, PlxLocalDMASIZ0, (unsigned long *)&mcon->DmaOp.Len,
		      4, XmemDrvrWRITE);
    /* Set transfer direction and termination interrupt */
    dptr = PlxDmaDprTERM_INT_ENB | PlxDmaDprDIR_LOC_PCI; 
    DrmLocalReadWrite(mcon,PlxLocalDMADPR0 ,&dptr, 4, XmemDrvrWRITE);

    mcon->RdTimer = timeout(RdDmaTimeout, (char *)mcon, XmemDrvrDMA_TIMEOUT);
    if (mcon->RdTimer < 0) {
      mcon->RdTimer = 0;
      pseterr(EBUSY);
      return SYSERR;
    }
    sreset(&mcon->RdDmaSemaphore);

    cmdma = PlxDmaCsrENABLE | PlxDmaCsrSTART; /* Command: Start transfer */
    DrmLocalReadWrite(mcon, PlxLocalDMACSR0, &cmdma, 1, XmemDrvrWRITE);

    if (swait(&mcon->RdDmaSemaphore, SEM_SIGABORT)) {
      sreset(&mcon->RdDmaSemaphore);
      pseterr(EINTR);
      return SYSERR;
    }
    if (mcon->RdTimer)
      CancelTimeout(&mcon->RdTimer);
    else {
      cmdma = 0;
      DrmLocalReadWrite(mcon, PlxLocalDMACSR0, &cmdma, 1, XmemDrvrREAD);
      if ((cmdma & PlxDmaCsrDONE) == 0) {
	pseterr(EBUSY);
	return SYSERR;
      }
    }
    if (! mapped)  /* mapping has to be cleared */
      cdcm_pci_unmap(mcon, mcon->DmaOp.Dma, mcon->DmaOp.Len, mcon->DmaOp.Flag);

    return OK;
    break;

  default:
    goto arg_err;
  }      
 access_err:
  cprintf("xmemDrvr:PageCopy: Write permission denied for segment ID=0x%X.\n",
	  (unsigned int)siod->Id);
  pseterr(EACCES);
  return SYSERR;
 address_err:
  cprintf("xmemDrvr:PageCopy: Illegal address.\n");
  pseterr(EFAULT);
  return SYSERR;
 arg_err:
  cprintf("xmemDrvr:PageCopy: Wrong argument.\n");
  pseterr(EINVAL);
  return SYSERR;
 device_err:
  cprintf("xmemDrvr:PageCopy: No such device.");
  pseterr(ENODEV);
  return SYSERR;
 nosegment:
  cprintf("xmemDrvr:PageCopy:No such segment defined:%d.\n", sgx);
  pseterr(ENXIO);
  return SYSERR;
}

/* we set a global variable for this, so that the stack is not filled up.
 * since the region that operates on dmachain is protected by 
 * mcon->BusySemaphore, we're safe.
 */
#define MAX_DMA_CHAIN XmemDrvrMAX_SEGMENT_SIZE/PAGESIZE+1
struct dmachain dmac[MAX_DMA_CHAIN];

/**
 * SegmentCopy - copies a segment to/from the VMIC's SDRAM.
 *
 * @param siod: contains the information of the transfer to be done
 * @param iod: direction: {XmemDrvrREAD,XmemDrvrWRITE}
 * @param ccon: client context
 * @param mcon: module context
 *
 * If the size of the transfer is >= the DMA threshold, the physical pages
 * of the user's buffer are found and passed to PageCopy(). 
 * If the size is smaller than the threshold, I/O is CPU-based.
 *
 * @return OK on success
 */
static int SegmentCopy(XmemDrvrSegIoDesc *siod, XmemDrvrIoDir iod,
		       XmemDrvrClientContext *ccon, XmemDrvrModuleContext *mcon)
{
  XmemDrvrSendBuf  sbuf;
  XmemDrvrSegIoDesc sio;
  XmemDrvrSegDesc *sgds;
  int err;
  unsigned int pg, pgs, myid, sgx, sram;

  FUNCT_TRACK;
  err = OK;

  swait(&mcon->BusySemaphore, SEM_SIGIGNORE); /* acquire the mutex */
  /* Inside protected region ("Busy" state). */

  if (siod->UserArray == NULL) goto address_err;
  if (! siod->Id) goto nodevice;
  if (! GetSeg(siod->Id, &sgx)) goto nosegment;

  bzero((void *)&sio, sizeof(XmemDrvrSegIoDesc));
  sio.Module = siod->Module;
  sio.Id     = siod->Id;
  sio.Offset = siod->Offset;

  if (siod->Size >= Wa->DmaThreshold) {
    /* |--> then siod->UserArray is a pointer to user space. */
    /* Map and lock the pages of the user's buffer. */
    pgs = cdcm_pci_mmchain_lock(mcon->Handle, &mcon->Dma, iod == XmemDrvrWRITE,
				ccon->Pid, siod->UserArray, siod->Size, dmac);
    if (ccon->Debug)
      cprintf("xmemDrvr: Page count from cdcm_pci_mmchain_lock: %d.\n", pgs);
    if (pgs <= 0 || pgs > MAX_DMA_CHAIN) goto badchain;

    mcon->DmaOp.Flag = iod == XmemDrvrWRITE;
    mcon->DmaOp.Buffer = siod->UserArray;

    for (pg=0; pg<pgs; pg++) {

      mcon->DmaOp.Dma = dmac[pg].address; /* this is a portable type */
      mcon->DmaOp.Len = dmac[pg].count;

      if (ccon->Debug) {
	cprintf("xmemDrvr:SegmentCopy:%d Addr:0x%X Size:%d Offset:0x%x\n",
		pg + 1, (int)mcon->DmaOp.Dma, (int)mcon->DmaOp.Len, 
		(int)sio.Offset);
      }
#ifdef __TEST_TT__
#ifdef __linux__
      getnstimeofday(&t_a1);
#else
      tt_na1 = nanotime(&tt_a1);
#endif /* !__linux__ */
#endif /* __TEST_TT__ */

      err = PageCopy(&sio, iod, 1); /* mapped = 1 */

      if (err < 0) break;
      sio.Offset += dmac[pg].count;
    }
    /* clear SG mapping (which also unlocks the pages) */
    cdcm_pci_mem_unlock(mcon->Handle, &mcon->Dma, ccon->Pid, 0);
  }
  else { /* siod->Size < Wa->DmaThreshold */
    /* Here, siod->UserArray is a pointer to kernel space. */
    sgds = &Wa->SegTable.Descriptors[sgx];
    sram = (unsigned int) sgds->Address + siod->Offset;

    if (iod == XmemDrvrWRITE) {
      if (! WrPermSeg(siod->Id, mcon->NodeId, &myid)) goto access_err;
      LongCopyToXmem(mcon->SDRam + sram, siod->UserArray, siod->Size);
    }
    else 
      LongCopyFromXmem(mcon->SDRam + sram, siod->UserArray, siod->Size);
  }

  if (err == OK && iod == XmemDrvrWRITE && siod->UpdateFlg) {
    sbuf.Module        = siod->Module;
    sbuf.UnicastNodeId = 0;
    sbuf.MulticastMask = 0;
    sbuf.Data          = siod->Id;
    sbuf.InterruptType = XmemDrvrNicSEGMENT_UPDATE | XmemDrvrNicBROADCAST;
    err = SendInterrupt(&sbuf);
  }

  ssignal(&mcon->BusySemaphore); /* release the mutex */

  return err;

 access_err:
  cprintf("xmemDrvr:SegmentCopy: Write permission denied, segment ID=0x%X.\n",
	  (unsigned int)siod->Id);
  ssignal(&mcon->BusySemaphore);
  pseterr(EACCES);
  return SYSERR;
 address_err:
  cprintf("xmemDrvr:SegmentCopy: illegal address.\n");
  ssignal(&mcon->BusySemaphore);
  pseterr(EFAULT);
  return SYSERR;
 badchain:
  cprintf("xmemDrvr:SegmentCopy:Bad DMA chain list length:%d [1..%lu]\n",
	  pgs, MAX_DMA_CHAIN);
  ssignal(&mcon->BusySemaphore);
  pseterr(ERANGE);
  return SYSERR;
 nodevice:
  cprintf("xmemDrvr:SegmentCopy:No such device.\n");
  ssignal(&mcon->BusySemaphore);
  pseterr(ENODEV);
  return SYSERR;
 nosegment:
  cprintf("xmemDrvr:SegmentCopy:No such segment defined:%d.\n", (int)sgx);
  ssignal(&mcon->BusySemaphore);
  pseterr(ENXIO);
  return SYSERR;
}

/**
 * FlushSegments - flushes segments to network
 *
 * @param mcon: module context
 * @param mask: bitmask which contains which segments need to be flushed
 *
 * two mutexes are acquired in this functions: mcon->TempbufSemaphore and 
 * mcon->BusySemaphore. The former provides  mutually exclusive access to 
 * mcon->Tempbuf (which is used as a cache to send the segments in pieces)
 * and the latter has to be acquired before any call to PageCopy().
 *
 * @return OK on success
 */
static int FlushSegments(XmemDrvrModuleContext *mcon, unsigned long mask)
{
  XmemDrvrSegIoDesc  sio;
  XmemDrvrSegDesc   *sgds;
  XmemDrvrSendBuf    sbuf;
  unsigned long msk, umsk;
  int i, pg, err, rsze;

  FUNCT_TRACK;

  swait(&mcon->TempbufSemaphore, SEM_SIGIGNORE); /* acquire the mutex */
  /* Tempbuf has been acquired */
  swait(&mcon->BusySemaphore, SEM_SIGIGNORE); /* acquire the mutex */
  /* Inside protected region ("Busy" state). */


  /* FIXME: So much unnecessary nesting. */
  umsk = 0;
  for (i = 0; i < XmemDrvrSEGMENTS; i++) {
    msk = 1 << i;
    if ((msk & mask) && (msk & Wa->SegTable.Used)) {
      umsk |= msk;
      sgds = &Wa->SegTable.Descriptors[i];

      bzero((void *) &sio, sizeof(XmemDrvrSegIoDesc));
      sio.Module = mcon->ModuleIndex + 1;
      sio.Id = msk;
      sio.UserArray = mcon->Tempbuf;
      sio.Offset = 0;
   
      /* chop up the segment in pagesized chunks and send them out */
      for (pg = 0; pg < sgds->Size; pg += PAGESIZE) {
	if (sgds->Size > pg) rsze = sgds->Size - pg;
	else                 rsze = sgds->Size;
	if (rsze > PAGESIZE) rsze = PAGESIZE;
	sio.Offset = pg;
	sio.Size = rsze;

	err = PageCopy(&sio, XmemDrvrREAD, 0); /* mapped = 0 */
	if (err < 0) {
	  ssignal(&mcon->BusySemaphore);
	  ssignal(&mcon->TempbufSemaphore);
	  return err;
	}
	err = PageCopy(&sio, XmemDrvrWRITE, 0); /* mapped = 0 */
	if (err < 0) {
	  ssignal(&mcon->BusySemaphore);
	  ssignal(&mcon->TempbufSemaphore);
	  return err;
	}
      }
    }
  }

  /* release mutexes */
  ssignal(&mcon->BusySemaphore);
  ssignal(&mcon->TempbufSemaphore);

  if (umsk) {
    sbuf.Module        = mcon->ModuleIndex + 1;
    sbuf.UnicastNodeId = 0;
    sbuf.MulticastMask = 0;
    sbuf.Data          = umsk;
    sbuf.InterruptType = XmemDrvrNicSEGMENT_UPDATE | XmemDrvrNicBROADCAST;
    return SendInterrupt(&sbuf);
  }
  return OK;
}

/**
 * RfmIo - read/writes VMIC RFM registers
 *
 * @param mcon: module context
 * @param riob: description of the block on which we want to perform I/O
 * @param flag: read/write flag
 *
 * This function uses the CDCM functions to access the I/O mapped memory.
 * Note that the memory barriers are not necessary since cdcm_{read,write}X 
 * already include them. 
 * Pay attention as well to the endianness: cdcm_cpu_to_{le,be}X ensure 
 * portable code. This is _address invariance_, which requires the 
 * PLX to be set to little endian (i.e. the PLX doesn't swap anything).
 *
 * @return OK on success
 */
static int RfmIo(XmemDrvrModuleContext *mcon, XmemDrvrRawIoBlock *riob,
		 XmemDrvrIoDir flag)
{

  unsigned long *vmap;
  unsigned long *uary;
  void *ioaddr;
  char *iod;

  int rval; /* Return value */
  int i = 0;
  int itms, offs;
  unsigned long ps;
  XmemDrvrSize size;

  FUNCT_TRACK;

  rval = OK;

  /* shorter names for the riob variables */
  uary = riob->UserArray;
  size = riob->Size;
  offs = riob->Offset;
  itms = riob->Items;

  vmap = (unsigned long *)mcon->Map;
  ioaddr = (void *)vmap;

  if (itms == 0) itms = 1;

  if (!recoset()) {         /* Catch bus errors */

    for (i = 0; i < itms; i++) { 

      if (flag == XmemDrvrWRITE) {
	switch (size) {
	case XmemDrvrBYTE:
	  cdcm_iowrite8((u_int8_t)uary[i], ioaddr + offs);
	  break;
	case XmemDrvrWORD:
	  cdcm_iowrite16(cdcm_cpu_to_le16((u_int16_t)uary[i]), ioaddr + offs);
	  break;
	case XmemDrvrLONG:
	  cdcm_iowrite32(cdcm_cpu_to_le32((u_int32_t)uary[i]), ioaddr + offs);
	  break;
	}
      } else {
	switch (riob->Size) {
	case XmemDrvrBYTE:
	  uary[i] = (unsigned long)cdcm_ioread8(ioaddr + offs);
	  break;
	case XmemDrvrWORD:
	  uary[i] = (unsigned long)cdcm_le16_to_cpu(cdcm_ioread16(ioaddr + offs));
	  break;
	case XmemDrvrLONG:
	  uary[i] = (unsigned long)cdcm_le32_to_cpu(cdcm_ioread32(ioaddr + offs));
	  break;
	}
      }
      offs += (int) size;
    }
  } else {
    disable(ps);

    if (flag == XmemDrvrWRITE)
      iod = "Write";
    else 
      iod = "Read";
    kkprintf("xmemDrvr: BUS-ERROR: Module:%d Addr:%x Dir:%s Data:%d\n",
	     (int)mcon->ModuleIndex + 1,(int)ioaddr + offs, iod, (int)uary[i]);

    pseterr(ENXIO); /* No such device or address */
    rval = SYSERR;
    restore(ps);
  }
  noreco(); /* Remove local bus trap */
  return rval;
}


/**
 * GetRfmReg - reads a register from the VMIC's RFM
 *
 * @param mcon: module context 
 * @param reg: register offset, from the VmicRfm enum
 * @param size: bytes to be read
 *
 * This is just a wrapper for a single read call through RfmIo().
 *
 * @return the register's value (unsigned long format)
 */
unsigned long GetRfmReg(XmemDrvrModuleContext *mcon, VmicRfm reg, int size)
{
  XmemDrvrRawIoBlock ioblk;
  unsigned long res;

  FUNCT_TRACK;

  ioblk.Items = 1;
  ioblk.Size = size;
  ioblk.Offset = reg;
  ioblk.UserArray = &res;
  RfmIo(mcon, &ioblk, XmemDrvrREAD);
  return res;
}

/**
 * SetRfmReg - sets the value of a VMIC's RFM's register
 *
 * @param mcon: module context
 * @param reg: register offset, from the VmicRfm enum
 * @param size: bytes to be written
 * @param data: value to be written into the register
 *
 * This is just a wrapper for a single write operation through RfmIo().
 *
 */
void SetRfmReg(XmemDrvrModuleContext *mcon, VmicRfm reg, int size,
	       unsigned long data)
{
  XmemDrvrRawIoBlock ioblk;

  FUNCT_TRACK;
  ioblk.Items = 1;
  ioblk.Size = size;
  ioblk.Offset = reg;
  ioblk.UserArray = &data;
  RfmIo(mcon,&ioblk,XmemDrvrWRITE);
}

/**
 * RawIo - Raw IO operations to the VMIC SDRam, in chunks of 4 bytes only.
 *
 * @param mcon: module context
 * @param riob: raw IO operation struct
 * @param flag: write to module if flag==XmemDrvrWRITE; read otherwise.
 *
 * Note that since this function is not meant to be called by a client,
 * access rights to the segment are not checked -- in other words, this 
 * function is only meant to be used by the driver's test program.
 *
 * @return OK on success
 */
static int RawIo(XmemDrvrModuleContext *mcon, XmemDrvrRawIoBlock *riob,
		 XmemDrvrIoDir flag)
{
  unsigned long *vmap; /* Module Memory map */
  unsigned long *uary;
  
  int rval; /* Return value */
  int i = 0;
  int itms, offs, lidx = 0;
  unsigned long ps;   /* Processor status word */
  char *iod;

  FUNCT_TRACK;

  rval = OK;

  vmap = (unsigned long *) mcon->SDRam;
  uary = riob->UserArray;
  offs = riob->Offset / sizeof(u_int32_t);
  itms = riob->Items;

  if (itms == 0) itms = 1;
  if (flag == XmemDrvrWRITE) 
    iod = "Write";
  else
    iod = "Read";

  if (!recoset()) { /* Catch bus errors */

    if (flag == XmemDrvrWRITE)
      LongCopyToXmem(  vmap + offs, riob->UserArray, itms*sizeof(u_int32_t));
    else 
      LongCopyFromXmem(vmap + offs, riob->UserArray, itms*sizeof(u_int32_t));

  } else {
    disable(ps);

    kkprintf("xmemDrvr: BUS-ERROR: Module:%d Addr:%x Dir:%s Data:%d\n",
	     (int)mcon->ModuleIndex+1, (int)&vmap[lidx], iod,(int) uary[i]);

    pseterr(ENXIO); /* No such device or address */
    rval = SYSERR;
    restore(ps);
  }
  noreco(); /* Remove local bus trap */
  return rval;
}

/**
 * Connect - Connect to a VMIC interrupt
 *
 * @param conx: contains the interrupt bitmask to connect to.
 * @param ccon: client context.
 *
 * The client is connect to the VMIC interrupt given by the bitmask
 * conx->Mask.
 *
 * @return EnableInterrupt(), OK on success.
 */
static int Connect(XmemDrvrConnection *conx, XmemDrvrClientContext *ccon)
{
  XmemDrvrModuleContext *mcon;
  int midx;

  FUNCT_TRACK;

  /* Get the module to make the connection on */
  midx = conx->Module - 1;
  if (midx < 0 || midx >= Wa->Modules)
    midx = ccon->ModuleIndex;

  mcon = &Wa->ModuleContexts[midx];

  mcon->Clients[ccon->ClientIndex] |= conx->Mask;
  return EnableInterrupts(mcon, conx->Mask);
}

/**
 * DisConnect - Disconnect from a VMIC interrupt
 *
 * @param conx: contains the interrupt bitmask to disconnect from.
 * @param ccon: client context.
 *
 * Disconnects a client from a VMIC interrupt, and if it was the only one
 * connected to it, disables that interrupts in the module.
 *
 * @return DisableInterrupts(), OK on success.
 */
static int DisConnect(XmemDrvrConnection *conx, XmemDrvrClientContext *ccon)
{
  XmemDrvrModuleContext *mcon;
  int i, midx;

  FUNCT_TRACK;

  /* Get the module to disconnect from */
  midx = conx->Module - 1;
  if (midx < 0 || midx >= Wa->Modules)
    midx = ccon->ModuleIndex;

  mcon  = &Wa->ModuleContexts[midx];

  mcon->Clients[ccon->ClientIndex] &= ~conx->Mask;

  /* Check to see if others are connected */
  for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {
    if (conx->Mask & mcon->Clients[i]) 
      return OK;
  }

  return DisableInterrupts(mcon, conx->Mask);
}

/**
 * DisConnectAll - Disconnect a client from all VMIC interrupts from all the
 * modules.
 *
 * @param ccon: client context
 *
 */
static void DisConnectAll(XmemDrvrClientContext *ccon) {

  XmemDrvrConnection     conx;
  XmemDrvrModuleContext *mcon;
  int i, cidx;

  FUNCT_TRACK;

  cidx = ccon->ClientIndex;
  for (i = 0; i < Wa->Modules; i++) {
    mcon = &Wa->ModuleContexts[i];
    if (mcon->Clients[cidx]) {
      conx.Module = i + 1;
      conx.Mask = mcon->Clients[cidx];
      DisConnect(&conx, ccon);
    }
  }
}

/* FIXME: Hack below needed until we fix drm_register_isr in the CDCM */

/**
 * IntrHandler - Interrupt handler
 *
 * @param m: module context
 *
 * The interrupt handler checks the source of the interrupt and interacts
 * with the clients accordingly.
 * In order to serve interrupts that are coming at a fast rate, the interrupt
 * status register (LISR) is checked and cleared in a while loop -- i.e. the
 * driver might serve more than one interrupt without exiting IntrHandler().
 * This approach has a serious drawback: LISR is read and cleared in a
 * non-atomic way!
 *
 */
#ifdef __linux__
cdcm_irqret_t IntrHandler(int irq, void *m)
{
#else /* LynxOS */
cdcm_irqret_t IntrHandler(void *m)
{
#endif /* !__linux__ */

  unsigned long isrc, msk, i;
  void *vmap;
  XmemDrvrQueue         *queue;
  XmemDrvrClientContext *ccon;
  XmemDrvrReadBuf        rbf;
  XmemDrvrSendBuf        sbuf __attribute__((__unused__));
  unsigned long          intcsr, usegs, node, data, dma;
  XmemDrvrModuleContext *mcon = (XmemDrvrModuleContext*)m;

  FUNCT_TRACK;

  DrmLocalReadWrite(mcon, PlxLocalINTCSR, &intcsr, 4, XmemDrvrREAD);

  if (intcsr & PlxIntcsrSTATUS_LOCAL) {

    vmap = mcon->Map;

    /* read the interrupt status register (LISR) */
    isrc = cdcm_le32_to_cpu(cdcm_ioread32(vmap + VmicRfmLISR));
    /* 
     * Clear LISR.
     * This is not done atomically --> it's dangerous.
     */
    cdcm_iowrite32(cdcm_cpu_to_le32(VmicLisrENABLE), vmap + VmicRfmLISR);

    isrc &= VmicLisrSOURCE_MASK; /* clean the bitmask */


    while (isrc) { /* While pending interrupts */

      bzero((void *) &rbf, sizeof(XmemDrvrReadBuf));
      rbf.Module = mcon->ModuleIndex + 1;
      usegs = 0;

      for (i = 0; i < VmicLisrSOURCES; i++) {
	msk = 1 << i;

	if (! (msk & isrc)) break;

	rbf.Mask |= msk;

	switch (msk) {
	  /*
	   * VmicLisrPARITY_ERROR: Parity error 
	   * VmicLisrWRITE_ERROR: Cant write a short or byte if parity on 
	   * VmicLisrLOST_SYNC: PLL unlocked, data was lost, or signal lost 
	   * VmicLisrRX_OVERFLOW: Receiver buffer overflow 
	   * VmicLisrRX_ALMOST_FULL: Receive buffer almost full 
	   * VmicLisrDATA_ERROR: Bad data received, error 
	   * VmicLisrROGUE_CLOBBER: This rogue master has clobbered a rogue packet 
	   * VmicLisrRESET_RQ: Reset me request from some other node 
	   */
	case VmicLisrPARITY_ERROR:
	case VmicLisrWRITE_ERROR:
	case VmicLisrLOST_SYNC:
	case VmicLisrRX_OVERFLOW:
	case VmicLisrRX_ALMOST_FULL:
	case VmicLisrDATA_ERROR:
	case VmicLisrROGUE_CLOBBER:
	case VmicLisrRESET_RQ:

	  /* do nothing */

	  break;


	  /*
	   * When a node comes up it sends out a pending-init interrupt
	   * with a non zero data field. Any client on a node connected
	   * to the pending-init interrupt will send back another init
	   * interrupt but with a zero data field. In this way, we will
	   * establish a node map of nodes connected to pending-init,
	   * and hence an algorithm on the client side to handle these
	   * interrupts can be established based on a knowledge of all
	   * nodes present and interested in pending-init interrupts.
	   */
	case VmicLisrPENDING_INIT:

	  data = cdcm_le32_to_cpu(cdcm_ioread32(vmap + VmicRfmINITD));
	  rbf.NdData[XmemDrvrIntIdxPENDING_INIT] = data;
	  
	  node = GetRfmReg(mcon, VmicRfmINITN, 1);
	  rbf.NodeId[XmemDrvrIntIdxPENDING_INIT] = node;

	  Wa->Nodes |= (1 << (node - 1)); /* update known nodes */
	    
	  /*
	   * If someone needs initializing, I broadcast back a pending init
	   * with zero data. Proceeding this way all nodes will know who is
	   * on-line. Everyone else will do the same, so eventually all the
	   * nodes will have introduced themselves. 
	   * Note that 'data' is tested to be not zero -- otherwise the 
	   * protocol would never end.
	   */

	  if (! data) break;

	  /* reset known nodes to the only two I know for the time being */
	  Wa->Nodes = (1 << (mcon->NodeId - 1)) | (1 << (node - 1));

#if 0     /* 
	   * commented out:
	   * it corresponds to the daemons to apply a particular policy.
	   */
	  sbuf.Module = mcon->ModuleIndex + 1;
	  sbuf.UnicastNodeId = 0;
	  sbuf.MulticastMask = 0;
	  sbuf.Data          = 0; /* Send zero data */
	  sbuf.InterruptType = XmemDrvrNicINITIALIZED | XmemDrvrNicBROADCAST;
#endif

	  break;


	  /* INT3 is SEGMENT_UPDATE, it's not for general use. */
	case VmicLisrINT3:

	  rbf.NdData[XmemDrvrIntIdxSEGMENT_UPDATE] = 
	    cdcm_le32_to_cpu(cdcm_ioread32(vmap + VmicRfmISD3));

	  node = GetRfmReg(mcon, VmicRfmSID3, 1);
	  rbf.NodeId[XmemDrvrIntIdxSEGMENT_UPDATE] = node;

	  /* pick up updated segments */
	  usegs = rbf.NdData[XmemDrvrIntIdxSEGMENT_UPDATE];

	  Wa->Nodes |= 1 << (node - 1); /* update known nodes */

	  break;


	case VmicLisrINT2:

	  rbf.NdData[XmemDrvrIntIdxINT_2] =
	    cdcm_le32_to_cpu(cdcm_ioread32(vmap + VmicRfmISD2));

	  node = GetRfmReg(mcon, VmicRfmSID2, 1);
	  rbf.NodeId[XmemDrvrIntIdxINT_2] = node;

	  Wa->Nodes |= 1 << (node - 1); /* update known nodes */
	  break;

	case VmicLisrINT1:

	  rbf.NdData[XmemDrvrIntIdxINT_1] = 
	    cdcm_le32_to_cpu(cdcm_ioread32(vmap + VmicRfmISD1));

	  node = GetRfmReg(mcon, VmicRfmSID1, 1);
	  rbf.NodeId[XmemDrvrIntIdxINT_1] = node;

	  Wa->Nodes |= 1 << (node -1); /* update known nodes */
	  break;

	default:
	  rbf.Mask = 0;
	  break;
	}
      }

      for (i=0; i<XmemDrvrCLIENT_CONTEXTS; i++) {

	ccon = &Wa->ClientContexts[i];

	if (usegs) { /* Or in updated segments in clients */
	  ccon->UpdatedSegments |= usegs; 
	}

	msk = mcon->Clients[i];

	if (!(msk & isrc)) break;

	queue = &ccon->Queue;
	if (queue->Size < XmemDrvrQUEUE_SIZE) {
	  queue->Entries[queue->Size++] = rbf; 
	  ssignal(&ccon->Semaphore); /* Wakeup client */
	} else {
	  queue->Size = XmemDrvrQUEUE_SIZE;
	  queue->Missed++;
	}

      } /* gone through all the interrupt sources */

      /* 
       * Read the interrupt status register (LISR).
       * As said before, this is non-atomic --> dangerous!
       */
      isrc = cdcm_le32_to_cpu(cdcm_ioread32(vmap + VmicRfmLISR));
      cdcm_iowrite32(cdcm_cpu_to_le32(VmicLisrENABLE), vmap + VmicRfmLISR);
      isrc &= VmicLisrSOURCE_MASK; /* re-evaluated in the while() */
    }

    /* 
     * Update enabled interrupts -- this is done using Connect(), I think this
     * is unnecessary here.
     */
    cdcm_iowrite32(cdcm_cpu_to_le32(mcon->InterruptEnable), vmap + VmicRfmLIER);
  }

  if (intcsr & PlxIntcsrSTATUS_DMA_CHAN_0) {   /* DMA Channel 0 for reading */

    dma = PlxDmaCsrCLEAR_INTERRUPT;
    DrmLocalReadWrite(mcon, PlxLocalDMACSR0, &dma, 1, XmemDrvrWRITE);

    ssignal(&mcon->RdDmaSemaphore); /* wake up caller */

#ifdef __TEST_TT__
#ifdef __linux__
    getnstimeofday(&t_a4);
#else
    tt_na4 = nanotime(&tt_a4);
#endif /* !__linux__ */
#endif /* __TEST_TT__ */

  }

  if (intcsr & PlxIntcsrSTATUS_DMA_CHAN_1) {   /* DMA Channel 1 for writing */

    dma = PlxDmaCsrCLEAR_INTERRUPT;
    DrmLocalReadWrite(mcon, PlxLocalDMACSR1, &dma, 1, XmemDrvrWRITE);

    ssignal(&mcon->WrDmaSemaphore); /* wake up caller */

#ifdef __TEST_TT__
#ifdef __linux__
    getnstimeofday(&t_a4);
#else
    tt_na4 = nanotime(&tt_a4);
#endif /* !__linux__ */
#endif /* __TEST_TT__ */

  }

  /* under Lynx + PowerPC, this is necessary to trigger the interrupts */
#if (!defined(__linux__) && defined(__powerpc__))
  iointunmask(mcon->IVector);
#endif

  /* 
   * FIXME: this is part of the hack related to the function definition.
   * It should simply be as it is under LynxOs (i.e. void return)
   */
  return CDCM_IRQ_HANDLED;
}


/**
 * XmemDrvrOpen - open service access point for the user
 *
 * @param s: working area
 * @param dnm: device number
 * @param flp: file pointer
 *
 * @return OK on success
 */
int XmemDrvrOpen(void *s, int dnm, struct file *flp)
{
  int cnum;                       /* Client number */
  XmemDrvrClientContext *ccon;    /* Client context */
  XmemDrvrQueue         *queue;   /* Client queue */
  XmemDrvrWorkingArea *wa = (XmemDrvrWorkingArea *)s;

  /* 
   * We allow one client per minor device, we use the minor device
   * number as an index into the client contexts array.
   */

  cnum = minor(flp->dev) - 1;
  if (cnum < 0 || cnum >= XmemDrvrCLIENT_CONTEXTS) {
    pseterr(EFAULT);
    return SYSERR;
  }

  ccon = &wa->ClientContexts[cnum];

  if (ccon->InUse) {
    pseterr(EBUSY);
    return SYSERR;
  }

  /* Initialize a client context */
  bzero((void *)ccon, sizeof(XmemDrvrClientContext));
  ccon->ClientIndex     = cnum;
  ccon->Timeout         = XmemDrvrDEFAULT_TIMEOUT;
  ccon->InUse           = 1;
  ccon->Pid             = getpid();
  ccon->UpdatedSegments = 0;
  ccon->Debug           = XmemDrvrDebugNONE;

  queue = &ccon->Queue;
  queue->Size   = 0;
  queue->Missed = 0;
  sreset(&ccon->Semaphore);

#if 0
  cprintf("xmemDrvr:Open:%d ClientSem:0x%X:%d PID:%d\n",
	  cnum,&ccon->Semaphore,ccon->Semaphore,ccon->Pid);
#endif

  return OK;
}

/**
 * XmemDrvrClose - routine called to close the device's access point
 *
 * @param s: working area
 * @param flp: file pointernn
 *
 * @return OK on success
 */
int XmemDrvrClose(void *s, struct file *flp)
{
  int cnum;                    /* Client number */
  XmemDrvrClientContext *ccon; /* Client context */
  XmemDrvrWorkingArea *wa __attribute__((__unused__)) = (XmemDrvrWorkingArea*)s;

  /* 
   * We allow one client per minor device, we use the minor device
   * number as an index into the client contexts array.
   */

  cnum = minor(flp->dev) - 1;

  if (cnum < 0 || cnum >= XmemDrvrCLIENT_CONTEXTS) {
    cprintf("xmemDrvrClose: Error bad client context: %d\n",cnum);
    pseterr(EFAULT);
    return SYSERR;
  }

  ccon = &Wa->ClientContexts[cnum];

  if (ccon->Debug & XmemDrvrDebugTRACE) {
    cprintf("xmemDrvrClose: Close client: %d for Pid: %d\n",
	    (int)ccon->ClientIndex, (int)ccon->Pid);
  }
  if (ccon->InUse == 0) {
    cprintf("xmemDrvrClose:Error:bad client context: %d Pid: %d Not in use\n",
	    (int)ccon->ClientIndex, (int)ccon->Pid);
    pseterr(EBADF); /* Bad file number */
    return SYSERR;
  }

  /* Cancel any pending timeouts */
  CancelTimeout(&ccon->Timer);

  /* Disconnect this client from events */
  DisConnectAll(ccon);

  /* Wipe out the client context */
  bzero((void *) ccon, sizeof(XmemDrvrClientContext));

  return(OK);
}

/**
 * XmemDrvrRead - Read entry point
 *
 * @param s: working area
 * @param flp: file pointer
 * @param u_buf: user buffer. results will go here
 * @param cnt: number of bytes to read
 *
 * @return number of bytes read
 */
int XmemDrvrRead(void *s, struct file *flp, char *u_buf, int cnt)
{   
  XmemDrvrClientContext *ccon;    /* Client context */
  XmemDrvrQueue         *queue;
  XmemDrvrReadBuf       *rb;
  XmemDrvrWorkingArea *wa = (XmemDrvrWorkingArea*)s;
  int           cnum; /* Client number */
  int           wcnt; /* Writable byte counts at arg address */
  int           pid;
  unsigned int  iq;
  unsigned long ps;

  cnum = minor(flp->dev) - 1;

  if (u_buf) {
    wcnt = wbounds((int)u_buf); /* Number of writeable bytes without error */
    if (wcnt < sizeof(XmemDrvrReadBuf)) {
      cprintf("xmemDrvr:Read: Target buffer too small: Context:%d Not Open\n",
	      cnum);
      pseterr(EINVAL);
      return SYSERR;
    }
  }

  ccon = &wa->ClientContexts[cnum];

  if (ccon->InUse != 1) {
    cprintf("xmemDrvr:Read: Bad FileDescriptor: Context:%d Not Open\n", cnum);
    pseterr(EBADF); /* Bad file number */
    return SYSERR;
  }

  /* 
   * Block any calls coming from unknown PIDs.
   * Only the PID that opened the driver is allowed to read.
   */

  pid = getpid();
  if (pid != ccon->Pid) {
    cprintf("xmemDrvrRead: Spurious read PID:%d for PID:%d on FD:%d\n",
	    (int)pid, (int)ccon->Pid, (int)cnum);
    pseterr(EBADF); /* Bad file number */
    return SYSERR;
  }

  queue = &ccon->Queue;

  if (queue->QueueOff) {
    disable(ps);
    {
      queue->Size   = 0;
      queue->Missed = 0;
      sreset(&ccon->Semaphore);
    }
    restore(ps);
  }

  if (ccon->Timeout) {
    ccon->Timer = timeout(ReadTimeout, (char *)ccon, ccon->Timeout);
    if (ccon->Timer < 0) {
      ccon->Timer = 0;
      queue->Size   = 0;
      queue->Missed = 0;
      sreset(&ccon->Semaphore);
      cprintf("xmemDrvr:Read: No timers available: Context:%d\n", cnum);
      pseterr(EBUSY); /* No available timers */
      return 0;
    }
  }

  if (swait(&ccon->Semaphore, SEM_SIGABORT)) {
    queue->Size   = 0;
    queue->Missed = 0;
    sreset(&ccon->Semaphore);
    if (ccon->Debug)
      cprintf("xmemDrvr:Read:Error ret from swait:Context:%d\n", cnum);
    pseterr(EINTR); /* We have been signaled */
    return 0;
  }

  if (ccon->Timeout) {
    if (ccon->Timer) {
      CancelTimeout(&ccon->Timer);
    }
    else {
      queue->Size   = 0;
      queue->Missed = 0;
      sreset(&ccon->Semaphore);
      if (ccon->Debug)
	cprintf("xmemDrvr:Read: TimeOut: Context:%d\n", cnum);
      pseterr(ETIME);
      return 0;
    }
  }

  rb = (XmemDrvrReadBuf *)u_buf;
  if (queue->Size) {
    disable(ps);
    {
      iq = --queue->Size;
      *rb = queue->Entries[iq];
    }
    restore(ps);
    return sizeof(XmemDrvrReadBuf);
  }

  queue->Size   = 0;
  queue->Missed = 0;
  sreset(&ccon->Semaphore);
  if (ccon->Debug) cprintf("xmemDrvr:Read: Queue Empty: Context:%d\n", cnum);
  pseterr(EINTR);
  return 0;
}

/**
 * XmemDrvrWrite - Write entry point
 *
 * @param s: working area
 * @param flp: file pointer
 * @param u_buf: user's buffer
 * @param cnt: byte count in buffer
 *
 * This entry point is used to wake up clients connected to a 'software
 * interrupt' called IntrDAEMON.
 * For some event it is advisable that certain clients (say clients A) subscribe
 * to its correspondent hardware interrupt, and then when they're called
 * they do some pre-processing before issuing a write() which will wake-up
 * the rest of the clients (say, clients B).
 * Therefore for a certain event we can set a hierarchy.
 *
 * An example of this would be SEGMENT_UPDATE: a daemon (client A) would
 * subscribe to IntrSEGMENT_UPDATE, and then when it's triggered, he would
 * copy the segment to RAM memory and issue a write() announcing it.
 * Then the clients connected to IntrDAEMON (clients B) (NB: they're not
 * subscribed to IntrSEGMENT_UPDATE) would be woken up -- then they could
 * read safely the updated segment from RAM memory.
 *
 * @return number of bytes written on success.
 */
int XmemDrvrWrite(void *s, struct file *flp, char *u_buf, int cnt)
{
  XmemDrvrModuleContext *mcon;
  XmemDrvrClientContext *ccon;
  XmemDrvrQueue         *queue;
  XmemDrvrReadBuf        rbf; /* read buffer (for the clients) */
  XmemDrvrWriteBuf      *wbf; /* write buffer */
  XmemDrvrWorkingArea *wa = (XmemDrvrWorkingArea*)s;
  int           cnum;
  int           pid;
  int           midx;
  unsigned int  i;
  unsigned long ps;
  unsigned long msk;

  /*
   * Note: The user buffer does not have to be checked with {r,w}bounds
   * because the read() and write() system calls have already done this.
   * {r,w}bounds are only meant to be used inside the ioctl() entry point.
   * Source: LynxOs man pages.
   */

  wbf = (XmemDrvrWriteBuf *)u_buf;

  /* Get client context of the writer */
  cnum = minor(flp->dev) - 1;
  ccon = &wa->ClientContexts[cnum];

  if (ccon->InUse != 1) {
    cprintf("xmemDrvr:Write: Bad FileDescriptor: Context:%d Not Open\n", cnum);
    pseterr(EBADF); /* Bad file number */
    return SYSERR;
  }

  /* Get the module's context */
  midx = wbf->Module - 1;
  if (midx < 0 || midx >= Wa->Modules) {
    pseterr(ENXIO); /* No such device or address */
    return SYSERR;
  }
  mcon = &wa->ModuleContexts[midx];

  /*
   * Block any calls coming from unknown PIDs.
   * Only the PID that opened the driver is allowed to read.
   */
  pid = getpid();
  if (pid != ccon->Pid) {
    cprintf("xmemDrvr:Write: Spurious read PID:%d for PID:%d on FD:%d\n",
	    (unsigned int)pid, (unsigned int)ccon->Pid, (unsigned int)cnum);
    pseterr(EBADF); /* Bad file number */
    return SYSERR;
  }

  /* Prepare the read buffer for suscribed clients */
  bzero((void *)&rbf, sizeof(XmemDrvrWriteBuf));
  rbf.Module = wbf->Module;
  rbf.NdData[XmemDrvrIntIdxDAEMON] = wbf->NdData;
  rbf.NodeId[XmemDrvrIntIdxDAEMON] = wbf->NodeId;
  rbf.Mask = XmemDrvrIntrDAEMON;

  /* then put it in the client's queue */
  for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {

    ccon = &Wa->ClientContexts[i];
    queue = &ccon->Queue;

    msk = mcon->Clients[i];

    if (! (msk & XmemDrvrIntrDAEMON))
      break; /* this client is not connected to IntrDAEMON */


    /* the client's connected to IntrDAEMON --> fill in his queue */

    disable(ps); /* acquire spinlock */

    if (queue->Size < XmemDrvrQUEUE_SIZE) {
      queue->Entries[queue->Size++] = rbf;
      ssignal(&ccon->Semaphore); /* Wake-up client */
    }
    else {
      queue->Size = XmemDrvrQUEUE_SIZE;
      queue->Missed++;
    }

    restore(ps); /* release spinlock */

  }

  return sizeof(XmemDrvrWriteBuf);
}

/**
 * XmemDrvrSelect - select service subroutine of the driver.
 *        [!!!!!! LINUX NOTE !!!!!!] ---> Will NOT work under Linux!
 *        Third parameter (condition to monitor) is void, i.e.
 *        it's not known. Never rely on it!.
 *
 * @param s: working area
 * @param flp: file pointer
 * @param wch: Read/Write direction
 * @param ffs: Selection structures
 *
 * Although there's some code below, this entry point has not been reviewed and
 * should not be used.
 *
 * @return OK on success
 */
int XmemDrvrSelect(void *s, struct file *flp, int wch, struct sel *ffs)
{
  XmemDrvrClientContext * ccon;
  XmemDrvrWorkingArea *wa = (XmemDrvrWorkingArea*)s;
  int cnum;

  cnum = minor(flp->dev) - 1;
  ccon = &wa->ClientContexts[cnum];

  if (wch == SREAD) {
    ffs->iosem = (int *)&ccon->Semaphore; /* Watch out here I hope   */
    return OK;                               /* the system dosn't swait */
  }                                           /* the read does it too !! */
  pseterr(EACCES);			      /* Permission denied */
  return SYSERR;
}


/**
 * XmemDrvrInstall - This routine is called at driver install with a 
 * parameter pointer, addressing a table initialised with the info table
 *
 * @param info: device info table
 *
 * @return char pointer to 'wa' (working area) on success.
 */
char* XmemDrvrInstall(void *info)
{
  XmemDrvrWorkingArea *wa;
  struct drm_node_s *handle;
  XmemDrvrModuleContext *mcon;
  unsigned int vadr;
  unsigned long ivec;
  unsigned long bigend;
  int midx, mid, cc;

  /* Allocate the driver working area. */
  wa = (XmemDrvrWorkingArea *)sysbrk(sizeof(XmemDrvrWorkingArea));
  if (!wa) {
    cprintf("xmemDrvr: NOT ENOUGH MEMORY(WorkingArea)\n");
    pseterr(ENOMEM);
    return (char *)SYSERR;
  }
  bzero((void *)wa, sizeof(XmemDrvrWorkingArea)); /* Clear working area */
  Wa = wa; /* Global working area pointer */

  Wa->DmaThreshold = XmemDrvrDEFAULT_DMA_THRESHOLD;

  SetEndian(); /* Set Big/Little endian flag in driver working area */

  for (midx = 0; midx < XmemDrvrMODULE_CONTEXTS; midx++) {
    cc = drm_get_handle(PCI_BUSLAYER, VMIC_VENDOR_ID, VMIC_DEVICE_ID, &handle);
    if (cc) {
      if (midx == 0) {
	sysfree((void *) wa, sizeof(XmemDrvrWorkingArea));
	Wa = NULL;
	cprintf("xmemDrvr: No VMIC card found: No hardware: Fatal\n");
	pseterr(ENODEV);
	return (char *) SYSERR;
      } else break;
    }

    mcon = &wa->ModuleContexts[midx];
    if (mcon->InUse == 0) {
      drm_device_read(handle, PCI_RESID_DEVNO, 0, 0, &mid);
      mcon->PciSlot     = mid;
      mcon->ModuleIndex = midx;
      mcon->Handle      = handle;
      mcon->InUse       = 1;

      /* Allocate space for the temporary buffer mcon->Tempbuf.
       * This buffer is used for copying up to PAGESIZE bytes of data from user
       * space, and is also used by FlushSegments() as a cache.
       * If the user wants to transfer more than PAGESIZE bytes, DMA is used.
       * This means the DMA threshold mustn't be set beyond PAGESIZE.
       */
      mcon->Tempbuf = (void *)sysbrk(PAGESIZE);
      if (!mcon->Tempbuf) {
	cprintf("xmemDrvr: NOT ENOUGH MEMORY(mod[%d]->Tempbuf)\n", midx);
	pseterr(ENOMEM);
	return ((char *)SYSERR);
      }

      /* initialise mutexes */
      sreset( &mcon->BusySemaphore);
      ssignal(&mcon->BusySemaphore);

      sreset( &mcon->TempbufSemaphore);
      ssignal(&mcon->TempbufSemaphore);

      /* initialise countdown semaphores */
      sreset( &mcon->RdDmaSemaphore);
      sreset( &mcon->WrDmaSemaphore);

      /* register ISR */
      cc = drm_register_isr(handle, IntrHandler, (void *)mcon);
      if (cc == SYSERR) {
	cprintf("xmemDrvr: Can't register ISR for timing module: %d\n",midx+1);
	return((char *)SYSERR);
      } 
      else {

	ivec = 0;
	DrmConfigReadWrite(mcon, PlxConfigPCIILR, &ivec, 1, XmemDrvrREAD);
	mcon->IVector = 0xFF & ivec;
#ifdef __powerpc__
	iointunmask(mcon->IVector);
#endif

      }
    }

    vadr = (int)NULL;
    cc = drm_map_resource(handle, PCI_RESID_BAR2, &vadr);
    if (cc || !vadr) {
      cprintf("xmemDrvr: Can't map memBAR2 for module: %d cc=%d, vadr=0x%x\n",
	      midx + 1, cc, vadr);
      return((char *)SYSERR);
    }
    mcon->Map = (VmicRfmMap *)vadr;

    vadr = (int)NULL;
    cc = drm_map_resource(handle, PCI_RESID_BAR3, &vadr);
    if (cc || !vadr) {
      cprintf("xmemDrvr: Can't map memory (BAR3) for module: %d\n",midx + 1);
      return((char *)SYSERR);
    }
    mcon->SDRam = (unsigned char *) vadr;

    vadr = (int) NULL;
    cc = drm_map_resource(handle, PCI_RESID_BAR0, &vadr);
    if (cc || !vadr) {
      cprintf("xmemDrvr: Can't map plx9656 (BAR0) local configuration: %d\n",
	      midx + 1);
      return((char *)SYSERR);
    }
    mcon->Local = (PlxLocalMap *)vadr;


    /*
     * Set up the BIGEND  local configuration register to do appropriate
     * endian mapping
     */
    if (Wa->Endian == XmemDrvrEndianBIG) {
      bigend = 0xC0; /* Only set big endian for DMA Channels: BIGEND=11000000 */
      DrmLocalReadWrite(mcon, PlxLocalBIGEND, &bigend, 1, XmemDrvrWRITE);
    }
    wa->Modules = midx + 1;
    Reset(mcon); /* Soft reset and initialize module */
  }
  return (char *)wa;
}

/**
 * XmemDrvrUninstall - routine called to uninstall the driver
 *
 * @param s: working area
 *
 * @return OK on success
 */
int XmemDrvrUninstall(void *s)
{
  XmemDrvrClientContext *ccon;
  XmemDrvrWorkingArea *wa = (XmemDrvrWorkingArea*)s;
  XmemDrvrModuleContext *mcon;
  int i;

  for (i = 0; i < Wa->Modules; i++) {
    mcon = &wa->ModuleContexts[i];
    if (mcon->InUse) {
      if (mcon->Local) {
	drm_unmap_resource(mcon->Handle, PCI_RESID_BAR0);
	mcon->LocalOpen = 0;
	mcon->Local = NULL;
      }
      if (mcon->Map) {
	drm_unmap_resource(mcon->Handle, PCI_RESID_BAR2);
	mcon->Map = NULL;
      }
      drm_unregister_isr(mcon->Handle);
      drm_free_handle(mcon->Handle);
      bzero((void *) mcon, sizeof(XmemDrvrModuleContext));
    }
  }

  for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {
    ccon = &wa->ClientContexts[i];
    if (ccon->InUse) {
      if (ccon->Timer) {
	CancelTimeout(&ccon->Timer);
      }
      ssignal(&ccon->Semaphore); /* Wakeup client */
      ccon->InUse = 0;
    }
  }

  sysfree((void *)wa, sizeof(XmemDrvrWorkingArea));
  Wa = NULL;
  cprintf("xmemDrvr: Driver: Uninstalled\n");
  return OK;
}



/*
 * FIXME: it would be _much_ cleaner to have the IOCTL entry point like a switch
 * that instead of having everything inside, calls for each case a different 
 * function. It's just a huge waste the way it's done now.
 */


/**
 * XmemDrvrIoctl - routine called to perform ioctl function
 *
 * @param s: working area
 * @param flp: file pointer
 * @param cm: IOCTL command
 * @param arg: data for the IOCTL
 *
 * @return OK on success
 */
int XmemDrvrIoctl(void *s, struct file * flp, int fct, char * arg)
{
  XmemDrvrModuleContext           *mcon;   /* Module context */
  XmemDrvrClientContext           *ccon;   /* Client context */
  XmemDrvrConnection              *conx;
  XmemDrvrClientList              *cls;
  XmemDrvrClientConnections       *ccn;
  XmemDrvrRawIoBlock              *riob;
  XmemDrvrVersion                 *ver;
  XmemDrvrModuleDescriptor        *mdesc;
  XmemDrvrSendBuf                 *sbuf;
  XmemDrvrRamAddress              *radr;
  XmemDrvrSegTable                *stab;
  XmemDrvrSegIoDesc               *siod;
  XmemDrvrWorkingArea *wa = (XmemDrvrWorkingArea*)s;


  int cm;
  int i, j, size, pid;
  int cnum;                 /* Client number */
  long lav, *lap;           /* Long Value pointed to by Arg */
  unsigned short sav;       /* Short argument and for Jtag IO */
  int rcnt, wcnt;           /* Readable, Writable byte counts at arg address */
  unsigned long ps;
  int err;
  unsigned long lptr[50];
  XmemDrvrConnection tempcon[XmemDrvrMODULE_CONTEXTS];
  void *temptr;


  cm = fct;

  /*
   * Check argument contains a valid address for reading or writing.
   * We can not allow bus errors to occur inside the driver due to  
   * the caller providing a garbage address in "arg". So if arg is  
   * not null set "rcnt" and "wcnt" to contain the byte counts which
   * can be read or written to without error.
   */
  if (arg != NULL) {

    rcnt = rbounds((int)arg); /* Number of readable bytes without error */
    wcnt = wbounds((int)arg); /* Number of writable bytes without error */

    if (rcnt == EFAULT || wcnt == EFAULT) {
      cprintf("xmemDrvrIoctl: ILLEGAL NON NULL ARG, RCNT=%d/%d\n", rcnt, 
	      sizeof(long));
      pseterr(EINVAL);
      return(SYSERR);
    }

    lav = *((long *)arg); /* Long argument value */
    lap =   (long *)arg ; /* Long argument pointer */

  }
  else {

    rcnt = 0;
    wcnt = 0;
    lav = 0;
    lap = NULL; /* Null arg = zero read/write counts */

  }

  sav = lav; /* Short argument value */

  /*
   * We allow one client per minor device, we use the minor device 
   * number as an index into the client contexts array.
   */

  cnum = minor(flp->dev) - 1;

  if ( cnum < 0 || cnum >= XmemDrvrCLIENT_CONTEXTS) {
    pseterr(ENODEV); /* No such device */
    return SYSERR;
  }

  /* We can't control a file which is not open. */

  ccon = &wa->ClientContexts[cnum];
  if (ccon->InUse == 0) {
    cprintf("xmemDrvrIoctl: DEVICE %2d IS NOT OPEN\n",cnum + 1);
    pseterr(EBADF); /* Bad file number */
    return SYSERR;
  }

  /* 
   * Block any calls coming from unknown PIDs.
   * Only the PID that opened the driver is allowed to call IOCTLs
   */

  pid = getpid();
  if (pid != ccon->Pid) {
    cprintf("xmemDrvrIoctl: Spurious IOCTL:%d by PID:%d for PID:%d on FD:%d\n",
	    (int)cm, (int)pid, (int)ccon->Pid, (int)cnum);
    pseterr(EBADF);           /* Bad file number */
    return SYSERR;
  }

  /* Set up some useful module pointers */
  mcon = &wa->ModuleContexts[ccon->ModuleIndex]; /* Default module selected */

  TraceIoctl(cm,arg,ccon);   /* Print debug message */



  /*
   * Decode callers command and do it.
   */

  switch (cm) {

  case XmemDrvrSET_SW_DEBUG: /* Set driver debug mode */
    IOCTL_TRACK("XmemDrvrSET_SW_DEBUG");
    if (lap) {
      ccon->Debug = (XmemDrvrDebug) lav;
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_SW_DEBUG:
    IOCTL_TRACK("XmemDrvrGET_SW_DEBUG");
    if (lap) {
      *lap = (long)ccon->Debug;
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_VERSION: /* Get version date */
    IOCTL_TRACK("XmemDrvrGET_VERSION");
    if (wcnt >= sizeof(XmemDrvrVersion)) {
      ver = (XmemDrvrVersion *) arg;
      return GetVersion(mcon,ver);
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrSET_TIMEOUT: /* Set the read timeout value */
    IOCTL_TRACK("XmemDrvrSET_TIMEOUT");
    if (lap) {
      if (lav < XmemDrvrMINIMUM_TIMEOUT && lav) {
	lav = XmemDrvrMINIMUM_TIMEOUT;
      }
      ccon->Timeout = lav; /* Note that if (!lav), this is properly handled */
      if (ccon->Debug) {
	if (lav)
	  cprintf("xmemDrvr: TicksPerSec:%d Timeout:%d\n", (int)tickspersec, 
		  (int)lav);
	else
	  cprintf("xmemDrvr: Disabled Client's Timeout.\n");
      }
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_TIMEOUT: /* Get the read timeout value */
    IOCTL_TRACK("XmemDrvrGET_TIMEOUT");
    if (lap) {
      *lap = ccon->Timeout;
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrSET_QUEUE_FLAG: /* Set queueing capabilities on/off */
    IOCTL_TRACK("XmemDrvrSET_QUEUE_FLAG");
    if (lap) {
      if (lav)
	ccon->Queue.QueueOff = 1;
      else
	ccon->Queue.QueueOff = 0;

      disable(ps);
      {
	ccon->Queue.Size   = 0;
	ccon->Queue.Missed = 0;
	sreset(&ccon->Semaphore);
      }
      restore(ps);
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_QUEUE_FLAG: /* 1-->Q_off, 0-->Q_on */
    IOCTL_TRACK("XmemDrvrGET_QUEUE_FLAG");
    if (lap) {
      *lap = ccon->Queue.QueueOff;
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_QUEUE_SIZE: /* Number of events on queue */
    IOCTL_TRACK("XmemDrvrGET_QUEUE_SIZE");
    if (lap) {
      *lap = ccon->Queue.Size;
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_QUEUE_OVERFLOW: /* Number of missed events */
    IOCTL_TRACK("XmemDrvrGET_QUEUE_OVERFLOW");
    if (lap) {

      disable(ps);
      *lap = ccon->Queue.Missed;
      ccon->Queue.Missed = 0;
      restore(ps);

      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_MODULE_DESCRIPTOR:
    IOCTL_TRACK("XmemDrvrGET_MODULE_DESCRIPTOR");
    if (wcnt >= sizeof(XmemDrvrModuleDescriptor)) {

      mdesc = (XmemDrvrModuleDescriptor *)arg;
      mdesc->Module  = mcon->ModuleIndex + 1;
      mdesc->PciSlot = mcon->PciSlot;
      mdesc->NodeId  = mcon->NodeId;
      mdesc->Map     = (unsigned char *)mcon->Map;
      mdesc->SDRam   = mcon->SDRam;

      return OK;

    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrSET_MODULE: /* Select the module to work with */
    IOCTL_TRACK("XmemDrvrSET_MODULE");
    if (lap) {
      if (lav >= 1 &&  lav <= Wa->Modules) {
	ccon->ModuleIndex = lav - 1;
	return OK;
      }
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_MODULE: /* Which module am I working with */
    IOCTL_TRACK("XmemDrvrGET_MODULE");
    if (lap) {
      *lap = ccon->ModuleIndex + 1;
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_MODULE_COUNT: /* The number of installed modules */
    IOCTL_TRACK("XmemDrvrGET_MODULE_COUNT");
    if (lap) {
      *lap = Wa->Modules;
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrSET_MODULE_BY_SLOT: /* Select the module to work with by ID */
    IOCTL_TRACK("XmemDrvrSET_MODULE_BY_SLOT");
    if (lap) {
      for (i = 0; i < Wa->Modules; i++) {
	mcon = &wa->ModuleContexts[i];
	if (mcon->PciSlot == lav) {
	  ccon->ModuleIndex = i;
	  return OK;
	}
      }
      pseterr(ENODEV);
      return SYSERR;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_MODULE_SLOT: /* Get the ID of the selected module */
    IOCTL_TRACK("XmemDrvrGET_MODULE_SLOT");
    if (lap) {
      *lap = mcon->PciSlot;
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrRESET: /* Reset the module, re-establish connections */
    IOCTL_TRACK("XmemDrvrRESET");
    return Reset(mcon);

  case XmemDrvrSET_COMMAND: /* Send a command to the VMIC module */
    IOCTL_TRACK("XmemDrvrSET_COMMAND");
    if (lap) {
      GetSetStatus(mcon, (XmemDrvrScr)lav, XmemDrvrWRITE);
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_STATUS: /* Read module status */
    IOCTL_TRACK("XmemDrvrGET_STATUS");
    if (lap) {
      *lap = (unsigned long)GetSetStatus(mcon, mcon->Command, XmemDrvrREAD);
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_NODES:
    IOCTL_TRACK("XmemDrvrGET_NODES");
    if (lap) {
      *lap = Wa->Nodes;
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_CLIENT_LIST: /* Get the list of driver clients */
    IOCTL_TRACK("XmemDrvrGET_CLIENT_LIST");
    if (wcnt >= sizeof(XmemDrvrClientList)) {

      cls = (XmemDrvrClientList *)arg;
      bzero((void *)cls, sizeof(XmemDrvrClientList));

      for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {
	ccon = &wa->ClientContexts[i];
	if (ccon->InUse)
	  cls->Pid[cls->Size++] = ccon->Pid;
      }

      return OK;

    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrCONNECT: /* Connect to an object interrupt */
    IOCTL_TRACK("XmemDrvrCONNECT");
    conx = (XmemDrvrConnection *)arg;
    if (rcnt >= sizeof(XmemDrvrConnection)) 
      return Connect(conx,ccon);
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrDISCONNECT: /* Disconnect from an object interrupt */
    IOCTL_TRACK("XmemDrvrDISCONNECT");
    conx = (XmemDrvrConnection *)arg;
    if (rcnt >= sizeof(XmemDrvrConnection)) {

      if (conx->Mask)
	return DisConnect(conx,ccon);
      else {
	DisConnectAll(ccon);
	return OK;
      }

    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_CLIENT_CONNECTIONS:
    /* Get the list of a client connections on module */
    IOCTL_TRACK("XmemDrvrGET_CLIENT_CONNECTIONS");

    if (wcnt >= sizeof(XmemDrvrClientConnections)) {

      ccn = (XmemDrvrClientConnections *)arg;
      temptr = ccn->Connections; /* store the address */

      ccn->Size = 0;
      size = 0;
      for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {
	ccon = &wa->ClientContexts[i];
	if (ccon->InUse && ccon->Pid == ccn->Pid) {
	  for (j = 0; j < Wa->Modules; j++) {
	    mcon = &Wa->ModuleContexts[j];
	    if (mcon->Clients[i]) {
	      tempcon[size].Module = j + 1;
	      tempcon[size].Mask   = mcon->Clients[i];
	      ccn->Size = ++size;
	    }
	  }
	}
      }
      if (!cdcm_copy_to_user(temptr, tempcon, ccn->Size))
	return OK;
      else pseterr(EFAULT);
      return SYSERR;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrSEND_INTERRUPT: /* Send an interrupt to other nodes */
    IOCTL_TRACK("XmemDrvrSEND_INTERRUPT");
    if (rcnt >= sizeof(XmemDrvrSendBuf)) { 
      sbuf = (XmemDrvrSendBuf *)arg;
      return SendInterrupt(sbuf);
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_XMEM_ADDRESS:       
    /* Get physical address of reflective memory SDRAM */
    IOCTL_TRACK("XmemDrvrGET_XMEM_ADDRESS");

    if (wcnt >= sizeof(XmemDrvrRamAddress)) {
      radr = (XmemDrvrRamAddress *)arg;
      i = radr->Module - 1;
      if (i >= 0 && i < Wa->Modules) {
	mcon = &Wa->ModuleContexts[i];
	radr->Address = mcon->SDRam;
	return OK;
      }
      pseterr(ENODEV);
      return SYSERR;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrSET_SEGMENT_TABLE:      
    /* Set the list of all defined xmem memory segments */
    IOCTL_TRACK("XmemDrvrSET_SEGMENT_TABLE");

    if (rcnt >= sizeof(XmemDrvrSegTable)) {
      stab = (XmemDrvrSegTable *)arg;
      LongCopy((unsigned long *)&Wa->SegTable, (unsigned long *)stab,
	       sizeof(XmemDrvrSegTable));
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_SEGMENT_TABLE: /* List of all defined xmem memory segments */
    IOCTL_TRACK("XmemDrvrGET_SEGMENT_TABLE");
    if (wcnt >= sizeof(XmemDrvrSegTable)) {
      stab = (XmemDrvrSegTable *) arg;
      LongCopy((unsigned long *)stab, (unsigned long *)&Wa->SegTable,
	       sizeof(XmemDrvrSegTable));
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrREAD_SEGMENT:   /* Copy from xmem segment to local memory */

    IOCTL_TRACK("XmemDrvrREAD_SEGMENT");

    if (wcnt < sizeof(XmemDrvrSegIoDesc)) {
      pseterr(EINVAL);
      return SYSERR;
    }
    siod = (XmemDrvrSegIoDesc *)arg;
    if (siod->UserArray == NULL) {
      cprintf("xmemDrvr: UserArray is NULL!\n");
      pseterr(EINVAL);
      return SYSERR;
    }

    if (ccon->Debug) {
      cprintf("xmemDrvr: Rd: Mod:%d Seg:0x%x Off:%d Sze:%d Uad:0x%x Udf:%d\n",
	      (int)siod->Module, (int)siod->Id, (int)siod->Offset, 
	      (int)siod->Size, (int)siod->UserArray, (int)siod->UpdateFlg);
    }

    if (siod->Size >= Wa->DmaThreshold) { /* use DMA engines */

      return SegmentCopy(siod, XmemDrvrREAD, ccon, mcon);

    }
    else { /* CPU-driven copy */

      temptr = siod->UserArray; /* Save the user space address */


      swait(&mcon->TempbufSemaphore, SEM_SIGIGNORE); /* acquire mutex */

      err = cdcm_copy_from_user(mcon->Tempbuf, siod->UserArray, siod->Size);
      if (err) {
	cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
	ssignal(&mcon->TempbufSemaphore);
	pseterr(EFAULT);
	return (SYSERR);
      }

      siod->UserArray = mcon->Tempbuf; /* fill with the kernel address */
      err = SegmentCopy(siod, XmemDrvrREAD, ccon, mcon);
      siod->UserArray = temptr; /* Restore the user space address */

      if (err != OK) {
	ssignal(&mcon->TempbufSemaphore); /* release mutex */
	return err;
      }

      err = cdcm_copy_to_user(siod->UserArray, mcon->Tempbuf, siod->Size);
      ssignal(&mcon->TempbufSemaphore); /* release mutex */


      if (err) {
	cprintf("xmemDrvr: Copy to user failed. Returned error: %d.\n", err);
	pseterr(EFAULT);
	return (SYSERR);
      }

      return OK;

    }
    pseterr(EINVAL); /* will never get to this point, anyway */
    return SYSERR;


  case XmemDrvrWRITE_SEGMENT:  /* Update the segment with new contents */

    IOCTL_TRACK("XmemDrvrWRITE_SEGMENT");

    if (rcnt < sizeof(XmemDrvrSegIoDesc)) {
      pseterr(EINVAL);
      return SYSERR;
    }
    siod = (XmemDrvrSegIoDesc *)arg;
    if (siod->UserArray == NULL) {
      cprintf("xmemDrvr: UserArray is NULL!\n");
      pseterr(EINVAL);
      return SYSERR;
    }

    if (ccon->Debug) {
      cprintf("xmemDrvr: Wr: Mod:%d Seg:0x%x Off:%d Sze:%d Uad:0x%x Udf:%d\n",
	      (int)siod->Module, (int)siod->Id, (int)siod->Offset,
	      (int)siod->Size, (int)siod->UserArray, (int)siod->UpdateFlg);
    }

#ifdef __TEST_TT__
#ifdef __linux__
    getnstimeofday(&t_a);
#else
    tt_na = nanotime(&tt_a);
#endif /* !__linux__ */
#endif /* __TEST_TT__ */

    if (siod->Size >= Wa->DmaThreshold) { /* use DMA engines */

      err = SegmentCopy(siod, XmemDrvrWRITE, ccon, mcon);

#ifdef __TEST_TT__
#ifdef __linux__
      getnstimeofday(&t_b);
      tt_diff = timespec_to_ns(&t_b) - timespec_to_ns(&t_a);
      tt_diff01 = timespec_to_ns(&t_a1) - timespec_to_ns(&t_a);
      tt_diff12 = timespec_to_ns(&t_a2) - timespec_to_ns(&t_a1);
      tt_diff23 = timespec_to_ns(&t_a3) - timespec_to_ns(&t_a2);
      tt_diff34 = timespec_to_ns(&t_a4) - timespec_to_ns(&t_a3);
#else
      tt_nb = nanotime(&tt_b);
      tt_diff   = (tt_b - tt_a  )*1000000000 + tt_nb - tt_na;
      tt_diff01 = (tt_a1 - tt_a )*1000000000 + tt_na1 - tt_na;
      tt_diff12 = (tt_a2 - tt_a1)*1000000000 + tt_na2 - tt_na1;
      tt_diff23 = (tt_a3 - tt_a2)*1000000000 + tt_na3 - tt_na2;
      tt_diff34 = (tt_a4 - tt_a3)*1000000000 + tt_na4 - tt_na3;
#endif /* !__linux__ */
      cprintf("xmemDrvr: dma total: %lu ns\n", tt_diff); 
      cprintf("@@ 01: %lu ns\n", tt_diff01); 
      cprintf("@@ 12: %lu ns\n", tt_diff12);
      cprintf("@@ 23: %lu ns\n", tt_diff23);
      cprintf("@@ 34: %lu ns\n", tt_diff34);
#endif /* __TEST_TT */

      return err;

    }
    else { /* CPU-driven copy */

      temptr = siod->UserArray; /* Save the user space address */


      swait(&mcon->TempbufSemaphore, SEM_SIGIGNORE); /* acquire mutex */

      err = cdcm_copy_from_user(mcon->Tempbuf, siod->UserArray, siod->Size);
      if (err) {
	cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
	ssignal(&mcon->TempbufSemaphore);
	pseterr(EFAULT);
	return (SYSERR);
      }

      siod->UserArray = mcon->Tempbuf; /* fill with the kernel address */
      err = SegmentCopy(siod, XmemDrvrWRITE, ccon, mcon);
      siod->UserArray = temptr; /* Restore the user space address */

      ssignal(&mcon->TempbufSemaphore); /* release mutex */
      /* Note that here we don't need to copy_to_user() */

#ifdef __TEST_TT__
#ifdef __linux__
      getnstimeofday(&t_b);
      tt_diff = timespec_to_ns(&t_b) - timespec_to_ns(&t_a);
      tt_diff01 = timespec_to_ns(&t_a1) - timespec_to_ns(&t_a);
      tt_diff12 = timespec_to_ns(&t_a2) - timespec_to_ns(&t_a1);
      tt_diff23 = timespec_to_ns(&t_a3) - timespec_to_ns(&t_a2);
      tt_diff34 = timespec_to_ns(&t_a4) - timespec_to_ns(&t_a3);
#else
      tt_nb = nanotime(&tt_b);
      tt_diff   = (tt_b -   tt_a)*1000000000 + tt_nb - tt_na;
      tt_diff01 = (tt_a1 -  tt_a)*1000000000 + tt_na1 - tt_na;
      tt_diff12 = (tt_a2 - tt_a1)*1000000000 + tt_na2 - tt_na1;
      tt_diff23 = (tt_a3 - tt_a2)*1000000000 + tt_na3 - tt_na2;
      tt_diff34 = (tt_a4 - tt_a3)*1000000000 + tt_na4 - tt_na3;
#endif /* !__linux__ */
      cprintf("xmemDrvr: longcopy total: %lu ns\n", tt_diff); 
      cprintf("@@ 01: %lu ns\n", tt_diff01); 
      cprintf("@@ 12: %lu ns\n", tt_diff12);
      cprintf("@@ 23: %lu ns\n", tt_diff23);
      cprintf("@@ 34: %lu ns\n", tt_diff34);
#endif /* __TEST_TT */

      return err; /* propagate OK or SYSERR from SegmentCopy() */

    }
    pseterr(EINVAL); /* will never get to this point, anyway */
    return SYSERR;

  case XmemDrvrCHECK_SEGMENT:  
    /* Check to see if a segment has been updated since last read */
    IOCTL_TRACK("XmemDrvrCHECK_SEGMENT");

    if (lap) {
      disable(ps);
      *lap = ccon->UpdatedSegments;
      ccon->UpdatedSegments = 0;
      restore(ps);
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrFLUSH_SEGMENTS:  
    /* Flush segments out to other nodes after PendingInit */
    IOCTL_TRACK("XmemDrvrFLUSH_SEGMENTS");

    if (lap) {
      return FlushSegments(mcon, *lap);
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrCONFIG_OPEN: /* Open the PLX9656 configuration */
    IOCTL_TRACK("XmemDrvrCONFIG_OPEN");
    mcon->ConfigOpen = 1;
    return OK;

  case XmemDrvrLOCAL_OPEN: /* Open the PLX9656 local configuration */
    IOCTL_TRACK("XmemDrvrLOCAL_OPEN");
    mcon->LocalOpen = 1;
    return OK;

  case XmemDrvrRFM_OPEN: /* Open the VMIC RFM configuration */
    IOCTL_TRACK("XmemDrvrRFM_OPEN");
    mcon->RfmOpen = 1;
    return OK;

  case XmemDrvrRAW_OPEN: /* Open the VMIC SDRAM */
    IOCTL_TRACK("XmemDrvrRAW_OPEN");
    mcon->RawOpen = 1;
    return OK;

  case XmemDrvrCONFIG_READ: /* Read the PLX9656 configuration registers */
    IOCTL_TRACK("XmemDrvrCONFIG_READ");

    if (wcnt < sizeof(XmemDrvrRawIoBlock)) {
      pseterr(EINVAL);
      return SYSERR;
    }
    if (mcon->ConfigOpen) {

      riob = (XmemDrvrRawIoBlock *)arg;
      if (riob->UserArray == NULL) {
	cprintf("xmemDrvr: UserArray is NULL!\n");
	pseterr(EINVAL);
	return SYSERR;
      }

      temptr = riob->UserArray; /* Save the user space address */
      err = cdcm_copy_from_user(lptr, riob->UserArray, 
				sizeof(long) * riob->Items);
      if (err) {
	cprintf("xmemDrvr: Copy from user failed. Returned error:%d.\n", err);
	pseterr(EFAULT);
	return SYSERR;
      }

      riob->UserArray = lptr; /* fill with the kernel address */
      err = DrmConfigReadWrite(mcon, riob->Offset, riob->UserArray,
			       riob->Size, XmemDrvrREAD);
      riob->UserArray = temptr; /* Restore user space address */

      if (err != OK) {
	pseterr(EIO);
	return SYSERR;
      }

      err = cdcm_copy_to_user(riob->UserArray, lptr, 
			      sizeof(long) * riob->Items);
      if (err) {
	pseterr(EFAULT);
	return SYSERR;
      }
      return OK;
	
    }
    pseterr(EACCES);
    return SYSERR;

  case XmemDrvrLOCAL_READ:  /* Read the PLX9656 local configuration registers */
    IOCTL_TRACK("XmemDrvrLOCAL_READ");
    if (wcnt < sizeof(XmemDrvrRawIoBlock)) {
      pseterr(EINVAL);
      return SYSERR;
    }
    if (mcon->LocalOpen) {

      riob = (XmemDrvrRawIoBlock *)arg;
      if (riob->UserArray == NULL) {
	cprintf("xmemDrvr: UserArray is NULL!\n");
	pseterr(EINVAL);
	return SYSERR;
      }

      temptr = riob->UserArray; /* Save the user space address */

      err = cdcm_copy_from_user(lptr, riob->UserArray, 
				sizeof(long) * riob->Items);
      if (err) {
	cprintf("xmemDrvr: Copy from user failed. returned error:%d.\n", err);
	pseterr(EFAULT);
	return SYSERR;
      }

      riob->UserArray = lptr; /* fill with the kernel address */
      err = DrmLocalReadWrite(mcon, riob->Offset, riob->UserArray,
			      riob->Size, XmemDrvrREAD);
      riob->UserArray = temptr; /* Restore user space address */
      if (err != OK) {
	pseterr(EIO);
	return SYSERR;
      }
      
      err = cdcm_copy_to_user(riob->UserArray, lptr, 
			      sizeof(long) * riob->Items);
      if (err) {
	pseterr(EFAULT);
	return SYSERR;
      }
      return OK;

    }
    pseterr(EACCES);
    return SYSERR;

  case XmemDrvrRFM_READ: /* Read from VMIC 5565 FPGA Register block RFM */

    IOCTL_TRACK("XmemDrvrRFM_READ");
    if (wcnt >= sizeof(XmemDrvrRawIoBlock)) {

      riob = (XmemDrvrRawIoBlock *)arg;
      if (riob->UserArray == NULL) {
	cprintf("xmemDrvr: UserArray is NULL!\n");
	pseterr(EINVAL);
	return SYSERR;
      }


      temptr = riob->UserArray; /* Save the user space address */
      err = cdcm_copy_from_user(lptr, riob->UserArray, 
				sizeof(long) * riob->Items);
      if (err) {
	cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
	pseterr(EFAULT);
	return SYSERR;
      }

      riob->UserArray = lptr; /* fill with the kernel address */
      err = RfmIo(mcon, riob, XmemDrvrREAD);
      riob->UserArray = temptr; /* Restore user space address */

      if (err != OK) {
	pseterr(EIO);
	return SYSERR;
      }

      err = cdcm_copy_to_user(riob->UserArray, lptr, 
			      sizeof(long) * riob->Items);
      if (err) {
	pseterr(EFAULT);
	return SYSERR;
      }
      return OK;

    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrRAW_READ: /* Read from VMIC 5565 SDRAM */
    IOCTL_TRACK("XmemDrvrRAW_READ");
    if (wcnt >= sizeof(XmemDrvrRawIoBlock)) {
      riob = (XmemDrvrRawIoBlock *)arg;
      if (riob->UserArray == NULL) {
	cprintf("xmemDrvr: UserArray is NULL!\n");
	pseterr(EINVAL);
	return SYSERR;
      }

      temptr = riob->UserArray; /* Store the user space address */
      err = cdcm_copy_from_user(lptr, riob->UserArray, 
				sizeof(long) * riob->Items);
      if (err) {
	cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
	pseterr(EFAULT);
	return SYSERR;

      }

      riob->UserArray = lptr; /* fill with the kernel address */
      err = RawIo(mcon, riob, XmemDrvrREAD);
      riob->UserArray = temptr; /* Restore user space address */

      if (err != OK) {
	pseterr(EIO);
	return SYSERR;
      }

      err = cdcm_copy_to_user(riob->UserArray, lptr, 
			      sizeof(long) * riob->Items);
      if (err) {
	pseterr(EFAULT);
	return SYSERR;
      }

      return OK;

    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrCONFIG_WRITE:
  /* Write to PLX9656 configuration registers (Experts only) */
    IOCTL_TRACK("XmemDrvrCONFIG_WRITE");

    if (rcnt < sizeof(XmemDrvrRawIoBlock)) {
      pseterr(EINVAL);
      return SYSERR;
    }

    if (mcon->ConfigOpen) {

      riob = (XmemDrvrRawIoBlock *)arg;
      if (riob->UserArray == NULL) {
	cprintf("xmemDrvr: UserArray is NULL!\n");
	pseterr(EINVAL);
	return SYSERR;
      }

      temptr = riob->UserArray; /* Store the user space address */
      err = cdcm_copy_from_user(lptr, riob->UserArray, 
				sizeof(long) * riob->Items);
      if (err) {
	cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
	pseterr(EFAULT);
	return SYSERR;
      }

      riob->UserArray = lptr; /* fill with the kernel address */
      err = DrmConfigReadWrite(mcon, riob->Offset, riob->UserArray,
			       riob->Size, XmemDrvrWRITE);
      riob->UserArray = temptr; /* Restore user space address */

      if (err != OK) {
	pseterr(EIO);
	return SYSERR;
      }

      /* No copy_to_user here */
      return OK;

    }
    pseterr(EACCES);
    return SYSERR;

  case XmemDrvrLOCAL_WRITE:  
    /* Write the PLX9656 local configuration registers (Experts only) */
    IOCTL_TRACK("XmemDrvrLOCAL_WRITE");
    if (rcnt >= sizeof(XmemDrvrRawIoBlock)) {
      pseterr(EINVAL);
      return SYSERR;
    }
    if (mcon->LocalOpen) {

      riob = (XmemDrvrRawIoBlock *)arg;
      if (riob->UserArray == NULL) {
	cprintf("xmemDrvr: UserArray is NULL!\n");
	pseterr(EINVAL);
	return SYSERR;
      }
      temptr = riob->UserArray; /* Store the user space address */
      err = cdcm_copy_from_user(lptr, riob->UserArray,
				sizeof(long) * riob->Items);
      if (err) {
	cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
	pseterr(EFAULT);
	return SYSERR;
      }


      riob->UserArray = lptr; /* fill with the kernel address */
      err = DrmConfigReadWrite(mcon, riob->Offset, riob->UserArray,
			       riob->Size, XmemDrvrWRITE);
      riob->UserArray = temptr; /* Restore user space address */

      if (err != OK) {
	pseterr(EIO);
	return SYSERR;
      }

      /* No copy_to_user here */
      return OK;

    }
    pseterr(EACCES);
    return SYSERR;

  case XmemDrvrRFM_WRITE: /* Write to VMIC 5565 FPGA Register block RFM */
    IOCTL_TRACK("XmemDrvrRFM_WRITE");
    if (rcnt >= sizeof(XmemDrvrRawIoBlock)) {
      riob = (XmemDrvrRawIoBlock *) arg;
      if (riob->UserArray == NULL) {
	cprintf("xmemDrvr: UserArray is NULL!\n");
	pseterr(EINVAL);
	return SYSERR;
      }

      temptr = riob->UserArray; /* Store the user space address */
      err = cdcm_copy_from_user(lptr, riob->UserArray, 
				sizeof(long) * riob->Items);

      if (err) {
	cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
	pseterr(EFAULT);
	return SYSERR;
      }

      riob->UserArray = lptr; /* fill with the kernel address */
      err = RfmIo(mcon, riob, XmemDrvrWRITE);
      riob->UserArray = temptr; /* Restore user space address */

      if (err != OK) {
	pseterr(EIO);
	return SYSERR;
      }

      /* No copy_to_user here */
      return OK;

    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrRAW_WRITE: /* Write to VMIC 5565 SDRAM */
    IOCTL_TRACK("XmemDrvrRAW_WRITE");
    if (rcnt >= sizeof(XmemDrvrRawIoBlock)) {
      riob = (XmemDrvrRawIoBlock *) arg;
      if (riob->UserArray == NULL) {
	cprintf("xmemDrvr: UserArray is NULL!\n");
	pseterr(EINVAL);
	return SYSERR;
      }

      temptr = riob->UserArray; /* Store the user space address */
      err = cdcm_copy_from_user(lptr, riob->UserArray, 
				sizeof(long) * riob->Items);
      if (err) {
	cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
	pseterr(EFAULT);
	return SYSERR;
      }

      riob->UserArray = lptr; /* fill with the kernel address */
      err = RawIo(mcon, riob, XmemDrvrWRITE);
      riob->UserArray = temptr; /* Restore user space address */

      if (err != OK) {
	pseterr(EIO);
	return SYSERR;
      }

      err = cdcm_copy_to_user(riob->UserArray, lptr,
			      sizeof(long) * riob->Items);
      if (err) {
	pseterr(EFAULT);
	return SYSERR;
      }

      return OK;

    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrSET_DMA_THRESHOLD: /* Set Drivers DMA threshold */
    IOCTL_TRACK("XmemDrvrSET_DMA_THRESHOLD");
    if (lap) {
      if (lav < PAGESIZE)
	wa->DmaThreshold = lav;
      else 
	cprintf("xmemDrvr: DMA_THRESHOLD should be less than PAGESIZE=%ld\n",
		   PAGESIZE);
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrGET_DMA_THRESHOLD: /* Get Drivers DMA threshold */
    IOCTL_TRACK("XmemDrvrGET_DMA_THRESHOLD");
    if (lap) {
      *lap = wa->DmaThreshold;
      return OK;
    }
    pseterr(EINVAL);
    return SYSERR;

  case XmemDrvrCONFIG_CLOSE: /* Close the PLX9656 configuration */
    IOCTL_TRACK("XmemDrvrCONFIG_CLOSE");
    mcon->ConfigOpen = 0;
    return OK;

  case XmemDrvrLOCAL_CLOSE: /* Close the PLX9656 local configuration */
    IOCTL_TRACK("XmemDrvrLOCAL_CLOSE");
    mcon->LocalOpen = 0;
    return OK;

  case XmemDrvrRFM_CLOSE: /* Close VMIC 5565 FPGA Register block RFM */
    IOCTL_TRACK("XmemDrvrRFM_CLOSE");
    mcon->RfmOpen = 0;
    return OK;

  case XmemDrvrRAW_CLOSE: /* Close VMIC 5565 SDRAM */
    IOCTL_TRACK("XmemDrvrRAW_CLOSE");
    mcon->RfmOpen = 0;
    return OK;

  default: break;
  }


  pseterr(ENOTTY); /* Inappropriate I/O control operation */
  cprintf("xmemDrvr: IOCTL Error: request failed.\n");
  cprintf("xmemDrvr: --> fct provided: %ud, cm provided: %ud. \n", fct, cm);
  return SYSERR;
}



/* driver entry points */
struct dldd entry_points = {
  XmemDrvrOpen,         /* open      */
  XmemDrvrClose,        /* close     */
  XmemDrvrRead,         /* read      */
  XmemDrvrWrite,        /* write     */
  XmemDrvrSelect,       /* select    */
  XmemDrvrIoctl,        /* ioctl     */
  XmemDrvrInstall,      /* install   */
  XmemDrvrUninstall,    /* uninstall */
  NULL			/* memory-mapped devices access */
};
