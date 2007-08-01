/* $Id: cdcmVme.c,v 1.3 2007/08/01 15:07:21 ygeorgie Exp $ */
/*
; Module Name:	 cdcmVme.c
; Module Descr:	 CDCM VME Linux driver that encapsulates user-written LynxOS
;		 driver. Consider it like a wrapper of the user driver.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: Jan, 2007
; Authors:	 Yury Ge<orgievskiy, Alain Gagnaire, Nicolas De Metz-Noblat
;		 CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmVme.c: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 4.0   ygeorgie   01/08/07   Full Lynx-like installation behaviour.
; 3.0   ygeorgie   24/04/07   Possibility to change IRQ scheduling priority.
; 2.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 1.0	ygeorgie   16/01/07   Initial version.
*/

#include "cdcmDrvr.h"
#include "cdcmLynxAPI.h"
#include "cdcmUsrKern.h"
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

/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern struct file_operations cdcm_fops; /* char dev file operations */
extern cdcmStatics_t cdcmStatT;	/* CDCM statics table */
extern int cdcm_err;		/* global error */
extern struct list_head *glob_cur_grp_ptr; /* current working group */

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
  char irq_name[0x80];		/* unique irq name */

  if (cdcm_irq_vtp[vector])	/* already used */
    return(SYSERR);
    
  cdcm_irq_vtp[vector] = (int(*)(void*))function; /* init pointer */

  /* build-up name */
  sprintf(irq_name, "%s_%d", cdcmStatT.cdcm_mod_name, vector);

  /*  */
  if ( (coco = xpc_vme_request_irq(vector, (void(*)(int,void*))cdcm_irq_handler, arg, irq_name)) ) {
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
 *		driver installation for every device he wants to manage. Like
 *		this all internal CDCM structures will be initialized. Consider
 *		it as the VME analogue to the drm_get_handle() for the PCI. 
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
  cdcm_dev_info_t *dit = NULL;
  xpc_vme_type_e access_type;
  int options = 0;		/* not yet supported */
  int cntr;
  unsigned int cpu_addr;
  unsigned int phys;
  struct cdcm_grp_info_t *curgrp = container_of(glob_cur_grp_ptr, struct cdcm_grp_info_t, grp_list);

  CDCM_DBG("%s %s(%x,%x,%x,%x,%x)\n", cdcmStatT.cdcm_mod_name, __FUNCTION__, vmeaddr, len, am, offset, size);

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
    PRNT_ABS_ERR("Unknown AM=%X in %s driver\n", am, cdcmStatT.cdcm_mod_name);
    return(-1);
  }
  
  
#if 0
  // CES-written code. Does not work. System hangs.
  unsigned int data;
  int err;
  if (size != 0) {
  /* Verify presence of module by a read test of n bytes at specified offset */
    err = xpc_vme_safe_pio(vmeaddr + offset, 0x0, access_type, 0, size, &data);
    if (err == (-EIO))
      return (-1);
    if (err == (-ENOMEM)) {
      printk("%s %s: VME ressource limit exhausted\n", cdcmStatT.cdcm_mod_name, __FUNCTION__);
      return (-1);
    }
    if (err != 0) {
      printk("%s %s: unknown error %d\n", cdcmStatT.cdcm_mod_name, __FUNCTION__, -err);
      return (-1);
    }
  }
#endif

  phys = xpc_vme_master_map(vmeaddr, 0x0, len, access_type, options);
  if (phys == (-1))
    return(phys);
  
  if (!(cpu_addr = (unsigned int)ioremap(phys, len))) {
    PRNT_ABS_ERR("ioremap error in %s driver\n", cdcmStatT.cdcm_mod_name);
    (void) xpc_vme_master_unmap(phys, len);
    return(-1);
  }

  
#if 1
  /* do actual hardware access */
  if (size != 0) {
    //unsigned long flags;
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

  if (!(dit = (cdcm_dev_info_t*)kzalloc(sizeof(*dit), GFP_KERNEL))) {
    PRNT_INFO_MSG("Can't allocate VME device info table in %s group.\n", curgrp->grp_name);
    cdcm_err = -ENOMEM;
    goto cdcm_err;
  }

  if (!(dit->di_vme = (struct vme_dev*)kzalloc(sizeof(*dit->di_vme), GFP_KERNEL)) ) { /* vme description table allocation */
    PRNT_INFO_MSG("Can't allocate VME device descriptor in %s group.\n", curgrp->grp_name);
    kfree(dit);
    cdcm_err = -ENOMEM;
    goto cdcm_err;
  }

  /* save VME device data */
  dit->di_vme->vd_virt_addr = cpu_addr;
  dit->di_vme->vd_phys_addr = phys;
  dit->di_vme->vd_len       = len;
  dit->di_vme->vd_am        = am;

  cdcmStatT.cdcm_drvr_type = 'V';

  /* initialize device info table */
  dit->di_alive = ALIVE_ALLOC | ALIVE_CDEV;
  dit->di_pci   = NULL;

  /* add device to the device linked list of the current group */
  list_add(&dit->di_list/*new*/, &curgrp->grp_dev_list/*head*/);
  
  PRNT_INFO_MSG("%s %s(%x,%x %x %x %x) return %x virt addr for %x phys addr\n", cdcmStatT.cdcm_mod_name, __FUNCTION__, vmeaddr, len, am, offset, size, cpu_addr, phys);
  
  return(cpu_addr);
  
#if 1
  berr:
  PRNT_ABS_ERR("For %s driver %s(%x,%x,%x,%x,%x) bus error!\n", cdcmStatT.cdcm_mod_name, __FUNCTION__, vmeaddr, len, am, offset, size);
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
		     cdcm_dev_info_t *infoT) /* device to bump off */
{
  unsigned int iex;

  iounmap((void __iomem *) infoT->di_vme->vd_virt_addr);
  iex = xpc_vme_master_unmap(infoT->di_vme->vd_phys_addr, infoT->di_vme->vd_len);
  kfree(infoT->di_vme);
  list_del_init(&infoT->di_list); /* exclude from the list */
  kfree(infoT); /* free memory */
  return(iex);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    return_controller
 * DESCRIPTION: Returns previously reserved VME/VSB area. We should exclude the
 *		device from the CDCM supervision.
 * RETURNS:     0              - if SUCCESS.
 *		negative value - otherwise.
 *-----------------------------------------------------------------------------
 */
unsigned int
return_controller(
		  unsigned int cpuaddr, /*  */
		  unsigned int len)     /*  */
{
  struct cdcm_grp_info_t *grp_it, *tmp_grp_it; /* group info tables */
  cdcm_dev_info_t *dev_it, *tmp_dev_it;	/* device info tables */

  /* find and exclude */
  list_for_each_entry_safe(grp_it, tmp_grp_it, &cdcmStatT.cdcm_grp_list_head, grp_list) { /* pass through all the groups */
    list_for_each_entry_safe(dev_it, tmp_dev_it, &grp_it->grp_dev_list, di_list) { /* pass through all the devices in the current group */
      if (cpuaddr == dev_it->di_vme->vd_virt_addr) /* gotcha! - bump off! */
	return(cdcm_exclude_vme_dev(dev_it));
    }
  }

  return((unsigned int) -1);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_vme_cleanup
 * DESCRIPTION: Free all system resources related to VME.
 * RETURNS:     void
 *-----------------------------------------------------------------------------
 */
void
cdcm_vme_cleanup(void)
{
  struct cdcm_grp_info_t *grp_it, *tmp_grp_it; /* group info tables */
  cdcm_dev_info_t *dev_it, *tmp_dev_it;	/* device info tables */

  /* exclude */
  list_for_each_entry_safe(grp_it, tmp_grp_it, &cdcmStatT.cdcm_grp_list_head, grp_list) { /* pass through all the groups */
    list_for_each_entry_safe(dev_it, tmp_dev_it, &grp_it->grp_dev_list, di_list) { /* pass through all the devices in the current group */
      cdcm_exclude_vme_dev(dev_it);
    }
  }
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
