/**
 * @file cdcmPci.c
 *
 * @brief LynxOS system wrapper routines are located here as long as Linux
 *        driver functions for initializing and setting up the pci driver.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 02/06/2006
 *
 * This functions are grouped together for LynxOS DRM implementation.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version $Id: cdcmPci.c,v 1.5 2009/01/09 10:26:03 ygeorgie Exp $
 */
#include "cdcmDrvr.h"
#include "cdcmLynxDefs.h"
#include "cdcmUsrKern.h"
#include "cdcmBoth.h"

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
 * @param buslayer_id - not used
 * @param vendor_id   - 
 * @param device_id   - 
 * @param node_h      - 
 *
 * @return DRM_OK      - (i.e. 0) if success
 * @return DRM_EFAULT
 * @return DRM_ENODEV etc... (see drm_errno.h for more info on ret codes)
 */
int drm_get_handle(int buslayer_id, int vendor_id, int device_id,
		   struct drm_node_s **node_h)
{
  cdcm_dev_info_t *dit;
  struct cdcm_grp_info_t *curgrp = container_of(glob_cur_grp_ptr,
						struct cdcm_grp_info_t,
						grp_list);
  struct pci_dev *pprev = (list_empty(&curgrp->grp_dev_list)) ? NULL : 
    (container_of(list_last(&curgrp->grp_dev_list), 
		  cdcm_dev_info_t, di_list))->di_pci;
  struct pci_dev *pcur = pci_get_device(vendor_id, device_id, pprev);
  
  cdcm_err = 0;	/* reset */

  if (!pcur)
    return(DRM_ENODEV);	/* not found */

  if (pci_enable_device(pcur)) {
    printk("Error enabling %s pci device %p\n", cdcmStatT.cdcm_mod_name, pcur);
    return(DRM_EBADSTATE);
  }

  if (!(dit = (cdcm_dev_info_t*)kzalloc(sizeof(*dit), GFP_KERNEL))) {
    printk("Can't alloc info table for %p pci device\n", pcur);
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
int drm_device_read(struct drm_node_s *node_h, int resource_id,
		    unsigned int offset, unsigned int size, void *buffer)
{
  int retcode = DRM_OK; /* 0 */
  u_int8_t pci_byte_val;
  u_int16_t pci_word_val;
  u_int32_t pci_dword_val;
  cdcm_dev_info_t *cast;
  unsigned int where;

  cast = (cdcm_dev_info_t *)node_h;

  switch (resource_id) {
  case PCI_RESID_REGS:	    /* 1 */
    /* From the LynxOS man pages:
     * PCI_RESID_REGS Returns the device specific PCI Configuration
     * register (32 bits) in the 32 bit location pointed to by the
     * buffer. The offset paramter contains the register number to
     * be read [0..63], the size parameter must be set to 0, 1, 2 or
     * 4; A size of zero will read 32 bits of the PCI configuration
     * space. A size of 1 will read a byte, 2 will read 16 bits and
     * 4 will read 32 bits. As the offset specified is the register
     * number, it is only possible to read only the lowest 8 or
     * 16 bits in a register, i.e only 4 byte aligned address can be
     * provided using this interface.
     */
    /* Note: pci_read_config_(d)word() converts the value read 
     * from little-endian (PCI registers) to the native byte order of 
     * the processor, so here we do not need to worry about endianness.
     */
    where = offset * 4; /* LynxOS accesses only 4byte-aligned addresses. */
    switch (size) {
    case 1:
      if (pci_read_config_byte(cast->di_pci, where, &pci_byte_val) < 0)
	return DRM_EFAULT;
      *(u_int8_t *)buffer = pci_byte_val; // read value
      break;
    case 2:
      if (pci_read_config_word(cast->di_pci, where, &pci_word_val) < 0)
	return DRM_EFAULT;
      *(u_int16_t *)buffer = pci_word_val; // read value
      break;
    default:
      if (pci_read_config_dword(cast->di_pci, where, &pci_dword_val) < 0)
	return DRM_EFAULT;
      *(u_int32_t *)buffer = pci_dword_val; // read value
      break;
    }
    break;
  case PCI_RESID_BUSNO:	    /* 2 */
    retcode = DRM_EINVALID;
    break;
  case PCI_RESID_DEVNO:	    /* 3 */
    *(unsigned int *)buffer = PCI_SLOT(cast->di_pci->devfn);
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
int drm_device_write(struct drm_node_s *node_h, int resource_id,
		     unsigned int offset, unsigned int size, void *buffer)
{
  int retcode = DRM_OK;		/* 0 */
  u_int8_t pci_byte_val;
  u_int16_t pci_word_val;
  u_int32_t pci_dword_val;
  cdcm_dev_info_t *cast;
  unsigned int where;

  cast = (cdcm_dev_info_t*) node_h;

  switch (resource_id) {
  case PCI_RESID_REGS:	    /* 1 */
    /* From LynxOS' man pages:
     * PCI_RESID_REGS Writes the device specific PCI Configuration
     * register (32 bits) with the 32 bit contents pointed to by the
     * buffer. The offset paramter contains the register number to
     * be written [0..63], the size parameter must be set to 0, 1, 2 or
     * 4; A size of zero will write 32 bits of the PCI configuration
     * space. A size of 1 will write a byte, 2 will write 16 bits and
     * 4 will write 32 bits. As the offset specified is the register
     * number, it is only possible to write only the lowest 8 or
     * 16 bits in a register, i.e only 4 byte aligned address can be
     * provided using this interface.
     */
    /* Note: pci_write_config_(d)word() converts the value from native 
     * endianness of the processor to little-endian (PCI registers), 
     * so here we do not need to worry about endianness.
     */
    where = offset * 4; /* LynxOS accesses only 4byte-aligned addresses. */
    switch (size) {
    case 1:
      pci_byte_val = *(u_int8_t *)buffer;
      if (pci_write_config_byte(cast->di_pci, where, pci_byte_val) < 0) 
	return DRM_EFAULT;
      break;
    case 2:
      pci_word_val = *(u_int16_t *)buffer;
      if (pci_write_config_word(cast->di_pci, where, pci_word_val) < 0)
	return DRM_EFAULT;
      break;
    default:
      pci_dword_val = *(u_int32_t *)buffer;
      if (pci_write_config_dword(cast->di_pci, where, pci_dword_val) < 0)
	return DRM_EFAULT;
      break;
    }
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
int drm_register_isr(struct drm_node_s *node_h, void *isr, void *arg)
{
  int cc;
  cdcm_dev_info_t *cast;
  cdcm_irqret_t (*isrcast)(int, void *);

  cast = (cdcm_dev_info_t*)node_h;
  isrcast = (cdcm_irqret_t (*)(int, void *))isr;

  printk("Registering IRQ #%d \n", cast->di_pci->irq);
  if ( (cc = request_irq(cast->di_pci->irq, isrcast, 
			 IRQF_SHARED | IRQF_DISABLED, DRIVER_NAME, arg)) ) {
    printk("Could not allocate IRQ #%d\n", cast->di_pci->irq);
    return(SYSERR);
  }
  printk("IRQ #%d registered\n", cast->di_pci->irq);
  return(DRM_OK);
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

  free_irq(cast->di_pci->irq, cast->di_pci); /* this is wrong since it should
					      * be the cookie passed to register_isr.
					      */
  return(DRM_OK);
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
  int bar;
  unsigned long mmio_length;
  cdcm_dev_info_t *cast;
  cast = (cdcm_dev_info_t*) node_h;
  bar = (resource_id - PCI_RESID_BAR0);
  printk("mapping PCI regs: BAR #%d\n", bar);
  if (bar < 0 || bar > 5)
    return DRM_EFAULT;

  mmio_length = pci_resource_len(cast->di_pci, bar);
  printk("Requesting PCI region, dev %p : bar #%d\n", cast->di_pci, bar);
  if(pci_request_region(cast->di_pci, bar, DRIVER_NAME) != 0)
    return DRM_EFAULT;
  printk("PCI Region request successful; Mapping the bar..\n");
  *vadrp = (unsigned int)pci_iomap(cast->di_pci, bar, mmio_length);
  printk("PCI Mapping successful: bar #%d mapped in addr 0x%x\n", bar, *vadrp);
  return(DRM_OK);
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
  int bar;
  cdcm_dev_info_t *cast;
  cast = (cdcm_dev_info_t*) node_h;

  bar = (resource_id - PCI_RESID_BAR0);
  if (bar < 0 || bar > 5)
    return DRM_EFAULT;
  pci_release_region(cast->di_pci, bar);
  return(DRM_OK);
}
