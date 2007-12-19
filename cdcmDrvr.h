/* $Id: cdcmDrvr.h,v 1.3 2007/12/19 09:02:05 ygeorgie Exp $ */
/**
 * @file cdcmDrvr.h
 *
 * @brief Common Driver Code Manager (CDCM) header file.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date May, 2006
 *
 * This is a CDCM header file which holds all major definitions of the CDCM
 * abstraction layer.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version 5.0  ygeorgie  01/08/2007  Full Lynx-like installation behaviour.
 * @version 4.0  ygeorgie  14/03/2007  Production release, CVS controlled.
 * @version 3.0  ygeorgie  16/01/2007  VME support.
 * @version 2.0  ygeorgie  27/07/2006  First working release.
 * @version 1.0  ygeorgie  31/05/2006  Initial version.
 */
#ifndef _CDCM_DRVR_H_INCLUDE_
#define _CDCM_DRVR_H_INCLUDE_

#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>

#include <linux/module.h>

#ifdef MODVERSIONS
#include <config/modversions.h>
#endif

#include <linux/string.h>
#include <linux/errno.h>
#include <linux/sched.h>

/* ------------------------------------------------ */
#include <linux/signal.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/major.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>       /* for request_region */
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <asm/serial.h>
#include <linux/delay.h>        /* for loops_per_jiffy */
#include <linux/poll.h>         /* for POLLIN, etc. */
#include <asm/system.h>
#include <asm/io.h>             /* for inb_p, outb_p, inb, outb, etc. */
#include <asm/irq.h>
#include <asm/dma.h>
#include <asm/bitops.h>
#include <asm/types.h>
#include <linux/termios.h>
#include <linux/workqueue.h>
#include <linux/hdlc.h>
#include <linux/kthread.h>

#include "cdcm.h" /* for CDCM_DNL */
#include "cdcmInfoT.h"	/* for 'cdcm_hdr_t' type */
#include <ces/xpc_vme.h>

#define kkprintf printk
#define cprintf printk

/* tnks to Linux kernel src */
#define KB      (1024UL)
#define MB      (1024UL*KB)
#define GB      (1024UL*MB)
#define B2KB(x) ((x) / 1024)
#define B2MB(x) (B2KB (B2KB (x)))

/* device minor numbers holding information about the devices and their
   channel numbers */
#define CDCM_DEV(inode) (iminor(inode) >> 4)
#define CDCM_CHAN(inode) (iminor(inode) & 0xf)


/* DeBuG Information Printout Level (IPL) */
typedef enum _dbgipl {
  IPL_NONE    = 0,	 /* silent mode */
  IPL_OPEN    = 1 << 0,	 /*  */
  IPL_CLOSE   = 1 << 1,	 /*  */
  IPL_READ    = 1 << 2,	 /*  */
  IPL_WRITE   = 1 << 3,	 /*  */
  IPL_SELECT  = 1 << 4,	 /*  */
  IPL_IOCTL   = 1 << 5,	 /*  */
  IPL_INSTALL = 1 << 6,	 /*  */
  IPL_UNINST  = 1 << 7,	 /*  */
  IPL_DBG     = 1 << 8,	 /* printout debug messages */
  IPL_ERROR   = 1 << 9,  /* printout error messages */
  IPL_INFO    = 1 << 10, /* printout information messages */
  IPL_ALL     = (~0)     /* verbose */
} dbgipl_t;

/* cdcm errors */
#define cdcm_errbase 0x200
typedef enum {
  cdcmerr_noerr       = 0,
  cdcmerr_Wrong_Ioctl = cdcm_errbase + 1
} cdcmerr;

/* device alive level in the CDCM environment */
typedef enum {
  ALIVE_DEAD  = 0x0,  /* device is dead         */
  ALIVE_ALLOC = 1<<0, /* structure is allocated */
  ALIVE_CDEV  = 1<<1, /* cdev is registered     */
  ALIVE_THRDS = 1<<2, /* threads are running    */
  ALIVE_FULL  = (~0)  /* all is done            */
} devAlive_t;

/* CDCM flags */
typedef enum {
  CDCMFLG_NOUSREP = 1<<0, /* don't execute user entry points */
  CDCMFLG_ALL     = (~0)  /* all are set */
} cdcmflg_t;


/* timer handle */
#define MAX_CDCM_TIMERS 16
typedef struct _cdcmTimer {
  pid_t               ct_on;	  /* on/off flag
				     0 - timer is not used, else
				     tid - who is using this timer */
  struct timer_list ct_timer;	  /*  */
  int               (*ct_uf)(void*); /* user function */
  char              *ct_arg;	     /* user arg */
} cdcmt_t;

/* LynxOS kernel semaphores implemented using Linux wait queues.
   We are using wait queues, but not kernel semaphores, because 
   the latter can be acquired only by functions that are allowed to
   sleep; interrupt handlers and deferrable functions cannot use them.
   CDCM semaphore is taken only once by the user. It can not be re-used */
typedef struct _cdcmWaitQueue {
  wait_queue_head_t wq_head; /* wait queue head for this semaphore */
  struct list_head  wq_sem_list; /* linked list */
  int *wq_usr_val; /* address of the user semaphore (NULL - not used).
		      Dereferense it to obtain current sem value */
  atomic_t wq_tflag; /* condition flag for waking up threads, that are waiting
			on the current queue:
			If it's > 0 - ssignal.
			One of the waiting threads awakened, and awaken thread
			is decreasing the flag value by one - i.e. you can
			consider it as a sort of semaphore signalling (up).
			This is used in swait and ssignal Lynx syscalls
			emulation.
			if it's < 0 - sreset.
			All waiting threads will be awakened, and wq_flag
			will be set to negative amount of anticipants */
  struct task_struct *wq_masta; /* my owner */
  int wq_init_val; /* initial user-set semaphore value */
  int wq_id; /* my id. Just to make debug printout more readable */
} cdcmsem_t;

#if 0
/* NOTE ON USAGE! (User - is a driver developer, who is using CDCM)
   If 'sem_usr_sem' is not null - user is using this semaphore. But semaphore
   can be also be used inside the thread, started by the user. So if 
   'sem_masta' is NULL - it means that thread was already killed, and this
   semaphore is prohibited to use!
   */
typedef struct _cdcmSemaphore {
  struct semaphore sem_sem;
  int *sem_usr_sem;		/* user value */
  struct task_struct *sem_masta; /* my owner */
  int sem_id;
} cdcmsem_t;
#endif

/* ioctl operation information */
#define DIR_NONE  (1 << 0) /* no params */
#define DIR_READ  (1 << 1) /* read params */
#define DIR_WRITE (1 << 2) /* write params */
#define DIR_RW    (DIR_READ | DIR_WRITE) /* both write and read params */
typedef struct _cdcmIoctl {
  int direction; /* DIR_READ or DIR_WRITE from user point of view */
  int size;	 /* data size in bytes */
} cdcmIoctl;


#if 0
/* process descr, currently using the driver */
typedef struct _cdcmProc {
  struct task_struct *cdcm_task; /* id */
} cdcmproc_t;
#endif

/* Used to describe VME devices */
struct vme_dev {
  unsigned int vd_virt_addr; /* virtual address */
  unsigned int vd_phys_addr; /* physical address */
  unsigned int vd_len;       /* size of the mapped memory */
  unsigned int vd_am;	     /* address modifier */
};

/* description of each module, handled by CDCM */
typedef struct _cdcmDevInfo { 
  struct list_head di_list;           /* device linked list */
  struct pci_dev  *di_pci;            /* PCI device pointer */
  struct vme_dev  *di_vme;            /* VME device pointer */
  devAlive_t       di_alive;          /* alive level */
  int              di_parent;	      /* parent group number */

  /* thread stuff */
  /*
  struct list_head  di_thr_list;  // TODO. thread list head
  int               di_thr_croak; // TODO. batch thread suicide
  struct completion di_thr_c;     // TODO. thread syncronization
  */
} cdcm_dev_info_t;

/* logical device grouping */
struct cdcm_grp_info_t {
  struct list_head grp_list;     /* group linked list */
  struct list_head grp_dev_list; /* group devices linked list
				    (all cdcm_dev_info_t) */
  int   grp_maj;                 /* group major number */
  int   grp_num;		 /* group number [1-21] */
  char  grp_name[CDCM_DNL];      /* group name as will
				    appear in /proc/devices and sysfs */

  char *grp_stat_tbl;		 /* user-defined statics table */

  char *grp_info_tbl;		 /* group info table (passed as a parameter
				    to install entry point) */
  int   grp_it_sz;		 /* info table size in bytes */
};

#define drm_node_s cdcm_grp_info_t

/* general CDCM statics table */
typedef struct _cdcmStatics {
  struct list_head cdcm_grp_list_head; /* controlled groups list
					  (all 'cdcm_grp_info_t') */
  char             cdcm_drvr_type;     /* driver type 'V' - VME, 'P' - PCI */
  char             cdcm_mod_name[CDCM_DNL];/* module name provided by the 
					      user space */
  cdcm_hdr_t      *cdcm_hdr;	       /* header passed from the user space */
  int              cdcm_major;	       /* service entry point major */

  struct list_head cdcm_mem_list_head; /* allocated memory list */
  dbgipl_t         cdcm_ipl;	       /* CDCM Information Printout Level */
  struct list_head cdcm_thr_list_head; /* thread list */
  //struct list_head cdcm_proc_list_head;	/* process list */
  cdcmt_t          cdcm_timer[MAX_CDCM_TIMERS]; /* timers */
  struct list_head cdcm_sem_list_head; /* semaphore list */
  cdcmflg_t        cdcm_flags;	       /* bitset flags */
} cdcmStatics_t;
/* cdcmStatT - global statics table defined in cdcmDrvr.c module */

#ifndef OK
#define OK 0
#endif

#ifndef SYSERR
#define SYSERR -1
#endif

#define FALSE	0
#define TRUE	1

#define SYSCALL	int

/*------------------- definitions for info printout -------------------------*/

/* indisputable info printout */
#define PRNT_ABS_INFO(format...)				\
do {								\
  printk("%s [CDCM_INFO] %s(): ", KERN_INFO, __FUNCTION__ );	\
  printk(format);						\
  printk("\n");							\
} while(0)

/* indisputable error printout */
#define PRNT_ABS_ERR(format...)					\
do {								\
  printk("%s [CDCM_ERR] %s(): ", KERN_ERR, __FUNCTION__ );	\
  printk(format);						\
  printk("\n");							\
} while(0)

/* indisputable warning printout */
#define PRNT_ABS_WARN(format...)				\
do {								\
  printk("%s [CDCM_WARN] %s(): ", KERN_WARNING, __FUNCTION__ );	\
  printk(format);						\
  printk("\n");							\
} while(0)

/* indisputable debugging printout */
#define PRNT_ABS_DBG_MSG(format...)					  \
do {									  \
  printk("%s [CDCM_DBG] %s()@%d: ", KERN_DEBUG, __FUNCTION__ , __LINE__); \
  printk(format);							  \
  printk("\n");								  \
} while(0)

/* for pure printing */
#define PRNT_OPEN(format, arg...) \
if (cdcmStatT.cdcm_ipl & IPL_OPEN) { PRNT_ABS_INFO(format, arg); }

#define PRNT_CLOSE(format, arg...) \
if (cdcmStatT.cdcm_ipl & IPL_CLOSE) { PRNT_ABS_INFO(format, arg); }

#define PRNT_READ(format, arg...) \
if  (cdcmStatT.cdcm_ipl & IPL_READ) {PRNT_ABS_INFO(format, arg); }

#define PRNT_WRITE(format, arg...) \
if (cdcmStatT.cdcm_ipl & IPL_WRITE) { PRNT_ABS_INFO(format, arg); }

#define PRNT_SELECT(format, arg...) \
if (cdcmStatT.cdcm_ipl & IPL_SELECT) { PRNT_ABS_INFO(format, arg); }

#define PRNT_IOCTL(format, arg...) \
if (cdcmStatT.cdcm_ipl & IPL_IOCTL) { PRNT_ABS_INFO(format, arg); }

#define PRNT_INSTALL(format, arg...) \
if (cdcmStatT.cdcm_ipl & IPL_INSTALL) { PRNT_ABS_INFO(format, arg); }

#define PRNT_UNINST(format, arg...) \
if (cdcmStatT.cdcm_ipl & IPL_UNINST) { PRNT_ABS_INFO(format, arg); }

#define PRNT_DBG(format, arg...) \
if (cdcmStatT.cdcm_ipl & IPL_DBG) { PRNT_ABS_DBG_MSG(format, arg); }

#define PRNT_ERR(format, arg...) \
if (cdcmStatT.cdcm_ipl & IPL_ERROR) { PRNT_ABS_ERR(format, arg); }

#define PRNT_INFO(format, arg...) \
if (cdcmStatT.cdcm_ipl & IPL_INFO) { PRNT_ABS_INFO(format, arg); }

#endif /* _CDCM_DRVR_H_INCLUDE_ */
