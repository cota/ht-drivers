/* ========================================================================== */
/* Emulate LynxOs under Linux for device drivers. I have some LynxOs drivers  */
/* I would like to emulate under Linux rather than do a port each time. As    */
/* long as the LynxOs driver is not too exotic and single threaded, this      */
/* should be a good way of getting a driver running without a port.           */
/* Julian Lewis May 2004.                                                     */
/* ========================================================================== */

#include <EmulateLynxOs.h>
#include <DrvrSpec.h>
#include <linux/version.h>

#ifndef L864
#include <linux/interrupt.h>
#endif

extern int debug;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
irqreturn_t LynxOsIntrHandler(int irq, void *arg, struct pt_regs *regs);
#else
irqreturn_t LynxOsIntrHandler(int irq, void *arg);
#endif

static DEFINE_MUTEX(emulate_mutex);

LynxOsIsr lynxos_isrs[LynxOsMAX_DEVICE_COUNT];

/* ============================================================================ */
/* LynxOs Memory allocation routines                                            */
/* ============================================================================ */

/* ============================================================================ */
/* Emulate LynxOs call sysbrk                                                   */

char *sysbrk(unsigned long size) {
char *cp;
int i;

   cp = vmalloc(size);
   if (cp) {
      for (i=0; i<size; i++) cp[i] = 0;

      if (debug & LynxOsDEBUG_MODULE)
	 printk(KERN_INFO "Debug: lynx_os: sysbrk: size: %ld pntr: 0x%lx OK\n",
		(unsigned long) size, (unsigned long) cp);

      return cp;
   }
   if (debug & LynxOsDEBUG_ERRORS)
      printk(KERN_WARNING "lynx_os: sysbrk: ENOMEM: alloc size= %ld pntr= 0x%lx\n",
	     (unsigned long) size, (unsigned long) cp);
   return NULL;
}

/* ============================================================================ */
/* Emulate LynxOs call sysfree                                                  */

void sysfree(char *cp, unsigned long size) {

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: lynx_os: sysfree: size: %ld pntr: 0x%lx\n",
	     (unsigned long) size, (unsigned long) cp);

   vfree(cp);
}

/* ============================================================================ */
/* LynxOs Timeout routines                                                      */
/* ============================================================================ */

#define TIMERS 16

typedef struct {
   int               TimerInUse;
   struct timer_list Timer;
   int             (*TimerCallback)();
   char             *Arg;
 } LynxOsTimer;

LynxOsTimer lynxos_timers[TIMERS];

/* ============================================================================ */

void LynxOsTimersInitialize() {

int i;

   mutex_lock(&emulate_mutex);
   for (i=0; i<TIMERS; i++) {
      lynxos_timers[i].TimerInUse = 0;
      init_timer(&lynxos_timers[i].Timer);
   }
   mutex_unlock(&emulate_mutex);
}

/* ============================================================================ */
/* Generic timer callback                                                       */

void timer_callback(unsigned long arg) {
int rubbish;

int i;

   mutex_lock(&emulate_mutex);
   i = arg -1;

   if ((i >=0 ) && (i < TIMERS)) {
      lynxos_timers[i].TimerInUse = 0;

      if (debug & LynxOsDEBUG_MODULE)
	 printk(KERN_INFO "Debug: lynx_os: timer_callback: arg: %u\n",
		(unsigned int) arg);

      rubbish = lynxos_timers[i].TimerCallback(lynxos_timers[i].Arg);
   }
   if (debug & LynxOsDEBUG_ERRORS) {
      printk(KERN_WARNING "lynx_os: timer_callback: No such timer: %d\n",
	  (unsigned int) arg);
   }
   mutex_unlock(&emulate_mutex);
}

/* ============================================================================ */
/* Emulate LynxOs call timeout                                                  */

int timeout(int (*func)(), char *arg, int interval) {
int i;

int interval_jiffies = msecs_to_jiffies(interval * 10); /* 10ms granularity */

   mutex_lock(&emulate_mutex);
   for (i=0; i<TIMERS; i++) {
      if (lynxos_timers[i].TimerInUse == 0) {
	 lynxos_timers[i].TimerInUse = 1;
	 lynxos_timers[i].Arg = arg;
	 lynxos_timers[i].TimerCallback = func;
	 lynxos_timers[i].Timer.data = (unsigned long) i+1;
	 lynxos_timers[i].Timer.function = timer_callback;
	 lynxos_timers[i].Timer.expires = jiffies + interval_jiffies;
	 add_timer(&lynxos_timers[i].Timer);

	 mutex_unlock(&emulate_mutex);
	 return i+1;
      }
   }

   printk(KERN_WARNING "lynx_os: timeout: EFAULT: No more timers\n");

   mutex_unlock(&emulate_mutex);
   return SYSERR;
}

/* ============================================================================ */
/* Emulate LynxOs call timeout                                                  */

int cancel_timeout(int arg) {

int i;

   mutex_lock(&emulate_mutex);
   i = arg -1;
   if ((i >=0 ) && (i < TIMERS)) {
      if (lynxos_timers[i].TimerInUse) {
	 del_timer_sync(&lynxos_timers[i].Timer);
	 lynxos_timers[i].TimerInUse = 0;

	 mutex_unlock(&emulate_mutex);
	 return OK;
      }

      mutex_unlock(&emulate_mutex);
      return OK;
   }

   mutex_unlock(&emulate_mutex);
   return SYSERR;
}

/* ============================================================================ */
/* LynxOs interrupt masking routines                                            */
/* ============================================================================ */

spinlock_t lynxos_cpu_lock = SPIN_LOCK_UNLOCKED;

/* ============================================================================ */
/* LynxOs bus error trap mechanism                                              */
/* I see no way to do this for the time being under Linux                       */
/* ============================================================================ */

int recoset() {return 0; };     /* Assumes no bus errors */
void noreco() {return  ; };

/* ============================================================================ */
/* LynxOs Memory bound checking                                                 */
/* Linux drivers do not access user address space directly as in LynxOs. So I   */
/* build a buffer locally in driver address space and set the bounds at that    */
/* time.                                                                        */
/* ============================================================================ */

static DEFINE_MUTEX(ctr_drvr_alloc);

static LynxOsMemoryAllocation allocations[LynxOsMEMORY_ALLOCATIONS];

static unsigned char allocblks[LynxOsMEMORY_ALLOCATIONS * LynxOsALLOCATION_SIZE];

static int max_alloc = 0;

void LynxOsMemoryInitialize() {
int i;

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: LynxOsMemoryInitialize: OK\n");

   mutex_lock(&ctr_drvr_alloc);

   for (i=0; i<LynxOsMEMORY_ALLOCATIONS; i++) {
      allocations[i].Pntr  = &(allocblks[i * LynxOsALLOCATION_SIZE]);
      allocations[i].Size  = 0;
      allocations[i].Flgs  = 0;
      allocations[i].InUse = 0;
   }

   mutex_unlock(&ctr_drvr_alloc);

// for (i=0; i<LynxOsMEMORY_ALLOCATIONS; i++) {
//    printk(KERN_INFO "Allocations pointers: %02d:0x%lx\n",i,(unsigned long) allocations[i].Pntr);
// }

}

char *LynxOsMemoryAllocate(size_t size, int flgs) {
int i, j;

   if (size < sizeof(long)) size = sizeof(long);
   if (size > LynxOsALLOCATION_SIZE) return (char *) -ENOMEM;

   mutex_lock(&ctr_drvr_alloc);

   for (i=0; i<LynxOsMEMORY_ALLOCATIONS; i++) {
      if (allocations[i].InUse == 0) {
	 allocations[i].Size  = size;
	 allocations[i].Flgs  = flgs;
	 allocations[i].InUse = 1;
	 for (j=0; j<size; j++) allocations[i].Pntr[j] = 0;

	 if (i > max_alloc) {
	    max_alloc = i;
	    printk(KERN_INFO "Debug: Max Alloc: %d Pntr: 0x%lX\n",max_alloc,(unsigned long) allocations[max_alloc].Pntr);
	 }
	 mutex_unlock(&ctr_drvr_alloc);

	 if (debug & LynxOsDEBUG_MODULE)
	    printk(KERN_INFO "Debug: LynxOsMemoryAllocate: Size: %u Flgs: 0x%x Pntr: 0x%x OK\n",
		   (unsigned int) size, (unsigned int) flgs, i);

	 return allocations[i].Pntr;
      }
   }

   mutex_unlock(&ctr_drvr_alloc);

   if (debug & LynxOsDEBUG_ERRORS)
      printk(KERN_WARNING "lynx_os: LynxOsMemoryAllocate: ENOMEM: No more allocation blocks\n");
   return (char *) -ENOMEM;
}

int LynxOsMemoryFree(char *pntr) {
int i;

   mutex_lock(&ctr_drvr_alloc);

   for (i=0; i<LynxOsMEMORY_ALLOCATIONS; i++) {
      if (pntr == allocations[i].Pntr) {

	 if (debug & LynxOsDEBUG_MODULE)
	    printk(KERN_INFO "Debug: LynxOsMemoryFree: Size: %u Flgs: 0x%x Pntr: 0x%lx OK\n",
		   (unsigned int)  allocations[i].Size,
		   (unsigned int)  allocations[i].Flgs,
		   (unsigned long) allocations[i].Pntr);

	 allocations[i].InUse = 0;

	 mutex_unlock(&ctr_drvr_alloc);

	 return 0;
      }
   }

   mutex_unlock(&ctr_drvr_alloc);

   if (debug & LynxOsDEBUG_ERRORS)
      printk(KERN_WARNING "lynx_os: LynxOsFree: EFAULT: Garbage pointer: 0x%lx\n",
	     (unsigned long) pntr);
   return -EFAULT;
}

/* ============================================================================ */

long rbounds(unsigned long pntr) {
int i;

   mutex_lock(&ctr_drvr_alloc);

   for (i=0; i<LynxOsMEMORY_ALLOCATIONS; i++) {
      if ((char *) pntr == allocations[i].Pntr) {
	 if (allocations[i].Flgs & LynxOsMemoryREAD_FLAG) {

	    if (debug & LynxOsDEBUG_MODULE)
	       printk(KERN_INFO "Debug: rbounds: Size: %u Flgs: 0x%x Pntr: 0x%lx OK\n",
		      (unsigned int)  allocations[i].Size,
		      (unsigned int)  allocations[i].Flgs,
		      (unsigned long) allocations[i].Pntr);

	    mutex_unlock(&ctr_drvr_alloc);

	    return allocations[i].Size;

	 } else {
	    if ((debug & LynxOsDEBUG_ERRORS) && (debug & LynxOsDEBUG_MODULE))
	       printk(KERN_WARNING "lynx_os: rbounds: No READ access: Pntr: 0x%x\n",
		      (unsigned int) pntr);

	    mutex_unlock(&ctr_drvr_alloc);

	    return 0;
	 }
      }
   }

   mutex_unlock(&ctr_drvr_alloc);

   if (debug & LynxOsDEBUG_ERRORS)
      printk(KERN_WARNING "lynx_os: rbounds: EFAULT: Garbage pointer: 0x%x\n",
	     (unsigned int) pntr);
   return 0;
}

/* ============================================================================ */

long wbounds(unsigned long pntr) {
int i;

   mutex_lock(&ctr_drvr_alloc);

   for (i=0; i<LynxOsMEMORY_ALLOCATIONS; i++) {
      if ((char *) pntr == allocations[i].Pntr) {
	 if (allocations[i].Flgs & LynxOsMemoryWRIT_FLAG) {

	    if (debug & LynxOsDEBUG_MODULE)
	       printk(KERN_INFO "Debug: wbounds: Size: %u Flgs: 0x%x Pntr: 0x%lx OK\n",
		      (unsigned int)  allocations[i].Size,
		      (unsigned int)  allocations[i].Flgs,
		      (unsigned long) allocations[i].Pntr);

	    mutex_unlock(&ctr_drvr_alloc);

	    return allocations[i].Size;

	 } else {
	    if ((debug & LynxOsDEBUG_ERRORS) && (debug & LynxOsDEBUG_MODULE))
	       printk(KERN_WARNING "lynx_os: wbounds: No WRITE access: Pntr: 0x%x\n",
		      (unsigned int) pntr);

	    mutex_unlock(&ctr_drvr_alloc);

	    return 0;
	 }
      }
   }

   mutex_unlock(&ctr_drvr_alloc);

   if (debug & LynxOsDEBUG_ERRORS)
      printk(KERN_WARNING "lynx_os: wbounds: EFAULT: Garbage pointer: 0x%x\n",
	     (unsigned int) pntr);
   return 0;
}

/* ============================================================================ */
/* LynxOs system kernel errors                                                  */
/* ============================================================================ */

int lynxos_error_num = 0;
int pseterr(int err) { lynxos_error_num = err; return err; }

/* ============================================================================ */
/* Assorted crap that I have to implement                                       */
/* ============================================================================ */

int getpid() { return current->pid; }

/* ============================================================================ */

void bzero(void *dst, size_t len) {
int i;
char *d;

   d = (char *) dst;
   for (i=0; i<len; i++) d[i] = 0;
}

void bcopy(void *src, void *dst, size_t len) {
int i;
char *d, *s;

   s = (char *) src;
   d = (char *) dst;
   for (i=0; i<len; i++) d[i] = s[i];
}

/* ============================================================================ */

void LynxOsIsrsInitialize() {

int i;

   for (i=0; i<LynxOsMAX_DEVICE_COUNT; i++) {
      bzero((void *) &(lynxos_isrs[i]),sizeof(LynxOsIsr));
   }
}

/* ============================================================================ */
/* LynxOs DRM Services for PCI                                                  */
/* ============================================================================ */

int drm_get_handle(int    buslayer_id,
		   int    vender_id,
		   int    device_id,
		   struct drm_node_s **node_h) {

int i, cc;
struct pci_dev *dev = NULL;
struct pci_dev *old = NULL;

   for (i=0; i<LynxOsMAX_DEVICE_COUNT; i++) {
      if (lynxos_isrs[i].Handle) old = lynxos_isrs[i].Handle;
      else break;
   }

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: lynx_os: drm_get_handle: Vid: 0x%x Did: 0x%x old: 0x%lx\n",
	     vender_id, device_id, (unsigned long) old);

   dev = pci_get_device(vender_id,device_id,old);
   if (!dev) {
      lynxos_error_num = -ENODEV;
      return 1;
   }

   cc = pci_enable_device(dev);
   if (cc) return cc;           /* function can fail */

   *node_h = dev;
   for (i=0; i<LynxOsMAX_DEVICE_COUNT; i++) {
      if (lynxos_isrs[i].Handle == 0) {
	 lynxos_isrs[i].Handle = dev;
	 lynxos_isrs[i].Slot = (dev->devfn >> 3);

	 printk("drm_get_handle: Irq: %d\n",dev->irq);

	 if (dev->irq) {
	    lynxos_isrs[i].Irq = dev->irq;
	    pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);
	 }
	 if (debug & LynxOsDEBUG_MODULE)
	    printk(KERN_INFO "Debug: lynx_os: drm_get_handle: vid: 0x%x did: 0x%x dev: 0x%lx irq: %d Enabled OK\n",
		   vender_id, device_id, (unsigned long) dev, (unsigned int) dev->irq);

	 return 0;
      }
   }
   if (debug & LynxOsDEBUG_ERRORS)
      printk(KERN_WARNING "lynx_os: drm_get_handle: ENOSPC: Too many devices\n");
   lynxos_error_num = -ENOSPC;
   return 1;
}

/* ============================================================================ */

int drm_free_handle(struct drm_node_s *node_h) {

int i;

   for (i=0; i<LynxOsMAX_DEVICE_COUNT; i++) {
      if (lynxos_isrs[i].Handle == node_h) {
	 if (lynxos_isrs[i].Arg || lynxos_isrs[i].Isr || lynxos_isrs[i].Irq) {
	    lynxos_error_num = -EBUSY;
	    return 1;
	 }
	 lynxos_isrs[i].Handle = 0;
      }
   }
   return 0;
}

/* ============================================================================ */

int drm_device_read (struct drm_node_s *node_h,
		     int                resource_id,
		     unsigned int       offset,
		     unsigned int       size,
		     void              *buffer) {
unsigned char byte;
unsigned short word;
unsigned int dword;
int i;

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: lynx_os: drm_device_read: dev: 0x%lx rid: %d ofs: %d sze: %d buf: 0x%lx\n",
	     (unsigned long) node_h, resource_id, offset, size, (unsigned long) buffer);

   if (resource_id == PCI_RESID_DEVNO) {
      for (i=0; i<LynxOsMAX_DEVICE_COUNT; i++) {
	 if (lynxos_isrs[i].Handle == node_h) {
	    *(int *) buffer = lynxos_isrs[i].Slot;
	    return 0;
	 }
      }
      return 1;
   }

   switch (size) {
      case 1:
	 if ((lynxos_error_num
	 =    pci_read_config_byte(node_h,resource_id+offset,&byte)) < 0) return 1;
	 *(unsigned char *) buffer = byte;

	 if (debug & LynxOsDEBUG_MODULE)
	    printk(KERN_INFO "Debug: lynx_os: drm_device_read: Read Byte: %d 0x%x OK\n",
		   (unsigned int) byte, (unsigned int) byte);

	 return 0;

      case 2:
	 if ((lynxos_error_num
	 =    pci_read_config_word(node_h,resource_id+offset,&word)) < 0) return 1;
	 *(unsigned short *) buffer = word;

	 if (debug & LynxOsDEBUG_MODULE)
	    printk(KERN_INFO "Debug: lynx_os: drm_device_read: Read Word: %d 0x%x OK\n",
		   (unsigned int) word, (unsigned int) word);

	 return 0;

      default:
	 if ((lynxos_error_num
	 =    pci_read_config_dword(node_h,resource_id+offset,&dword)) < 0) return 1;
	 *(unsigned int *) buffer = dword;

	 if (debug & LynxOsDEBUG_MODULE)
	    printk(KERN_INFO "Debug: lynx_os: drm_device_read: Read DWord: %d 0x%x OK\n",
		   (unsigned int) dword, (unsigned int) dword);

	 return 0;
   }
   return 1;
}

/* ============================================================================ */

int drm_device_write(struct drm_node_s *node_h,
		     int                resource_id,
		     unsigned int       offset,
		     unsigned int       size,
		     void              *buffer) {

unsigned char byte;
unsigned short word;
unsigned int dword;

   switch (size) {
      case 1:
	 byte = *(unsigned char *) buffer;
	 if ((lynxos_error_num
	 =    pci_write_config_byte(node_h,resource_id+offset,byte)) < 0) return 1;
	 return 0;

      case 2:
	 word = *(unsigned short *) buffer;
	 if ((lynxos_error_num
	 =    pci_write_config_word(node_h,resource_id+offset,word)) < 0) return 1;
	 return 0;

      default:
	 dword = *(unsigned int *) buffer;
	 if ((lynxos_error_num
	 =    pci_write_config_dword(node_h,resource_id+offset,dword)) < 0) return 1;
	 return 0;
   }
   return 1;
}

/* ============================================================================ */

int drm_locate(struct drm_node_s *node) {
   return 1;
}

/* ============================================================================ */

int drm_register_isr(struct drm_node_s *node_h,
		     irqreturn_t       (*isr)(),
		     void              *arg) {

int i;
unsigned char irq;

   irq = node_h->irq;

#ifndef SA_SHIRQ
#define SA_SHIRQ IRQF_SHARED
#endif
   if ((lynxos_error_num = request_irq(irq,LynxOsIntrHandler,SA_SHIRQ,LynxOsMAJOR_NAME,arg))
   <0) {
      printk(KERN_WARNING "lynx_os: drm_register_isr: Can't request_irq: Irq: %u Arg: %lx\n",
	     (unsigned int) irq, (unsigned long) arg);
      return 1;
   }
   for(i=0; i<LynxOsMAX_DEVICE_COUNT; i++) {
      if (lynxos_isrs[i].Handle == node_h) {
	 if (lynxos_isrs[i].Arg == 0) {
	    lynxos_isrs[i].Arg = (unsigned long) arg;
	    lynxos_isrs[i].Irq = irq;
	    lynxos_isrs[i].Isr = isr;
	    if (debug & LynxOsDEBUG_MODULE)
	       printk(KERN_INFO "Debug: lynx_os: drm_register_isr: Node: 0x%lx Arg: 0x%lx Irq: %u\n",
		      (unsigned long) node_h, (unsigned long) arg, (unsigned int) irq);
	    return 0;
	 }
      }
   }
   lynxos_error_num = -ENOSPC;
   free_irq(irq,arg);
   return 1;
}

/* ============================================================================ */

int drm_unregister_isr(struct drm_node_s *node_h) {

int i;
unsigned int *arg;

   for(i=0; i<LynxOsMAX_DEVICE_COUNT; i++) {
      if (lynxos_isrs[i].Handle == node_h) {
	 arg = (unsigned int *) lynxos_isrs[i].Arg;
	 free_irq(lynxos_isrs[i].Irq,arg);
	 lynxos_isrs[i].Irq = 0;
	 lynxos_isrs[i].Arg = 0;
	 lynxos_isrs[i].Isr = 0;
	 return 0;
      }
   }
   lynxos_error_num = -EFAULT;
   return 1;
}

/* ============================================================================ */

#define RESOURCE_MAPS 48

typedef struct {
   struct drm_node_s *Handle;
   unsigned int       ResId;
   unsigned int       Start;
   unsigned int       Extent;
   unsigned long      VAddr;
 } LynxOsPciResourceMap;

static LynxOsPciResourceMap maps[RESOURCE_MAPS];
static int                res_init = 0;

int drm_map_resource(struct drm_node_s *node_h,
		     int                resource_id,
		     unsigned long     *vadrp) {

int i, bar;
unsigned int pstr, pend, pext;  /* Physical mem start end and extent */

char bar_name[21];
char *bar_text = "LynxOsDrvr PCI BAR-  ";
char *cp = &(bar_name[19]);

   if (res_init == 0) {
      res_init = 1;
      for (i=0; i<RESOURCE_MAPS; i++) {
	 maps[i].Handle = NULL;
	 maps[i].ResId = 0;
	 maps[i].Start = 0;
	 maps[i].VAddr = 0;
	 maps[i].Extent = 0;
      }
   }

   for (i=0; i<20; i++) bar_name[i] = bar_text[i]; bar_name[20] = 0;
   bar = (resource_id - PCI_RESID_BAR0) >> 2;
   if ((bar < 0) || (bar > 5)) {
      lynxos_error_num = -EFAULT;
      return 1;
   }
   *cp = bar + '0';

   for (i=0; i<RESOURCE_MAPS; i++) {
      if (maps[i].Handle == NULL) {

	 pstr = pci_resource_start(node_h,bar);
	 pend = pci_resource_end(node_h,bar);
	 pext = pend - pstr;

	 request_mem_region(pstr,pext,bar_name);
	 *vadrp = (unsigned long) ioremap_nocache(pstr,pext);

	 maps[i].Handle = node_h;
	 maps[i].ResId = resource_id;
	 maps[i].Start = pstr;
	 maps[i].VAddr = *vadrp;
	 maps[i].Extent = pext;

	 return 0;
      }
   }
   lynxos_error_num = -ENOSPC;
   return 1;
}

/* ============================================================================ */

int drm_unmap_resource(struct drm_node_s *node_h,
		       int                resource_id) {

int i;

   for (i=0; i<RESOURCE_MAPS; i++) {
      if ((maps[i].Handle == node_h)
      &&  (maps[i].ResId == resource_id)) {

	 iounmap((void *) maps[i].VAddr);
	 release_mem_region(maps[i].Start,maps[i].Extent);

	 maps[i].Handle = NULL;
	 maps[i].ResId = 0;
	 maps[i].Start = 0;
	 maps[i].VAddr = 0;
	 maps[i].Extent = 0;

	 return 0;
      }
   }
   lynxos_error_num = -EFAULT;
   return 1;
}
