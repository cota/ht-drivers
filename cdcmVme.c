/* $Id: cdcmVme.c,v 1.2 2007/04/30 09:15:02 ygeorgie Exp $ */
/*
; Module Name:	 cdcmVme.c
; Module Descr:	 CDCM VME Linux driver that encapsulates user-written LynxOS
;		 driver. Consider it like a wrapper of the user driver.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: Jan, 2007
; Authors:	 Yury Georgievskiy, Alain Gagnaire, Nicolas De Metz-Noblat
;		 CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmVme.c: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 3.0   ygeorgie   24/04/07   Possibility to change IRQ scheduling priority.
; 2.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 1.0	ygeorgie   16/01/07   Initial version.
*/

#include "cdcmDrvr.h"
#include "cdcmLynxAPI.h"
#include "generalDrvrHdr.h" /* for WITHIN_RANGE */
#include <asm/unistd.h>     /* for _syscall */
#include <linux/irq.h>
#include <platforms/rio.h>  /* for RIO_ACT_VMEIRQ1 */
#include <ces/xpc_vme.h>

/* page qualifier */
#define VME_PG_SHARED  0x00
#define VME_PG_PRIVATE 0x02

/* for _syscall's. Othervise we've got undefined symbol */
static int errno;

/* we'll need this system calls to get some info */
static __inline__ _syscall1(int,sched_get_priority_min,int,policy)
static __inline__ _syscall1(int,sched_get_priority_max,int,policy)

/* external functions */
extern int cdcm_user_part_device_recodnizer(struct pci_dev*, char*);
extern int cdcm_set_unique_device_name(char devName[CDCM_DNL]);
extern int cdcm_user_get_dev_name(void*, char devName[CDCM_DNL]);
extern struct cdev* cdcm_get_cdev(char devName[CDCM_DNL], int, dev_t*);


/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern struct file_operations cdcm_fops; /* char dev file operations */
extern cdcmStatics_t cdcmStatT;	/* CDCM statics table */
extern int cdcm_err;		/* global error */


/* saved vme vectors */
typedef vme_vec_t vme_vec_tab_t[XPC_VME_MAX_IRQ];
vme_vec_tab_t cdcm_vme_vec = { {0,0} }; /* all the rest will be init to zero */

/* for interrupt handler workaround ('vtp' stands for Vector Table Pointer) */
static int (*cdcm_irq_vtp[XPC_VME_MAX_IRQ])(void*) = {
  [0 ... sizeof(cdcm_irq_vtp)/sizeof(*cdcm_irq_vtp) - 1] = NULL
};

//static spinlock_t xxx_lock;


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_irq_handler
 * DESCRIPTION: Workaround to implement LynxOS interrupt handler.
 * RETURNS:     exactly what Lynx interrupt handler should return.
 *-----------------------------------------------------------------------------
 */
static int
cdcm_irq_handler(
		 int vect,	/* irq number */
		 void *arg)	/* user handler argument */
{
  return((*cdcm_irq_vtp[vect])(arg));
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    vme_intset
 * DESCRIPTION: Wrapper CES-like function.
 * RETURNS:     OK (i.e. 0) - on success
 *		SYSERR      - if vector is out of range 
 *-----------------------------------------------------------------------------
 */
int
vme_intset(
	   int vector,		  /* VME vector to be installed */
	   int(*function)(void*), /* user driver's interrupt handler */
	   char *arg,		  /* pointer to argument structure */
	   vme_vec_t *sav)	  /* to save previous entry */
{
  int coco = OK;		/* 0 */
  //char irq_name[0x100];

  if (cdcm_irq_vtp[vector])	/* already used */
    return(SYSERR);
    
  cdcm_irq_vtp[vector] = (int(*)(void*))function; /* init pointer */

  /* build-up name */
  //sprintf(irq_name, "%s", CDCM_USR_DRVR_NAME);
  //printk("irq_name is %s\n", irq_name);

  /*  */
  if ( (coco = xpc_vme_request_irq(vector, (void(*)(int,void*))cdcm_irq_handler, arg, CDCM_USR_DRVR_NAME)) ) {
    cdcm_irq_vtp[vector] = NULL; /* roll back */
    coco = SYSERR;		/* convert to Lynx-CES-BSP-like */
  }

  if (sav) {  /* give previous to the user */
    sav->rout = cdcm_vme_vec[vector].rout;
    sav->arg  = cdcm_vme_vec[vector].arg;
  }

  /* now safe current */
  cdcm_vme_vec[vector].rout = function;
  cdcm_vme_vec[vector].arg = arg;

  return(coco);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    vme_intclr
 * DESCRIPTION: 
 * RETURNS:     OK
 *-----------------------------------------------------------------------------
 */
int
vme_intclr(
	   int vector,      /* VME vector to be removed */
	   vme_vec_t *sav) /* to restore previous entry */
{
  xpc_vme_free_irq(vector);
  
  if (sav) {
    cdcm_vme_vec[vector].rout = sav->rout;
    cdcm_vme_vec[vector].arg  = sav->arg;
  } else {
    cdcm_vme_vec[vector].rout = 0;
    cdcm_vme_vec[vector].arg  = 0;
  }

  return(OK);			/* 0 */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    find_controller
 * DESCRIPTION: Maps a specified part of the VME address space into PCI address
 *		space. This function should be called to check module presence.
 *		We suppose that user will call it at the very beginning of the
 *		driver installation. Like this all internal CDCM structures
 *		will be initialized. You can consider it as the VME analogue
 *		to the 'drm_get_handle' in the PCI world. 
 * RETURNS:     pointer to be used by the caller to access the specified VME
 *		address range - if SUCCESS.
 *		-1            - if case of FAILURE.
 *-----------------------------------------------------------------------------
 */
unsigned int
find_controller(
  unsigned int vmeaddr,  /* start address of VME address range to map */
  unsigned int len,      /* size of address range to map in bytes */
  unsigned int am,       /* VME address modifier */
  unsigned int offset,   /* offset from vmeaddr (bytes) at which a read test
			    should be made */
  unsigned int size,     /* size in bytes of read-test zone
			    (0 to disable read test) */
  struct pdparam_master *param) /* structure holding AM-code and mode flags
				   NOT implemented for now!!! */
{
  struct cdcmDevInfo_t *infoT;
  //unsigned long flags;
  xpc_vme_type_e access_type;
  int options = 0;		/* not yet supported */
  int cntr;
  unsigned int cpu_addr;
  unsigned int phys;

  char usrDevNm[CDCM_DNL]; /* user-defined device name */
  static int firstOccurrence = 0; /* some init code should be done only once */

  CDCM_DBG("%s %s(%x,%x,%x,%x,%x)\n", CDCM_USR_DRVR_NAME, __FUNCTION__, vmeaddr, len, am, offset, size);

  if (cdcmStatT.cdcm_drvr_type == 'P') { /* already declared as PCI */
    PRNT_ABS_ERR("%s driver type mismatch!\n", CDCM_USR_DRVR_NAME);
    return(-1);
  }

  if (!firstOccurrence) { /* will enter here only once */
    ++firstOccurrence;
    cdcmStatT.cdcm_drvr_type = 'V';
  }

  /* Encode AM into access_type */
  switch (am) {
  case 0x29:
    access_type = XPC_VME_A16_STD_USER;
    break;
  case 0x2d:
    access_type = XPC_VME_A16_STD_USER | XPC_VME_PTYPE_SUP;
    break;
  case 0x39:
    access_type = XPC_VME_A24_STD_USER;
    break;
  case 0x3d:
    access_type = XPC_VME_A24_STD_USER | XPC_VME_PTYPE_SUP;
    break;
  case 0x09:
    access_type = XPC_VME_A32_BLT_USER;
    break;
  case 0x0d:
    access_type = XPC_VME_A32_BLT_USER | XPC_VME_PTYPE_SUP;
    break;
  case 0x0b:
    access_type = XPC_VME_A32_BLT_USER;
    break;
  case 0x08:
    access_type = XPC_VME_A32_MBLT_USER;
    break;
  default:
    PRNT_ABS_ERR("Unknown AM=%X in %s driver\n", am, CDCM_USR_DRVR_NAME);
    return(-1);
  }
  
  
#if 0
  CES-written code. Does not work. System hangs.
  unsigned int data;
  int err;
  if (size != 0) {
    /* Verify presence of module by a read test of n bytes at specified offset */
    err = xpc_vme_safe_pio(vmeaddr + offset, 0x0, access_type, 0, size, &data);
    if (err == (-EIO))
      return (-1);
    if (err == (-ENOMEM)) {
      printk("%s find_controller: VME ressource limit exhausted\n", CDCM_USR_DRVR_NAME);
      return (-1);
    }
    if (err != 0) {
      printk("%s find_controller: unknown error %d\n", CDCM_USR_DRVR_NAME, -err);
      return (-1);
    }
  }
#endif

  phys = xpc_vme_master_map(vmeaddr, 0x0, len, access_type, options);
  if (phys == (-1))
    return(phys);
  
  if (!(cpu_addr = (unsigned int)ioremap(phys, len))) {
    PRNT_ABS_ERR("ioremap error in %s driver\n", CDCM_USR_DRVR_NAME);
    (void) xpc_vme_master_unmap(phys, len);
    return(-1);
  }

  
#if 1
  /* do actual hardware access */
  if (size != 0) {
    //spin_lock_irqsave(&xxx_lock, flags);
    switch(size) {
    case 1:
      if (inb(cpu_addr + offset) == 0xffffffff ) goto berr;
      break;
    case 2:
      if (inw(cpu_addr + offset) == 0xffffffff ) goto berr;
      break;
    case 4:
      if (inl(cpu_addr + offset) == 0xffffffff ) goto berr;
      break;
    default:
      for (cntr = 0; cntr < size; cntr++)
	if (inb(cpu_addr + offset) == 0xffffffff ) goto berr;
    }
    //spin_unlock_irqrestore(&xxx_lock, flags);
  }
#endif

  /* device is present - register it */

  if (!(infoT = (struct cdcmDevInfo_t *)kzalloc(sizeof(struct cdcmDevInfo_t), GFP_KERNEL))) {
    PRNT_INFO_MSG("Can't allocate info table for %s dev.\n", CDCM_USR_DRVR_NAME);
    cdcm_err = -ENOMEM;
    goto cdcm_err;
  }

  if (!(infoT->di_vme = (struct vme_dev*)kmalloc(sizeof(infoT->di_vme), GFP_KERNEL)) ) { /* vme description table allocation */
    PRNT_INFO_MSG("Can't allocate VME descriptor for %s dev.\n", CDCM_USR_DRVR_NAME);
    kfree(infoT);
    cdcm_err = -ENOMEM;
    goto cdcm_err;
  }

  if (cdcm_user_get_dev_name((void*)infoT->di_vme, usrDevNm)) {
    PRNT_INFO_MSG("%s device at %#x is NOT supported.\n", CDCM_USR_DRVR_NAME, vmeaddr);
    cdcm_err = -ENODEV;
    kfree(infoT->di_vme);
    kfree(infoT);
    goto cdcm_err;
  }
  cdcm_set_unique_device_name(usrDevNm);

  /* register char device and get device number */
  infoT->di_cdev = cdcm_get_cdev(usrDevNm, cdcmStatT.cdcm_chan_per_dev, &infoT->di_dev_num);
  if (IS_ERR(infoT->di_cdev)) {
    PRNT_ABS_ERR("Can't register %s as character device... Aborting!", usrDevNm);
    kfree(infoT->di_vme);
    kfree(infoT);
    return PTR_ERR(infoT->di_cdev);
  }

  /* safe virt <-> phys VME address mapping */
#if 0
  *infoT->di_vme = (struct vme_dev) {
    .vd_virt_addr = cpu_addr,
    .vd_phys_addr = phys,
    .vd_len       = len,
    .vd_am        = am
  };
#endif

  infoT->di_vme->vd_virt_addr = cpu_addr;
  infoT->di_vme->vd_phys_addr = phys;
  infoT->di_vme->vd_len       = len;
  infoT->di_vme->vd_am        = am;

  /* initialize device info table */
  infoT->di_line  = cdcmStatT.cdcm_dev_am; /* set device index */
  infoT->di_alive = ALIVE_ALLOC | ALIVE_CDEV;
  infoT->di_pci   = NULL;
  strcpy(infoT->di_name, usrDevNm); /* set device name */

  /* add device to the device linked list */
  list_add(&infoT->di_list, &cdcmStatT.cdcm_dev_list_head);
  ++cdcmStatT.cdcm_dev_am; /* increase found device counter */

  
  PRNT_INFO_MSG("%s %s(%x,%x %x %x %x) return %x virt addr for %x phys addr\n", CDCM_USR_DRVR_NAME, __FUNCTION__, vmeaddr, len, am, offset, size, cpu_addr, phys);
  
  return(cpu_addr);
  
#if 1
  berr:
  PRNT_ABS_ERR("For %s driver %s(%x,%x,%x,%x,%x) bus error!\n", CDCM_USR_DRVR_NAME, __FUNCTION__, vmeaddr, len, am, offset, size);
  cdcm_err:
  iounmap((void __iomem *) phys);
  xpc_vme_master_unmap(phys, len);
  return(-1);
#endif
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_exclude_vme_dev
 * DESCRIPTION: Device exterminator
 * RETURNS:     exactly what 'xpc_vme_master_unmap()' returns:
 *		0       - Success
 *		-EINVAL - Error, invalid window address or size.
 *-----------------------------------------------------------------------------
 */
static int
cdcm_exclude_vme_dev(
		     struct cdcmDevInfo_t *infoT) /* device to bump off */
{
  unsigned int iex;

  iounmap((void __iomem *) infoT->di_vme->vd_virt_addr);
  iex = xpc_vme_master_unmap(infoT->di_vme->vd_phys_addr, infoT->di_vme->vd_len);
  cdev_del(infoT->di_cdev);	/* remove char device from the kernel */
  /* release device numbers */
  unregister_chrdev_region(infoT->di_dev_num, cdcmStatT.cdcm_chan_per_dev);
  kfree(infoT->di_vme);
  list_del_init(&infoT->di_list); /* exclude from the list */
  kfree(infoT); /* free memory */
  --cdcmStatT.cdcm_dev_am; /* decrease device amount counter */
  return(iex);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    return_controller
 * DESCRIPTION: Returns previously reserved VME/VSB area. We should exclude it
 *		from the CDCM supervision.
 * RETURNS:     0              - if SUCCESS.
 *		negative value - otherwise.
 *-----------------------------------------------------------------------------
 */
unsigned int
return_controller(
		  unsigned int cpuaddr, /*  */
		  unsigned int len)     /*  */
{
  struct cdcmDevInfo_t *infoT, *tmpT;

  /* exclude it from CDCM */
  list_for_each_entry_safe(infoT, tmpT, &cdcmStatT.cdcm_dev_list_head, di_list) {
    if (cpuaddr == infoT->di_vme->vd_virt_addr) /* gotcha! - bump off! */
      return(cdcm_exclude_vme_dev(infoT));
  }

  return((unsigned int) -1);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_vme_cleanup
 * DESCRIPTION: Free system resources and deallocates 'cdcmDevInfo_t'
 *		structures.
 * RETURNS:     void
 *-----------------------------------------------------------------------------
 */
void
cdcm_vme_cleanup(void)
{
  struct cdcmDevInfo_t *infoT, *tmpT;
  
  list_for_each_entry_safe(infoT, tmpT, &cdcmStatT.cdcm_dev_list_head, di_list)
    cdcm_exclude_vme_dev(infoT);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_vme_irq_sched_prio
 * DESCRIPTION: Change IRQ scheduling priority for the given interrupt level.
 *		VME Interrupt leves are [1-7]
 * RETURNS:	negative val                            - if fails.
 *		previous priority (some positive value) - if all OK.
 *-----------------------------------------------------------------------------
 */
int
cdcm_vme_irq_sched_prio(
			short irq_line, /* [1-7] interrupt request line
					   (interrupt level) */
			int sched_prio) /* New RT priority
					   Must be within allowable range
					   if 0 - just give back current 
					   prio value */
{
  struct task_struct *irq_thr = NULL;
  int prio_max = 0, prio_min = 0;
  struct sched_param param = {0};
  int last_prio;
  int rc;

  /* check if IRQ level is in range */
  if (!WITHIN_RANGE(1, irq_line, 7))
    return(-ERANGE);

  /* get pid of irq kernel thread, registered for the given interrupt level */
  irq_thr = irq_descp(irq_line + RIO_ACT_VMEIRQ1 - 1)->thread;

  last_prio = irq_thr->rt_priority;

  if (!sched_prio) /* just give back current value */
    return(last_prio);

  /* check if new RT priority is exactly as the old one */
  if (last_prio == sched_prio)
    return(-EINVAL);  /* nothing to do */

  /* get max/min priorityes */
  prio_min = sched_get_priority_min(irq_thr->policy);
  prio_max = sched_get_priority_max(irq_thr->policy);
  
  /* check new IRQ priority level is in range */
  if (!WITHIN_RANGE(prio_min, sched_prio, prio_max))
    return(-ERANGE);

  param.sched_priority = sched_prio; /* set new priority */

  if ( (rc = sched_setscheduler(irq_thr, irq_thr->policy, &param)) )
    return(rc);	/* error */
  
  return(last_prio);
}
