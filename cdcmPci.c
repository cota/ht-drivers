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
 * @version
 */
#include "list_extra.h" /* for extra handy list operations */
#include "cdcmDrvr.h"
#include "cdcmLynxDefs.h"
#include "cdcmBoth.h"
#include "cdcmPci.h"

/* external crap */
extern cdcmStatics_t cdcmStatT;	/* CDCM statics table */
extern int cdcm_err;		/* global error */

static struct cdcm_pci_isr isrs[CDCM_MAX_PCI_ISRS];
static int nr_isrs; //!< counts how many isrs have already been installed

static void *pci_bar_mappings[6]; /* keep track of bar mappings */


/**
 * @brief Claims access to a specified device. LynxOs DRM Services for PCI.
 *
 * @param buslayer_id - not used
 * @param vendor_id   -
 * @param device_id   -
 * @param node_h      -
 *
 * @note See drm_errno.h for more info on ret codes.
 *
 * @return DRM_OK        - (i.e. 0) if success
 * @return DRM_ENODEV    -
 * @return DRM_EBADSTATE -
 * @return DRM_ENOMEM    -
 */
int drm_get_handle(int buslayer_id, int vendor_id, int device_id,
		   struct drm_node_s **node_h)
{
	struct cdcm_dev_info *dit = NULL;
	struct pci_dev *pprev = (list_empty(&cdcmStatT.cdcm_dev_list)) ?
		NULL :
		list_last_entry(&cdcmStatT.cdcm_dev_list,
				struct cdcm_dev_info, di_list)->di_pci;

	struct pci_dev *pcur = pci_get_device(vendor_id, device_id, pprev);

	cdcm_err = 0; /* reset */

	if (!pcur)
		return DRM_ENODEV; /* not found */

	if (pci_enable_device(pcur)) {
		printk("Error enabling %s pci device %p\n",
		       cdcmStatT.cdcm_mn, pcur);
		return DRM_EBADSTATE;
  }


	if (!(dit = (struct cdcm_dev_info*)kzalloc(sizeof(*dit),
						   GFP_KERNEL))) {
		printk("Can't alloc info table for %p pci device\n", pcur);
		pci_disable_device(pcur);
		pci_dev_put(pcur);
		cdcm_err = -ENOMEM;
		return DRM_ENOMEM;
	}

	/* hook MFD (My Fucking Data) */
	pci_set_drvdata(pcur, dit);

	dit->di_pci   = pcur;
	dit->di_alive = ALIVE_ALLOC | ALIVE_CDEV;
	dit->di_vme   = NULL;
	*node_h = (struct drm_node_s*)dit;

	/* add device to the device linked list */
	list_add_tail(&dit->di_list, &cdcmStatT.cdcm_dev_list);

	return DRM_OK; /* 0 */
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
	struct cdcm_dev_info *cast = NULL;
	struct cdcm_dev_info *dit;  /* just as an example on howto get MFD */

	cast = (struct cdcm_dev_info*) dev_n;
	dit = pci_get_drvdata(cast->di_pci); /* get previously saved MFD */

	list_del(&cast->di_list);
	pci_disable_device(cast->di_pci);
	pci_dev_put(cast->di_pci);
	kfree(cast);
	return DRM_OK; /* 0 */
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
  struct cdcm_dev_info *cast = (struct cdcm_dev_info*) node_h;
  unsigned int where;

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
  struct cdcm_dev_info *cast = (struct cdcm_dev_info*) node_h;
  unsigned int where;

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
  struct cdcm_dev_info *cast;
  cast = (struct cdcm_dev_info*) node;

  return DRM_EINVALID;
}

/**
 * cdcm_irq_handler - wrapper for LynxOS interrupt handling
 *
 * @param irq - irq number
 * @param cookie - cookie passed to the ISR; identifies the module
 *
 * Comment from kernel/irq/manage.c:
 * 	Dev_id must be globally unique. Normally the address of the
 *	device data structure is used as the cookie. Since the handler
 *	receives this value it makes sense to use it.
 *
 * We directly call dev_id 'cookie'. Since this constraint is imposed by Linux,
 * we use it to our advantage; just comparing irq number and address of the
 * cookie, we ensure that we're calling the right handler.
 *
 * @return IRQ_HANDLED - on success
 * @return IRQ_NONE - otherwise
 */
irqreturn_t cdcm_irq_handler(int irq, void *cookie)
{
  int i;

  for (i = 0; i < nr_isrs; i++) {
    if (isrs[i].irq == irq && isrs[i].cookie == cookie) {
      isrs[i].isr(cookie); /* call the driver's ISR */
      return IRQ_HANDLED;
    }
  }
  return IRQ_NONE;
}

/**
 * drm_register_isr - wrapper for LynxOs interrupt registering
 *
 * @param node_h - node handle
 * @param isr - driver's interrupt handler
 * @param arg - cookie to identify the module
 *
 * @return irq number - on success
 * @return SYSERR - on failure
 */
int drm_register_isr(struct drm_node_s *node_h, void (*isr)(void *), void *arg)
{
  int rc;
  struct cdcm_dev_info *cast;

  cast = (struct cdcm_dev_info *)node_h;

  if (nr_isrs >= CDCM_MAX_PCI_ISRS) {
    PRNT_ABS_ERR("Too many PCI ISR's registered");
    return SYSERR;
  }

  rc = request_irq(cast->di_pci->irq, cdcm_irq_handler,
		    IRQF_SHARED | IRQF_DISABLED, __drvrnm(), arg);

  if (rc) {
    PRNT_ABS_ERR("Could not register the ISR for IRQ #%d", cast->di_pci->irq);
    return SYSERR;
  }

  isrs[nr_isrs].irq = cast->di_pci->irq;
  isrs[nr_isrs].cookie = arg;
  isrs[nr_isrs].isr = isr;
  nr_isrs++;
  return cast->di_pci->irq;
}

/**
 * drm_unregister_isr - unregister all ISR's for the device
 *
 * @param node_h - node handle
 *
 * @return DRM_OK - on success
 * @return SYSERR - on failure (wrong handle or no interrupts registered)
 */
int drm_unregister_isr(struct drm_node_s *node_h)
{
  int i;
  int freed_isrs = 0;
  struct cdcm_dev_info *cast = (struct cdcm_dev_info*) node_h;

  for (i = 0; i < nr_isrs; i++)
    if (cast->di_pci->irq == isrs[i].irq) {
      free_irq(isrs[i].irq, isrs[i].cookie);
      freed_isrs++;
    }
  if (!freed_isrs)
    return SYSERR;
  return DRM_OK;
}

/**
 * @brief get a PCI bar mapped and hook it into 
 *
 * @param dev - PCI device that owns the BAR
 * @param bar - BAR number
 * @param maxlen - length of the memory to map
 *
 * @ref maxlen specifies the maximum length to map -- pass 0 to get
 * access to the full bar without checking its length first
 *
 * @return address of the mapping - on success
 * @return NULL - on failure
 */
static void *cdcm_pci_iomap(struct pci_dev *dev, int bar, unsigned long maxlen)
{
  pci_bar_mappings[bar] = pci_iomap(dev, bar, maxlen);
  return pci_bar_mappings[bar];
}

/**
 * @brief unmap a PCI bar mapping
 *
 * @param dev - PCI device that owns the BAR
 * @param bar - BAR number
 */
static void cdcm_pci_iounmap(struct pci_dev *dev, int bar)
{
  pci_iounmap(dev, pci_bar_mappings[bar]);
  pci_bar_mappings[bar] = NULL;
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
	struct cdcm_dev_info *cast = (struct cdcm_dev_info*) node_h;
	bar = (resource_id - PCI_RESID_BAR0);
	printk("mapping PCI regs: BAR #%d\n", bar);
	if (bar < 0 || bar > 5)
		return DRM_EFAULT;

	mmio_length = pci_resource_len(cast->di_pci, bar);
	printk("Requesting PCI region, dev %p : bar #%d\n", cast->di_pci, bar);
	if (pci_request_region(cast->di_pci, bar, __drvrnm()) != 0)
		return DRM_EFAULT;
	printk("PCI Region request successful; Mapping the bar..\n");
	*vadrp = (unsigned int)cdcm_pci_iomap(cast->di_pci, bar, mmio_length);
	printk("PCI Mapping successful: bar #%d mapped in addr 0x%x\n",
	       bar, *vadrp);
	return DRM_OK;
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
	struct cdcm_dev_info *cast = (struct cdcm_dev_info*) node_h;

	bar = (resource_id - PCI_RESID_BAR0);
	if (bar < 0 || bar > 5)
		return DRM_EFAULT;
	cdcm_pci_iounmap(cast->di_pci, bar);
	pci_release_region(cast->di_pci, bar);
	return DRM_OK;
}
