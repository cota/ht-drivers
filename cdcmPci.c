/* $Id: cdcmPci.c,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcmPci.c
; Module Descr:	 LynxOS system wrapper routines are located here as long as
;		 Linux driver functions for initializing and setting up the 
;		 pci driver. This functions are grouped together for LynxOS 
;		 DRM implementation.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: Feb, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmPci.c: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   02/06/06   Initial version.
*/

#include "cdcmDrvr.h"
#include "cdcmLynxDefs.h"

/* external functions */
extern int cdcm_user_part_device_recodnizer(struct pci_dev*, char*);
extern int cdcm_set_unique_device_name(char devName[CDCM_DNL]);
extern int cdcm_user_get_dev_name(void*, char devName[CDCM_DNL]);
extern struct cdev* cdcm_get_cdev(char devName[CDCM_DNL], int, dev_t*);

/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern struct file_operations cdcm_fops; /* char dev file operations */
extern cdcmStatics_t cdcmStatT;	/* CDCM statics table */
extern int cdcm_err;		/* global error */

/* static functions declaration */
static int  cdcm_init_one(struct pci_dev*, const struct pci_device_id*);
static void cdcm_remove_one(struct pci_dev*);


static struct pci_device_id cdcm_pci_tbl[] = {
  { PCI_DEVICE(PCI_ANY_ID, PCI_ANY_ID) }, /* vendorId and deviceId will be
					     specifyed during 'drm_get_handle'
					     call */
  { 0, } /* terminate list */
};

/* CDCM driver description */
static struct pci_driver cdcm_pci_driver = {
  .name           = "cdcmPCI",
  .id_table       = cdcm_pci_tbl,
  .probe          = cdcm_init_one,
  .remove         = __devexit_p(cdcm_remove_one)
};

int cdcm_dbg_cntr = 0; /* TODO. REMOVE. for deadlock debugging */

/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_init_one.
 * DESCRIPTION: Called by the kernel for each found device, based on the 
 *		VendorID and DeviceID. This function is registered as a 
 *		.probe handler in struct pci_driver.
 * RETURNS:	0        - if success.
 *		negative - otherwise.
 *-----------------------------------------------------------------------------
 */
static int __devinit
cdcm_init_one(
	      struct pci_dev             *pdev, /*  */
	      const struct pci_device_id *ent)  /*  */
{
  struct cdcmDevInfo_t *infoT;
  char usrDevNm[CDCM_DNL]; /* user-defined device name */

  if (pci_enable_device(pdev)) {
    PRNT_INFO_MSG("Error enabling %s pci device %p\n", CDCM_DEV_NAME, pdev);
    return(-EIO);
  }

  PRNT_INSTALL("%s pci device %p enabled.\n", CDCM_DEV_NAME, pdev);

  /* NEWBE NOTE. just an examples howto access pci device... */
  //int ssid, airq;
  //u8 assignedIRQ;
  //pci_read_config_byte(pdev, PCI_INTERRUPT_LINE, &assignedIRQ);
  //pci_read_config_word(pdev, PCI_SUBSYSTEM_ID, &ssid);
  //pci_read_config_byte(pdev, PCI_INTERRUPT_LINE, &airq);


  /* get user-designed device name */
  if (cdcm_user_get_dev_name((void*)pdev, usrDevNm)) {
    u16 subsysID;
    pci_read_config_word(pdev, PCI_SUBSYSTEM_ID, &subsysID);
    PRNT_INFO_MSG("Device %p with SubsystemID %d is NOT supported.\n", pdev, subsysID);
    return(-ENODEV); /* not supported */
  }
  cdcm_set_unique_device_name(usrDevNm);

  /* allocate device info table */
  if (!(infoT = (struct cdcmDevInfo_t *)kmalloc(sizeof(struct cdcmDevInfo_t), GFP_KERNEL))) {
    PRNT_ABS_WARN("Can't allocate info table for %s dev.", usrDevNm);
    cdcm_err = -ENOMEM;
    return(cdcm_err);
  }

  /* register char device and get device number */
  infoT->di_cdev = cdcm_get_cdev(usrDevNm, cdcmStatT.cdcm_chan_per_dev, &infoT->di_dev_num);
  
  if (IS_ERR(infoT->di_cdev)) {
    PRNT_ABS_ERR("Can't register %s as character device... Aborting!", usrDevNm);
    kfree(infoT);
    return PTR_ERR(infoT->di_cdev);
  }

  /* initialize device info table */
  infoT->di_line  = cdcmStatT.cdcm_dev_am; /* set device index */
  infoT->di_alive = ALIVE_ALLOC | ALIVE_CDEV;
  infoT->di_pci   = pdev;
  infoT->di_vme   = NULL;
  strcpy(infoT->di_name, usrDevNm); /* set device name */
  //INIT_LIST_HEAD(&infoT->di_thr_list);

  /* hook MFD (My Fucken Data) */
  pci_set_drvdata(pdev, infoT);

  /* add device to the device linked list */
  list_add(&infoT->di_list, &cdcmStatT.cdcm_dev_list_head);
  ++cdcmStatT.cdcm_dev_am; /* increase found device counter */

  return(0); /* all OK */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_remove_one.
 * DESCRIPTION: Called by the kernel for each device been removing.
 * RETURNS:     void
 *-----------------------------------------------------------------------------
 */
static void __devexit
cdcm_remove_one(
		struct pci_dev *dev)
{
  struct cdcmDevInfo_t *infoT = pci_get_drvdata(dev); /* MFD */
  PRNT_INFO("Removing %s [%p] device\n", infoT->di_name, dev);

  /* disable it first */
  pci_disable_device(dev);

  //CDCM_DBG("==============> [%p] Unregistering cdev...\n", dev);
  cdev_del(infoT->di_cdev);	/* remove char device from the kernel */
  //CDCM_DBG("==============> [%p] Unregistering cdev DONE\n", dev);

  /* release device numbers */
  unregister_chrdev_region(infoT->di_dev_num, cdcmStatT.cdcm_chan_per_dev);
 
  //CDCM_DBG("================> [%p] Excluding from list...\n", dev);
  list_del_init(&infoT->di_list); /* exclude from device linked list */
  //CDCM_DBG("================> [%p] Excluding from list DONE\n", dev);
  
  //CDCM_DBG("================> [%p] Freeng memory...\n", dev);
  kfree(infoT);	/* chiao bambino! */
  //CDCM_DBG("================> [%p] Freeng memory DONE\n", dev);

  --cdcmStatT.cdcm_dev_am; /* decrease device amount counter */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_pci_cleanup
 * DESCRIPTION: Free system used resources.
 * RETURNS:     void
 *-----------------------------------------------------------------------------
 */
void
cdcm_pci_cleanup(void)
{
#if 0
  struct cdcmDevInfo_t *infoT, *tmpT;
  /*
    NEWBIE NOTE
    Keep it here only for educating driver writers (and myself in case of
    serious brain damage)
    'list_for_each_entry' is not working here, because in this case
    'list_del_init' call fails with message:
    'Unable to handle kernel paging request at virtual address 00100100'
    (which is exactly 'LIST_POISON1' value).
    IMHO this is because of the 'prefetch' in 'list_for_each' definition.
    'list_for_each_entry_safe' is not using 'prefetch'
  */
  list_for_each_entry_safe(infoT, tmpT, &cdcmStatT.cdcm_dev_list_head, di_list) {
    if (infoT->di_alive & ALIVE_CDEV) {
        cdev_del(&infoT->di_cdev); /* remove char device from the system */
      }

    list_del_init(&infoT->di_list);
    kfree(infoT);
    cntr++;
  }
#endif

  pci_unregister_driver(&cdcm_pci_driver);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    drm_get_handle.
 * DESCRIPTION: Wrapper. LynxOs DRM Services for PCI.
 * RETURNS:	DRM_OK      - (i.e. 0) if success 
 *		DRM_EFAULT
 *		DRM_ENODEV etc...
 *		(see drm_errno.h for more info on ret codes)
 *-----------------------------------------------------------------------------
 */
int
drm_get_handle(
	       int buslayer_id, /*  */
	       int vendor_id,   /*  */
	       int device_id,   /*  */
	       struct drm_node_s **node_h) /*  */
{
  struct cdcmDevInfo_t *devInfo;
  static int firstOccurrence = 0; /* some code should be done only once */
  int rc;

  if (cdcmStatT.cdcm_drvr_type == 'V') {
    PRNT_ABS_ERR("%s driver type mismatch!\n", CDCM_USR_DRVR_NAME);
    return(DRM_EBUSY);
  }

  if (!firstOccurrence) { /* will enter here only once */
    /* If here, then it's the first call to this function.
       In this case we should call Linux procedure for PCI driver installation.
       Everything will be setup during the first call to 'drm_get_handle' 
       Linux wrapper. Subsequent calls will only return data structures which 
       were previously setup in this chunk of code */
    ++firstOccurrence;
    cdcmStatT.cdcm_drvr_type = 'P';
    
    
    /* register PCI driver */
    cdcm_pci_tbl[0] = (struct pci_device_id) {
      PCI_DEVICE(vendor_id, device_id)
    };
    
    if ((rc = pci_register_driver(&cdcm_pci_driver)) < 0) {
      PRNT_INFO_MSG("pci_register_driver() call failed, error=%d", rc);
      cdcm_err = rc;	  /* set Linux error */
      return(DRM_ENODEV); /* let's return 'No device mached' */
    }
  }
  
  *node_h = NULL;
  
  /* claim device */
  if (cdcmStatT.cdcm_dev_active == cdcmStatT.cdcm_dev_am)
    /* all are claimed already i.e. busy */
    return(DRM_EBUSY);
  
  list_for_each_entry(devInfo, &cdcmStatT.cdcm_dev_list_head, di_list) {
    if (!devInfo->di_claimed) {
      ++devInfo->di_claimed;
      ++cdcmStatT.cdcm_dev_active;
      *node_h = devInfo;
      return(DRM_OK);		/* 0 */
    }
  }
  
  return(DRM_EFAULT);  /* should not reach this point */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    drm_free_handle.
 * DESCRIPTION: Wrapper. LynxOs DRM Services for PCI.
 * RETURNS:	DRM_OK      - (i.e. 0) if success 
 *		DRM_EFAULT
 *		DRM_ENODEV
 *		(see drm_errno.h for more info on ret codes)
 *-----------------------------------------------------------------------------
 */
int
drm_free_handle(
		struct drm_node_s *dev_n)
{
  if (dev_n->di_alive && dev_n->di_claimed) {
    dev_n->di_claimed = 0;	/* reset */
    --cdcmStatT.cdcm_dev_active;
  } else {
    return(DRM_EFAULT);	/* device is dead or not claimed */
  }

  return(DRM_OK);		/* 0 */
}

/*-----------------------------------------------------------------------------
 * FUNCTION:    drm_device_read
 * DESCRIPTION: Wrapper. LynxOs DRM Services for PCI.
 * RETURNS:	DRM_OK       - (i.e. 0) if success 
 *		DRM_EINVALID - The node_h is not a valid drm handle.
 *		               The offset parameter is invalid.
 *		               The resource_id parameter is invalid.
 *		DRM_EFAULT   - The node_h or buffer is not a valid pointer
 * 
 *		See drm_errno.h for more info on ret codes.
 *		See Lynx man for more info on the function.
 *-----------------------------------------------------------------------------
 */
int
drm_device_read(
		struct drm_node_s *node_h,      /*  */
		int                resource_id,	/*  */
		unsigned int       offset,      /*  */
		unsigned int       size,        /*  */
		void              *buffer)      /*  */
{
  int retcode = DRM_OK;		/* 0 */
  u16 pci_word_val;

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
    pci_read_config_word(node_h->di_pci, PCI_SUBSYSTEM_ID, &pci_word_val);
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


/*-----------------------------------------------------------------------------
 * FUNCTION:    drm_device_write
 * DESCRIPTION: Wrapper. LynxOs DRM Services for PCI.
 * RETURNS:	DRM_OK      - (i.e. 0) if success 
 *		DRM_EFAULT
 *		DRM_ENODEV
 *		(see drm_errno.h for more info on ret codes)
 *-----------------------------------------------------------------------------
 */
int
drm_device_write(
		 struct drm_node_s *node_h,      /*  */
		 int                resource_id, /*  */
		 unsigned int       offset,      /*  */
		 unsigned int       size,        /*  */
		 void              *buffer)      /*  */
{
  int retcode = DRM_OK;		/* 0 */

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
