/**
 * @file cdcmDrvr.h
 *
 * @brief Common Driver Code Manager (CDCM) header file.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 31/05/2006
 *
 * This is a CDCM header file which holds all major definitions of the CDCM
 * abstraction layer.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version
 */
#ifndef _CDCM_DRVR_H_INCLUDE_
#define _CDCM_DRVR_H_INCLUDE_

#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>

//#include <linux/config.h>

#ifdef MODVERSIONS
#include <config/modversions.h>
#endif

#include <linux/string.h>
#include <linux/errno.h>
#include <linux/sched.h>
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

#include "cdcmInfoT.h"	/* for 'cdcm_hdr_t' type */
#include "drvr_dbg_prnt.h"

#define kkprintf printk
#define cprintf printk


/*! @name device alive level in the CDCM environment
 */
//@{
typedef enum {
	ALIVE_DEAD  = 0x0,  //!< device is dead
	ALIVE_ALLOC = 1<<0, //!< structure is allocated
	ALIVE_CDEV  = 1<<1, //!< cdev is registered
	ALIVE_THRDS = 1<<2, //!< threads are running
	ALIVE_FULL  = (~0)  //!< all is done
} devAlive_t;
//@}


/*! @name CDCM-tune flags
 */
//@{
typedef enum {
	CDCMFLG_NOUSREP = 1<<0, //!< don't execute user entry points
	CDCMFLG_ALL     = (~0)  //!< all are set
} cdcmflg_t;
//@}


/*! @name timer handling mechanism
 */
//@{
#define MAX_CDCM_TIMERS 16
typedef struct _cdcmTimer {
	pid_t ct_on;	  /**< on/off flag
			     0   - timer is not used, else
			     tid - who is using this timer */
	struct timer_list ct_timer; //!<
	int (*ct_uf)(void*); //!< user function
	char *ct_arg; //!< user arg
} cdcmt_t;
//@}

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
struct cdcm_proc {
	struct task_struct *cdcm_task; //!< id
};
#endif


/*! @name Used to describe VME devices
 */
//@{
struct vme_dev {
	unsigned int vd_virt_addr; //!< virtual address
	unsigned int vd_phys_addr; //!< physical address
	unsigned int vd_len;       //!< size of the mapped memory
	unsigned int vd_am;	   //!< address modifier
};
//@}


/*! @name description of each module, handled by CDCM
 */
//@{
struct cdcm_dev_info {
	struct list_head di_list;  //!< device linked list
	struct pci_dev  *di_pci;   //!< PCI device pointer
	struct vme_dev  *di_vme;   //!< VME device pointer
	devAlive_t       di_alive; //!< alive level

	/* Thread Stuff. TODO.
	   struct list_head  di_thr_list;  //!< thread list head
	   int               di_thr_croak; //!< batch thread suicide
	   struct completion di_thr_c;     //!< thread syncronization
	*/
};
//@}
#define drm_node_s cdcm_dev_info


/* general CDCM statics table */
typedef struct _cdcmStatics {
	struct list_head cdcm_dev_list; /**< controlled device list head
					   described by cdcm_dev_info */
	char *cdcm_st; //!< statics table, returned by install entry point
	char *cdcm_mn; //!< module name provided by the user
	cdcm_hdr_t *cdcm_hdr; //!< header passed from the user space
	int cdcm_major; //!< driver major number

	struct list_head cdcm_mem_list_head; //!< allocated memory list
	dbgipl_t cdcm_ipl; //!< CDCM Information Printout Level
	struct list_head cdcm_thr_list_head; /* thread list */
	//struct list_head cdcm_proc_list_head;	/* process list */
	cdcmt_t cdcm_timer[MAX_CDCM_TIMERS]; /* timers */
	struct list_head cdcm_sem_list_head; /* semaphore list */
	cdcmflg_t cdcm_flags;	       /* bitset flags */
} cdcmStatics_t;

/* TODO. Initialize cdcmStatT */
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

#endif /* _CDCM_DRVR_H_INCLUDE_ */
