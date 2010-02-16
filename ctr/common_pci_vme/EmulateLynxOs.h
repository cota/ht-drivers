/* ============================================================================ */
/* Emulate LynxOs under Linux for device drivers. I have some LynxOs drivers I  */
/* would like to emulate under Linux rather than do a port each time. As long   */
/* as the LynxOs driver is not too exotic and single threaded, this should be   */
/* a good way of getting a driver running without a port.                       */
/* Julian Lewis May 2004.                                                       */
/* ============================================================================ */

#ifndef EMULATE_LYNX_OS
#define EMULATE_LYNX_OS

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <linux/fcntl.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/interrupt.h>

#include <linux/pci.h>

/* lynxos semaphores */
#include "cdcmSem.h"

#define kkprintf printk
#define cprintf printk

/* ============================================================================ */
/* Debug options are controlled via IOCTL Cmd = 0 argument value                */
/* ============================================================================ */

#define LynxOsDEBUG_DRIVER 0x01
#define LynxOsDEBUG_ERRORS 0x02
#define LynxOsDEBUG_MODULE 0x04

/* ============================================================================ */
/* LynxOs Memory allocation routines                                            */
/* ============================================================================ */

char *sysbrk(unsigned long size);
void sysfree(char *p, unsigned long size);

/* ============================================================================ */
/* LynxOs Timeout routines                                                      */
/* ============================================================================ */

void LynxOsTimersInitialize();

int timeout(int (*func)(), char *arg, int interval);
int cancel_timeout(int num);

/* ============================================================================ */
/* LynxOs interrupt masking routines                                            */
/* ============================================================================ */
extern spinlock_t lynxos_cpu_lock;

#define disable(x) spin_lock_irqsave(&lynxos_cpu_lock,x)
#define restore(x) spin_unlock_irqrestore(&lynxos_cpu_lock,x)

/* ============================================================================ */
/* LynxOs bus error trap mechanism                                              */
/* ============================================================================ */

int recoset();
void noreco();

/* ============================================================================ */
/* LynxOs Memory bound checking                                                 */
/* ============================================================================ */

#define LynxOsMemoryREAD_FLAG 0x01
#define LynxOsMemoryWRIT_FLAG 0x02

typedef struct {
   char   *Pntr;
   size_t  Size;
   int     Flgs;
   int     InUse;
 } LynxOsMemoryAllocation;

#define LynxOsMEMORY_ALLOCATIONS 2

void LynxOsMemoryInitialize();
char *LynxOsMemoryAllocate(size_t size, int flgs);
int LynxOsMemoryFree(char *pntr);

long rbounds(unsigned long addr);
long wbounds(unsigned long addr);

/* ============================================================================ */
/* LynxOs system kernel errors                                                  */
/* ============================================================================ */

#define SYSERR -1
#define OK 0

extern int lynxos_error_num;
int pseterr(int err);

/* ============================================================================ */
/* Assorted crap that I have to implement                                       */
/* ============================================================================ */

int getpid();
void bzero(void *dst, size_t len);
void bcopy(void *src, void *dst, size_t len);

/* ============================================================================ */
/* LynxOs DRM Services for PCI                                                  */
/* ============================================================================ */

#define drm_node_s pci_dev
typedef struct drm_node_s  *drm_node_handle;

#define PCI_BUSLAYER 1

#define PCI_RESID_REGS  0
#define PCI_RESID_DEVNO 0xFFFF
#define PCI_RESID_BAR2  0x18
#define PCI_RESID_BAR1  0x14
#define PCI_RESID_BAR0  0x10

void LynxOsIsrsInitialize();

int drm_get_handle(int    buslayer_id,
		   int    vender_id,
		   int    device_id,
		   struct drm_node_s **node_h);

int drm_free_handle(struct drm_node_s *node_h);

int drm_device_read (struct drm_node_s *node_h,
		     int                resource_id,
		     unsigned int       offset,
		     unsigned int       size,
		     void              *buffer);

int drm_device_write(struct drm_node_s *node_h,
		     int                resource_id,
		     unsigned int       offset,
		     unsigned int       size,
		     void              *buffer);

int drm_locate(struct drm_node_s *node);

int drm_register_isr(struct drm_node_s *node_h,
		     irqreturn_t       (*isr)(),
		     void              *arg);

int drm_map_resource(struct drm_node_s *node_h,
		     int                resource_id,
		     unsigned long     *vaddr);

int drm_unmap_resource(struct drm_node_s *node_h,
		       int                resource_id);

int drm_unregister_isr(struct drm_node_s *node_h);

int drm_unregister_isr(struct drm_node_s *node_h);

/* ============================================================================ */
/* The Unix driver will get hold of the entry points by using the LynxOs dldd   */
/* entry points structure. The instance must be called "entry_points".          */
/* ============================================================================ */

struct dldd {
  int (*dldd_open)();
  int (*dldd_close)();
  int (*dldd_read)();
  int (*dldd_write)();
  int (*dldd_select)();
  int (*dldd_ioctl)();
  char *(*dldd_install)();
  int (*dldd_uninstall)();
};

struct LynxFile {
   int dev;
};

struct LynxSel {
   int *iosem;
};

#define minor(x) (x & 0xFF)

typedef struct {
   struct drm_node_s *Handle;
   irqreturn_t   (*Isr)();
   unsigned long Arg;
   unsigned int  Irq;
   unsigned int  Slot;
 } LynxOsIsr;

#endif
