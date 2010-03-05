/**
 * @file skeldrvr.c
 *
 * @brief Skel driver framework
 *
 * This framework implements a standard SKEL compliant driver.
 * To implement a specific driver for a given hardware module
 * you only need to declare the specific part.
 * See the documentation at: ???
 *
 * Copyright (c) 2008-09 CERN
 * @author Julian Lewis <julian.lewis@cern.ch>
 * @author Emilio G. Cota <emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#include <cdcm/cdcm.h>
#include <skeldefs.h>
#include <skeluser.h>
#include <skeluser_ioctl.h>
#include <skeldrvr.h>
#include <skeldrvrP.h>

int skel_isr(void *cookie);

#include <skelvme.h>
#include <skelpci.h>
#include <skelcar.h>

/*
 * @todo use kernel coding style throughout the file
 * @todo clean-up the IOCTL entry point
 */

/*
 * Common variables
 * These variables are shared between the user and the generic parts of skel
 */
SkelDrvrWorkingArea *Wa;	/* working area */

/*
 * Basic skeleton driver standard IOCTL names used by both the driver
 * and by the test program and library.
 */
static const char *SkelStandardIoctlNames[] = {
   [_IOC_NR(SkelDrvrIoctlSET_DEBUG)] = "SET_DEBUG",
   [_IOC_NR(SkelDrvrIoctlGET_DEBUG)] = "GET_DEBUG",

   [_IOC_NR(SkelDrvrIoctlGET_VERSION)] = "GET_VERSION",

   [_IOC_NR(SkelDrvrIoctlSET_TIMEOUT)] = "SET_TIMEOUT",
   [_IOC_NR(SkelDrvrIoctlGET_TIMEOUT)] = "GET_TIMEOUT",

   [_IOC_NR(SkelDrvrIoctlSET_QUEUE_FLAG)] = "SET_QUEUE_FLAG",
   [_IOC_NR(SkelDrvrIoctlGET_QUEUE_FLAG)] = "GET_QUEUE_FLAG",
   [_IOC_NR(SkelDrvrIoctlGET_QUEUE_SIZE)] = "GET_QUEUE_SIZE",
   [_IOC_NR(SkelDrvrIoctlGET_QUEUE_OVERFLOW)] = "GET_QUEUE_OVERFLOW",

   [_IOC_NR(SkelDrvrIoctlSET_MODULE)] = "SET_MODULE",
   [_IOC_NR(SkelDrvrIoctlGET_MODULE)] = "GET_MODULE",
   [_IOC_NR(SkelDrvrIoctlGET_MODULE_COUNT)] = "GET_MODULE_COUNT",
   [_IOC_NR(SkelDrvrIoctlGET_MODULE_MAPS)] = "GET_MODULE_MAPS",

   [_IOC_NR(SkelDrvrIoctlCONNECT)] = "CONNECT",
   [_IOC_NR(SkelDrvrIoctlGET_CLIENT_LIST)] = "GET_CLIENT_LIST",
   [_IOC_NR(SkelDrvrIoctlGET_CLIENT_CONNECTIONS)] = "GET_CLIENT_CONNECTIONS",

   [_IOC_NR(SkelDrvrIoctlENABLE)] = "ENABLE",
   [_IOC_NR(SkelDrvrIoctlRESET)] = "RESET",

   [_IOC_NR(SkelDrvrIoctlGET_STATUS)] = "GET_STATUS",
   [_IOC_NR(SkelDrvrIoctlGET_CLEAR_STATUS)] = "GET_CLEAR_STATUS",

   [_IOC_NR(SkelDrvrIoctlRAW_READ)] = "RAW_READ",
   [_IOC_NR(SkelDrvrIoctlRAW_WRITE)] = "RAW_WRITE",

   [_IOC_NR(SkelDrvrIoctlJTAG_OPEN)] = "JTAG_OPEN",
   [_IOC_NR(SkelDrvrIoctlJTAG_READ_BYTE)] = "JTAG_READ_BYTE",
   [_IOC_NR(SkelDrvrIoctlJTAG_WRITE_BYTE)] = "JTAG_WRITE_BYTE",
   [_IOC_NR(SkelDrvrIoctlJTAG_CLOSE)] = "JTAG_CLOSE"
};

#define SkelDrvrSTANDARD_IOCTL_CALLS ARRAY_SIZE(SkelStandardIoctlNames)

static const char *SkelDrvrDebugNames[] = {
   "AssertionViolation",
   "IoctlTrace",
   "ClientWarning",
   "ModuleWarning",
   "Information",
   "EmulationOn"
};

const char *GetDebugFlagName(SkelDrvrDebugFlag debf)
{

   S32 i;
   U32 msk;

   for (i = 0; i < SkelDrvrDEBUG_NAMES; i++) {
      msk = 1 << i;
      if (msk & debf)
	 return SkelDrvrDebugNames[i];
   }
   return SkelDrvrDebugNames[0];
}

/**
 * @brief check if an IOCTL belongs to the user's bottom-half
 *
 * @param cm - IOCTL number number
 *
 * @return 1 - if the IOCTL belongs to the user
 * @return 0 - otherwise
 */
static int is_user_ioctl(int nr)
{
   return WITHIN_RANGE(_IOC_NR(SkelUserIoctlFIRST), _IOC_NR(nr),
		       _IOC_NR(SkelUserIoctlLAST)) && _IOC_TYPE(nr) == SKELUSER_IOCTL_MAGIC;
}

static const char *GetIoctlName(int nr)
{
   static char dtxt[128];
   int first;

   /* 1st skel's IOCTL */
   first = _IOC_NR(nr) - _IOC_NR(SkelDrvrIoctlSET_DEBUG);

   /* skel's IOCTL */
   if (WITHIN_RANGE(0, first, _IOC_NR(SkelDrvrIoctlLAST_STANDARD)) && _IOC_TYPE(nr) == SKEL_IOCTL_MAGIC)
      return SkelStandardIoctlNames[_IOC_NR(nr)];

   /* non-skel IOCTL */
   if (is_user_ioctl(nr))
      return SkelUserGetIoctlName(nr);

   /* cannot recognise the IOCTL number */
   ksprintf(dtxt, "(illegal)");
   return dtxt;
}

/**
 * @brief debug an IOCTL call
 *
 * @param ccon - client context
 * @param cm - IOCTL command number
 * @param lav - long argument value passed to the IOCTL call
 */
static void DebugIoctl(SkelDrvrClientContext * ccon, int cm, int32_t lav)
{
   if (!(ccon->Debug & SkelDrvrDebugFlagTRACE))
      return;

   report_client(ccon, SkelDrvrDebugFlagTRACE, "module#%d:IOCTL:%s:Arg:%d[0x%x]", ccon->ModuleNumber, GetIoctlName(cm), lav, lav);
}

/*
 * Handle timeout callback from a clients read
 */
static void client_timeout(SkelDrvrClientContext * ccon)
{
   SkelDrvrQueue *q = &ccon->Queue;
   unsigned long flags;

   cdcm_spin_lock_irqsave(&q->lock, flags);
   ccon->Timer = 0;
   ccon->Queue.Size = 0;
   ccon->Queue.Missed = 0;
   cdcm_spin_unlock_irqrestore(&q->lock, flags);

   report_client(ccon, SkelDrvrDebugFlagWARNING, "Client Timeout");
}

/*
 * Handle timeout callback from a module mutex wait
 */
static void module_timeout(SkelDrvrModuleContext * mcon)
{
   report_module(mcon, SkelDrvrDebugFlagMODULE, "Module Timeout");
}

/*
 * Lock module for exclusive access
 */
static int LockModule(SkelDrvrModuleContext * mcon, int use_timer)
{
   int ticks = use_timer && mcon->Timeout ? mcon->Timeout : -1;

   if (tswait(&mcon->Semaphore, SEM_SIGABORT, ticks)) {
      module_timeout(mcon);
      pseterr(EINTR);
      return SYSERR;
   }

   return OK;
}

/*
 * Unlock module exclusive access
 */
static int UnLockModule(SkelDrvrModuleContext * mcon)
{
   ssignal(&mcon->Semaphore);
   return OK;
}

static void SetEndian(void)
{

#ifdef CDCM_LITTLE_ENDIAN
   Wa->Endian = InsLibEndianLITTLE;
#else
   Wa->Endian = InsLibEndianBIG;

#endif
}

static void __update_mcon_status(SkelDrvrModuleContext * mcon, SkelDrvrStandardStatus flag)
{
   mcon->StandardStatus |= flag;
};

/**
 * @brief set module context status
 *
 * @param mcon - module context
 * @param flag - flag to set it to
 *
 * Note: this function acquires the module's mutex
 *
 * @return 0 - on success
 * @return -1 - on failure
 */
static int update_mcon_status(SkelDrvrModuleContext * mcon, SkelDrvrStandardStatus flag)
{
   if (cdcm_mutex_lock_interruptible(&mcon->mutex)) {
      pseterr(EINTR);
      return SYSERR;
   }
   __update_mcon_status(mcon, flag);
   cdcm_mutex_unlock(&mcon->mutex);

   return 0;
}

/* also used to check for Bus Errors */
static S32 GetVersion(SkelDrvrModuleContext * mcon, SkelDrvrVersion * ver)
{
   if (!recoset()) {
      ver->DriverVersion = COMPILE_TIME;
      SkelUserGetModuleVersion(mcon, ver->ModuleVersion);

   } else {
      noreco();
      SK_ERROR("BUS-ERROR @ module#%d", mcon->ModuleNumber);
      if (update_mcon_status(mcon, SkelDrvrStandardStatusBUS_FAULT))
	 return SYSERR;

      pseterr(ENXIO);
      return SYSERR;
   }
   noreco();
   return OK;
}

static unsigned int RawIo(SkelDrvrModuleContext * mcon, SkelDrvrRawIoBlock * riob, int flag)
{
   InsLibAnyAddressSpace *anyas = NULL;
   InsLibModlDesc *modld = NULL;
   void *mmap = NULL;
   char *cp;

   modld = mcon->Modld;
   anyas = InsLibGetAddressSpace(modld, riob->SpaceNumber);

   if (!anyas) {
      report_module(mcon, SkelDrvrDebugFlagMODULE, "%s:IllegalAddressSpace", __FUNCTION__);
      pseterr(ENXIO);
      return SYSERR;
   }

   /*
    * In some cases (mainly PCI), the size of the mapping is not
    * provided in the XML file. In Lynx there's no easy way to
    * get the mapping size, so we just leave it up to the user
    * not to cause a bus error.
    */
   if (anyas->WindowSize && riob->Offset >= anyas->WindowSize) {
      report_module(mcon, SkelDrvrDebugFlagMODULE, "%s: Offset out of range", __FUNCTION__);
      pseterr(EINVAL);
      return SYSERR;
   }

   if (!riob->DataWidth)
      riob->DataWidth = anyas->DataWidth;
   cp = anyas->Mapped;
   mmap = &cp[riob->Offset];

   /* Catch bus errors */
   if (!recoset()) {
      if (flag) {
	 switch (riob->DataWidth) {
	 case 8:
	    if (anyas->Endian == InsLibEndianBIG)
	       cdcm_iowrite8(riob->Data, mmap);
	    else
	       cdcm_iowrite8(riob->Data, mmap);
	    noreco();
	    return OK;
	 case 16:
	    if (anyas->Endian == InsLibEndianBIG)
	       cdcm_iowrite16be(riob->Data, mmap);
	    else
	       cdcm_iowrite16le(riob->Data, mmap);
	    noreco();
	    return OK;
	 case 32:
	    if (anyas->Endian == InsLibEndianBIG)
	       cdcm_iowrite32be(riob->Data, mmap);
	    else
	       cdcm_iowrite32le(riob->Data, mmap);
	    noreco();
	    return OK;
	 default:
	    noreco();
	    pseterr(EINVAL);
	    return SYSERR;
	 }
      } else {
	 switch (riob->DataWidth) {
	 case 8:
	    if (anyas->Endian == InsLibEndianBIG)
	       riob->Data = cdcm_ioread8(mmap);
	    else
	       riob->Data = cdcm_ioread8(mmap);
	    noreco();
	    return OK;
	 case 16:
	    if (anyas->Endian == InsLibEndianBIG)
	       riob->Data = cdcm_ioread16be(mmap);
	    else
	       riob->Data = cdcm_ioread16le(mmap);
	    noreco();
	    return OK;
	 case 32:
	    if (anyas->Endian == InsLibEndianBIG)
	       riob->Data = cdcm_ioread32be(mmap);
	    else
	       riob->Data = cdcm_ioread32le(mmap);
	    noreco();
	    return OK;
	 default:
	    noreco();
	    pseterr(EINVAL);
	    return SYSERR;
	 }
      }
   } else {
      noreco();
      SK_ERROR("BUS-ERROR: Module:%d. Addr:0x%x", mcon->ModuleNumber, (int) mmap);
      if (update_mcon_status(mcon, SkelDrvrStandardStatusBUS_FAULT))
	 return SYSERR;
      pseterr(ENXIO);
      return SYSERR;
   }
}

static void Reset(SkelDrvrModuleContext * mcon)
{
   SkelDrvrModConn *connected = &mcon->Connected;
   unsigned long flags;


   SkelUserHardwareReset(mcon);

   cdcm_spin_lock_irqsave(&connected->lock, flags);
   SkelUserEnableInterrupts(mcon, connected->enabled_ints);
   cdcm_spin_unlock_irqrestore(&connected->lock, flags);

   sreset(&mcon->Semaphore);
   ssignal(&mcon->Semaphore);
}

static void GetStatus(SkelDrvrModuleContext * mcon, SkelDrvrStatus * ssts)
{

   SkelUserGetHardwareStatus(mcon, &ssts->HardwareStatus);
   ssts->StandardStatus = mcon->StandardStatus;
   return;
}

/*
 * call with the queue's lock held
 */
static inline void __q_put(const SkelDrvrReadBuf * rb, SkelDrvrClientContext * ccon)
{
   SkelDrvrQueue *q = &ccon->Queue;

   q->Entries[q->WrPntr] = *rb;
   q->WrPntr = (q->WrPntr + 1) % SkelDrvrQUEUE_SIZE;

   if (q->Size < SkelDrvrQUEUE_SIZE) {
      q->Size++;
      ssignal(&ccon->Semaphore);
   } else {
      q->Missed++;
      q->RdPntr = (q->RdPntr + 1) % SkelDrvrQUEUE_SIZE;
   }
}

/*
 * put read buffer on the queue of a client
 */
static inline void q_put(const SkelDrvrReadBuf * rb, SkelDrvrClientContext * ccon)
{
   unsigned long flags;

   cdcm_spin_lock_irqsave(&ccon->Queue.lock, flags);
   __q_put(rb, ccon);
   cdcm_spin_unlock_irqrestore(&ccon->Queue.lock, flags);
}

/**
 * @brief put the 'read buffer' @rb on the queues of clients
 *
 * @param rb - read buffer to be put in the queues
 * @param cmask - mask of clients whose queues need to be updated
 */
static inline void put_queues(const SkelDrvrReadBuf * rb, const uint32_t cmask)
{
   uint32_t client;
   int i;

   for (i = 0; i < SkelDrvrCLIENT_CONTEXTS; i++) {
      client = cmask & (1 << i);
      if (!client)
	 continue;

      if (Wa->Clients[i])
	 q_put(rb, Wa->Clients[i]);

      if (!(cmask & ~client))
	 break;			/* no one else connected to this interrupt */
   }
}

/* Note: call this function with @connected's lock held */
static inline void __fill_clients_queues(const SkelDrvrModConn * connected, const SkelDrvrReadBuf * rb, const uint32_t imask)
{
   uint32_t interrupt, clients;
   SkelDrvrReadBuf rbuf = *rb;	/* local copy of rb */
   int i;

   for (i = 0; i < SkelDrvrINTERRUPTS; i++) {

      interrupt = imask & (1 << i);
      clients = connected->clients[i];
      if (!interrupt || !clients)
	 continue;

      /* set one bit at a time on the clients' queues */
      rbuf.Connection.ConMask = interrupt;
      put_queues(&rbuf, clients);

      if (!(imask & ~interrupt))
	 break;			/* no remaining interrupts */
   }
}

/**
 * @brief fill the queues of clients connected to the interrupt given by mask
 *
 * @param connected - connections on the module struct
 * @param rb - read buffer to put in the queues
 * @param imask - interrupt mask
 */
static inline void fill_clients_queues(SkelDrvrModConn * connected, const SkelDrvrReadBuf * rb, const uint32_t imask)
{
   unsigned long flags;

   cdcm_spin_lock_irqsave(&connected->lock, flags);
   __fill_clients_queues(connected, rb, imask);
   cdcm_spin_unlock_irqrestore(&connected->lock, flags);
}

/**
 * @brief interrupt handler---reads the HW and update clients' queues
 *
 * @param cookie - module context
 *
 * @return 0 - on success
 * @return -1 - on failure
 */
static int IntrHandler(void *cookie)
{
   SkelDrvrModuleContext *mcon = (SkelDrvrModuleContext *) cookie;
   SkelDrvrModConn *connected;
   SkelDrvrReadBuf rb;
   uint32_t interrupt;

   /* initialise read buffer */
   rb.Connection.Module = mcon->ModuleNumber;

   /* read interrupt mask from hardware */
   SkelUserGetInterruptSource(mcon, &rb.Time, &interrupt);

   if (!interrupt)
      return OK;

   connected = &mcon->Connected;

   fill_clients_queues(connected, &rb, interrupt);

   return OK;
}

/**
 * @brief skel interrupt handler switch
 *
 * @param cookie - cookie passed to the ISR
 *
 * Depending on skel's configuration, either the user's or the default
 * interrupt handler is used to serve interrupts.
 *
 * @return returned value from the interrupt handler
 */
int skel_isr(void *cookie)
{
   if (SkelConf.intrhandler)
      return SkelConf.intrhandler(cookie);
   return IntrHandler(cookie);
}

/**
 * @brief remove a module, given by its module context
 *
 * @param mcon - module context
 *
 * This function is called before freeing a module context. It unhooks
 * the user's bottom-half, unregisters the ISR and unmaps any memory
 * mappings that may have been done. All this when applicable and
 * for any kind of supported bus.
 */
static void RemoveModule(SkelDrvrModuleContext * mcon)
{
   if (mcon->StandardStatus & SkelDrvrStandardStatusEMULATION)
      return;			/* no ISR, no mappings.. exit */

   /* unhook user's stuff */
   SkelUserModuleRelease(mcon);

   switch (mcon->Modld->BusType) {
   case InsLibBusTypePMC:
   case InsLibBusTypePCI:
      return RemovePciModule(mcon);
   case InsLibBusTypeVME:
      return RemoveVmeModule(mcon);
   case InsLibBusTypeCARRIER:
      return RemoveCarModule(mcon);
   default:
      break;
   }
}

/**
 * @brief get the address of a module's context
 *
 * @param modnr - module number (verbatim from the XML file)
 *
 * @return address of the modules' mcon - on success
 * @return NULL - if the module is not found
 */
SkelDrvrModuleContext *get_mcon(int modnr)
{
   SkelDrvrModuleContext *m;
   int i;

   for (i = 0; i < Wa->InstalledModules; i++) {
      m = &Wa->Modules[i];
      if (m->ModuleNumber == modnr)
	 return m;
   }
   return NULL;
}

/**
 * @brief connect a client to an interrupt
 *
 * @param ccon - client context
 * @param conx - connection description
 */
static void Connect(SkelDrvrClientContext * ccon, SkelDrvrConnection * conx)
{
   SkelDrvrModuleContext *mcon;
   SkelDrvrModConn *connected;
   unsigned int j, imsk, cmsk;
   unsigned long flags;

   if (!conx->Module)
      mcon = get_mcon(ccon->ModuleNumber);
   else
      mcon = get_mcon(conx->Module);

   if (mcon == NULL)
      return;

   connected = &mcon->Connected;
   cmsk = 1 << ccon->ClientIndex;
   imsk = conx->ConMask;

   cdcm_spin_lock_irqsave(&connected->lock, flags);
   for (j = 0; j < SkelDrvrINTERRUPTS; j++) {
      if (!(imsk & (1 << j)))
	 continue;
      connected->clients[j] |= cmsk;
      connected->enabled_ints |= imsk;
      SkelUserEnableInterrupts(mcon, connected->enabled_ints);
   }
   cdcm_spin_unlock_irqrestore(&connected->lock, flags);
}

/**
 * @brief disconnect a client from certain interrupts
 *
 * @param ccon - client context
 * @param conx - module and interrupts to disconnect from
 *
 * If no module is specificied, current client's module is taken.
 * An empty interrupt mask means 'disconnect from all the interrupts for
 * this module'
 */
static void DisConnect(SkelDrvrClientContext * ccon, SkelDrvrConnection * conx)
{
   SkelDrvrModuleContext *mcon;
   SkelDrvrModConn *connected;
   unsigned int j, imsk, cmsk;
   unsigned long flags;

   if (!conx->Module)
      mcon = get_mcon(ccon->ModuleNumber);
   else
      mcon = get_mcon(conx->Module);

   if (mcon == NULL)
      return;

   connected = &mcon->Connected;

   /* Disconnect from all the ints on this module */
   if (!conx->ConMask)
      conx->ConMask = ~conx->ConMask;

   cmsk = 1 << ccon->ClientIndex;
   imsk = conx->ConMask;

   cdcm_spin_lock_irqsave(&connected->lock, flags);

   for (j = 0; j < SkelDrvrINTERRUPTS; j++) {
      /* check interrupt mask and that there are clients connected */
      if (!(imsk & (1 << j)) || !(connected->clients[j] & cmsk))
	 continue;
      connected->clients[j] &= ~cmsk;

      if (connected->clients[j])
	 continue;
      /*
       * if there are no more clients connected to it, disable the
       * interrupt on the module
       */
      connected->enabled_ints &= ~imsk;
      SkelUserEnableInterrupts(mcon, connected->enabled_ints);
   }

   cdcm_spin_unlock_irqrestore(&connected->lock, flags);
}

/**
 * @brief disconnect a client from all the interrupts on any module
 *
 * @param ccon - client context
 */
static void DisConnectAll(SkelDrvrClientContext * ccon)
{

   SkelDrvrConnection conx;
   unsigned int i;

   conx.ConMask = 0;
   for (i = 0; i < Wa->InstalledModules; i++) {
      conx.Module = Wa->Modules[i].ModuleNumber;
      DisConnect(ccon, &conx);
   }
}

static inline void set_mcon_defaults(SkelDrvrModuleContext * mcon)
{
   /* default timeout */
   mcon->Timeout = SkelDrvrDEFAULT_MODULE_TIMEOUT;

   /* debugging */
   mcon->Debug = Wa->Drvrd->DebugFlags;

   /* emulation */
   if (Wa->Drvrd->EmulationFlags) {
      mcon->Debug |= SkelDrvrDebugFlagEMULATION;
      mcon->StandardStatus |= SkelDrvrStandardStatusEMULATION;
   }

   /* initialise the spinlock */
   cdcm_spin_lock_init(&mcon->lock);

   /* initialise the connection's lock */
   cdcm_spin_lock_init(&mcon->Connected.lock);

   /* initialise the mutex */
   cdcm_mutex_init(&mcon->mutex);

   /* initialise the semaphore */
   sreset(&mcon->Semaphore);
   ssignal(&mcon->Semaphore);

   /*
    * Set the module status to NO_ISR as a default.
    * If there's an ISR, the ISR installers will clear this bit.
    */
   update_mcon_status(mcon, SkelDrvrStandardStatusNO_ISR);
}

/**
 * @brief unmap mapping flagged as 'FreeModifierFlag'
 *
 * @param mcon - module context
 *
 * An address space with the 'FreeModifierFlag' set will be erased shortly after
 * being mapped; in between, a SkelUser function to initialise the module is
 * called.
 */
static void unmap_flagged(SkelDrvrModuleContext * mcon)
{
   InsLibBusType bus = mcon->Modld->BusType;

   switch (bus) {
   case InsLibBusTypeVME:
      unmap_vmeas(mcon, 0);
      break;
      /* do nothing for PCI or Carrier */
   case InsLibBusTypePMC:
   case InsLibBusTypePCI:
   case InsLibBusTypeCARRIER:
      break;
   default:
      break;
   }
}

/**
 * @brief start-up a module; put it into a 'ready-to-use' state
 *
 * @param mcon - module context
 *
 * @return 0 - on failure
 * @return 1 - on success
 */
static int AddModule(SkelDrvrModuleContext * mcon)
{
   InsLibBusType bus;
   int mod_ok;

   bus = mcon->Modld->BusType;

   switch (bus) {
   case InsLibBusTypeVME:
      mod_ok = AddVmeModule(mcon);
      break;
   case InsLibBusTypePMC:
   case InsLibBusTypePCI:
      mod_ok = AddPciModule(mcon);
      break;
   case InsLibBusTypeCARRIER:
      mod_ok = AddCarModule(mcon);
      break;
   default:
      SK_ERROR("Unsupported BUS type: %d", bus);
      mod_ok = 0;
   }

   if (!mod_ok)			/* AddModule didn't work */
      return 0;

   /* user's module installation bottom-half */
   mod_ok = SkelUserModuleInit(mcon);

   if (mod_ok == SkelUserReturnFAILED) {
      /* abort: delete mappings, unregister ISR */
      RemoveModule(mcon);
      return 0;
   }

   /*
    * Sometimes the user just wants to:
    * 1. Map a certain hardware region
    * 2. Configure it (that's why SkelUserModuleInit is for)
    * 3. Unmap it (so that we don't run out of address space)
    * So at this stage we check for each of the mappings and unmap
    * them if the flag 'FreeModifierFlag' is set.
    */
   unmap_flagged(mcon);

   /* everything OK */
   Wa->InstalledModules++;

   return 1;
}

/**
 * @brief allocate and initialise the working area
 *
 * @param drvrd - driver descriptor
 *
 * Note: do not use SK_INFO and friends before Wa is properly set.
 *
 * @return 0 - on failure
 * @return 1 - on success
 */
static int wa_init(InsLibDrvrDesc * drvrd)
{
   SkelDrvrWorkingArea *wa;

   /* Allocate the driver working area. */
   wa = (SkelDrvrWorkingArea *) sysbrk(sizeof(SkelDrvrWorkingArea));

   if (wa == NULL) {
      kkprintf("Skel: Cannot allocate enough memory (WorkingArea)");
      pseterr(ENOMEM);
      return 0;
   }

   Wa = wa;			/* Global working area pointer */

   /* initialise Wa */
   bzero((void *) Wa, sizeof(SkelDrvrWorkingArea));
   SetEndian();
   /* hook driver description onto Wa */
   Wa->Drvrd = drvrd;

   return 1;
}

/**
 * @brief install modules defined in the XML description
 *
 * @return -1 -- didn't install all the modules requested in .xml
 * @return -2 -- requested module amount exeeds supported one
 * @return  0 -- all OK
 */
static int modules_install(void)
{
   SkelDrvrModuleContext *mcon;
   InsLibModlDesc *modld;
   int mod_ok;
   int i, rc = 0;

   /* safety first */
   if (Wa->Drvrd->ModuleCount > SkelDrvrMODULE_CONTEXTS) {
      SK_ERROR("Requested Module amount (%d) exeeds supported"
	       " one (%d). SkelDrvrMODULE_CONTEXTS should be" " extended\n", Wa->Drvrd->ModuleCount, SkelDrvrMODULE_CONTEXTS);
      return -2;
   }

   modld = Wa->Drvrd->Modules;
   /*
    * go through the module descriptions and install the hardware.
    * In case of failure, the subscript 'i' is not incremented.
    */
   for (i = 0; i < Wa->Drvrd->ModuleCount;) {
      if (!modld)
	 break;
      /* initialise the module context */
      mcon = &Wa->Modules[i];
      mcon->Modld = modld;
      mcon->ModuleNumber = modld->ModuleNumber;

      /* the rest of mcon are the default values */
      set_mcon_defaults(mcon);

      /*
       * once mcon is set-up, create the mappings, register the ISR
       * and initialise the hardware
       */
      mod_ok = AddModule(mcon);

      if (mod_ok) {
	 SK_INFO("Module#%d installed OK", modld->ModuleNumber);

	 mcon->InUse = 1;
	 i++;			/* ensure that there are no gaps in Wa->Modules */
      } else {
	 SK_WARN("Error installing Module#%d", modld->ModuleNumber);
	 rc = -1;
      }

      modld = modld->Next;
   }

   return rc;
}

char *SkelDrvrInstall(void *infofile)
{
   InsLibDrvrDesc **drvrinfo = infofile;
   InsLibDrvrDesc *drvrd;

   drvrd = InsLibCloneOneDriver(*drvrinfo);

   if (drvrd == NULL) {
      kkprintf("InsLibCloneOneDriver failed\n");
      pseterr(ENOMEM);
      return (char *) SYSERR;
   }

   if (!wa_init(drvrd)) {
      kkprintf("Working Area Initialisation failed\n");
      pseterr(ENOMEM);
      goto out_err;
   }

   if (!Wa->Drvrd->ModuleCount) {
      SK_WARN("BUG in the descriptor tree: ModuleCount is empty");
      pseterr(EINVAL);
      goto out_err;
   }

   modules_install();

   if (Wa->InstalledModules != Wa->Drvrd->ModuleCount) {
      if (!Wa->InstalledModules) {
	 SK_WARN("Not installed any modules");
	 pseterr(ENODEV);
	 goto out_err;
      } else {
	 SK_WARN("Only installed %d out of %d modules", Wa->InstalledModules, Wa->Drvrd->ModuleCount);
      }
   } else {
      SK_INFO("Installed %d (out of %d) modules", Wa->InstalledModules, Wa->Drvrd->ModuleCount);
   }
   return (char *) Wa;
 out_err:
   if (drvrd)
      InsLibFreeDriver(drvrd);
   if (Wa)
      sysfree((void *) Wa, sizeof(SkelDrvrWorkingArea));
   return (char *) SYSERR;
}

/* Note: call with the queue's lock held */
static void __reset_queue(SkelDrvrClientContext * ccon)
{
   SkelDrvrQueue *q = &ccon->Queue;

   q->Size = 0;
   q->RdPntr = 0;
   q->WrPntr = 0;
   q->Missed = 0;
   sreset(&ccon->Semaphore);
}

static void reset_queue(SkelDrvrClientContext * ccon)
{
   unsigned long flags;

   cdcm_spin_lock_irqsave(&ccon->Queue.lock, flags);
   __reset_queue(ccon);
   cdcm_spin_unlock_irqrestore(&ccon->Queue.lock, flags);
}

/*
 * initialise a client's context
 * @return 0 - on success
 * @return -1 - on failure
 */
int client_init(SkelDrvrClientContext * ccon, int cidx, struct cdcm_file *flp)
{
   int client_ok;

   bzero((void *) ccon, sizeof(SkelDrvrClientContext));

   ccon->cdcmf = flp;
   ccon->Timeout = SkelDrvrDEFAULT_CLIENT_TIMEOUT;
   ccon->Pid = getpid();
   ccon->ClientIndex = cidx;

   /* select by default the first installed module */

   if (Wa->InstalledModules)
      ccon->ModuleNumber = Wa->Modules[0].ModuleNumber;

   cdcm_spin_lock_init(&ccon->lock);

   cdcm_spin_lock_init(&ccon->Queue.lock);
   reset_queue(ccon);

   /* user's client initialisation bottom-half */
   client_ok = SkelUserClientInit(ccon);
   if (client_ok == SkelUserReturnFAILED)	/* errno must be set */
      return -1;

   return 0;
}

/*
 * Driver's open entry point
 */
int SkelDrvrOpen(void *wa, int dnm, struct cdcm_file *flp)
{
   SkelDrvrClientContext *ccon;	/* Client context */
   int cidx;			/* Client index */

   ccon = (SkelDrvrClientContext *) sysbrk(sizeof(SkelDrvrClientContext));
   if (ccon == NULL) {
      pseterr(ENOMEM);		// No more memory available
      return SYSERR;
   }

   /* Restriction from client mask uint32_t */
   /* SkelDrvrCLIENT_CONTEXTS must be no more than 32  */

   for (cidx = 0; cidx < SkelDrvrCLIENT_CONTEXTS; cidx++) {
      if (Wa->Clients[cidx] == NULL) {
	 Wa->Clients[cidx] = ccon;
	 break;
      }
   }
   if (cidx == SkelDrvrCLIENT_CONTEXTS) {
      sysfree((void *) ccon, sizeof(SkelDrvrClientContext));
      pseterr(ENOSPC);		// No space left
      return SYSERR;
   }

   /* Initialise the client's context */

   if (client_init(ccon, cidx, flp)) {
      Wa->Clients[cidx] = NULL;
      sysfree((void *) ccon, sizeof(SkelDrvrClientContext));
      pseterr(EBADF);
      return SYSERR;
   }

   flp->buffer = (char *) ccon;
   return OK;
}

/**
 * @brief Close E.P.
 *
 * @param wa  -- working area
 * @param flp -- file pointer
 *
 * <long-description>
 *
 * @return SYSERR -- failed
 * @return OK     -- success
 */
int SkelDrvrClose(void *wa, struct cdcm_file *flp)
{
   SkelDrvrClientContext *ccon;	/* Client context */

   ccon = (SkelDrvrClientContext *) flp->buffer;
   if (!ccon) {
      pseterr(EADDRNOTAVAIL);	/* Address not available */
      return SYSERR;
   }
   DisConnectAll(ccon);
   SkelUserClientRelease(ccon);	/* hook the user */
   report_client(ccon, SkelDrvrDebugFlagTRACE, "Close called");
   Wa->Clients[ccon->ClientIndex] = NULL;
   sysfree((void *) ccon, sizeof(SkelDrvrClientContext));
   return OK;
}

/**
 * @brief uninstall all the modules
 */
static void modules_uninstall(void)
{
   SkelDrvrModuleContext *mcon;
   int i;

   /* Uninstall all the modules */
   for (i = 0; i < Wa->InstalledModules; i++) {
      mcon = &Wa->Modules[i];

      SK_INFO("Uninstalling Module#%d (out of %d)", mcon->ModuleNumber, Wa->InstalledModules);
      RemoveModule(mcon);
   }
}

int SkelDrvrUninstall(void *wa)
{
   SkelDrvrClientContext *ccon;
   int i;

   for (i = 0; i < SkelDrvrCLIENT_CONTEXTS; i++) {
      ccon = Wa->Clients[i];
      if (!ccon)
	 continue;

      /* A Non null client context means its still open */

      report_client(ccon, 0, "fd still open: Not uninstalled");
      pseterr(EBUSY);
      return SYSERR;
   }

   SK_INFO("Uninstalling driver...");

   modules_uninstall();

   SK_INFO("Driver uninstalled");

   InsLibFreeDriver(Wa->Drvrd);
   sysfree((void *) Wa, sizeof(SkelDrvrWorkingArea));
   Wa = NULL;

   return OK;
}

/* Note: call with the queue's lock held */
static void __q_get(SkelDrvrReadBuf * rb, SkelDrvrQueue * q)
{
   *rb = q->Entries[q->RdPntr];
   q->RdPntr = (q->RdPntr + 1) % SkelDrvrQUEUE_SIZE;
   q->Size--;
}

/**
 * @brief read from a client's queue
 *
 * @param rb - read buffer to put the read information in
 * @param ccon - client context
 */
static void q_get(SkelDrvrReadBuf * rb, SkelDrvrClientContext * ccon)
{
   SkelDrvrQueue *q = &ccon->Queue;
   unsigned long flags;

   cdcm_spin_lock_irqsave(&q->lock, flags);
   __q_get(rb, q);
   cdcm_spin_unlock_irqrestore(&q->lock, flags);
}

static int SkelDrvrRead(void *wa, struct cdcm_file *flp, char *u_buf, int len)
{
   SkelDrvrClientContext *ccon;
   SkelDrvrQueue *q;
   SkelDrvrReadBuf *rb;
   unsigned long flags;

   ccon = (SkelDrvrClientContext *) flp->buffer;
   if (!ccon) {
      pseterr(EADDRNOTAVAIL);	/* Address not available */
      return SYSERR;
   }
   q = &ccon->Queue;

   cdcm_spin_lock_irqsave(&q->lock, flags);
   if (q->QueueOff)
      __reset_queue(ccon);
   cdcm_spin_unlock_irqrestore(&q->lock, flags);

   /* wait for something new in the queue */
   if (tswait(&ccon->Semaphore, SEM_SIGABORT, ccon->Timeout)) {
      client_timeout(ccon);
      pseterr(EINTR);
      return 0;
   }

   /* read from the queue */
   rb = (SkelDrvrReadBuf *) u_buf;
   q_get(rb, ccon);

   return sizeof(SkelDrvrReadBuf);
}

/*
 * read entry point switch
 * Depending on skel's configuration, either the user's or the default
 */
int skel_read(void *wa, struct cdcm_file *f, char *buf, int len)
{
   if (SkelConf.read)
      return SkelConf.read(wa, f, buf, len);
   return SkelDrvrRead(wa, f, buf, len);
}

/*
 * default write entry point---simulates interrupts
 */
static int SkelDrvrWrite(void *wa, struct cdcm_file *flp, char *u_buf, int len)
{
   SkelDrvrModuleContext *mcon;
   SkelDrvrConnection *conx;
   SkelDrvrModConn *connected;
   SkelDrvrReadBuf rb;

   conx = (SkelDrvrConnection *) u_buf;

   mcon = get_mcon(conx->Module);
   if (mcon == NULL) {
      pseterr(EFAULT);
      return SYSERR;
   }
   connected = &mcon->Connected;

   bzero((void *) &rb, sizeof(SkelDrvrReadBuf));

   /* initialise read buffer */
   rb.Connection = *conx;
   SkelUserGetUtc(mcon, &rb.Time);

   fill_clients_queues(connected, &rb, conx->ConMask);

   return sizeof(SkelDrvrConnection);
}

/*
 * write entry point switch
 * Depending on skel's configuration, either the user's or the default
 * write() entry point is called.
 */
int skel_write(void *wa, struct cdcm_file *f, char *buf, int len)
{
   if (SkelConf.write)
      return SkelConf.write(wa, f, buf, len);
   return SkelDrvrWrite(wa, f, buf, len);
}

/**
 * @brief
 *
 * @param wa  -- Working area
 * @param flp -- File pointer
 * @param wch -- Read/Write direction
 * @param ffs -- Selection structures
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
int SkelDrvrSelect(void *wa, struct cdcm_file *flp, int wch, struct sel *ffs)
{
   SkelDrvrClientContext *ccon;

   ccon = (SkelDrvrClientContext *) flp->buffer;
   if (!ccon) {
      pseterr(EADDRNOTAVAIL);	/* Address not available */
      return SYSERR;
   }

   if (wch == SREAD) {
      ffs->iosem = (S32 *) & ccon->Semaphore;	/* Watch out here I hope   */
      return OK;		/* the system dosn't swait */
   }
   /* the read does it too !! */
   pseterr(EACCES);		/* Permission denied */
   return SYSERR;
}

static void get_module_maps_ioctl(SkelDrvrModuleContext * mcon, void *arg)
{
   InsLibAnyModuleAddress *ma;
   InsLibAnyAddressSpace *asp;
   InsLibModlDesc *modld;
   SkelDrvrMaps *m = (SkelDrvrMaps *) arg;
   int i = 0;

   bzero((void *) m, sizeof(SkelDrvrMaps));

   modld = mcon->Modld;
   if (!modld)
      return;

   ma = (InsLibAnyModuleAddress *) modld->ModuleAddress;
   if (!ma)
      return;

   asp = ma->AnyAddressSpace;

   while (asp && i < SkelDrvrMAX_MAPS) {

      m->Maps[i].Mapped = (unsigned long) asp->Mapped;
      m->Maps[i].SpaceNumber = asp->SpaceNumber;
      m->Mappings++;

      i++;
      asp = asp->Next;
   }
}

/**
 * @brief check if the bounds of an IOCTL are correct
 *
 * @param nr - IOCTL number
 * @param arg - IOCTL passed argument
 *
 * @return 1 - if the bounds are OK
 * @return 0 - otherwise
 */
static int bounds_ok(int nr, int arg)
{
   int r = rbounds(arg);
   int w = wbounds(arg);

   switch (_IOC_DIR(nr)) {
   case _IOC_READ:
      if (w < _IOC_SIZE(nr))
	 return 0;
      break;
   case _IOC_WRITE:
      if (r < _IOC_SIZE(nr))
	 return 0;
      break;
   case (_IOC_READ | _IOC_WRITE):
      if (w < _IOC_SIZE(nr) || r < _IOC_SIZE(nr))
	 return 0;
      break;
   case _IOC_NONE:
   default:
      break;
   }

   return 1;
}

static int get_queue_size(SkelDrvrQueue * q)
{
   unsigned long flags;
   int retval;

   cdcm_spin_lock_irqsave(&q->lock, flags);
   retval = q->Size;
   cdcm_spin_unlock_irqrestore(&q->lock, flags);

   return retval;
}

static int get_queue_overflow(SkelDrvrQueue * q)
{
   unsigned long flags;
   int retval;

   cdcm_spin_lock_irqsave(&q->lock, flags);
   retval = q->Missed;
   q->Missed = 0;
   cdcm_spin_unlock_irqrestore(&q->lock, flags);

   return retval;
}

static int get_queue_flag(SkelDrvrQueue * q)
{
   unsigned long flags;
   int retval;

   cdcm_spin_lock_irqsave(&q->lock, flags);
   retval = q->QueueOff;
   cdcm_spin_unlock_irqrestore(&q->lock, flags);

   return retval;
}

static void set_queue_flag(SkelDrvrQueue * q, int value)
{
   unsigned long flags;

   cdcm_spin_lock_irqsave(&q->lock, flags);
   q->QueueOff = !!value;
   cdcm_spin_unlock_irqrestore(&q->lock, flags);
}

int SkelDrvrIoctl(void *wa,	/* Working area */
		  struct cdcm_file *flp,	/* File pointer */
		  int cm,	/* IOCTL command */
		  char *arg)
{				/* Data for the IOCTL */

   SkelDrvrModuleContext *mcon;	/* Module context */
   SkelDrvrClientContext *ccon;	/* Client context */
   SkelDrvrConnection *conx;
   SkelDrvrVersion *ver;
   SkelDrvrClientList *cls;
   SkelDrvrClientConnections *ccn;
   SkelDrvrRawIoBlock *riob;
   SkelDrvrStatus *ssts;
   SkelDrvrDebug *db;

   S32 i, j;
   S32 clientnr;		/* Client number */
   S32 lav, *lap;		/* Long Value pointed to by Arg */
   U16 sav;			/* Short argument and for Jtag IO */
   int rcnt, wcnt;		/* Readable, Writable byte counts at arg address */
   U32 cmsk;
   unsigned long flags;

   /* Check argument contains a valid address for reading or writing. */
   /* We can not allow bus errors to occur inside the driver due to   */
   /* the caller providing a garbage address in "arg". So if arg is   */
   /* not null set "rcnt" and "wcnt" to contain the byte counts which */
   /* can be read or written to without error. */

   if (arg != NULL) {
      rcnt = rbounds((int) arg);	/* Number of readable bytes without error */
      wcnt = wbounds((int) arg);	/* Number of writable bytes without error */

      if (!bounds_ok(cm, (int) arg)) {
	 SK_WARN("SkelDrvrIoctl: ILLEGAL NON NULL ARG, "
		 "RCNT=%d/%d, WCNT=%d/%d, cmd=%d (%s) pid=%d",
		 rcnt, _IOC_SIZE(cm), wcnt, _IOC_SIZE(cm), cm, GetIoctlName(cm), (int) getpid());

	 pseterr(EINVAL);	/* Invalid argument */
	 return SYSERR;
      }
      lav = *((S32 *) arg);	/* Long argument value */
      lap = (S32 *) arg;	/* Long argument pointer */
   } else {
      rcnt = 0;
      wcnt = 0;
      lav = 0;
      lap = NULL;		/* Null arg = zero read/write counts */
   }
   sav = (U16) lav;		/* Short argument value */

   /* We can't control a file which is not open. */

   ccon = (SkelDrvrClientContext *) flp->buffer;
   if (!ccon) {
      pseterr(EADDRNOTAVAIL);	/* Address not available */
      return SYSERR;
   }
   clientnr = ccon->ClientIndex;

   /* Set up some useful module pointers */

   /* Note: mcon might fail, although in some cases we don't care */
   mcon = get_mcon(ccon->ModuleNumber);

   DebugIoctl(ccon, cm, lav);

   /* ================================= */
   /* Decode callers command and do it. */
   /* ================================= */

   switch (cm) {

      /* @todo GET/SET debug: the value in ClientPid is ignored */
   case SkelDrvrIoctlSET_DEBUG:
      db = (SkelDrvrDebug *) arg;
      ccon->Debug = db->DebugFlag;
      return OK;

   case SkelDrvrIoctlGET_DEBUG:
      db = (SkelDrvrDebug *) arg;
      db->DebugFlag = ccon->Debug;
      return OK;
      break;

   case SkelDrvrIoctlGET_VERSION:
      ver = (SkelDrvrVersion *) arg;
      GetVersion(mcon, ver);
      return OK;
      break;

   case SkelDrvrIoctlSET_TIMEOUT:
      ccon->Timeout = lav;
      return OK;

   case SkelDrvrIoctlGET_TIMEOUT:
      if (lap) {
	 *lap = ccon->Timeout;
	 return OK;
      }
      break;

   case SkelDrvrIoctlSET_QUEUE_FLAG:
      set_queue_flag(&ccon->Queue, lav);
      return OK;

   case SkelDrvrIoctlGET_QUEUE_FLAG:
      if (lap) {
	 *lap = get_queue_flag(&ccon->Queue);
	 return OK;
      }
      break;

   case SkelDrvrIoctlGET_QUEUE_SIZE:
      if (lap) {
	 *lap = get_queue_size(&ccon->Queue);
	 return OK;
      }
      break;

   case SkelDrvrIoctlGET_QUEUE_OVERFLOW:
      if (lap) {
	 *lap = get_queue_overflow(&ccon->Queue);
	 return OK;
      }
      break;

   case SkelDrvrIoctlSET_MODULE:
      mcon = get_mcon(lav);
      if (mcon == NULL) {
	 SK_WARN("SET_MODULE: Module %d doesn't exist", lav);
	 pseterr(EINVAL);
	 return SYSERR;
      }
      ccon->ModuleNumber = lav;
      return OK;
      break;

   case SkelDrvrIoctlGET_MODULE:
      if (lap) {
	 *lap = ccon->ModuleNumber;
	 return OK;
      }
      break;

   case SkelDrvrIoctlGET_MODULE_COUNT:
      if (lap) {
	 *lap = Wa->InstalledModules;
	 return OK;
      }
      break;

   case SkelDrvrIoctlGET_MODULE_MAPS:
      get_module_maps_ioctl(mcon, arg);
      return OK;
      break;

   case SkelDrvrIoctlCONNECT:
      conx = (SkelDrvrConnection *) arg;
      if (conx->ConMask)
	 Connect(ccon, conx);
      else {
	 if (!conx->Module)
	    DisConnectAll(ccon);
	 else
	    DisConnect(ccon, conx);
      }
      return OK;
      break;

   case SkelDrvrIoctlGET_CLIENT_LIST:
      cls = (SkelDrvrClientList *) lap;
      bzero((void *) cls, sizeof(SkelDrvrClientList));
      for (i = 0; i < SkelDrvrCLIENT_CONTEXTS; i++) {
	 ccon = Wa->Clients[i];
	 if (ccon)
	    cls->Pid[cls->Size++] = ccon->Pid;
      }
      return OK;
      break;

   case SkelDrvrIoctlGET_CLIENT_CONNECTIONS:
      ccn = (SkelDrvrClientConnections *) arg;
      if (ccn->Pid == 0)
	 ccn->Pid = ccon->Pid;
      ccn->Size = 0;

      cmsk = 0;
      for (j = 0; j < SkelDrvrCLIENT_CONTEXTS; j++) {
	 ccon = Wa->Clients[j];
	 if ((ccon) && (ccn->Pid == ccon->Pid)) {
	    cmsk |= 1 << j;
	 }
      }
      ccon = Wa->Clients[clientnr];
      if (cmsk == 0)
	 break;

      for (i = 0; i < SkelDrvrMODULE_CONTEXTS; i++) {
	 SkelDrvrModConn *connected = &Wa->Modules[i].Connected;

	 cdcm_spin_lock_irqsave(&connected->lock, flags);
	 for (j = 0; j < SkelDrvrINTERRUPTS; j++) {
	    if (!(connected->clients[j] & cmsk))
	       continue;
	    ccn->Connections[ccn->Size].Module = i + 1;
	    ccn->Connections[ccn->Size].ConMask = 1 << j;
	    if (++(ccn->Size) >= SkelDrvrCONNECTIONS)
	       break;
	 }
	 cdcm_spin_unlock_irqrestore(&connected->lock, flags);
      }
      return OK;
      break;

   case SkelDrvrIoctlENABLE:
      if (SkelUserHardwareEnable(mcon, lav) == SkelUserReturnOK)
	 mcon->StandardStatus &= ~SkelDrvrStandardStatusDISABLED;
      else
	 mcon->StandardStatus |= SkelDrvrStandardStatusDISABLED;
      return OK;
      break;

   case SkelDrvrIoctlRESET:
      Reset(mcon);
      return OK;
      break;

   case SkelDrvrIoctlGET_STATUS:
      ssts = (SkelDrvrStatus *) arg;
      GetStatus(mcon, ssts);
      return OK;
      break;

   case SkelDrvrIoctlJTAG_OPEN:
      if (LockModule(mcon, 0) == OK) {	/* Lock forever, no timeout */
	 mcon->StandardStatus |= SkelDrvrStandardStatusFLASH_OPEN;
	 return OK;
      }
      break;

   case SkelDrvrIoctlJTAG_CLOSE:
      mcon->StandardStatus &= ~SkelDrvrStandardStatusFLASH_OPEN;
      mcon->FlashOpen = 0;
      Reset(mcon);
      UnLockModule(mcon);	/* unlock the module (was locked by JTAG_OPEN) */
      return OK;

   case SkelDrvrIoctlJTAG_READ_BYTE:
      if (mcon->StandardStatus & SkelDrvrStandardStatusFLASH_OPEN) {
	 if (SkelUserJtagReadByte(mcon, lap) == SkelUserReturnOK)
	    return OK;
      }
      pseterr(EBUSY);		/* Device busy, not opened */
      return SYSERR;

   case SkelDrvrIoctlJTAG_WRITE_BYTE:	/* Up date configuration bit stream accross JTAG */
      if (mcon->StandardStatus & SkelDrvrStandardStatusFLASH_OPEN) {
	 if (SkelUserJtagWriteByte(mcon, (lav & 0xFF)) == SkelUserReturnOK)
	    return OK;
      }
      pseterr(EBUSY);		/* Device busy, not opened */
      return SYSERR;

   case SkelDrvrIoctlRAW_READ:
      riob = (SkelDrvrRawIoBlock *) arg;
      if (SkelConf.rawio)
	 return SkelConf.rawio(mcon, riob, 0);
      return RawIo(mcon, riob, 0);
      break;

   case SkelDrvrIoctlRAW_WRITE:
      riob = (SkelDrvrRawIoBlock *) arg;
      if (SkelConf.rawio)
	 return SkelConf.rawio(mcon, riob, 1);
      return RawIo(mcon, riob, 1);
      break;

   default:
      if (is_user_ioctl(cm)) {
	 SkelUserReturn uret;

	 if (mcon == NULL) {
	    pseterr(EINVAL);
	    return SYSERR;
	 }

	 uret = SkelUserIoctls(ccon, mcon, cm, arg);
	 return uret == SkelUserReturnOK ? OK : SYSERR;
      }
      break;
   }

   pseterr(ENOTTY);		/* IOCTL error - ENOTTY dictated by POSIX */
   return SYSERR;
}

struct dldd entry_points = {
   SkelDrvrOpen,
   SkelDrvrClose,
   skel_read,
   skel_write,
   SkelDrvrSelect,
   SkelDrvrIoctl,
   SkelDrvrInstall,
   SkelDrvrUninstall
};
