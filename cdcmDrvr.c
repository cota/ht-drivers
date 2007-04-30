/* $Id: cdcmDrvr.c,v 1.2 2007/04/30 09:35:07 ygeorgie Exp $ */
/*
; Module Name:	 cdcmDrvr.c
; Module Descr:	 CDCM Linux driver that encapsulates user-written LynxOS
;		 driver. Consider it like a wrapper of the user driver.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: Feb, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmDrvr.c: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   02/06/06   Initial version.
*/

#include "cdcmDrvr.h"
#include "cdcmLynxAPI.h"
#include "cdcmMem.h"

MODULE_DESCRIPTION("Common Driver Code Manager (CDCM) Driver");
MODULE_AUTHOR("Yury Georgievskiy, CERN AB/CO");
MODULE_LICENSE("GPL");

/* driver parameter. Info file path. */
static char cdcmInfoF[32];
module_param_string(ipath, cdcmInfoF, 32, 0);
MODULE_PARM_DESC(ipath, "Absolute path name of the info file.");

/* external crap */
extern char* cdcm_get_info_table(char*, int*);
extern void  cdcm_pci_cleanup(void);
extern void  cdcm_vme_cleanup(void);

extern struct dldd entry_points; /* declared in the user driver part */

/* pointer to the driver working area (statics table). Not interpreted by 
   CDCM. Passed as a parameter to all 'dldd' entry points */
static char *LynxOsWorkingArea = NULL;

/* device info table and it's size, obtained from the info file.
   Not interpreted here. Passed as a parameter to dldd entry points */
static char *devInfoT = NULL;
static int   devInfoTSz;

/* general statics data that holds all information about current devices, 
   threads, allocated mem, etc., managed by CDCM.
   Note, that during such structure init all the fields, that were not
   initialized here, will be zero-out!   */
cdcmStatics_t cdcmStatT = {
  .cdcm_dev_list_head   = LIST_HEAD_INIT(cdcmStatT.cdcm_dev_list_head),
  .cdcm_drvr_type       = 0,	/* 'P'(for PCI) or 'V' (for VME) */
  .cdcm_dev_am          = 0,
  .cdcm_dev_active      = 0,
  .cdcm_dev_num         = 0,	/* dynamically assigned device number */
  .cdcm_mem_list_head   = LIST_HEAD_INIT(cdcmStatT.cdcm_mem_list_head),
  .cdcm_chan_per_dev    = 1,	/* minor per each device */
  .cdcm_ipl             = IPL_ALL,
  .cdcm_thr_list_head   = LIST_HEAD_INIT(cdcmStatT.cdcm_thr_list_head),
  .cdcm_flags           = 0
};


/* LynxOs system kernel-level errno. is set _ONLY_ by 'pseterr' function */
int cdcm_err = 0;

struct file_operations cdcm_fops;

/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_get_cdev
 * DESCRIPTION: Register character device in the kernel.
 * RETURNS:	pointer to allocated structure - if success
 *		negative value                 - if fails
 *-----------------------------------------------------------------------------
 */
struct cdev*
cdcm_get_cdev(
	      char devName[CDCM_DNL], /* desired name */
	      int min_am,	      /* minor dev amount */
	      dev_t *devn)  /* will hold the first number in allocated range */
{
  int rc;
  struct cdev *dev_cdev = NULL; /* for cdev allocation */

  /* get device numbers */
  if ( (rc = alloc_chrdev_region(devn, 0, min_am, devName)) < 0 ) {
    PRNT_ABS_WARN("alloc_chrdev_region() call failed with error=%d", rc);
    goto errexit;
  }

  /* allocate cdev struct */
  if (!(dev_cdev = cdev_alloc())) {
    PRNT_ABS_WARN("cdev_alloc() call failed with error=%d", rc);
    unregister_chrdev_region(*devn, min_am);
    rc = -ENOMEM;
    goto errexit;
  }
    /* init character device */
  cdev_init(dev_cdev, &cdcm_fops);
  dev_cdev->owner = THIS_MODULE;

  /* add character device to the kernel */
  if ( (rc = cdev_add(dev_cdev, *devn, min_am)) ) {
    PRNT_ABS_WARN("cdev_add() call failed with error=%d", rc);
    cdev_del(dev_cdev);
    unregister_chrdev_region(*devn, min_am);
    goto errexit;
  }

  return(dev_cdev);
  
 errexit:
  cdcm_err = rc;
  return ERR_PTR(rc);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_set_unique_device_name
 * DESCRIPTION: Creates uniqe device name.
 *		Device naming convention is the following:
 *		If we'll have similar devices (i.e. with the same names), then
 *		they will be distinguish by the index, i.e. DEVNAME_00,
 *		DEVNAME_01 etc...
 *		If we'll have only one device then it will be named DEVNAME_00.
 * RETURNS:     number of found devices of the similar type.
 *-----------------------------------------------------------------------------
 */
int
cdcm_set_unique_device_name(
			    char devName[CDCM_DNL])   /*  */
{
  struct cdcmDevInfo_t *diptr;
  int noffdev = 0;	/* number of find devices of our type */
  
  list_for_each_entry(diptr, &cdcmStatT.cdcm_dev_list_head, di_list)
    /* pass through all the devices to find the last similar in the list */
    if (!strncmp(diptr->di_name, devName, strlen(devName)))
      ++noffdev;
  
  /* NEWBE NOTE. Use strsep() to work with the strings. 
     See 'lib/string.c' file for more details */
  
  /* build-up a unique device name */
  snprintf(devName, CDCM_DNL, "%s_%02d", devName, noffdev);
  
  return(noffdev);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_fop_read
 * DESCRIPTION: Lynx read stub.
 * RETURNS:	how many bytes was read - if success.
 *		negative value          - if error.
 *-----------------------------------------------------------------------------
 */
static ssize_t
cdcm_fop_read(
	      struct file *filp, /*  */
	      char __user *buf,	 /* user space */
	      size_t size,	 /* how many bytes to read */
	      loff_t *off)	 /* offset */
{
  cdcmm_t *iobuf = NULL;
  struct cdcm_file lynx_file = {
    .dev = filp->f_dentry->d_inode->i_rdev, /* set Device Number */ 
    .access_mode = (filp->f_flags & O_ACCMODE),
    .position = *off
  };

  //PRNT_INFO("Read device with minor = %d", MINOR(lynx_file.dev));
  cdcm_err = 0;	/* reset */
  
  if (!access_ok(VERIFY_WRITE, buf, size)) {
    PRNT_ABS_ERR("Can't verify user buffer for writing.");
    return(-EFAULT);
  }

  if ( (iobuf = cdcm_mem_alloc(size, _IOC_READ|CDCM_M_FLG_READ)) == ERR_PTR(-ENOMEM))
    return(-ENOMEM);
  
  
  cdcm_err = entry_points.dldd_read(LynxOsWorkingArea, &lynx_file, iobuf->cmPtr, size);
  if (cdcm_err == SYSERR)
    cdcm_err = -EAGAIN;
  else {
    __copy_to_user(buf,  iobuf->cmPtr, size);
    *off = lynx_file.position;
  }

  cdcm_mem_free(iobuf);

  return(cdcm_err);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_fop_write
 * DESCRIPTION: Lynx write stub.
 * RETURNS:	how many bytes was written - if success.
 *		negative value             - if error.
 *-----------------------------------------------------------------------------
 */
static ssize_t
cdcm_fop_write(
	       struct file *filp,      /*  */
	       const char __user *buf, /* user space */
	       size_t size,	       /* how many bytes to write */
	       loff_t *off)	       /* offset */
{
  cdcmm_t *iobuf = NULL;
  struct cdcm_file lynx_file = {
    .dev = filp->f_dentry->d_inode->i_rdev, /* set Device Number */  
    .access_mode = (filp->f_flags & O_ACCMODE),
    .position = *off
  };
  
  //PRNT_INFO("Write device with minor = %d", MINOR(lynx_file.dev));
  cdcm_err = 0; /* reset */
  
  if (!access_ok(VERIFY_READ, buf, size)) {
    PRNT_ABS_ERR("Can't verify user buffer for reading.");
    return(-EFAULT);
  }

  if ( (iobuf = cdcm_mem_alloc(size, _IOC_WRITE|CDCM_M_FLG_WRITE)) == ERR_PTR(-ENOMEM))
    return(-ENOMEM);
  
  __copy_from_user(iobuf->cmPtr, buf, size);

  cdcm_err = entry_points.dldd_write(LynxOsWorkingArea, &lynx_file, iobuf->cmPtr, size);
  if (cdcm_err SYSERR)
    cdcm_err = -EAGAIN;
  else
    *off = lynx_file.position;

  cdcm_mem_free(iobuf);

  return(cdcm_err);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_fop_poll
 * DESCRIPTION: Lynx select stub.
 *		---------------- !NOT IMPLEMENTED! ----------------------------
 *		I see no way to get descriptor set (in, out, except) to pass
 *		them as a third parameter (condition to monitor - SREAD,
 *		SWRITE, SEXCEPT) to the LynxOS 'select' driver entry point.
 *		We are entering 'cdcm_fop_poll()' from do_select(), but 
 *		descriptor set stays in 'fd_set_bits *fds' var of do_select.
 *		IMHO there is no way to hook it somehow... Fuck!
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
static unsigned int
cdcm_fop_poll(
	      struct file* filp, /*  */
	      poll_table* wait)  /*  */
{
  int mask = POLLERR;
  struct cdcm_sel sel;
  struct cdcm_file lynx_file;

  lynx_file.dev = filp->f_dentry->d_inode->i_rdev; /* set Device Number */

  if (entry_points.dldd_select(LynxOsWorkingArea, &lynx_file, SREAD|SWRITE|SEXCEPT, &sel) == OK)
    mask = POLLERR;		/* for now - we are NEVER cool*/

  return(mask);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_fop_ioctl
 * DESCRIPTION: CDCM ioctl routine. Service ioctl (with CDCM_SRVIO_ prefix) 
 *		are normally called by the installation program to get all
 *		needed information. All the rest considered to be user part
 *		and is passed downstream.
 *		0 or some positve value         - in case of success.
 * RETURNS:	-EINVAL, -ENOMEM, -EIO, etc...  - if fails.
 *-----------------------------------------------------------------------------
 */
static int 
cdcm_fop_ioctl(
	       struct inode  *inode, /* inode struct pointer */
	       struct file   *file,  /* file struct pointer */
	       unsigned int  cmd,    /* IOCTL command number */
	       unsigned long arg)    /* user args */
{
  struct cdcm_file lynx_file;
  int iodir = 0, iosz = 0;
  cdcmm_t *iobuf = NULL;
  int rc = 0;	/* ret code */

  int cntr = 0;
  struct cdcmDevInfo_t *diptr;
  modinfo_t *dev_i;

  cdcm_err = 0; /* reset */

  /* first, check if it's a service ioctl */
  switch (cmd) {
  case CDCM_SRVIO_GET_MOD_AMOUNT:
    return(cdcmStatT.cdcm_dev_am);
    break;
  case CDCM_SRVIO_GET_MOD_INFO:
    dev_i = kmalloc(cdcmStatT.cdcm_dev_am * sizeof(*dev_i), GFP_KERNEL);
    
    if (IS_ERR(dev_i))
      return(-ENOMEM);
    
    /* setup device names list */
    list_for_each_entry(diptr, &cdcmStatT.cdcm_dev_list_head, di_list) {
      strncpy(dev_i[cntr].info_dname, diptr->di_name, CDCM_DNL);
      dev_i[cntr].info_major = MAJOR(diptr->di_dev_num);
      dev_i[cntr].info_minor = MINOR(diptr->di_dev_num);
      cntr++;
    }
    
    if (copy_to_user((void __user *)arg, dev_i, cdcmStatT.cdcm_dev_am * sizeof(*dev_i)))
      rc = -EFAULT;
    
    kfree(dev_i);
    return(rc);
    break;
  default:
    /* user ioctl */
    iodir = _IOC_DIR(cmd);
    iosz  = _IOC_SIZE(cmd);
    break;
  }

  if (iodir != _IOC_NONE) { /* we should move user <-> driver data */
    if ( (iobuf = cdcm_mem_alloc(iosz, iodir|CDCM_M_FLG_IOCTL)) == ERR_PTR(-ENOMEM))
      return(-ENOMEM);
  }
  
  if (iodir & _IOC_WRITE) {
    if (!access_ok(VERIFY_READ, (void __user*)arg, iosz)) {
      PRNT_ABS_ERR("Can't verify user buffer for reading.");
      cdcm_mem_free(iobuf);
      return(-EFAULT);
    }
    __copy_from_user(iobuf->cmPtr, (char*)arg, iosz); /* take data from user */
  }
  
  /* Carefull with the type correspondance.
     int in Lynx <-> unsigned int in Linux */
  lynx_file.dev = (int)inode->i_rdev; /* set Device Number */

  /* hook the user ioctl */
  rc = entry_points.dldd_ioctl(LynxOsWorkingArea, &lynx_file, cmd, iobuf->cmPtr);
  if (rc == SYSERR) {
    cdcm_mem_free(iobuf);
    return(-(cdcm_err)); /* error is set by the user */
  }
  
  if (iodir & _IOC_READ) {
    if (!access_ok(VERIFY_WRITE, (void __user*)arg, iosz)) {
      PRNT_ABS_ERR("Can't verify user buffer for writing.");
      cdcm_mem_free(iobuf);
      return(-EFAULT);
    }
    __copy_to_user((char*)arg, iobuf->cmPtr, iosz); /* give data to the user */
  }
  
  cdcm_mem_free(iobuf);
  return(rc);	/* we cool */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_fop_mmap
 * DESCRIPTION: NOT IMPLEMENTED YET.
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
static int 
cdcm_fop_mmap(
	      struct file           *file, /*  */
	      struct vm_area_struct *vma)  /*  */
{
  return(0); /* success */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_fop_open
 * DESCRIPTION: Lynx open stub.
 * RETURNS:	0               - if success.
 *		negative number - if fails.
 *-----------------------------------------------------------------------------
 */
static int
cdcm_fop_open(
	      struct inode *inode, /* inode pointer */
	      struct file  *filp)  /* file pointer */
{
  int dnum;
  struct cdcm_file lynx_file;
  int minor;

  minor = iminor(inode);
  cdcm_err = 0;	/* reset */


  /* enforce read-only access to this chrdev */
  /* 
  if ((filp->f_mode & FMODE_READ) == 0)
    return -EINVAL;
  if (filp->f_mode & FMODE_WRITE)
    return -EINVAL;
  */

  if (inode->i_rdev == cdcmStatT.cdcm_dev_num)
    /* service inode - no need to fall in user driver part */
    return(0);

  /* fill in Lynxos file */
  dnum = lynx_file.dev = (int)inode->i_rdev;
  lynx_file.access_mode = (filp->f_flags & O_ACCMODE);
  
  if (entry_points.dldd_open(LynxOsWorkingArea, dnum, &lynx_file) == OK) {
    //filp->private_data = (void*)dnum; /* set for us */
    /* you can use private_data for your own purposes */
    return(0);	/* success */
  }

  cdcm_err = -ENODEV; /* TODO. What val to return??? */
  return(cdcm_err); /* failure */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_fop_release
 * DESCRIPTION: Lynx close stub.
 * RETURNS:	0               - if success.
 *		negative number - if fails.
 *-----------------------------------------------------------------------------
 */
static int
cdcm_fop_release(
		 struct inode *inode, /* inode pointer */
		 struct file *filp)   /* file pointer */
{
  struct cdcm_file lynx_file;
  int minor;
  
  minor = iminor(inode);
  cdcm_err = 0;	/* reset */

  if (inode->i_rdev == cdcmStatT.cdcm_dev_num)
    /* service inode - no need to fall in user driver part */
    return(0);

  /* fill in Lynxos file */
  lynx_file.dev = (int)inode->i_rdev;
  lynx_file.access_mode = (filp->f_flags & O_ACCMODE);

  if (entry_points.dldd_close(LynxOsWorkingArea, &lynx_file) == OK)
    return(0);			/* we cool */

  cdcm_err = -ENODEV; /* TODO. What val to return??? */	
  return(cdcm_err); /* failure */
}


/* CDCM entry points */
struct file_operations cdcm_fops = {
  .owner   = THIS_MODULE,
  .read    = cdcm_fop_read,   /* read   */
  .write   = cdcm_fop_write,  /* write  */
  .poll    = cdcm_fop_poll,   /* select - NOT COMPATIBLE! */
  .ioctl   = cdcm_fop_ioctl,  /* ioctl  */
  .mmap    = cdcm_fop_mmap,   /* mmap   */
  .open    = cdcm_fop_open,   /* open   */
  .release = cdcm_fop_release /* close  */
};


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_driver_cleanup.
 * DESCRIPTION: Called when module is unloading from the kernel.
 * RETURNS:     void
 *-----------------------------------------------------------------------------
 */
static void __exit
cdcm_driver_cleanup(void)
{
  int cntr;

  /* first, let user to cleanup */
  if (!(cdcmStatT.cdcm_flags & CDCMFLG_NOUSREP))
    entry_points.dldd_uninstall(LynxOsWorkingArea);

  /* cleanup all captured memory */ 
  cdcm_mem_cleanup_all();

  /* free info table mem */
  if (devInfoT)
    sysfree(devInfoT, devInfoTSz);
  
  /* stop timers */
  for (cntr = 0; cntr < MAX_CDCM_TIMERS; cntr++)
    if (cdcmStatT.cdcm_timer[cntr].ct_on)
      del_timer_sync(&cdcmStatT.cdcm_timer[cntr].ct_timer);

  /* cleaning-up */
  if (cdcmStatT.cdcm_dev_am) {
    PRNT_ABS_WARN("Not all devices unload from the kernel!!!");
    /* TODO Perform correct cleaning up... */
  }

  if (cdcmStatT.cdcm_drvr_type == 'P')
    cdcm_pci_cleanup();
  else if (cdcmStatT.cdcm_drvr_type == 'V')
    cdcm_vme_cleanup();

  /* remove CDCM char device from the kernel */
  cdev_del(cdcmStatT.cdcm_cdev);
  
  /* release device numbers */
  unregister_chrdev_region(cdcmStatT.cdcm_dev_num, 1);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_driver_init.
 * DESCRIPTION: Main driver initialization.
 * RETURNS:     0        - in case of success.
 *              non zero - otherwise.
 *-----------------------------------------------------------------------------
 */
static int __init
cdcm_driver_init(void)
{
  int cntr = 0;

  /* get module info table */
  if ( !(devInfoT = cdcm_get_info_table(cdcmInfoF, &devInfoTSz)) ) {
    PRNT_ABS_ERR("Can't read info file."); /* fatal. barf */
    return(-ENOENT);
  }

  /* init semaphore mutexes */
  for (cntr = 0; cntr < MAX_CDCM_SEM; cntr++)
    cdcmStatT.cdcm_s[cntr].wq_usr_val = NULL;

  /*  */
  cdcmStatT.cdcm_cdev = cdcm_get_cdev(CDCM_DEV_NAME, 1, &cdcmStatT.cdcm_dev_num);
  
  if (IS_ERR(cdcmStatT.cdcm_cdev)) {
    PRNT_ABS_ERR("Can't register CDCM as character device... Aborting!");
    return PTR_ERR(cdcmStatT.cdcm_cdev);
  }

  cdcm_mem_init(); /* init our memory manager */

  /* hook the user */
  LynxOsWorkingArea = entry_points.dldd_install(devInfoT);
  if ((int)LynxOsWorkingArea == SYSERR) {  
    /* user installation procedure fails. fatal. barf */
    PRNT_ABS_ERR("Can't get driver working area... Aborting!");
    LynxOsWorkingArea = NULL;
    cdcmStatT.cdcm_flags |= CDCMFLG_NOUSREP;
    cdcm_driver_cleanup();	/* cleanup */
    return(-ENOEXEC);
   }

  /* Normally if here, then our driver is already registered in the kernel,
     CDCM statics table is filled with a proper values. We can continue to
     work with them. (Registration is done by calling 'drm_get_handle()'
     of find_controller() stub functions (depends on the driver type)
     during 'entry_points.dldd_install()' call) */

  if (!cdcmStatT.cdcm_drvr_type) { /* user was not calling 'drm_get_handle()'
				      or 'find_controller()' We can not proceed
				      without them! */
    PRNT_ABS_ERR("Can't proceed without devices... Aborting!!!");
    return(-EACCES);
    /* TODO. Perform correct cleaning up */
  }

  return(OK);
}

module_init(cdcm_driver_init);
module_exit(cdcm_driver_cleanup);
