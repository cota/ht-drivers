/* $Id: cdcmPci.c,v 1.4 2007/12/19 09:02:05 ygeorgie Exp $ */
/**
 * @file cdcmPci.c
 *
 * @brief LynxOS system wrapper routines are located here as long as Linux
 *        driver functions for initializing and setting up the pci driver.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Feb, 2006
 *
 * This functions are grouped together for LynxOS DRM implementation.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version 5.0  ygeorgie  01/08/2007  Full Lynx-like installation behaviour.
 * @version 4.0  ygeorgie  24/07/2007  Heavily modified to use
 *                                     @e pci_get_device() instead of
 *                                     @e pci_register_driver()during 
 *                                     @e drm_get_handle() function call.
 * @version 3.0  ygeorgie  14/03/2007  Production release, CVS controlled.
 * @version 2.0  ygeorgie  27/07/2006  First working release.
 * @version 1.0  ygeorgie  02/06/2006  Initial version.
 */
#include "cdcmDrvr.h"
#include "cdcmLynxDefs.h"
#include "cdcmUsrKern.h"

/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern struct file_operations cdcm_fops; /* char dev file operations */
extern cdcmStatics_t cdcmStatT;	/* CDCM statics table */
extern int cdcm_err;		/* global error */
extern struct list_head *glob_cur_grp_ptr; /* current working group */

int cdcm_dbg_cntr = 0; /* TODO. REMOVE. for deadlock debugging */


/**
 * @brief Free system used resources. !TODO!
 *
 * @param none
 *
 * @return void
 */
void cdcm_pci_cleanup(void)
{
#if 0
  struct cdcm_dev_info_t *infoT, *tmpT;
  /*
    NEWBIE NOTE
    Keep it here only for educating driver writers (and myself as a newbie
    and in case of serious brain damage)
    'list_for_each_entry' is not working here, as in this case
    'list_del_init' call fails with message:
    'Unable to handle kernel paging request at virtual address 00100100'
    (which is exactly 'LIST_POISON1' value).
    Maybe that's because of the 'prefetch' in 'list_for_each' definition?
    'list_for_each_entry_safe' is not using 'prefetch'
  */
  list_for_each_entry_safe(infoT, tmpT, &cdcmStatT.cdcm_grp_list_head, di_list) {
    
  }
#endif
}


/**
 * @brief Wrapper. LynxOs DRM Services for PCI.
 *
 * @param buslayer_id - 
 * @param vendor_id   - 
 * @param device_id   - 
 * @param node_h      - 
 *
 * @return DRM_OK      - (i.e. 0) if success
 * @return DRM_EFAULT
 * @return DRM_ENODEV etc... (see drm_errno.h for more info on ret codes)
 */
int drm_get_handle(int buslayer_id, int vendor_id, int device_id, struct drm_node_s **node_h)
{
  cdcm_dev_info_t *dit;
  struct cdcm_grp_info_t *curgrp = container_of(glob_cur_grp_ptr, struct cdcm_grp_info_t, grp_list);
  struct pci_dev *pprev = (list_empty(&curgrp->grp_dev_list)) ? NULL : (container_of(list_last(&curgrp->grp_dev_list), cdcm_dev_info_t, di_list))->di_pci;
  struct pci_dev *pcur = pci_get_device(vendor_id, device_id, pprev);
  
  cdcm_err = 0;	/* reset */

  if (!pcur)
    return(DRM_ENODEV);	/* not found */

  if (pci_enable_device(pcur)) {
    PRNT_INFO("Error enabling %s pci device %p\n", cdcmStatT.cdcm_mod_name, pcur);
    return(DRM_EBADSTATE);
  }

  if (!(dit = (cdcm_dev_info_t*)kzalloc(sizeof(*dit), GFP_KERNEL))) {
    PRNT_INFO("Can't alloc info table for %p pci device\n", pcur);
    pci_disable_device(pcur);
    pci_dev_put(pcur);
    cdcm_err = -ENOMEM;
    return(DRM_ENOMEM);
  }

  /* hook MFD (My Fucking Data) */
  pci_set_drvdata(pcur, dit);

  dit->di_pci    = pcur;
  dit->di_parent = curgrp->grp_num;
  dit->di_alive  = ALIVE_ALLOC | ALIVE_CDEV;
  *node_h = (struct drm_node_s*)dit;
  cdcmStatT.cdcm_drvr_type = 'P';

  /* add device to the device linked list of the current group */
  list_add(&dit->di_list, &curgrp->grp_dev_list);

  return(DRM_OK); /* 0 */
}


/**
 * @brief Wrapper. LynxOs DRM Services for PCI.
 *
 * @param dev_n - 
 *
 * @return DRM_OK      - (i.e. 0) if success
 * @return DRM_EFAULT
 * @return DRM_ENODEV (see /usr/include/sys/drm_errno.h for more info on
 *                     ret codes)
 */
int drm_free_handle(struct drm_node_s *dev_n)
{
  cdcm_dev_info_t *cast;
  cdcm_dev_info_t *dit;  /* just as an example for the newbies */

  cast = (cdcm_dev_info_t*) dev_n;
  dit = pci_get_drvdata(cast->di_pci);

  list_del(&cast->di_list);
  pci_disable_device(cast->di_pci);
  pci_dev_put(cast->di_pci);
  kfree(cast);
  return(DRM_OK); /* 0 */
}


/**
 * @brief Wrapper. LynxOs DRM Services for PCI. !TODO!
 *
 * @param node_h      - 
 * @param resource_id - 
 * @param offset      - 
 * @param size        - 
 * @param buffer      - 
 *
 * @return DRM_OK       - (i.e. 0) if success
 * @return DRM_EINVALID - The node_h is not a valid drm handle.
 *                        The offset parameter is invalid.
 *		          The resource_id parameter is invalid.
 * @return DRM_EFAULT   - The node_h or buffer is not a valid pointer
 *
 *                        See drm_errno.h for more info on ret codes.
 *		          See Lynx man for more info on the function.
 */
int drm_device_read(struct drm_node_s *node_h, int resource_id, unsigned int offset, unsigned int size, void *buffer)
{
  int retcode = DRM_OK; /* 0 */
  u16 pci_word_val;
  cdcm_dev_info_t *cast;
  
  cast = (cdcm_dev_info_t*) node_h;

  switch (resource_id) {
  case PCI_RESID_REGS:	    /* 1 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BUSNO:	    /* 2 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_DEVNO:	    /* 3 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_FUNCNO:    /* 4 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_DEVID:	    /* 5 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_VENDORID:  /* 6 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_CMDREG:    /* 7 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_REVID:	    /* 8 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_STAT:	    /* 9 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_CLASSCODE: /* 16 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_SUBSYSID:  /* 17 */
    pci_read_config_word(cast->di_pci, PCI_SUBSYSTEM_ID, &pci_word_val);
    *(unsigned int*) buffer = pci_word_val;
    break;
  case PCI_RESID_SUBSYSVID: /* 18 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR0:	    /* 10 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR1:	    /* 11 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR2:	    /* 12 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR3:	    /* 13 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR4:	    /* 14 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR5:	    /* 15 */
    retcode = DRM_EINVALID;
    break;
  default:
    retcode = DRM_EINVALID;
    break;
  }

  return(retcode);
}


/**
 * @brief Wrapper. LynxOs DRM Services for PCI. !TODO!
 *
 * @param node_h
 * @param resource_id
 * @param offset
 * @param size
 * @param buffer
 *
 * @return DRM_OK      - (i.e. 0) if success 
 * @return DRM_EFAULT
 * @return DRM_ENODEV
 *         (see drm_errno.h for more info on ret codes)
 */
int drm_device_write(struct drm_node_s *node_h, int resource_id, unsigned int offset, unsigned int size, void *buffer)
{
  int retcode = DRM_OK;		/* 0 */
  cdcm_dev_info_t *cast;
  
  cast = (cdcm_dev_info_t*) node_h;

  switch (resource_id) {
  case PCI_RESID_REGS:	    /* 1 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BUSNO:	    /* 2 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_DEVNO:	    /* 3 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_FUNCNO:    /* 4 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_DEVID:	    /* 5 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_VENDORID:  /* 6 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_CMDREG:    /* 7 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_REVID:	    /* 8 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_STAT:	    /* 9 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_CLASSCODE: /* 16 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_SUBSYSID:  /* 17 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_SUBSYSVID: /* 18 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR0:	    /* 10 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR1:	    /* 11 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR2:	    /* 12 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR3:	    /* 13 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR4:	    /* 14 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_BAR5:	    /* 15 */
    retcode = DRM_EINVALID;
    break;
  default:
    retcode = DRM_EINVALID;
    break;
  }

  return(retcode);
}


/**
 * @brief Wrapper. LynxOs DRM Services for PCI. !TODO!
 *
 * @param node - 
 *
 * @return 
 */
int drm_locate(struct drm_node_s *node)
{
  cdcm_dev_info_t *cast;
  cast = (cdcm_dev_info_t*) node;

  return(DRM_EINVALID);
}


/**
 * @brief Wrapper. LynxOs DRM Services for PCI. !TODO!
 *
 * @param node_h - 
 * @param isr    - 
 * @param arg    - 
 *
 * @return 
 */
int drm_register_isr(struct drm_node_s *node_h, void (*isr)(), void *arg)
{
  cdcm_dev_info_t *cast;
  cast = (cdcm_dev_info_t*) node_h;

  return(DRM_EINVALID);
}


/**
 * @brief Wrapper. LynxOs DRM Services for PCI. !TODO!
 *
 * @param node_h -
 *
 * @return 
 */
int drm_unregister_isr(struct drm_node_s *node_h)
{
  cdcm_dev_info_t *cast;
  cast = (cdcm_dev_info_t*) node_h;

  return(DRM_EINVALID);
}


/**
 * @brief Wrapper. LynxOs DRM Services for PCI. !TODO!
 *
 * @param node_h      - 
 * @param resource_id - 
 * @param vadrp       - 
 *
 * @return 
 */
int drm_map_resource(struct drm_node_s *node_h, int resource_id, unsigned int *vadrp)
{
  cdcm_dev_info_t *cast;
  cast = (cdcm_dev_info_t*) node_h;

  return(DRM_EINVALID);
}


/**
 * @brief Wrapper. LynxOs DRM Services for PCI. !TODO!
 *
 * @param node_h      - 
 * @param resource_id - 
 *
 * @return 
 */
int drm_unmap_resource(struct drm_node_s *node_h, int resource_id)
{
  cdcm_dev_info_t *cast;
  cast = (cdcm_dev_info_t*) node_h;

  return(DRM_EINVALID);
}
